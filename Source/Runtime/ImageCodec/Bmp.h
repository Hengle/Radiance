// Bmp.h
// Bmp Decode/Encode
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "ImageCodec.h"
#include "../PushPack.h"


namespace image_codec {
namespace bmp {

//
// Supports encode/decode of 8 bit raw or RLE, and 24 bit uncompressed images.

//////////////////////////////////////////////////////////////////////////////////////////
// BMP Decode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL DecodeHeader(const void *buff, AddrSize buffLength, Image &out);
RADRT_API bool RADRT_CALL Decode(const void *buff, AddrSize buffLength, Image &out);

//////////////////////////////////////////////////////////////////////////////////////////
// BMP Encode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL Encode(const Image &in, const Mipmap &mip, bool compressRLE, void *&outData, AddrSize &outSize);

} // bmp
} // image_codec


#include "../PopPack.h"
