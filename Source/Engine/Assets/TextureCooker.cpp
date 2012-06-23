// TextureCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "TextureCooker.h"
#include "TextureParser.h"
#include <Runtime/Stream.h>
#include "../Engine.h"

using namespace pkg;

namespace asset {

BOOST_STATIC_ASSERT(sizeof(TextureTag)==1);

TextureCooker::TextureCooker() : Cooker(1) {
}

TextureCooker::~TextureCooker() {
}

CookStatus TextureCooker::CheckRebuild(int flags, int allflags) {
	const bool * b = asset->entry->KeyValue<bool>("Localized", flags);
	if (!b)
		return CS_NeedRebuild; // force error is cook path.

	if (CompareVersion(flags) ||
		CompareModifiedTime(flags) ||
		CompareCachedFileTimeKey(flags, "Source.File", (*b) ? "Localized" : 0))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

CookStatus TextureCooker::Status(int flags, int allflags) {
	flags &= P_AllTargets;
	allflags &= P_AllTargets;

	if (flags == 0) { // only build generics if all platforms are identical to eachother.
		if (MatchTargetKeys(allflags, allflags)==allflags)
			return CheckRebuild(flags, allflags);
		return CS_Ignore;
	}

	if (MatchTargetKeys(allflags, allflags)==allflags)
		return CS_Ignore;

	// only build ipad if different from iphone
	if ((flags&P_TargetIPad) && (allflags&P_TargetIPhone)) {
		if (MatchTargetKeys(P_TargetIPad, P_TargetIPhone))
			return CS_Ignore;
	}

	return CheckRebuild(flags, allflags);
}

int TextureCooker::Compile(int flags, int allflags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
		
	// we need the asset parser to apply compression, and in the case of IOS
	// platforms we need need it to apply PVR compression (if selected) so if
	// we are doing generics then force the target flags.
	
	int parseTarget = flags&pkg::P_AllTargets;
	if (parseTarget == 0) { // generics
		parseTarget = LowBitVal(allflags&P_AllTargets);
	}
	
	TextureTag tag;
	tag.flags = 0;

	const bool *b = asset->entry->KeyValue<bool>("Wrap.S", flags);
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

	b = asset->entry->KeyValue<bool>("Filter", flags);
	if (!b)
		return SR_MetaError;
	tag.flags |= *b ? TextureTag::Filter : 0;

	b = asset->entry->KeyValue<bool>("Localized", flags);
	if (!b)
		return SR_MetaError;
	tag.flags |= *b ? TextureTag::Localized : 0;

	// Cache here.
	CompareCachedFileTimeKey(flags, "Source.File", (*b) ? "Localized" : 0);

	BinFile::Ref fp = OpenTagWrite(flags);
	if (!fp)
		return SR_IOError;

	if (fp->ob.get().Write(&tag, sizeof(tag), 0) != sizeof(tag))
		return SR_IOError;

	for (int lang = StringTable::LangId_EN; lang < StringTable::LangId_MAX; ++lang) {

		if (!((1<<lang)&languages))
			continue;

		pkg::Asset::Ref localizedAsset = engine->sys->packages->Asset(asset->id, pkg::Z_Unique);

		TextureParser::Ref parser = TextureParser::Cast(localizedAsset);
		if (!parser)
			return SR_ParseError;

		parser->langId = (StringTable::LangId)lang;

		int r = localizedAsset->Process(
			xtime::TimeSlice::Infinite, 
			flags|parseTarget|P_Parse|P_TargetDefault|P_NoDefaultMedia
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

		path += L".bin";

		fp = OpenWrite(path.c_str, flags);
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

			for (UReg f = 0; f < image->frameCount; ++f) {
				const image_codec::Frame &frame = image->frames[f];

				os << (U32)frame.mipCount << (U32)frame.flags;

				for (UReg i = 0; i < frame.mipCount; ++i) {
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

int TextureCooker::MatchTargetKeys(int flags, int allflags) {
	int x = asset->entry->MatchTargetKeys<String>("Source.File", flags, allflags)&
			asset->entry->MatchTargetKeys<bool>("Resize", flags, allflags)&
			asset->entry->MatchTargetKeys<bool>("Mipmap", flags, allflags)&
			asset->entry->MatchTargetKeys<bool>("Wrap.S", flags, allflags)&
			asset->entry->MatchTargetKeys<bool>("Wrap.T", flags, allflags)&
			asset->entry->MatchTargetKeys<bool>("Wrap.R", flags, allflags)&
			asset->entry->MatchTargetKeys<bool>("Localized", flags, allflags);

	if (x) {
		const bool *b = asset->entry->KeyValue<bool>("Resize", flags);
		if (b && *b) { // make sure resized sizes match if we're resizing
			x &= asset->entry->MatchTargetKeys<int>("Resize.Width", flags, allflags)&
				asset->entry->MatchTargetKeys<int>("Resize.Height", flags, allflags);
		}

		const String *s = asset->entry->KeyValue<String>("Compression", flags);
		if (s) {
			if ((allflags&P_TargetIOS) && (allflags&~P_TargetIOS)) {   
				// IOS and non-IOS targets selected, they can never match since compression formats
				// are different.
				if (*s != "None")
					x &= flags;					
			}

			x &= asset->entry->MatchTargetKeys<String>("Compression", flags, allflags);
		}
	}

	return x;
}

void TextureCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<TextureCooker>();
}

} // asset
