// ZLib.cpp
// Inflate / Deflate Routines.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "ZLib.h"
#include "../Utils.h"
#include "../Stream.h"
#include "zlib123/zlib_original.h"
#include <stdlib.h>

using namespace stream;


namespace data_codec {
namespace zlib {

RAD_ZONE_DEC(RADRT_API, ZZLib);
RAD_ZONE_DEF(RADRT_API, ZZLib, "ZLib", ZRuntime);

RADRT_API AddrSize RADRT_CALL PredictEncodeSize(AddrSize dataSize)
{
	return (AddrSize)compressBound((uLong)dataSize);
}

// allocator callbacks.
static voidpf heap_allocate(voidpf opaque, uInt items, uInt size)
{
	return (voidpf)safe_zone_malloc(ZZLib, items*size, 0);
}

static void heap_free(voidpf opaque, voidpf address)
{
	zone_free(address);
}

Encoder::Encoder() : m_zStream(0)
{
}

Encoder::~Encoder()
{
	if (m_zStream)
	{
		AddrSize temp = 0;
		End(0, temp);
		RAD_ASSERT(!m_zStream);
	}
}

bool Encoder::Begin(UReg compression)
{
	RAD_ASSERT(!m_zStream);
	m_end = false;

	m_zStream = new (ZZLib) z_stream;
	if (!m_zStream) return false;

	z_stream *stream = (z_stream*)m_zStream;
	stream->zalloc = heap_allocate;
	stream->zfree = heap_free;

	bool ok = deflateInit(stream, (int)compression) == (long)Z_OK;

	if (!ok)
	{
		delete (z_stream*)m_zStream;
		m_zStream = 0;
	}

	m_end = !ok;

	return ok;
}

bool Encoder::Encode(const void *src, AddrSize &srcLen, void *dst, AddrSize &dstLen)
{
	RAD_ASSERT(m_zStream);
	RAD_ASSERT(src&&dst);

	z_stream *stream = (z_stream*)m_zStream;

	stream->next_in = (Bytef*)src;
	stream->avail_in = (uInt)srcLen;
	stream->next_out = (Bytef*)dst;
	stream->avail_out = (uInt)dstLen;

	int err = deflate(stream, 0);

	srcLen = srcLen - (AddrSize)stream->avail_in;
	dstLen = dstLen - (AddrSize)stream->avail_out;

	return (err == Z_BUF_ERROR || err == Z_OK);
}

bool Encoder::End(void *dst, AddrSize &dstLen)
{
	if (m_end)
	{
		dstLen = 0;
		return true;
	}

	RAD_ASSERT(m_zStream);
	z_stream *stream = (z_stream*)m_zStream;

	stream->next_in = 0;
	stream->avail_in = 0;
	stream->next_out = (Bytef*)dst;
	stream->avail_out = (uInt)dstLen;

	int err = Z_OK;

	if (!m_end)
	{
		err = deflate(stream, Z_FINISH);
		dstLen = dstLen - (AddrSize)stream->avail_out;
	}

	if (err == Z_STREAM_END || 0 == dst)
	{
		m_end = true;
		deflateEnd(stream);
		delete (z_stream*)m_zStream;
		m_zStream = 0;
		err = Z_OK;
	}

	return (err == Z_BUF_ERROR || err == Z_OK);
}

Decoder::Decoder() : m_zStream(0), m_end(false)
{
}

Decoder::~Decoder()
{
	if (m_zStream)
	{
		AddrSize temp = 0;
		End(0, temp);
		RAD_ASSERT(!m_zStream);
	}
}

bool Decoder::Begin()
{
	RAD_ASSERT(!m_zStream);

	m_zStream = new (ZZLib) z_stream;
	if (!m_zStream) return false;

	z_stream *stream = (z_stream*)m_zStream;
	stream->zalloc = heap_allocate;
	stream->zfree = heap_free;

	bool ok = inflateInit(stream) == Z_OK;

	if (!ok)
	{
		delete (z_stream*)m_zStream;
		m_zStream = 0;
	}

	m_end = !ok;

	return ok;
}

bool Decoder::Decode(const void *src, AddrSize &srcLen, void *dst, AddrSize &dstLen)
{
	RAD_ASSERT(m_zStream);
	RAD_ASSERT(src&&dst);

	z_stream *stream = (z_stream*)m_zStream;

	stream->next_in = (Bytef*)src;
	stream->avail_in = (uInt)srcLen;
	stream->next_out = (Bytef*)dst;
	stream->avail_out = (uInt)dstLen;

	int err = inflate(stream, 0);

	srcLen = srcLen - (AddrSize)stream->avail_in;
	dstLen = dstLen - (AddrSize)stream->avail_out;

	return (err == Z_BUF_ERROR || err == Z_OK || err == Z_STREAM_END);
}

//////////////////////////////////////////////////////////////////////////////////////////
// data_codec::zlib::Decoder::End()
//////////////////////////////////////////////////////////////////////////////////////////

bool Decoder::End(void *dst, AddrSize &dstLen)
{
	if (m_end)
	{
		dstLen = 0;
		return true;
	}

	RAD_ASSERT(m_zStream);
	z_stream *stream = (z_stream*)m_zStream;

	stream->next_in = 0;
	stream->avail_in = 0;
	stream->next_out = (Bytef*)dst;
	stream->avail_out = (uInt)dstLen;

	int err = Z_OK;

	if (!m_end)
	{
		err = inflate(stream, Z_FINISH);
	}

	dstLen = dstLen - (AddrSize)stream->avail_out;

	if (err != Z_BUF_ERROR || 0 == dst)
	{
		m_end = true;
		inflateEnd(stream);
		delete (z_stream*)m_zStream;
		m_zStream = 0;
		err = Z_OK;
	}

	return (err == Z_BUF_ERROR || err == Z_OK) && (m_end || stream->avail_out == 0);
}

//////////////////////////////////////////////////////////////////////////////////////////
// data_codec::zlib::StreamEncode()
//////////////////////////////////////////////////////////////////////////////////////////

#define STACK_ENCODE
RADRT_API bool RADRT_CALL StreamEncode(InputStream &in, OutputStream &out, UReg compression)
{
	const int BufSize = 256;

#if defined(STACK_ENCODE)
	U8 inbuff[BufSize];
	U8 outbuff[BufSize];
#else
	U8* inbuff = zone_malloc(ZZLib, BufSize, 0);
	if (!inbuff) return false;
	U8* outbuff = zone_malloc(ZZLib, BufSize, 0);
	if (!outbuff)
	{
		zone_free(inbuff);
		return false;
	}
#endif

	UReg errorCode;
	z_stream stream;
	int err;

	memset(&stream, 0, sizeof(stream));

	stream.zalloc = heap_allocate;
	stream.zfree = heap_free;

	err = (int)deflateInit(&stream, (int)compression);

	if (err == Z_OK)
	{
		stream.next_out = (Bytef*)outbuff;
		stream.avail_out = (uInt)BufSize;

		for (;;)
		{
			SPos inputSize = in.Read(inbuff, BufSize, &errorCode);
			if (inputSize != BufSize && errorCode != ErrorUnderflow)
			{
				err = Z_BUF_ERROR;
				break; // error condition.
			}

			if (inputSize == 0) // no more input left, close it out...
			{
				for(;;)
				{
					err = deflate(&stream, Z_FINISH);

					// used up our output space.
					SPos sizeToWrite = BufSize - stream.avail_out;

					if (out.Write(outbuff, sizeToWrite, 0) != sizeToWrite)
					{
						err = Z_STREAM_ERROR;
						break;
					}

					if (err != Z_BUF_ERROR && err != Z_OK)
					{
						break;
					}

					stream.next_out  = (Bytef*)outbuff;
					stream.avail_out = BufSize;
				}

				if (err == Z_STREAM_END)
				{
					err = Z_OK;
				}

				break; // done.
			}
			else
			{
				stream.next_in = (Bytef*)inbuff;
				stream.avail_in = (uInt)inputSize;

				for(;;)
				{
					err = deflate(&stream, 0);

					if (err == Z_BUF_ERROR || err == Z_OK)
					{
						// want more input?
						if (stream.avail_in==0) break;

						// want more output?
						if (stream.avail_out==0)
						{
							if (out.Write(outbuff, BufSize, 0) != BufSize)
							{
								err = Z_BUF_ERROR;
								break;
							}

							stream.next_out  = (Bytef*)outbuff;
							stream.avail_out = BufSize;
						}
					}
					else
					{
						break;
					}
				}

				if (err != Z_OK && err != Z_BUF_ERROR) break;
			}
		}

		deflateEnd(&stream);
	}

#if !defined(STACK_ENCODE)
	zone_free(inbuff);
	zone_free(outbuff);
#endif

	return err == Z_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// data_codec::zlib::StreamDecode()
//////////////////////////////////////////////////////////////////////////////////////////

#define STACK_DECODE
RADRT_API bool RADRT_CALL StreamDecode(stream::InputStream &in, stream::OutputStream &out)
{
	const int BufSize = 256;

#if defined(STACK_DECODE)
	U8 inbuff[BufSize];
	U8 outbuff[BufSize];
#else
	U8* inbuff = zone_malloc(ZZLib, BufSize, 0);
	if (!inbuff) return false;
	U8* outbuff = zone_malloc(ZZLib, BufSize, 0);
	if (!outbuff)
	{
		zone_free(inbuff);
		return false;
	}
#endif

	UReg errorCode;
	z_stream stream;
	int err;

	memset(&stream, 0, sizeof(stream));

	stream.zalloc = heap_allocate;
	stream.zfree = heap_free;

	err = inflateInit(&stream);

	if (err == Z_OK)
	{
		stream.next_out  = (Bytef*)outbuff;
		stream.avail_out = (uInt)BufSize;

		for (;;)
		{
			SPos inputSize = in.Read(inbuff, BufSize, &errorCode);
			if (inputSize != BufSize && errorCode != ErrorUnderflow)
			{
				err = Z_BUF_ERROR;
				break; // error condition.
			}

			if (inputSize == 0) // no more input left, close it out...
			{
				for(;;)
				{
					err = inflate(&stream, Z_FINISH);

					// used up our output space.
					SPos sizeToWrite = BufSize - stream.avail_out;

					if (out.Write(outbuff, sizeToWrite, 0) != sizeToWrite)
					{
						err = Z_STREAM_ERROR;
						break;
					}

					if (err != Z_BUF_ERROR && err != Z_OK)
					{
						break;
					}

					stream.next_out  = (Bytef*)outbuff;
					stream.avail_out = BufSize;
				}

				if (err == Z_STREAM_END)
				{
					err = Z_OK;
				}

				break; // done.
			}
			else
			{
				stream.next_in  = (Bytef*)inbuff;
				stream.avail_in = (uInt)inputSize;

				for(;;)
				{
					err = inflate(&stream, 0);

					if (err == Z_BUF_ERROR || err == Z_OK || err == Z_STREAM_END)
					{
						// want more input?
						if (stream.avail_in==0) break;

						// want more output?
						if (stream.avail_out==0)
						{
							if (out.Write(outbuff, BufSize, 0) != BufSize)
							{
								err = Z_BUF_ERROR;
								break;
							}

							stream.next_out  = (Bytef*)outbuff;
							stream.avail_out = BufSize;
						}
					}
					else
					{
						break;
					}
				}

				if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END) break;
			}
		}

		inflateEnd(&stream);
	}

#if !defined(STACK_DECODE)
	zone_free(inbuff);
	zone_free(outbuff);
#endif

	return err == Z_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// data_codec::zlib::Encode()
//////////////////////////////////////////////////////////////////////////////////////////
// Deflates a block of data.

RADRT_API bool RADRT_CALL Encode(const void* data, AddrSize dataSize, UReg compression, void* out, AddrSize* outSize)
{
	RAD_ASSERT(data&&dataSize);
	RAD_ASSERT(out&&outSize);

	uLongf predictedSize;

#if defined(RAD_OPT_DEBUG)
	predictedSize = compressBound((uLong)dataSize);
	RAD_ASSERT(predictedSize == *outSize);
#endif

	predictedSize = (uLongf)*outSize;
	int zerr = (int)compress2((Bytef*)out, &predictedSize, (const Bytef*)data, (uLong)dataSize, (int)compression, heap_allocate, heap_free, 0);
	*outSize = (AddrSize)predictedSize;

	return zerr == Z_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// data_codec::zlib::Decode()
//////////////////////////////////////////////////////////////////////////////////////////
// Inflates a block of data.

RADRT_API bool RADRT_CALL Decode(const void* data, AddrSize dataSize, void* out, AddrSize outSize)
{
	RAD_ASSERT(data&&dataSize);
	RAD_ASSERT(out&&outSize);

	uInt dummySize = (uInt)outSize;
	int zerr = uncompress((Bytef*)out, (uLongf*)&dummySize, (const Bytef*)data, (uInt)dataSize, heap_allocate, heap_free, 0);

	return zerr == Z_OK;
}

} // zlib
} // data_codec

