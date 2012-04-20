// Bmp.h
// Bmp Decode/Encode
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ImageCodec.h"
#include "../PushPack.h"


namespace image_codec {
namespace png {

//////////////////////////////////////////////////////////////////////////////////////////
// PNG Decode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL DecodeHeader(const void *buff, AddrSize buffLength, Image &out);
RADRT_API bool RADRT_CALL Decode(const void *buff, AddrSize buffLength, Image &out);

//////////////////////////////////////////////////////////////////////////////////////////
// PNG Encode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL Encode(const Image &in, const Mipmap &mip, bool interlaced, void *&outData, AddrSize &outSize);

} // png
} // image_codec


#include "../PopPack.h"
