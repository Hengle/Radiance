// ZLib.h
// Inflate / Deflate Routines.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntDataCodec.h"
#include "../StreamDef.h"
#include "../PushPack.h"


namespace data_codec {
namespace zlib {

enum
{
	NoCompression = 0,
	FastestCompression = 1,
	BestCompression = 9
};

class RADRT_CLASS Encoder
{
public:
	Encoder();
	~Encoder();

	bool Begin(UReg compression);
	//
	// srcLen and dstLen must be filled in by caller, and are updated
	// by Encode with the # of bytes consumed from each stream.
	//
	bool Encode(const void *src, AddrSize &srcLen, void *dst, AddrSize &dstLen);
	//
	// End() must be called in a loop to flush the encoder and write the remaining output
	// bytes. The encode is complete when the function returns without consuming the entire
	// destination buffer.
	//
	bool End(void *dst, AddrSize &dstLen);

private:

	void *m_zStream;
	bool m_end;
};

class RADRT_CLASS Decoder
{
public:
	Decoder();
	~Decoder();

	bool Begin();
	//
	// srcLen and dstLen must be filled in by caller, and are updated
	// by Decode with the # of bytes consumed from each stream.
	//
	bool Decode(const void *src, AddrSize &srcLen, void *dst, AddrSize &dstLen);
	//
	// End() must be called in a loop to flush the decoder and write the remaining output
	// bytes. The decode is complete when the function returns without consuming the entire
	// destination buffer.
	//
	bool End(void *dst, AddrSize &dstLen);

private:

	void *m_zStream;
	bool m_end;
};

RADRT_API AddrSize RADRT_CALL PredictEncodeSize(AddrSize dataSize);

// use PredictEncodeSize to allocate the out buffer for Encode(). Caller must fill in size
// of out buffer in outSize variable before call. Upon return the # of bytes written to out
// buffer are returned in outSize.
RADRT_API bool RADRT_CALL Encode(const void *data, AddrSize dataSize, UReg compression, void *out, AddrSize *outSize);

RADRT_API bool RADRT_CALL StreamEncode(stream::InputStream &in, stream::OutputStream &out, UReg compression);
RADRT_API bool RADRT_CALL Decode(const void* data, AddrSize dataSize, void* out, AddrSize outSize);
RADRT_API bool RADRT_CALL StreamDecode(stream::InputStream &in, stream::OutputStream &out);

} // zlib
} // data_codec


#include "../PopPack.h"
