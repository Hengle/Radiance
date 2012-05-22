// TextureParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

// It should be noted here that the Radiance DDS library is used here.
// It should *also* be noted that this code *does not compress or decompress DXT formatted data* using
// the Radiance DDS library, rather the code is used to simply unwrap the DDS format around DXT compressed
// blocks.

// DDS files are not written by this code.

// DXT compression, should it be enabled by a texture asset, is performed by the PVRTexLib provided
// by PowerVR. The software used to compress this data is linked as a DLL and is therefore not part
// of the Radiance Engine.

// Therefore the use of this code, unmodified, is not subject to any patents related to S3TC compression 
// formats and licensees of this software are not required to seek an S3TC license.

// Furthermore: All DXT related functionality is not linked into any shipping builds of 
// a game. PVRTexLib, DDS, JPEG, TGA loading is only part of development/tools builds of the engine.

#include "TextureParser.h"
#include "../App.h"
#include "../Engine.h"
#include <Runtime/File.h>

#if defined(RAD_OPT_TOOLS)
#include <Runtime/ImageCodec/Tga.h>
#include <Runtime/ImageCodec/Dds.h>
#include <Runtime/ImageCodec/Bmp.h>
#include <Runtime/ImageCodec/Png.h>
#endif
#if defined(RAD_OPT_PC_TOOLS)
#include <PVRTexLib/PVRTexLib.h>
#include "../Renderer/GL/GLTexture.h"
#include <algorithm>
using namespace pvrtexlib;
#undef min
#undef max
#undef DeleteFile
enum 
{
	MinMipSize = 1
};
#endif

using namespace pkg;

namespace asset {

TextureParser::TextureParser()
: m_state(S_None), m_load(false) {
#if defined(RAD_OPT_TOOLS)
	m_langId = App::Get()->langId;
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
		m_buf.Close();
		return SR_Success;
	}

	if ((flags&P_Cancel)) {
		m_images.clear();
		STLContainerShrinkToSize(m_images);
#if defined(RAD_OPT_TOOLS)
		m_bufs.clear();
		STLContainerShrinkToSize(m_bufs);
#endif
		m_buf.Close();
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
		m_buf.Close();
		m_images.clear();
#if defined(RAD_OPT_TOOLS)
		m_bufs.clear();
#endif
	 	m_state = S_None; // we are fully loading
	}

	int r = SR_Success;

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked && ((flags&(P_Info|P_Parse|P_Unformatted)) || !(flags&P_FastPath))) {
		switch (m_state) {
		case S_None:
			r = Load(engine, time, asset, flags);
			break;
		case S_Loading:
			r = Loading(engine, time, asset, flags);
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

			CookStatus status = m_cooker->Status(0, P_TARGET_FLAGS(flags));

			if (status == CS_Ignore)
				return SR_CompilerError;

			if (status == CS_NeedRebuild) {
				COut(C_Info) << asset->path.get() << " is out of date, rebuilding..." << std::endl;
				int r = m_cooker->Cook(0, P_TARGET_FLAGS(flags));
				if (r != SR_Success)
					return r;
			}
			else {
				COut(C_Info) << asset->path.get() << " is up to date, using cache." << std::endl;
			}

			// Load TAG

			file::HBufferedAsyncIO buf;
			int media = file::AllMedia;
			int r = m_cooker->LoadTag(
				0,
				media,
				buf,
				file::HIONotify()
			);
			if (r < SR_Success)
				return r;
			buf->WaitForCompletion();
			const asset::TextureTag *tag = (const asset::TextureTag*)buf->data->ptr.get();
			m_tag = *tag;

			// Load Texture

			WString path(string::Widen(asset->path));

			if ((m_tag.flags&TextureTag::Localized) && (m_langId != StringTable::LangId_EN)) {
				path += L"_";
				path += string::Widen(StringTable::Langs[m_langId]);
			}

			path += L".bin";

			media = file::AllMedia;
			r = m_cooker->LoadFile( // load cooked data.
				path.c_str(),
				0,
				media,
				m_buf,
				file::HIONotify()
			);

			if (r < SR_Success)
				return r;
		}
		else {
		StringTable::LangId langId = m_langId;
#else
		StringTable::LangId langId = App::Get()->langId;
#endif
		const asset::TextureTag *tag = (const asset::TextureTag*)asset->entry->TagData(P_TARGET_FLAGS(flags));
		if (!tag)
			return SR_MetaError;
		m_tag = *tag;

		// load file...
		WString path(L"Cooked/");
		path += string::Widen(asset->path);

		if ((m_tag.flags&TextureTag::Localized) && (langId != StringTable::LangId_EN)) {
			path += L"_";
			path += string::Widen(StringTable::Langs[langId]);
		}

		path += L".bin";

		int media = file::AllMedia;
		int r = engine.sys->files->LoadFile(
			path.c_str(),
			media,
			m_buf,
			file::HIONotify(),
			8,
			r::ZTextures
		);

		if (r < file::Success)
			return r;
#if defined(RAD_OPT_TOOLS)
		}
#endif
		m_state = S_Loading;
		if (!time.remaining)
			return SR_Pending;
	}

	RAD_ASSERT(m_buf);
	if (m_buf->result == file::Pending) {
		if (time.infinite)
			m_buf->WaitForCompletion();
		else
			return SR_Pending;
	}

	if (m_buf->result < file::Success)
		return m_buf->result;

	const AddrSize size = m_buf->data->size.get();
	const void *data = m_buf->data->ptr.get();
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

		for (UReg f = 0; f < img->frameCount; ++f) {
			image_codec::Frame &frame = img->frames[f];
			CHECK_SIZE(sizeof(U32));
			img->AllocateMipmaps(f, *reinterpret_cast<const U32*>(bytes));
			bytes += sizeof(U32);
			
			CHECK_SIZE(sizeof(U32));
			frame.flags = *reinterpret_cast<const U32*>(bytes);
			frame.flags |= image_codec::SharedFrameFlagRef;
			bytes += sizeof(U32);

			for (UReg m = 0; m < frame.mipCount; ++m) {
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

		m_images.push_back(img);
	}

	m_state = S_Done;
	m_load = true;

#undef CHECK_SIZE

	return SR_Success;
}

#if defined(RAD_OPT_TOOLS)

int TextureParser::Load(
	Engine &engine, 
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {
	const String *name = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));

	if (!name)
		return pkg::SR_MetaError;

	WString wname(string::Widen(name->c_str()));

	const bool *localized = asset->entry->KeyValue<bool>("Localized", P_TARGET_FLAGS(flags));
	if (!localized)
		return pkg::SR_MetaError;

	if (*localized && (m_langId != StringTable::LangId_EN)) {
		// create localized file path.
		wchar_t ext[file::MaxFilePathLen+1];
		wchar_t path[file::MaxFilePathLen+1];

		file::FileExt(wname.c_str(), ext, file::MaxFilePathLen+1);
		file::SetFileExt(wname.c_str(), 0, path, file::MaxFilePathLen+1);
	
		wname = path;
		wname += L"_";
		wname += string::Widen(StringTable::Langs[m_langId]);
		wname += ext;
	}

	int media = file::AllMedia;

	if (!(flags&P_NoDefaultMedia)) {
		if (wname.empty() || !engine.sys->files->FileExists(wname.c_str(), media)) {
			wname = L"Textures/Missing_Texture.tga";
			if (!engine.sys->files->FileExists(wname.c_str(), media)) {
				return pkg::SR_MissingFile;
			}
		}
	}

	m_fmt = Format(wname.c_str());

	if (m_fmt < 0)
		return pkg::SR_InvalidFormat;

	int r;

	if (flags&P_Info) { // read enough to parse header
		file::HFile file;
		file::HBufferedAsyncIO buf;

		r = engine.sys->files->OpenFile(
			wname.c_str(),
			media,
			file,
			file::HIONotify()
		);

		RAD_ASSERT(r <= file::Success);
		if (r == file::Success) {
			buf = engine.sys->files->SafeCreateBufferedIO(Kilo);
			r = file->Read(buf, 0, Kilo, file::HIONotify());
			m_bufs.push_back(buf);
		}
	}
	else
	{
		wchar_t base[file::MaxFilePathLen+1];
		file::FileBaseName(wname.c_str(), base, file::MaxFilePathLen+1);
		if (base[0] == L'+') { // bundle, load all frames simultaneously
			wchar_t ext[file::MaxFilePathLen+1];
			wchar_t path[file::MaxFilePathLen+1];

			file::FilePathName(wname.c_str(), path, file::MaxFilePathLen+1);
			file::FileExt(wname.c_str(), ext, file::MaxFilePathLen+1);

			// skip +digit
			const wchar_t *postDigit = base;
			if (base[1] != 0)
				postDigit += 2;

			for (int i = 0;; ++i)
			{
				file::HBufferedAsyncIO buf;
				wchar_t x[file::MaxFilePathLen+1];

				string::sprintf(x, L"%ls/+%d%ls%ls", path, i, postDigit, ext);

				r = engine.sys->files->LoadFile(
					x,
					media,
					buf,
					file::HIONotify()
				);

				if (r < file::Success)
					break;
				m_bufs.push_back(buf);
			}

			if (!m_bufs.empty() && (r == file::ErrorFileNotFound))
				r = file::Pending;
		}
		else {
			file::HBufferedAsyncIO buf;
			r = engine.sys->files->LoadFile(
				wname.c_str(), 
				media,
				buf,
				file::HIONotify()
			);
			m_bufs.push_back(buf);
		}
	}

	if (r < file::Success) {
		m_bufs.clear();
		return r;
	}

	m_state = S_Loading;
	
	if (time.remaining) {
		r = Loading(engine, time, asset, flags);
	}

	return r;
}

int TextureParser::Loading(
	Engine &engine,
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {
	for (IOVec::const_iterator it = m_bufs.begin(); it != m_bufs.end(); ++it) {
		const file::HBufferedAsyncIO &buf = *it;

		if (buf->result == file::Pending) {
			if (time.infinite) {
				buf->WaitForCompletion();
			}
			else {
				return SR_Pending;
			}
		}

		if (buf->result < file::Success) {
			return buf->result;
		}
	}

	m_state = S_Parsing;
	return time.remaining ? Parsing(engine, time, asset, flags) : SR_Pending;
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
				m_bufs[0]->data->ptr,
				m_bufs[0]->data->size,
				*img
			) ? SR_Success : SR_InvalidFormat;
			break;
		case F_Dds:
			r = image_codec::dds::DecodeHeader(
				m_bufs[0]->data->ptr,
				m_bufs[0]->data->size,
				*img
			) ? SR_Success : SR_InvalidFormat;
			break;
		case F_Png:
			r = image_codec::png::DecodeHeader(
				m_bufs[0]->data->ptr,
				m_bufs[0]->data->size,
				*img
			) ? SR_Success : SR_InvalidFormat;
			break;
		case F_Bmp:
			r = image_codec::bmp::DecodeHeader(
				m_bufs[0]->data->ptr,
				m_bufs[0]->data->size,
				*img
			) ? SR_Success : SR_InvalidFormat;
			break;
		}

		if (r == SR_Success)
			m_images.push_back(img);
	} else {
		for (IOVec::iterator it = m_bufs.begin(); it != m_bufs.end(); ++it) {
			file::HBufferedAsyncIO &buf = *it;
			image_codec::Image::Ref img(new (ZEngine) image_codec::Image(r::ZTextures));

			switch (m_fmt) {
			case F_Tga:
				r = image_codec::tga::Decode(
					buf->data->ptr,
					buf->data->size,
					*img
				) ? SR_Success : SR_InvalidFormat;
				break;
			case F_Dds:
				r = image_codec::dds::Decode(
					buf->data->ptr,
					buf->data->size,
					*img,
					false, // no-ref
					false // no decompress
				) ? SR_Success : SR_InvalidFormat;
				break;
			case F_Png:
				r = image_codec::png::Decode(
					buf->data->ptr,
					buf->data->size,
					*img
				) ? SR_Success : SR_InvalidFormat;
				break;
			case F_Bmp:
				r = image_codec::bmp::Decode(
					buf->data->ptr,
					buf->data->size,
					*img
				) ? SR_Success : SR_InvalidFormat;
				break;
			}

			if (r != SR_Success)
				break;

			m_images.push_back(img);
			buf.Close(); // free data.
		}
	}

	if (r == SR_Success) {
		ImageVec::const_iterator it;
		for (it = m_images.begin(); it != m_images.end(); ++it) {
			const image_codec::Image::Ref &img = *it;
			if (img->frameCount < 1)
				break;
			UReg i;
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
					
					// Test wrap/mipmap contraints on IOS.
					if (tag.flags&(TextureTag::WrapS|TextureTag::WrapT|TextureTag::WrapR|TextureTag::Mipmap)) {
						for (ImageVec::iterator it = m_images.begin(); it != m_images.end(); ++it) {
							image_codec::Image::Ref &image = *it;

							UReg f;
							for (f = 0; f < image->frameCount; ++f) {
								const image_codec::Frame &frame = image->frames[f];
								
								if (frame.mipCount > 0) {
									const image_codec::Mipmap &mip = frame.mipmaps[0];
									if ((PowerOf2(mip.width) != mip.width) || (PowerOf2(mip.height) != mip.height)) {
										COut(C_Warn) << "Error (iOS Target): " << asset->path.get() << " is not a power of 2 but is flagged for mipmap/wrap." << std::endl;
										return SR_MetaError;
									}
								}
							}
						}
					}
				}

				if (flags&P_TargetIOS) {
					bool warn = false;

					for (ImageVec::iterator it = m_images.begin(); it != m_images.end(); ++it) {
						image_codec::Image::Ref &img = *it;

						if (img->format == image_codec::Format_RGB888) {
							if (!warn) {
								warn = true;
								COut(C_Warn) << "Converting: " << asset->path.get() << " expanding RGB texture to RGBA for iOS target." << std::endl;
								
								m_header.format = image_codec::Format_RGBA8888;
							}

							for (UReg i = 0; i < img->frameCount; ++i) {
								image_codec::Frame &sf = img->frames[i];
								for (UReg k = 0; k < sf.mipCount; ++k) {
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
			
			if (r == SR_Success) {
				r = Compress(
					engine,
					time,
					asset,
					flags
				);
			}
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

int TextureParser::Format(const wchar_t *name) {
	wchar_t ext[file::MaxFilePathLen+1];
	file::FileExt(name, ext, file::MaxFilePathLen+1);

	if (!string::icmp(ext, L".tga")) {
		return F_Tga;
	}

	if (!string::icmp(ext, L".dds")) {
		return F_Dds;
	}

	if (!string::icmp(ext, L".png")) {
		return F_Png;
	}

	if (!string::icmp(ext, L".bmp")) {
		return F_Bmp;
	}

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

						for (UReg i = 0; i < src.frameCount; ++i) {
							img->AllocateMipmaps(i, 1);
							img->AllocateMipmap(i, 0, (UReg)*w, (UReg)*h, (*w)*src.bpp, (*w)*(*h)*src.bpp);

							SizeBuffer a, b;
							FormatSize(a, (AddrSize)(m_header.width*m_header.height*src.bpp));
							FormatSize(b, (AddrSize)(*w)*(*h)*src.bpp);

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

	// Compression?
	const String *s = asset->entry->KeyValue<String>("Compression", P_TARGET_FLAGS(flags));

	if (!s)
		return SR_MetaError;

	bool compressed = *s != "None" && (m_header.format == image_codec::Format_RGB888 || 
									   m_header.format == image_codec::Format_RGBA8888);

#if !defined(RAD_OPT_WIN)
	if (!(flags&P_TargetIOS))
		compressed = false;
#endif
	
	if (compressed)
		return SR_Success; // PVRTexLib generates mipmaps
		
	if (*b) {
		int w = m_header.width;
		int h = m_header.height;
		UReg numMips = 1;

		while (w > MinMipSize || h > MinMipSize) {
			if (w > MinMipSize)
				w >>= 1;
			if (h > MinMipSize)
				h >>= 1;
			w = std::max<int>(w, MinMipSize);
			h = std::max<int>(h, MinMipSize);
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

				for (UReg i = 0; i < src.frameCount; ++i) {
					w = m_header.width;
					h = m_header.height;

					if (src.frames[i].mipCount < 1 ||
						src.frames[i].mipmaps[0].width != (UReg)w ||
						src.frames[i].mipmaps[0].height != (UReg)h) {
						return SR_InvalidFormat;
					}

					img->AllocateMipmaps(i, numMips);
					for (UReg m = 0; m < numMips; ++m) {
						img->AllocateMipmap(i, m, w, h, w*src.bpp, w*h*src.bpp);
						
						if (w == m_header.width && h == m_header.height) {
							memcpy(
								img->frames[i].mipmaps[m].data,
								src.frames[i].mipmaps[0].data,
								w*h*src.bpp
							);
						} else {
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

							memcpy(
								img->frames[i].mipmaps[m].data,
								temp,
								w*h*src.bpp
							);
						}

						if (w > MinMipSize)
							w >>= 1;
						if (h > MinMipSize)
							h >>= 1;

						w = std::max<int>(w, MinMipSize);
						h = std::max<int>(h, MinMipSize);
					}
				}
			}

			zone_free(temp);
		}
	}

	return SR_Success;
}

int TextureParser::Compress(
	Engine &engine,
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {
#if defined(RAD_OPT_PC_TOOLS)
	if ((flags&P_Unformatted) || 
		(m_header.format != image_codec::Format_RGB888 &&
		 m_header.format != image_codec::Format_RGBA8888)) {
		return SR_Success;
	}

	// Compression?
	const String *s = asset->entry->KeyValue<String>("Compression", P_TARGET_FLAGS(flags));

	if (!s)
		return SR_MetaError;

	if (*s == "None")
		return SR_Success;
	
	if (!IsPowerOf2(m_header.width) || 
		!IsPowerOf2(m_header.height)) {
		COut(C_Error) << "Error: " << asset->path.get() << " is flagged for DXT/PVR compression but is not a power of two. " << std::endl;
		return SR_MetaError;
	}
	
	if (m_header.width != m_header.height) {
		if (flags&(P_TargetIOS)) {
			COut(C_Error) << "Error: " << asset->path.get() << " is flagged for DXT/PVR compression but is not square. " << std::endl;
			return SR_MetaError;
		}
	}
		
	// Mipmap?
	const bool *b = asset->entry->KeyValue<bool>("Mipmap", P_TARGET_FLAGS(flags));
	if (!b)
		return SR_MetaError;
	
	int numMips = 1;
	
	if (*b) {
		int w = m_header.width;
		int h = m_header.height;
		
		while (w > MinMipSize || h > MinMipSize) {
			if (w > MinMipSize)
				w >>= 1;
			if (h > MinMipSize)
				h >>= 1;
			w = std::max<int>(w, MinMipSize);
			h = std::max<int>(h, MinMipSize);
			++numMips;
		}
	}

	UReg format;
	PixelType pvrFormat=OGL_RGBA_8888;
	bool twiddle = false;
	bool flip = false;

	{
		if (m_images.empty())
			return SR_InvalidFormat;
		const image_codec::Image::Ref &img = m_images[0];

		if (flags&P_TargetIOS) {	
			if (*s == "DXT1/PVR2" ||
				*s == "DXT3/PVR2" ||
				*s == "DXT5/PVR2") {
				twiddle = true;
				pvrFormat = OGL_PVRTC2;
				format = (img->bpp==4) ? image_codec::dds::Format_PVR2A : image_codec::dds::Format_PVR2;
			} else {
				twiddle = true;
				pvrFormat = OGL_PVRTC4;
				format = (img->bpp==4) ? image_codec::dds::Format_PVR4A : image_codec::dds::Format_PVR4;
			}
		} else {
#if !defined(RAD_OPT_WIN)
			COut(C_Warn) << "Warning: " << asset->path.get() << " is flagged for DXT compression but a compressor is not available on this platform. Compression setting ignored." << std::endl;
			return SR_Success;
#endif

			if (*s == "DXT1/PVR2" ||
				*s == "DXT1/PVR4") {
#if defined(RAD_OPT_WIN)
				pvrFormat = D3D_DXT1;
#endif
				format = img->bpp==4 ? image_codec::dds::Format_DXT1A : image_codec::dds::Format_DXT1;
			} else if (*s == "DXT3/PVR2" || *s == "DXT3/PVR4") {
#if defined(RAD_OPT_WIN)
				pvrFormat = D3D_DXT3;
#endif
				format = image_codec::dds::Format_DXT3;
			} else {
#if defined(RAD_OPT_WIN)
				pvrFormat = D3D_DXT5;
#endif
				format = image_codec::dds::Format_DXT5;
			}
		}
	}
	m_header.format = format;
	m_header.numMips = numMips;

	if (!(flags&(P_Load|P_Parse)))
		return SR_Success;

	U8 *temp = 0;

	for (ImageVec::iterator it = m_images.begin(); it != m_images.end(); ++it) {
		image_codec::Image::Ref &img = *it;
		image_codec::Image src;
		src.Swap(*img);
		img->bpp = 0;
		img->format = format;
		
		img->AllocateFrames(src.frameCount);
		for (UReg i = 0; i < src.frameCount; ++i) {
			const image_codec::Frame &sf = src.frames[i];
			if (sf.mipCount < 1)
				continue;
			const image_codec::Mipmap &sm = sf.mipmaps[0];

			switch (format) {
			case image_codec::dds::Format_DXT1:
			case image_codec::dds::Format_DXT1A:
			case image_codec::dds::Format_DXT3:
			case image_codec::dds::Format_DXT5:
			case image_codec::dds::Format_PVR2:
			case image_codec::dds::Format_PVR2A:
			case image_codec::dds::Format_PVR4:
			case image_codec::dds::Format_PVR4A:
				{
					if (!temp)
						temp = (U8*)safe_zone_malloc(image_codec::ZImageCodec, sm.width*sm.height*4);

					U8 *data = (U8*)sm.data;
					if (src.bpp != 4) {
						image_codec::ConvertPixelData(
							sm.data, 
							sm.dataSize, 
							temp, 
							0, 
							src.format, 
							image_codec::Format_RGBA8888
						);
						data = temp;
					}

					PVRTRY {
						PVRTextureUtilities u;
						CPVRTexture pvrSrc(
							(unsigned int)sm.width,
							(unsigned int)sm.height,
							0,
							1,
							false, // border
							twiddle, // twiddle
							false, // cube map
							false, // volume
							false, // false mips
							src.bpp==4, // alpha
							false, // flipped
							DX10_R8G8B8A8_UNORM, // pixel type
							0.f, // normal map scale
							data
						);

						if (numMips > 1) {
							CPVRTextureHeader pvrMip(pvrSrc.getHeader());
							pvrMip.setMipMapCount(numMips-1);
							u.ProcessRawPVR(pvrSrc, pvrMip);
						}

						CPVRTexture pvrDst(pvrSrc.getHeader());
						pvrDst.setPixelType(pvrFormat);

						u.CompressPVR(pvrSrc, pvrDst, 0);
						if (ExtractPVR(engine, i, pvrDst, *img) != SR_Success) {
							if (temp)
								zone_free(temp);
							COut(C_Error) << "PVRTexLib compression failure: failed to extract PVR texture data!" << std::endl;
							return SR_CompilerError;
						}
					}
					PVRCATCH(e) {
						if (temp)
							zone_free(temp);
						COut(C_Error) << "PVRTexLib exception: " << e.what() << std::endl;
						return SR_CompilerError;
					}
				}
				break;
			}
		}

		AddrSize srcSize=0;
		AddrSize dstSize=0;

		for (UReg i = 0; i < src.frameCount; ++i) {
			for (UReg m = 0; m < src.frames[i].mipCount; ++m) {
				srcSize += src.frames[i].mipmaps[m].dataSize;
				dstSize += img->frames[i].mipmaps[m].dataSize;
			}
		}

		SizeBuffer sa, sb;
		FormatSize(sa, srcSize);
		FormatSize(sb, dstSize);

		COut(C_Info) << "Compressed " << asset->path.get() << " (" << 
			m_header.width << "x" << m_header.height << "x" << src.bpp << ") @ " << sa << " to (" << 
			img->frames[0].mipmaps[0].width << "x" << img->frames[0].mipmaps[0].height << ") @ " << sb << " as " << *s << std::endl;
	}

	if (temp)
		zone_free(temp);

#endif // defined(RAD_OPT_PC_TOOLS)
	return SR_Success;
}
	
#if defined(RAD_OPT_PC_TOOLS)
namespace {
enum 
{
	PVR_MGLPVR2 = 0xc,
	PVR_OGLPVR2 = 0x18,
	PVR_MGLPVR4 = 0xd,
	PVR_OGLPVR4 = 0x19,
	PVR_DXT1 = 0x20,
	PVR_DXT2,
	PVR_DXT3,
	PVR_DXT4,
	PVR_DXT5
};	
} // namespace
	
int TextureParser::ExtractPVR(
	Engine &engine,
	int frame,
	CPVRTexture &src,
	image_codec::Image &img
) {
	const U8 *data = (const U8*)src.getData().getData();
	AddrSize len = (AddrSize)src.getData().getDataSize();

	bool hasMips = src.hasMips() && src.getMipMapCount()>0;
	bool hasAlpha = src.hasAlpha();
	
#if defined(RAD_OPT_DEBUG)
	{
		UReg format=0;

		// What format?
		switch (src.getPixelType()) {
			case PVR_MGLPVR2:
			case PVR_OGLPVR2:
				format = (hasAlpha) ? image_codec::dds::Format_PVR2A : image_codec::dds::Format_PVR2;
				break;
			case PVR_MGLPVR4:
			case PVR_OGLPVR4:
				format = (hasAlpha) ? image_codec::dds::Format_PVR4A : image_codec::dds::Format_PVR4;
				break;
			case PVR_DXT1:
			case PVR_DXT2:
				format = (hasAlpha) ? image_codec::dds::Format_DXT1A : image_codec::dds::Format_DXT1;
				break;
			case PVR_DXT3:
			case PVR_DXT4:
				format = image_codec::dds::Format_DXT3;
				break;
			case PVR_DXT5:
				format = image_codec::dds::Format_DXT5;
				break;
		}
		RAD_ASSERT(img.format==format);
	}
#endif
	
	UReg numMips = (UReg)src.getMipMapCount()+1;
	img.AllocateMipmaps(frame, numMips);
	
	UReg w = (UReg)src.getWidth();
	UReg h = (UReg)src.getHeight();
	
	for (UReg m = 0; m < numMips; ++m) {
		image_codec::Mipmap &mip = img.frames[frame].mipmaps[m];
		AddrSize blockSize = 0;
		
		// What format?
		switch (src.getPixelType()) {
			case PVR_MGLPVR2:
			case PVR_OGLPVR2:
				blockSize = std::max<AddrSize>(w/8,2)*std::max<AddrSize>(h/4,2)*8;
				break;
			case PVR_MGLPVR4:
			case PVR_OGLPVR4:
				blockSize = std::max<AddrSize>(w/4,2)*std::max<AddrSize>(h/4,2)*8;
				break;
			case PVR_DXT1:
			case PVR_DXT2:
				blockSize = std::max<AddrSize>((w*h/16)*8, 8);
				break;
			case PVR_DXT3:
			case PVR_DXT4:
			case PVR_DXT5:
				blockSize = std::max<AddrSize>((w*h/16)*16, 16);
				break;
		}
		
		img.AllocateMipmap(frame, m, w, h, 0, blockSize);
		memcpy(mip.data, data, blockSize);
		data += blockSize;
		w = (UReg)std::max<UReg>(w>>1, 1);
		h = (UReg)std::max<UReg>(h>>1, 1);
	}

	return SR_Success;
}
#endif

#endif // defined(RAD_OPT_TOOLS)

void TextureParser::Register(Engine &engine) {
	static pkg::Binding::Ref r = engine.sys->packages->Bind<TextureParser>();
}

} // asset
