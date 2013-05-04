/*! \file TextureParserDXT.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "TextureParser.h"
#include <Runtime/ImageCodec/Dds.h>
#include <Runtime/Stream/MemoryStream.h>
#include <nvtt/nvtt.h>
#include <algorithm>

/*

 The NVidia Texture Tools are used, unmodified as a DLL linked against this application. No source code from NVTT
 is compiled into any products using this file. Furthermore this file is only linked into development builds of
 the engine, shipping games do not include this code.

 https://code.google.com/p/nvidia-texture-tools/wiki/FAQ

 Can I use the NVIDIA Texture Tools in the US? Do I have to obtain a license of the S3TC patent (US patent 5,956,431)?
       
	   NVIDIA has a license of the S3TC patent that covers all our products, including our Texture Tools. 
	   You don't have to obtain a license of the S3TC patent to use any of NVIDIA's products, 
	   but certain uses of NVIDIA Texture Tools source code cannot be considered NVIDIA products anymore. 
	   Keep in mind that the NVIDIA Texture Tools are licensed under the MIT license and thus are provided without 
	   warranty of any kind.
*/

enum {
	kMinMipSize = 1
};

class nvttOutputImage : public nvtt::OutputHandler {
public:

	nvttOutputImage(image_codec::Image &output, int frame) : m_img(output), m_frame(frame), m_ofs(0), m_mip(0) {
		RAD_ASSERT(frame < output.frameCount); // must be preallocated!
	}

	virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) {
		m_ofs = 0;
		m_mip = miplevel;
		RAD_ASSERT(m_mip < m_img.frames[m_frame].mipCount);
		m_img.AllocateMipmap(m_frame, miplevel, width, height, 0, size);
	}

	virtual bool writeData(const void *data, int size) {
		RAD_ASSERT(m_ofs+size <= (int)m_img.frames[m_frame].mipmaps[m_mip].dataSize);
		U8 *dst = (U8*)m_img.frames[m_frame].mipmaps[m_mip].data;
		memcpy(dst+m_ofs, data, size);
		m_ofs += size;
		return true;
	}

private:

	int m_ofs;
	int m_mip;
	int m_frame;
	image_codec::Image &m_img;
};

using namespace pkg;

namespace asset {

int TextureParser::CompressDXT(
	Engine &engine,
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {

	// Compression mode
	const String *mode = asset->entry->KeyValue<String>("Compression.DXT.Mode", P_TARGET_FLAGS(flags));
	if (!mode)
		return SR_MetaError;
	if (*mode == "Disabled")
		return SR_Success;

	const String *border = asset->entry->KeyValue<String>("Compression.DXT.Border", P_TARGET_FLAGS(flags));
	if (!border)
		return SR_MetaError;

	const String *filter = asset->entry->KeyValue<String>("Compression.DXT.Mipmap.Filter", P_TARGET_FLAGS(flags));
	if (!filter)
		return SR_MetaError;

	const String *kaiser = asset->entry->KeyValue<String>("Compression.DXT.Mipmap.Kaiser.Width", P_TARGET_FLAGS(flags));
	if (!kaiser)
		return SR_MetaError;

	float kaiserWidth;
	sscanf(kaiser->c_str, "%f", &kaiserWidth);

	const String *imgType = asset->entry->KeyValue<String>("Compression.ImageType", P_TARGET_FLAGS(flags));
	if (!imgType)
		return SR_MetaError;

	const bool kNormalMap = *imgType == "NormalMap";
	
	// Mipmap?
	const bool *mipmap = asset->entry->KeyValue<bool>("Mipmap", P_TARGET_FLAGS(flags));
	if (!mipmap)
		return SR_MetaError;
	
	int numMips = 1;
	
	if (*mipmap) {
		int w = m_header.width;
		int h = m_header.height;
		
		while (w > kMinMipSize || h > kMinMipSize) {
			if (w > kMinMipSize)
				w >>= 1;
			if (h > kMinMipSize)
				h >>= 1;
			w = std::max<int>(w, kMinMipSize);
			h = std::max<int>(h, kMinMipSize);
			++numMips;
		}
	}

	int format;
	int bpp;
	nvtt::Format nvttFormat;
	
	{
		if (m_images.empty())
			return SR_InvalidFormat;
		const image_codec::Image::Ref &img = m_images[0];

		bpp = img->bpp;

		if (*mode == "DXT5") {
			if (kNormalMap) {
				nvttFormat = nvtt::Format_DXT5;
				format = image_codec::dds::Format_DXT5;
			} else {
				nvttFormat = (img->bpp==4) ? nvtt::Format_DXT5 : nvtt::Format_DXT1;
				format = (img->bpp==4) ? image_codec::dds::Format_DXT5 : image_codec::dds::Format_DXT1;
			}
		} else {
			if (kNormalMap) {
				nvttFormat = nvtt::Format_DXT1;
				format = image_codec::dds::Format_DXT1;
			} else {
				nvttFormat = (img->bpp==4) ? nvtt::Format_DXT3 : nvtt::Format_DXT1;
				format = (img->bpp==4) ? image_codec::dds::Format_DXT3 : image_codec::dds::Format_DXT1;
			}
		}
	}

	String strFormat;
	if (nvttFormat == nvtt::Format_DXT1) {
		if (kNormalMap) {
			strFormat = "DXT1n";
		} else {
			strFormat = "DXT1";
		}
	} else if(nvttFormat == nvtt::Format_DXT5) {
		if (kNormalMap) {
			strFormat = "DXT5n";
		} else {
			strFormat = "DXT5";
		}
	} else {
		RAD_ASSERT(!kNormalMap);
		RAD_ASSERT(nvttFormat == nvtt::Format_DXT3);
		strFormat = "DXT3";
	}

	m_header.format = format;
	m_header.numMips = numMips;

	if (!(flags&(P_Load|P_Parse)))
		return SR_Success;

	U8 *temp = 0;

	String sQuality(CStr("HQ"));
	if (flags&P_FastCook)
		sQuality = CStr("LQ");

	nvtt::Compressor compressor;
	compressor.enableCudaAcceleration(true);

	nvtt::CompressionOptions compressionOptions;
	
	if (flags&P_FastCook) { 
		compressionOptions.setQuality(nvtt::Quality_Fastest);
	} else {
		compressionOptions.setQuality(nvtt::Quality_Highest);
	}
		
	for (ImageVec::iterator it = m_images.begin(); it != m_images.end(); ++it) {
		image_codec::Image::Ref &img = *it;
		image_codec::Image src;
		src.Swap(*img);
		img->bpp = 0;
		img->format = format;

		COut(C_Info) << "Compressing " << asset->path.get() << " (" << 
			m_header.width << "x" << m_header.height << "x" << src.bpp << ") as " << sQuality << " " << strFormat << std::endl;
				
		img->AllocateFrames(src.frameCount);
		for (int i = 0; i < src.frameCount; ++i) {
			const image_codec::Frame &sf = src.frames[i];
			if (sf.mipCount < 1)
				continue;
			const image_codec::Mipmap &sm = sf.mipmaps[0];

			if (!temp)
				temp = (U8*)safe_zone_malloc(image_codec::ZImageCodec, sm.width*sm.height*4);

			if (src.format != image_codec::Format_BGRA8888) {
				// nvtt only takes BGRA8888
				image_codec::ConvertPixelData(
					sm.data, 
					sm.dataSize, 
					temp, 
					0, 
					src.format, 
					image_codec::Format_BGRA8888
				);
			}

			// not using nvtt normal map functions to exercise code shared
			// between compression paths.

			img->AllocateMipmaps(i, numMips);

			nvtt::InputOptions inputTexture;
			inputTexture.setTextureLayout(
				nvtt::TextureType_2D,
				sm.width,
				sm.height
			);
			inputTexture.setMipmapGeneration(*mipmap);
			inputTexture.setAlphaMode(((bpp == 4) && !kNormalMap) ? nvtt::AlphaMode_Transparency : nvtt::AlphaMode_None);
			
			if (*border == "Clamp") {
				inputTexture.setWrapMode(nvtt::WrapMode_Clamp);
			} else if (*border == "Wrap") {
				inputTexture.setWrapMode(nvtt::WrapMode_Repeat);
			} else {
				inputTexture.setWrapMode(nvtt::WrapMode_Mirror);
			}

			if (kNormalMap) {

				nvtt::InputOptions workTexture;
				workTexture.setTextureLayout(
					nvtt::TextureType_2D,
					sm.width,
					sm.height
				);

				workTexture.setMipmapData(
					temp,
					sm.width,
					sm.height
				);

				workTexture.setMipmapGeneration(*mipmap);
				if (*mipmap) {
					if ((*filter == "Box") || (flags&P_FastCook)) { // forces box filter on
						workTexture.setMipmapFilter(nvtt::MipmapFilter_Box);
					} else if (*filter == "Triangle") {
						workTexture.setMipmapFilter(nvtt::MipmapFilter_Triangle);
					} else {
						workTexture.setMipmapFilter(nvtt::MipmapFilter_Kaiser);
						workTexture.setKaiserParameters(kaiserWidth, 4.f, 1.f);
					}
				}

				nvttOutputImage outputWriter(*img, i);

				nvtt::OutputOptions outputOptions;
				outputOptions.setOutputHeader(false);
				outputOptions.setOutputHandler(&outputWriter);

				compressionOptions.setFormat(nvtt::Format_RGBA);

				if (!compressor.process(workTexture, compressionOptions, outputOptions)) {
					COut(C_Error) << "NVTT failure: unable to mipmap normalmap texture!" << std::endl;
					return SR_CompilerError;
				}

				// img is full of mipmapped normal data
				// normalize and swizzle into appropriate channels for DXT
				image_codec::Frame &frame = img->frames[i];

				for (int k = 0; k < frame.mipCount; ++k) {
					image_codec::Mipmap &m = frame.mipmaps[k];

					int flags = kGenNormalMapFlag_BGRA;
					if (nvttFormat == nvtt::Format_DXT1) {
						flags |= kGenNormalMapFlag_DXT1n;
					} else {
						RAD_ASSERT(nvttFormat == nvtt::Format_DXT5);
						flags |= kGenNormalMapFlag_DXT5n;
					}

					NormalizeNormalMap(m.data, m.width, m.height, 4, flags);
					SwizzleNormalMap(m.data, m.width, m.height, 4, flags);
					
					inputTexture.setMipmapData(
						m.data,
						m.width,
						m.height,
						1,
						0,
						k
					);

					zone_free(m.data);
				}

			} else {
				inputTexture.setMipmapData(
					temp,
					sm.width,
					sm.height
				);

				if (*mipmap) {
					if ((*filter == "Box") || (flags&P_FastCook)) { // forces box filter on
						inputTexture.setMipmapFilter(nvtt::MipmapFilter_Box);
					} else if (*filter == "Triangle") {
						inputTexture.setMipmapFilter(nvtt::MipmapFilter_Triangle);
					} else {
						inputTexture.setMipmapFilter(nvtt::MipmapFilter_Kaiser);
						inputTexture.setKaiserParameters(kaiserWidth, 4.f, 1.f);
					}
				}
			}

			compressionOptions.setFormat(nvttFormat);

			inputTexture.setRoundMode(nvtt::RoundMode_None);

			nvttOutputImage outputWriter(*img, i);

			nvtt::OutputOptions outputOptions;
			outputOptions.setOutputHeader(false);
			outputOptions.setOutputHandler(&outputWriter);

			if (!compressor.process(inputTexture, compressionOptions, outputOptions)) {
				COut(C_Error) << "NVTT failure: unable to compress texture!" << std::endl;
				return SR_CompilerError;
			}
		}

		AddrSize srcSize=0;
		AddrSize dstSize=0;

		for (int i = 0; i < src.frameCount; ++i) {
			for (int m = 0; m < src.frames[i].mipCount; ++m) {
				srcSize += src.frames[i].mipmaps[m].dataSize;
				dstSize += img->frames[i].mipmaps[m].dataSize;
			}
		}

		SizeBuffer sa, sb;
		FormatSize(sa, srcSize);
		FormatSize(sb, dstSize);

		COut(C_Info) << "Compressed " << asset->path.get() << " (" << 
			m_header.width << "x" << m_header.height << "x" << src.bpp << ") @ " << sa << " to (" << 
			img->frames[0].mipmaps[0].width << "x" << img->frames[0].mipmaps[0].height << ") @ " << sb << " as " << strFormat << std::endl;
	}

	if (temp)
		zone_free(temp);

	return SR_Success;
}

} // asset
