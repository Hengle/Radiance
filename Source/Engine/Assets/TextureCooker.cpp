/*! \file TextureCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "TextureCooker.h"
#include "TextureParser.h"
#include <Runtime/Stream.h>
#include "../Engine.h"

using namespace pkg;

namespace asset {

BOOST_STATIC_ASSERT(sizeof(TextureTag)==1);

TextureCooker::TextureCooker() : Cooker(3) {
}

TextureCooker::~TextureCooker() {
}

CookStatus TextureCooker::Status(int flags) {
	const bool * b = asset->entry->KeyValue<bool>("Localized", flags);
	if (!b)
		return CS_Error;

	if (CompareVersion(flags) ||
		CompareModifiedTime(flags) ||
		CompareCachedFileTimeKey(flags, "Source.File", (*b) ? "Localized" : 0) ||
		CheckFastCook(flags)) {
		return CS_NeedRebuild;
	}
	return CS_UpToDate;
}

int TextureCooker::Compile(int flags) {

	const bool * b = asset->entry->KeyValue<bool>("Localized", flags);
	if (!b)
		return SR_MetaError;

	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File", (*b) ? "Localized" : 0);
	CheckFastCook(flags);

	TextureTag tag;
	tag.flags = 0;

	b = asset->entry->KeyValue<bool>("Wrap.S", flags);
	if (!b)
		return SR_MetaError;
	tag.flags |= *b ? TextureTag::WrapS : 0;

	b = asset->entry->KeyValue<bool>("Wrap.T", flags);
	if (!b)
		return SR_MetaError;
	tag.flags |= *b ? TextureTag::WrapT : 0;

	b = asset->entry->KeyValue<bool>("Wrap.R", flags);
	if (!b)
		return SR_MetaError;
	tag.flags |= *b ? TextureTag::WrapR : 0;

	b = asset->entry->KeyValue<bool>("Mipmap", flags);
	if (!b)
		return SR_MetaError;
	tag.flags |= *b ? TextureTag::Mipmap : 0;

	const String *s = asset->entry->KeyValue<String>("Filter", flags);
	if (s) {
		if (*s == "Bilinear") {
			tag.flags |= TextureTag::FilterBilinear;
		} else if (*s == "Trilinear") {
			tag.flags |= TextureTag::FilterTrilinear;
		}
	} else {
		b = asset->entry->KeyValue<bool>("Filter", flags);
		if (!b)
			return SR_MetaError;
		tag.flags |= *b ? TextureTag::FilterBilinear : 0;
	}

	b = asset->entry->KeyValue<bool>("Localized", flags);
	if (!b)
		return SR_MetaError;
	tag.flags |= *b ? TextureTag::Localized : 0;

	// Cache here.
	CompareCachedFileTimeKey(flags, "Source.File", (*b) ? "Localized" : 0);

	BinFile::Ref fp = OpenTagWrite();
	if (!fp)
		return SR_IOError;

	if (fp->ob.get().Write(&tag, sizeof(tag), 0) != sizeof(tag))
		return SR_IOError;

	for (int lang = StringTable::LangId_EN; lang < StringTable::LangId_MAX; ++lang) {

		if (!((1<<lang)&languages))
			continue;

		pkg::Asset::Ref localizedAsset = engine->sys->packages->Asset(asset->id, pkg::Z_Unique);

		TextureParser *parser = TextureParser::Cast(localizedAsset);
		if (!parser)
			return SR_ParseError;

		parser->langId = (StringTable::LangId)lang;

		int r = localizedAsset->Process(
			xtime::TimeSlice::Infinite, 
			flags|P_Parse|P_TargetDefault|P_NoDefaultMedia
		);

		if (r != SR_Success) {
			if (lang != StringTable::LangId_EN)
				cout.get() << "ERROR cooking localized variant for " << StringTable::LangTitles[lang] << std::endl;
			return r;
		}
		
		String path(CStr(asset->path));

		if (lang != StringTable::LangId_EN) {
			path += "_";
			path +=StringTable::Langs[lang];
		}

		path += ".bin";

		fp = OpenWrite(path.c_str);
		if (!fp) {
			cout.get() << "ERROR failed to open '" << asset->path.get() << ".bin'!" << std::endl;
			return SR_IOError;
		}

		const int numImages = parser->numImages;

		stream::OutputStream os(fp->ob);
		os << (U32)numImages;

		for (int i = 0; i < numImages; ++i) {
			const image_codec::Image *image = parser->Image(i);

			os << (U32)image->format << (U32)image->bpp << (U32)image->frameCount;

			for (int f = 0; f < image->frameCount; ++f) {
				const image_codec::Frame &frame = image->frames[f];

				os << (U32)frame.mipCount << (U32)frame.flags;

				for (int i = 0; i < frame.mipCount; ++i) {
					const image_codec::Mipmap &mip = frame.mipmaps[i];

					os << (U32)mip.width << (U32)mip.height << (U32)mip.stride << (U32)mip.dataSize;

					if (os.Write(mip.data, (stream::SPos)mip.dataSize, 0) != (stream::SPos)mip.dataSize)
						return SR_IOError;

					if (mip.dataSize&3) {
						char padd[3] = {0, 0, 0};
						stream::SPos len = (stream::SPos)(4-(mip.dataSize&3));
						if (os.Write(padd, len, 0) != len)
							return SR_IOError;
					}
				}
			}
		}

		if (!(tag.flags&TextureTag::Localized))
			break; // don't do languages for this texture.
	}

	return SR_Success;
}
int TextureCooker::CheckFastCook(int flags) {
	const char *sz = TargetString(flags, "P_FastCook");

	if (sz && !string::cmp(sz, "true")) {
		// last time was a fast cook
		if (flags&P_FastCook)
			return 0; // no change
		SetTargetString(flags, "P_FastCook", "false");
		return -1; // fast data is low quality we want full quality now.
	} else if (!sz) { // no quality string was set
		if (flags&P_FastCook) {
			SetTargetString(flags, "P_FastCook", "true");
		} else {
			SetTargetString(flags, "P_FastCook", "false");
		}
		return -1; // have to rebuild.
	}

	// last cook was not a fast cook.
	// we never overwrite HQ textures with low quality fast ones.
	return 0;
}

void TextureCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<TextureCooker>();
}

} // asset
