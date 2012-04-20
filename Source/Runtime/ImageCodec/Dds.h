// Dds.h
// DDS Decode/Encode
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ImageCodec.h"
#include "../PushPack.h"


namespace image_codec {
namespace dds {

#define RAD_IMAGECODEC_DDSFORMAT(type) RAD_IMAGECODEC_IMAGEFORMAT(DDSFamily, type)

enum
{
	_Format_DXT1, // opaque or 1 bit alpha. decompress: RGBA8888
	_Format_DXT1A, // Not used by encode/decode, just to distinguish from opaque DXT1.
	_Format_DXT3, // explicit alpha. decompress: RGBA8888
	_Format_DXT5, // interpolated alpha. decompress: RGBA8888
	_Format_FP16, // 16 bit floating point.
	_Format_FP32, // 32 bit floating point.
	_Format_FP416, // 4 channel 16 bit floating point
	_Format_FP432, // 4 channel 32 bit floating point
	_Format_S8888, // Signed 8 bits per channel (4 channels).
	_Format_PVR2, // PowerVR 2BPP
	_Format_PVR2A, // PowerVR 2BPP+A
	_Format_PVR4, // PowerVR 4BPP
	_Format_PVR4A, // PowerVR 4BPP+A

	Format_DXT1 = RAD_IMAGECODEC_DDSFORMAT(_Format_DXT1),
	Format_DXT1A = RAD_IMAGECODEC_DDSFORMAT(_Format_DXT1A),
	Format_DXT3 = RAD_IMAGECODEC_DDSFORMAT(_Format_DXT3),
	Format_DXT5 = RAD_IMAGECODEC_DDSFORMAT(_Format_DXT5),

	// These formats are not supported by the image library,
	// they are used as extension formats to support PVR.
	Format_PVR2  = RAD_IMAGECODEC_DDSFORMAT(_Format_PVR2),
	Format_PVR2A = RAD_IMAGECODEC_DDSFORMAT(_Format_PVR2A),
	Format_PVR4  = RAD_IMAGECODEC_DDSFORMAT(_Format_PVR4),
	Format_PVR4A = RAD_IMAGECODEC_DDSFORMAT(_Format_PVR4A),

	// NOTE: These formats are not supported by pixel conversion routines!
	Format_FP16 = RAD_IMAGECODEC_DDSFORMAT(_Format_FP16),
	Format_FP32 = RAD_IMAGECODEC_DDSFORMAT(_Format_FP32),
	Format_FP416 = RAD_IMAGECODEC_DDSFORMAT(_Format_FP416),
	Format_FP432 = RAD_IMAGECODEC_DDSFORMAT(_Format_FP432),
	Format_S8888 = RAD_IMAGECODEC_DDSFORMAT(_Format_S8888),

	RAD_FLAG_BIT(EncodeFlagCubemap, 0),
	RAD_FLAG(EncodeFlagMipmaps)
};

enum
{
	RAD_FLAG_BIT(FrameFlagCubemapPositiveX, FirstUserFrameBit),
	RAD_FLAG(FrameFlagCubemapNegativeX),
	RAD_FLAG(FrameFlagCubemapPositiveY),
	RAD_FLAG(FrameFlagCubemapNegativeY),
	RAD_FLAG(FrameFlagCubemapPositiveZ),
	RAD_FLAG(FrameFlagCubemapNegativeZ)
};

//////////////////////////////////////////////////////////////////////////////////////////
// DDS Decode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL DecodeHeader(const void *buff, AddrSize buffLength, Image &out);
RADRT_API bool RADRT_CALL Decode(const void *buff, AddrSize buffLength, Image &out, bool refSrcData, bool decompress);

//////////////////////////////////////////////////////////////////////////////////////////
// DDS Encode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL Encode(const Image &in, UReg encodeFormat, UReg encodeFlags, void *&outData, AddrSize &outSize);

//////////////////////////////////////////////////////////////////////////////////////////
// DXT Encode/Decode
//////////////////////////////////////////////////////////////////////////////////////////

// DXT Decode always decompresses to RGBA8888
RADRT_API bool RADRT_CALL DXTDecode(const void* dxtBuff, AddrSize dxtBuffLength, UReg dxtFormat, UReg width, UReg height, void* outBuff);
RADRT_API bool RADRT_CALL DXTEncode(const void* src, AddrSize srcLength, UReg srcFormat, UReg dstFormat, UReg width, UReg height, void* outBuff);
RADRT_API AddrSize RADRT_CALL DXTEncodeSize(UReg format, UReg width, UReg height);

} // dds
} // image_codec


#include "../PopPack.h"
