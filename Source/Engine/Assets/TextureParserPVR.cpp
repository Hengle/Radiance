/*! \file TextureParserPVR.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "TextureParser.h"
#include <Runtime/ImageCodec/Dds.h>

#if !defined(RAD_OPT_PC_TOOLS)
#error "This file is only for pc tools builds!"
#endif

#if defined(RAD_OPT_WIN)
#define _WINDLL_IMPORT 1
#endif

#include <PVRTexLib/PVRTTexture.h>
#include <PVRTexLib/PVRTextureHeader.h>
#include <PVRTexLib/PVRTextureUtilities.h>
#include <algorithm>

using namespace pvrtexture;
#undef min
#undef max
#undef DeleteFile

enum {
	kMinMipSize = 1
};

using namespace pkg;

namespace asset {

int TextureParser::CompressPVR(
	Engine &engine,
	const xtime::TimeSlice &time,
	const pkg::Asset::Ref &asset,
	int flags
) {
	// Compression mode
	const String *s = asset->entry->KeyValue<String>("Compression.PVR", P_TARGET_FLAGS(flags));

	if (!s)
		return SR_MetaError;

	if (*s == "Disabled")
		return SR_Success;
	
	// Mipmap?
	const bool *b = asset->entry->KeyValue<bool>("Mipmap", P_TARGET_FLAGS(flags));
	if (!b)
		return SR_MetaError;

	const String *imgType = asset->entry->KeyValue<String>("Compression.ImageType", P_TARGET_FLAGS(flags));
	if (!imgType)
		return SR_MetaError;

	const bool kNormalMap = *imgType == "NormalMap";
	
	int numMips = 1;
	
	if (*b) {
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
	EPVRTPixelFormat pvrFormat;
	
	{
		if (m_images.empty())
			return SR_InvalidFormat;
		const image_codec::Image::Ref &img = m_images[0];

		if (*s == "PVR2") {
			if (kNormalMap) {
			} else {
				pvrFormat = (img->bpp==4) ? ePVRTPF_PVRTCI_2bpp_RGBA : ePVRTPF_PVRTCI_2bpp_RGB;
				format = (img->bpp==4) ? image_codec::dds::Format_PVR2A : image_codec::dds::Format_PVR2;
			}
		} else {
			if (kNormalMap) {
			} else {
				pvrFormat = (img->bpp==4) ? ePVRTPF_PVRTCI_4bpp_RGBA : ePVRTPF_PVRTCI_4bpp_RGB;
				format = (img->bpp==4) ? image_codec::dds::Format_PVR4A : image_codec::dds::Format_PVR4;
			}
		}
	}

	String strFormat;
	if (pvrFormat == ePVRTPF_PVRTCI_2bpp_RGB) {
		if (kNormalMap) {
			strFormat = "PVR2n";
		} else {
			strFormat = "PVR2";
		}
	} else if(pvrFormat == ePVRTPF_PVRTCI_4bpp_RGB) {
		if (kNormalMap) {
			strFormat = "PVR4n";
		} else {
			strFormat = "PVR4";
		}
	} else if (pvrFormat == ePVRTPF_PVRTCI_2bpp_RGBA) {
		RAD_ASSERT(!kNormalMap);
		strFormat = "PVR2a";
	} else {
		RAD_ASSERT(!kNormalMap);
		RAD_ASSERT(pvrFormat == ePVRTPF_PVRTCI_4bpp_RGBA);
		strFormat = "PVR4a";
	}

	m_header.format = format;
	m_header.numMips = numMips;

	if (!(flags&(P_Load|P_Parse)))
		return SR_Success;

	U8 *temp = 0;

	String sQuality(CStr("HQ"));
	if (flags&P_FastCook)
		sQuality = CStr("LQ");

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

			CPVRTextureHeader pvrHeader(
				(uint64)PVRStandard8PixelType.PixelTypeID,
				(uint32)sm.width,
				(uint32)sm.height,
				(uint32)1,
				(uint32)1,
				(uint32)1,
				(uint32)1,
				ePVRTCSpacelRGB,
				ePVRTVarTypeUnsignedByteNorm,
				false // not-premultiplied.
			);

			CPVRTexture pvrTex(
				pvrHeader,
				data
			);

			if (numMips > 1) {
				if (!GenerateMIPMaps(pvrTex, eResizeCubic, numMips)) {
					COut(C_Error) << "PVRTexLib failure: failed to create mimaps!" << std::endl;
					return SR_CompilerError;
				}
			}

			if (kNormalMap) {
				// normalize and swizzle normal map data.

				const U8 *data = (const U8*)pvrTex.getDataPtr();
				const CPVRTextureHeader &header = pvrTex.getHeader();

				int z = header.getNumMIPLevels();

				int w = (int)pvrTex.getWidth();
				int h = (int)pvrTex.getHeight();

				for (int k = 0; k < z; ++k) {

					NormalizeNormalMap((void*)data, w, h, 4, 0);
					SwizzleNormalMap((void*)data, w, h, 4, kGenNormalMapFlag_DXT1n);

					data += w*h*4;

					w >>= 1;
					h >>= 1;

					if (w < 1)
						w = 1;
					if (h < 1)
						h = 1;
				}
			}

			ECompressorQuality quality = ePVRTCBest;
			if (flags&P_FastCook)
				quality = ePVRTCFast;

			if (!Transcode(pvrTex, pvrFormat, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, quality, false)) {
				COut(C_Error) << "PVRTexLib failure: failed to compress texture!" << std::endl;
				return SR_CompilerError;
			}

			if (ExtractPVR(engine, i, pvrTex, *img) != SR_Success) {
				if (temp)
					zone_free(temp);
				COut(C_Error) << "PVRTexLib compression failure: failed to extract PVR texture data!" << std::endl;
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
			img->frames[0].mipmaps[0].width << "x" << img->frames[0].mipmaps[0].height << ") @ " << sb << " as " << *s << std::endl;
	}

	if (temp)
		zone_free(temp);

	return SR_Success;
}
	
int TextureParser::ExtractPVR(
	Engine &engine,
	int frame,
	CPVRTexture &src,
	image_codec::Image &img
) {
	const U8 *data = (const U8*)src.getDataPtr();
	const CPVRTextureHeader &header = src.getHeader();

	int numMips = header.getNumMIPLevels();
		
	img.AllocateMipmaps(frame, numMips);
	
	int w = (int)src.getWidth();
	int h = (int)src.getHeight();
	
	for (int m = 0; m < numMips; ++m) {
		image_codec::Mipmap &mip = img.frames[frame].mipmaps[m];
		AddrSize blockSize = 0;
		
		// What format?
		switch (src.getPixelType().PixelTypeID) {
			case ePVRTPF_PVRTCI_2bpp_RGB:
			case ePVRTPF_PVRTCI_2bpp_RGBA:
				blockSize = std::max<AddrSize>(w/8,2)*std::max<AddrSize>(h/4,2)*8;
				break;
			case ePVRTPF_PVRTCI_4bpp_RGB:
			case ePVRTPF_PVRTCI_4bpp_RGBA:
				blockSize = std::max<AddrSize>(w/4,2)*std::max<AddrSize>(h/4,2)*8;
				break;
			/*case ePVRTPF_DXT1:
			case ePVRTPF_DXT2:
				blockSize = std::max<AddrSize>((w*h/16)*8, 8);
				break;
			case ePVRTPF_DXT3:
			case ePVRTPF_DXT4:
			case ePVRTPF_DXT5:
				blockSize = std::max<AddrSize>((w*h/16)*16, 16);
				break;*/
		}
		
		img.AllocateMipmap(frame, m, w, h, 0, blockSize);
		memcpy(mip.data, data, blockSize);
		data += blockSize;
		w = std::max(w>>1, 1);
		h = std::max(h>>1, 1);
	}

	return SR_Success;
}

} // asset
