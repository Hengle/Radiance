/*! \file TextureParser.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

/* It should be noted here that the Radiance DDS library is used here.
   It should *also* be noted that this code *does not compress or decompress DXT formatted data* using
   the Radiance DDS library, rather the code is used to simply unwrap the DDS format around DXT compressed
   blocks.
  
   DDS files are not written by this code.
  
   DXT compression, should it be enabled by a texture asset, is performed by the texture tools provided by
   NVidia, see TextureParserDXT.cpp . The software used to compress this data is linked as a DLL and is therefore 
   not part of the Radiance Engine, and its use should fall under the patent licenses obtained by NVidia.
  
   Therefore the use of this code, unmodified, should not be subject to any patents related to S3TC compression 
   formats and licensees of this software should not be required to seek an S3TC license. However, this should
   not be taken as law, and any concerned parties should consult their legal counsel.
  
   Furthermore: All DXT related functionality is not linked into any shipping builds of 
   a game. PVRTexLib, NVTT, ATI_Compress, DDS, JPEG, TGA loading is only part of development/tools builds of 
   the engine.
*/

#include RADPCH
#include "TextureParser.h"
#include "../App.h"
#include "../Engine.h"
#include <Runtime/File.h>

#if defined(RAD_OPT_TOOLS)
#include <Runtime/ImageCodec/Tga.h>
#include <Runtime/ImageCodec/Dds.h>
#include <Runtime/ImageCodec/Bmp.h>
#include <Runtime/ImageCodec/Png.h>
#include "../Renderer/GL/GLTable.h"
#include "../Renderer/GL/GLTexture.h"
enum {
	kMinMipSize = 1
};
#endif

using namespace pkg;

namespace asset {

TextureParser::TextureParser()
: m_state(S_None), m_load(false) {
#if defined(RAD_OPT_TOOLS)
	m_langId = App::Get()->langId;
	m_compressionMode = kCompressionMode_Default;
#endif
}

TextureParser::~TextureParser() {
}

const image_codec::Image *TextureParser::Image(int idx) const  {
	return m_images[idx].get();
}

int TextureParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const AssetRef &asset,
	int flags
) {
	if (flags&(P_Unload|P_Trim)) {
		if (flags&P_Unload)
			m_load = false;
		m_state = headerValid ? S_Header : S_None;
		m_images.clear();
		STLContainerShrinkToSize(m_images);
#if defined(RAD_OPT_TOOLS)
		m_bufs.clear();
		STLContainerShrinkToSize(m_bufs);
		m_cooker.reset();
#endif
		m_mm.reset();
		return SR_Success;
	}

	if ((flags&P_Cancel)) {
		m_images.clear();
		STLContainerShrinkToSize(m_images);
#if defined(RAD_OPT_TOOLS)
		m_bufs.clear();
		STLContainerShrinkToSize(m_bufs);
#endif
		m_mm.reset();
		m_load = false;
		if (m_state < S_Done)
			m_state = S_None;
		return SR_Success;
	}

	if (!(flags&(P_Parse|P_Load|P_VidBind|P_Info)))
		return SR_Success;

	if (imgValid || (headerValid && (flags&P_Info)) || (m_load && (flags&P_Load)))
		return SR_Success;

	if (m_state < 0)
		return m_state; // error code

	if (m_state == S_Header) {
		m_mm.reset();
		m_images.clear();
#if defined(RAD_OPT_TOOLS)
		m_bufs.clear();
#endif
	 	m_state = S_None; // we are fully loading
	}

	int r = SR_Success;

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked && ((flags&(P_Info|P_Parse|P_Unformatted)) || !(flags&P_FastPath))) {
		RAD_ASSERT(m_state != S_Loading); // only used in cooked path.
		switch (m_state) {
		case S_None:
			r = Load(engine, time, asset, flags);
			break;
		case S_Parsing:
			r = Parsing(engine, time, asset, flags);
			break;
		}
	}
	else
#endif
	{
		r = LoadCooked(engine, time, asset, flags);
	}

	if (r < 0) {
		m_state = r;
	}

	return r;
}

#define CHECK_SIZE(_size) if (((bytes+(_size))-reinterpret_cast<const U8*>(data)) > (int)size) return pkg::SR_CorruptFile

int TextureParser::LoadCooked(
	Engine &engine,
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (m_state == S_Done)
		return SR_Success;

	if (m_state == S_None) {

#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked) {

			m_cooker = asset->AllocateIntermediateCooker();

			CookStatus status = m_cooker->Status(P_TARGET_FLAGS(flags));

			if (status == CS_Ignore)
				return SR_CompilerError;

			if (status == CS_NeedRebuild) {
				COut(C_Info) << asset->path.get() << " is out of date, rebuilding..." << std::endl;
				int r = m_cooker->Cook(P_TARGET_FLAGS(flags));
				if (r != SR_Success)
					return r;
			}
			else {
				COut(C_Info) << asset->path.get() << " is up to date, using cache." << std::endl;
			}

			// Load TAG
			file::MMapping::Ref mm = m_cooker->LoadTag();
			if (!mm)
				return SR_FileNotFound;

			const asset::TextureTag *tag = (const asset::TextureTag*)mm->data.get();
			m_tag = *tag;

			// Load Texture

			String path(CStr(asset->path));

			if ((m_tag.flags&TextureTag::Localized) && (m_langId != StringTable::LangId_EN)) {
				path += "_";
				path += StringTable::Langs[m_langId];
			}

			path += ".bin";

			m_mm = m_cooker->MapFile(path.c_str, r::ZTextures);
			if (!m_mm)
				return SR_FileNotFound;
		}
		else {
		StringTable::LangId langId = m_langId;
#else
		StringTable::LangId langId = App::Get()->langId;
#endif
		const asset::TextureTag *tag = (const asset::TextureTag*)asset->entry->tagData.get();
		if (!tag)
			return SR_MetaError;
		m_tag = *tag;

		// load file...
		String path(CStr("Cooked/"));
		path += CStr(asset->path);

		if ((m_tag.flags&TextureTag::Localized) && (langId != StringTable::LangId_EN)) {
			path += "_";
			path += StringTable::Langs[langId];
		}

		path += ".bin";

		m_mm = engine.sys->files->MapFile(path.c_str, r::ZTextures);
		if (!m_mm)
			return SR_FileNotFound;
#if defined(RAD_OPT_TOOLS)
		}
#endif
		m_state = S_Loading;
		if (!time.remaining)
			return SR_Pending;
	}

	const AddrSize size = m_mm->size.get();
	const void *data = m_mm->data.get();
	const U8 *bytes = reinterpret_cast<const U8*>(data);

	CHECK_SIZE(sizeof(U32));
	U32 numImages = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);

	m_images.reserve(numImages);
	for (U32 i = 0; i < numImages; ++i) {
		image_codec::Image::Ref img(new (ZEngine) image_codec::Image(r::ZTextures));

		U32 format, bpp, numFrames;
		CHECK_SIZE(sizeof(U32)*3);
		format = *reinterpret_cast<const U32*>(bytes);
		bytes += sizeof(U32);
		bpp = *reinterpret_cast<const U32*>(bytes);
		bytes += sizeof(U32);
		numFrames = *reinterpret_cast<const U32*>(bytes);
		bytes += sizeof(U32);

		img->format = format;
		img->bpp = bpp;
		img->AllocateFrames(numFrames);

		for (int f = 0; f < img->frameCount; ++f) {
			image_codec::Frame &frame = img->frames[f];
			CHECK_SIZE(sizeof(U32));
			img->AllocateMipmaps(f, *reinterpret_cast<const U32*>(bytes));
			bytes += sizeof(U32);
			
			CHECK_SIZE(sizeof(U32));
			frame.flags = *reinterpret_cast<const U32*>(bytes);
			frame.flags |= image_codec::SharedFrameFlagRef;
			bytes += sizeof(U32);

			for (int m = 0; m < frame.mipCount; ++m) {
				image_codec::Mipmap &mip = frame.mipmaps[m];

				CHECK_SIZE(sizeof(U32));
				mip.width = *reinterpret_cast<const U32*>(bytes);
				bytes += sizeof(U32);
				CHECK_SIZE(sizeof(U32));
				mip.height = *reinterpret_cast<const U32*>(bytes);
				bytes += sizeof(U32);
				CHECK_SIZE(sizeof(U32));
				mip.stride = *reinterpret_cast<const U32*>(bytes);
				bytes += sizeof(U32);
				CHECK_SIZE(sizeof(U32));
				mip.dataSize = *reinterpret_cast<const U32*>(bytes);
				bytes += sizeof(U32);
				CHECK_SIZE(mip.dataSize);
				mip.data = const_cast<U8*>(bytes);
				bytes += mip.dataSize;

				if (mip.dataSize&3) // align
					bytes += 4-(mip.dataSize&3);
			}
		}

		if (m_images.empty()) {
			m_header.format = img->format;
			m_header.numMips = (img->frameCount>0) ? img->frames->mipCount : 0;
			m_header.width = (img->frameCount>0) && (img->frames->mipCount>0) ? img->frames->mipmaps->width : 0;
			m_header.height = (img->frameCount>0) && (img->frames->mipCount>0) ? img->frames->mipmaps->height : 0;
		}

		m_images.push_back(img);
	}

	m_state = S_Done;
	m_load = true;

#undef CHECK_SIZE

	return SR_Success;
}

#if defined(RAD_OPT_TOOLS)

int TextureParser::GetCompressionMode(const pkg::Asset::Ref &asset, int flags) {
	if (m_compressionMode != kCompressionMode_Default)
		return m_compressionMode;

	// android target MUST force a compression mode (this path only gets hit during cooking).
	RAD_VERIFY(!(flags&P_TargetAndroid));
	RAD_ASSERT(flags&P_AllTargets);

	const bool *enabled = asset->entry->KeyValue<bool>("Compression.Enabled", flags);
	if (!enabled)
		return SR_MetaError;

	if (!*enabled)
		return kCompressionMode_Disabled;

	if (flags&P_TargetiOS) {
		// pvr?
		const String *compression = asset->entry->KeyValue<String>("Compression.PVR", flags);
		if (!compression)
			return SR_MetaError;

		if (*compression != "Disabled")
			return kCompressionMode_PVR;
		return kCompressionMode_Disabled;
	}

	// PC target
	RAD_VERIFY(flags&P_TargetPC);
	const String *compression = asset->entry->KeyValue<String>("Compression.DXT.Mode", flags);
	if (!compression)
		return SR_MetaError;

	if (*compression != "Disabled")
		return kCompressionMode_DXT;
	return kCompressionMode_Disabled;
}

int TextureParser::SourceModifiedTime(
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags,
	xtime::TimeDate &td
) {
	const String *name = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));

	if (!name)
		return pkg::SR_MetaError;

	const bool *localized = asset->entry->KeyValue<bool>("Localized", P_TARGET_FLAGS(flags));
	if (!localized)
		return pkg::SR_MetaError;

	String sname(*name);

	if (*localized && (m_langId != StringTable::LangId_EN)) {
		String ext = file::GetFileExtension(sname.c_str);
		String path = file::SetFileExtension(sname.c_str, 0);
			
		sname = path;
		sname += "_";
		sname += StringTable::Langs[m_langId];
		sname += ext;
	}

	if (!(flags&P_NoDefaultMedia)) {
		if (sname.empty || !engine.sys->files->FileExists(sname.c_str)) {
			sname = CStr("Textures/Missing_Texture.tga");
			if (!engine.sys->files->FileExists(sname.c_str)) {
				return pkg::SR_FileNotFound;
			}
		}
	}

	String absPath;
	if (!engine.sys->files->GetAbsolutePath(sname.c_str, absPath, file::kFileMask_Base))
		return SR_FileNotFound;
	if (!engine.sys->files->GetFileTime(absPath.c_str, td))
		return SR_FileNotFound;
	return SR_Success;
}

int TextureParser::Load(
	Engine &engine, 
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {
	const String *name = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));

	if (!name)
		return pkg::SR_MetaError;

	const bool *localized = asset->entry->KeyValue<bool>("Localized", P_TARGET_FLAGS(flags));
	if (!localized)
		return pkg::SR_MetaError;

	String sname(*name);

	if (*localized && (m_langId != StringTable::LangId_EN)) {
		String ext = file::GetFileExtension(sname.c_str);
		String path = file::SetFileExtension(sname.c_str, 0);
			
		sname = path;
		sname += "_";
		sname += StringTable::Langs[m_langId];
		sname += ext;
	}

	if (!(flags&P_NoDefaultMedia)) {
		if (sname.empty || !engine.sys->files->FileExists(sname.c_str)) {
			sname = CStr("Textures/Missing_Texture.tga");
			if (!engine.sys->files->FileExists(sname.c_str)) {
				return pkg::SR_FileNotFound;
			}
		}
	}

	m_fmt = Format(sname.c_str);

	if (m_fmt < 0)
		return pkg::SR_InvalidFormat;

	if (flags&P_Info) { // read enough to parse header
		file::MMFile::Ref file = engine.sys->files->OpenFile(sname.c_str, r::ZTextures);
		if (!file)
			return SR_FileNotFound;
		file::MMapping::Ref mm = file->MMap(0, kKilo*4, r::ZTextures);
		if (!mm)
			return SR_IOError;
		m_bufs.push_back(mm);
	} else {
		String base = file::GetBaseFileName(sname.c_str);
		if (!base.empty && base[0] == '+') { // bundle, load all frames simultaneously
			String path = file::GetFilePath(sname.c_str);
			String ext = file::GetFileExtension(sname.c_str);

			// skip +digit
			const char *postDigit = base.c_str;
			if (base[1] != 0)
				postDigit += 2;

			for (int i = 0;; ++i) {
				file::MMapping::Ref mm;
				char x[256];

				string::sprintf(x, "%s/+%d%s%s", path.c_str.get(), i, postDigit, ext.c_str.get());

				mm = engine.sys->files->MapFile(x, r::ZTextures);
				if (!mm)
					break;

				m_bufs.push_back(mm);
			}

			if (m_bufs.empty())
				return SR_FileNotFound;
		}
		else {
			file::MMapping::Ref mm = engine.sys->files->MapFile(sname.c_str, r::ZTextures);
			if (!mm)
				return SR_FileNotFound;
			m_bufs.push_back(mm);
		}
	}

	m_state = S_Parsing;
	
	int r = SR_Pending;

	if (time.remaining) {
		r = Parsing(engine, time, asset, flags);
	}

	return r;
}

int TextureParser::Parsing(
	Engine &engine,
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {
	RAD_ASSERT(m_fmt >= 0);

	if (m_bufs.empty())
		return SR_IOError;

	int r = SR_InvalidFormat;

	if (flags&P_Info) {
		image_codec::Image::Ref img(new (ZEngine) image_codec::Image(r::ZTextures));

		switch (m_fmt) {
		case F_Tga:
			r = image_codec::tga::DecodeHeader(
				m_bufs[0]->data,
				m_bufs[0]->size,
				*img
			) ? SR_Success : SR_InvalidFormat;
			break;
		case F_Dds:
			r = image_codec::dds::DecodeHeader(
				m_bufs[0]->data,
				m_bufs[0]->size,
				*img
			) ? SR_Success : SR_InvalidFormat;
			break;
		case F_Png:
			r = image_codec::png::DecodeHeader(
				m_bufs[0]->data,
				m_bufs[0]->size,
				*img
			) ? SR_Success : SR_InvalidFormat;
			break;
		case F_Bmp:
			r = image_codec::bmp::DecodeHeader(
				m_bufs[0]->data,
				m_bufs[0]->size,
				*img
			) ? SR_Success : SR_InvalidFormat;
			break;
		}

		if (r == SR_Success)
			m_images.push_back(img);
	} else {
		for (IOVec::iterator it = m_bufs.begin(); it != m_bufs.end(); ++it) {
			file::MMapping::Ref &mm = *it;
			image_codec::Image::Ref img(new (ZEngine) image_codec::Image(r::ZTextures));

			switch (m_fmt) {
			case F_Tga:
				r = image_codec::tga::Decode(
					mm->data,
					mm->size,
					*img
				) ? SR_Success : SR_InvalidFormat;
				break;
			case F_Dds:
				r = image_codec::dds::Decode(
					mm->data,
					mm->size,
					*img,
					false, // no-ref
					false // no decompress
				) ? SR_Success : SR_InvalidFormat;
				break;
			case F_Png:
				r = image_codec::png::Decode(
					mm->data,
					mm->size,
					*img
				) ? SR_Success : SR_InvalidFormat;
				break;
			case F_Bmp:
				r = image_codec::bmp::Decode(
					mm->data,
					mm->size,
					*img
				) ? SR_Success : SR_InvalidFormat;
				break;
			}

			if (r != SR_Success)
				break;

			m_images.push_back(img);
			mm.reset(); // free data.
		}
	}

	if (r == SR_Success) {
		ImageVec::const_iterator it;
		for (it = m_images.begin(); it != m_images.end(); ++it) {
			const image_codec::Image::Ref &img = *it;
			if (img->frameCount < 1)
				break;
			int i;
			for (i = 0; i < img->frameCount; ++i) {
				if (img->frames[i].mipCount < 1)
					break;
				if (i == 0 && it == m_images.begin()) {
					// copy to header.
					m_header.format = img->format;
					m_header.width = (int)img->frames[i].mipmaps[0].width;
					m_header.height = (int)img->frames[i].mipmaps[0].height;
					m_header.numMips = (int)img->frames[i].mipCount;
				} else {
					if (img->frames[i].mipmaps[0].width != m_header.width)
						break;
					if (img->frames[i].mipmaps[0].height != m_header.height)
						break;
				}
			}
			if (i != img->frameCount)
				break;
		}

		if (it != m_images.end())
			r = SR_InvalidFormat;

		if (r == SR_Success) {
			if (flags&(P_Load|P_Parse)) {
				if (!(flags&P_Unformatted)) {
					TextureTag tag;
					tag.flags = 0;

					const bool *b = asset->entry->KeyValue<bool>("Wrap.S", P_TARGET_FLAGS(flags));
					if (!b)
						return SR_MetaError;
					tag.flags |= *b ? TextureTag::WrapS : 0;

					b = asset->entry->KeyValue<bool>("Wrap.T", P_TARGET_FLAGS(flags));
					if (!b)
						return SR_MetaError;
					tag.flags |= *b ? TextureTag::WrapT : 0;

					b = asset->entry->KeyValue<bool>("Wrap.R", P_TARGET_FLAGS(flags));
					if (!b)
						return SR_MetaError;
					tag.flags |= *b ? TextureTag::WrapR : 0;

					b = asset->entry->KeyValue<bool>("Mipmap", P_TARGET_FLAGS(flags));
					if (!b)
						return SR_MetaError;
					tag.flags |= *b ? TextureTag::Mipmap : 0;

					b = asset->entry->KeyValue<bool>("Filter", P_TARGET_FLAGS(flags));
					if (!b)
						return SR_MetaError;
					tag.flags |= *b ? TextureTag::Filter : 0;
					
					// Test wrap/mipmap contraints on mobile devices.
					if (tag.flags&(TextureTag::WrapS|TextureTag::WrapT|TextureTag::WrapR|TextureTag::Mipmap)) {
						for (ImageVec::iterator it = m_images.begin(); it != m_images.end(); ++it) {
							image_codec::Image::Ref &image = *it;

							int f;
							for (f = 0; f < image->frameCount; ++f) {
								const image_codec::Frame &frame = image->frames[f];
								
								if (frame.mipCount > 0) {
									const image_codec::Mipmap &mip = frame.mipmaps[0];
									if ((PowerOf2(mip.width) != mip.width) || (PowerOf2(mip.height) != mip.height)) {
										COut(C_Warn) << "Error (Mobile Target): " << asset->path.get() << " is not a power of 2 but is flagged for mipmap/wrap." << std::endl;
										return SR_MetaError;
									}
								}
							}
						}
					}
				}

				if (flags&P_TargetiOS) {
					// iOS GPU's will expand RGB to RGBA on upload (slow).
					// Verified by testing, get warnings in GL profiler in XCode.
					// This should be silent, we don't want artists using RGBA unnecessarily.

					for (ImageVec::iterator it = m_images.begin(); it != m_images.end(); ++it) {
						image_codec::Image::Ref &img = *it;

						if (img->format == image_codec::Format_RGB888) {
							m_header.format = image_codec::Format_RGBA8888;
							
							for (int i = 0; i < img->frameCount; ++i) {
								image_codec::Frame &sf = img->frames[i];
								for (int k = 0; k < sf.mipCount; ++k) {
									image_codec::Mipmap &sm = sf.mipmaps[k];

									U8 *data = (U8*)safe_zone_malloc(image_codec::ZImageCodec, sm.width*sm.height*4);
									image_codec::ConvertPixelData(sm.data, sm.dataSize, data, 0, img->format, image_codec::Format_RGBA8888);
									zone_free(sm.data);
									sm.data = data;
									sm.stride = sm.width*4;
									sm.dataSize = sm.width*sm.height*4;
								}
							}

							img->format = image_codec::Format_RGBA8888;
							img->bpp = 4;
						}
					}
				}
			}

			r = Resize(
				engine,
				time,
				asset,
				flags
			);

			if (r == SR_Success) {
				r = Mipmap(
					engine,
					time,
					asset,
					flags
				);
			}
			
#if defined(RAD_OPT_PC_TOOLS)
			if (r == SR_Success) {
				r = Compress(
					engine,
					time,
					asset,
					flags
				);
			}
#endif
		}
	}

	m_state = (flags&P_Info) ? S_Header : S_Done; // note if r < 0 Process() will set m_state to error.
	m_load = m_state == S_Done;

	m_bufs.clear(); // release file data.

	if (r < 0) {
		m_images.clear();
		STLContainerShrinkToSize(m_images);
	}

	return r;
}

int TextureParser::Format(const char *name) {
	String ext = file::GetFileExtension(name);

	if (ext == ".tga")
		return F_Tga;
	if (ext == ".dds")
		return F_Dds;
	if (ext == ".png")
		return F_Png;
	if (ext == ".bmp")
		return F_Bmp;

	return -1;
}

int TextureParser::Resize(
	Engine &engine,
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if ((flags&P_Unformatted) || (RAD_IMAGECODEC_FAMILY(m_header.format) == image_codec::DDSFamily)) {
		return SR_Success;
	}

	// resize?
	const bool *b = asset->entry->KeyValue<bool>("Resize", P_TARGET_FLAGS(flags));
	if (!b)
		return SR_MetaError;

	if (*b) {
		const int *w = asset->entry->KeyValue<int>("Resize.Width", P_TARGET_FLAGS(flags));
		const int *h = asset->entry->KeyValue<int>("Resize.Height", P_TARGET_FLAGS(flags));

		if (!w || !h || *w < 1 || *h < 1)
			return SR_MetaError;

		if (*w != m_header.width || 
			*h != m_header.height) {
			// format supports resize?
			if (RAD_IMAGECODEC_FAMILY(m_header.format) != image_codec::DDSFamily) {
				if (flags&(P_Load|P_Parse)) {
					GLenum internal, format, type;
					r::GLImageFormat(m_header.format, internal, format, type, GL_ALPHA);

					// Use temp buffer as gluScaleImage corrupts memory (overruns)
					// when non-power-of 2
					// height+2: http://xscreensaver.sourcearchive.com/documentation/5.07-0ubuntu2/grab-ximage_8c-source.html

					void *temp = safe_zone_malloc(
						image_codec::ZImageCodec, 
						PowerOf2(*w)*
						(PowerOf2(*h)+2) *
						m_images[0]->bpp
					);

					for (ImageVec::iterator it = m_images.begin(); it != m_images.end(); ++it) {
						image_codec::Image::Ref &img = *it;
						image_codec::Image src;
						src.Swap(*img);

						img->format = src.format;
						img->bpp = src.bpp;
						img->AllocateFrames(src.frameCount);

						for (int i = 0; i < src.frameCount; ++i) {
							img->AllocateMipmaps(i, 1);
							img->AllocateMipmap(i, 0, *w, *h, (*w)*src.bpp, (*w)*(*h)*src.bpp);

							SizeBuffer a, b;
							FormatSize(a, (AddrSize)(m_header.width*m_header.height*src.bpp));
							FormatSize(b, (AddrSize)(*w)*(*h)*src.bpp);

							{
								r::GLTable::Lock L(r::gl.mutex);

								gluScaleImage(
									format,
									(GLint)m_header.width,
									(GLint)m_header.height,
									type,
									src.frames[i].mipmaps[0].data,
									*w,
									*h,
									type,
									temp
								);
							}

							memcpy(img->frames[i].mipmaps[0].data, temp, img->frames[i].mipmaps[0].dataSize);

							COut(C_Info) << "Resized: " << asset->path.get() << " from (" << 
								m_header.width << "x" << m_header.height << "x" << src.bpp << ") @ " << a << " to (" << *w << "x" << *h << "x" << src.bpp << ") @ " << b << "..." << std::endl;
						}
					}

					zone_free(temp);
				}
				
				m_header.width = *w;
				m_header.height = *h;
				m_header.numMips = 1;
			}
		}
	}

	return SR_Success;
}

int TextureParser::Mipmap(
	Engine &engine,
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {

	if ((flags&P_Unformatted) ||
		(RAD_IMAGECODEC_FAMILY(m_header.format) == image_codec::DDSFamily)) {
		return SR_Success;
	}

	// Mipmap?
	const bool *b = asset->entry->KeyValue<bool>("Mipmap", P_TARGET_FLAGS(flags));
	if (!b)
		return SR_MetaError;

	if (!*b)
		return SR_Success;

	// Compression?
	int compressionMode = GetCompressionMode(asset, flags);
	if (compressionMode < 0)
		return compressionMode;

	if ((compressionMode == kCompressionMode_DXT) ||
		(compressionMode == kCompressionMode_PVR))
		return SR_Success; // libraries generate mipmaps.
			
	int w = m_header.width;
	int h = m_header.height;
	int numMips = 1;

	while (w > kMinMipSize || h > kMinMipSize) {
		if (w > kMinMipSize)
			w >>= 1;
		if (h > kMinMipSize)
			h >>= 1;
		w = std::max<int>(w, kMinMipSize);
		h = std::max<int>(h, kMinMipSize);
		++numMips;
	}
		
	if (m_header.numMips == numMips)
		return SR_Success; // complete mipmaps provided.

	m_header.numMips = numMips;

	if (!(flags&(P_Load|P_Parse)))
		return SR_Success;

	if (numMips > 1) {
		GLenum internal, format, type;
		r::GLImageFormat(m_header.format, internal, format, type, GL_ALPHA);

		// Use temp buffer as gluScaleImage corrupts memory (overruns)
		// when non-power-of 2
		// height+2: http://xscreensaver.sourcearchive.com/documentation/5.07-0ubuntu2/grab-ximage_8c-source.html

		void *temp = safe_zone_malloc(
			image_codec::ZImageCodec, 
			PowerOf2(m_header.width)*
			(PowerOf2(m_header.height)+2) *
			m_images[0]->bpp
		);
			
		for (ImageVec::iterator it = m_images.begin(); it != m_images.end(); ++it) {
			image_codec::Image::Ref &img = *it;
			image_codec::Image src;
			src.Swap(*img);

			img->bpp = src.bpp;
			img->format = src.format;
			img->AllocateFrames(src.frameCount);

			for (int i = 0; i < src.frameCount; ++i) {
				w = m_header.width;
				h = m_header.height;

				if (src.frames[i].mipCount < 1 ||
					src.frames[i].mipmaps[0].width != (UReg)w ||
					src.frames[i].mipmaps[0].height != (UReg)h) {
					return SR_InvalidFormat;
				}

				img->AllocateMipmaps(i, numMips);
				for (int m = 0; m < numMips; ++m) {
					img->AllocateMipmap(i, m, w, h, w*src.bpp, w*h*src.bpp);
						
					if (w == m_header.width && h == m_header.height) {
						memcpy(
							img->frames[i].mipmaps[m].data,
							src.frames[i].mipmaps[0].data,
							w*h*src.bpp
						);
					} else {
						{
							r::GLTable::Lock L(r::gl.mutex);
							gluScaleImage(
								format,
								(GLint)m_header.width,
								(GLint)m_header.height,
								type,
								src.frames[i].mipmaps[0].data,
								w,
								h,
								type,
								temp
							);
						}

						memcpy(
							img->frames[i].mipmaps[m].data,
							temp,
							w*h*src.bpp
						);
					}

					if (w > kMinMipSize)
						w >>= 1;
					if (h > kMinMipSize)
						h >>= 1;

					w = std::max<int>(w, kMinMipSize);
					h = std::max<int>(h, kMinMipSize);
				}
			}
		}

		zone_free(temp);
	}

	return SR_Success;
}

#if defined(RAD_OPT_PC_TOOLS)

int TextureParser::Compress(
	Engine &engine,
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if ((flags&P_Unformatted) || 
		(m_header.format != image_codec::Format_RGB888 &&
		 m_header.format != image_codec::Format_RGBA8888)) {
		return SR_Success;
	}

	int compressionMode = GetCompressionMode(asset, P_TARGET_FLAGS(flags));
	if (compressionMode < 0)
		return compressionMode;

	if (compressionMode == kCompressionMode_Disabled)
		return SR_Success; // no compression.

	// check various constraints so we get errors on any platform:
	bool reqPOW2 = false;

	const String *pvr = asset->entry->KeyValue<String>("Compression.PVR", P_TARGET_FLAGS(flags));
	if (*pvr != "Disabled")
		reqPOW2 = true;

	const String *dxt = asset->entry->KeyValue<String>("Compression.DXT.Mode", P_TARGET_FLAGS(flags));
	if (*dxt != "Disabled")
		reqPOW2 = true;

	const String *atitc = asset->entry->KeyValue<String>("Compression.ATITC", P_TARGET_FLAGS(flags));
	if (*atitc != "Disabled")
		reqPOW2 = true;

	const String *etc = asset->entry->KeyValue<String>("Compression.ETC", P_TARGET_FLAGS(flags));
	if (*etc != "Disabled")
		reqPOW2 = true;

	if (reqPOW2) {
		if (!IsPowerOf2(m_header.width) || 
			!IsPowerOf2(m_header.height)) {
			COut(C_Error) << "Error: " << asset->path.get() << " is flagged for compression but is not a power of two." << std::endl;
			return SR_MetaError;
		}
	
		if ((*pvr != "Disabled") && (m_header.width != m_header.height)) {
			COut(C_Error) << "Error: " << asset->path.get() << " is flagged for PVR compression but is not square (iOS constraint)." << std::endl;
			return SR_MetaError;
		}
	}

	RAD_VERIFY(compressionMode != kCompressionMode_Default);

	switch (compressionMode) {
	case kCompressionMode_DXT:
		return CompressDXT(engine, time, asset, flags);
	case kCompressionMode_PVR:
		return CompressPVR(engine, time, asset, flags);
	}

	return SR_Success;
}

#endif

#endif // defined(RAD_OPT_TOOLS)

void TextureParser::Register(Engine &engine) {
	static pkg::Binding::Ref r = engine.sys->packages->Bind<TextureParser>();
}

} // asset
