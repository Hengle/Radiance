// Jpg.h
// Jpg Decode/Encode
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ImageCodec.h"
#include "../PushPack.h"


namespace image_codec {
namespace jpg {

//////////////////////////////////////////////////////////////////////////////////////////
// JPEG Decode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL DecodeHeader(const void *buff, AddrSize buffLength, Image &out);
RADRT_API bool RADRT_CALL Decode(const void *buff, AddrSize buffLength, bool highQualityUpsampling, bool interBlockSmoothing, Image &out);

//////////////////////////////////////////////////////////////////////////////////////////
// JPEG Encode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL Encode(const Image &in, const Mipmap &mip, float quality_ZeroToOne, void *&outData, AddrSize &outSize);

} // jpg
} // image_codec


#include "../PopPack.h"
