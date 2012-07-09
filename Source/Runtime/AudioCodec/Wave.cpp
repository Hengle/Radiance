// Wave.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Wave.h"
#include "../Stream.h"
#include "../StringBase.h"
#include "../Endian.h"
#include <algorithm>
#include <limits>

#undef min
#undef max

using namespace string;
using namespace stream;
using namespace endian;

namespace audio_codec {
namespace wave {

///////////////////////////////////////////////////////////////////////////////

Decoder::Decoder() : m_init(false), m_stream(0), m_pos(0)
{
}

Decoder::~Decoder()
{
	if (m_init)
		Finalize();
}

bool Decoder::Initialize(stream::InputStream &stream)
{
	RAD_ASSERT(!m_init);

	m_stream = &stream;

	char buff[4];
	if (stream.Read(buff, 4, 0) != 4)
		return false;
	if (ncmp(buff, "RIFF", 4))
		return false; // not a wave file
	if (!stream.SeekIn(StreamCur, 4, 0))
		return false;
	if (stream.Read(buff, 4, 0) != 4)
		return false;
	if (ncmp(buff, "WAVE", 4))
		return false;

	int tagLen = SeekTag("fmt ");
	if (tagLen != 16 && tagLen != 18)
		return false;
	if (tagLen == 18)
	{
		// some apps have bad headers.
		if (!stream.SeekIn(StreamCur, 2, 0))
			return false;
	}

	U16 u16;
	U32 u32;

	if (!stream.Read(&u16))
		return false;
	u16 = SwapLittle(u16);
	if (u16 != 1)
		return false; // only support byte formats.
	
	if (!stream.Read(&u16))
		return false;
	m_info.channels = (int)SwapLittle(u16);

	if (!stream.Read(&u32))
		return false;
	m_info.rate = (int)SwapLittle(u32);

	if (!stream.Read(&u32))
		return false;
	m_info.bytesPerSecond = (int)SwapLittle(u32);

	int align;
	if (!stream.Read(&u16))
		return false;
	align = (int)SwapLittle(u16);

	if (!stream.Read(&u16))
		return false;
	u16 = SwapLittle(u16);
	m_info.bytesPerSample = (int)(u16/8);

	tagLen = SeekTag("data");

	if (align != m_info.channels*m_info.bytesPerSample)
		return false;

	m_pos = m_start = stream.InPos();
	if (tagLen)
	{
		m_info.numBytes = tagLen;
	}
	else
	{ // estimate stream size
		m_info.numBytes = stream.Size() - m_start;
	}

	m_info.numSamples = m_info.numBytes / m_info.bytesPerSample;
	m_info.duration = ((float)m_info.numBytes) / m_info.bytesPerSecond;
	m_init = true;
	return true;
}

int Decoder::SeekTag(const char *tag)
{
	char buff[4];
	int size = 0;

	while (m_stream->Read(buff, 4, 0) == 4)
	{
		int tagLen;
		if (m_stream->Read(&tagLen))
		{
			tagLen = SwapLittle(tagLen);
		}
		else
		{
			tagLen = 0;
		}

		if (!tagLen)
			break;

		if (!ncmp(buff, tag, 4))
		{
			size = tagLen;
			break;
		}

		// skip past tag data.
		if (!m_stream->SeekIn(StreamCur, tagLen, 0))
			break;
	}

	return size;
}

void Decoder::Finalize()
{
	m_stream = 0;
	m_init = false;
}

bool Decoder::Decode(void *buffer, AddrSize bufSize, AddrSize &bytesDecoded)
{
	SPos sizeToRead = std::min((SPos)bufSize, std::numeric_limits<SPos>::max());

	if (sizeToRead + m_pos >= (m_info.numBytes+m_start))
		sizeToRead = (m_info.numBytes+m_start)-m_pos;
	UReg errorCode = ErrorGeneric;

	if (sizeToRead)
		bytesDecoded = (AddrSize)m_stream->Read(buffer, sizeToRead, &errorCode);

	if (errorCode == Success || ((errorCode == ErrorUnderflow) && bytesDecoded))
	{
		m_pos += (SPos)bytesDecoded;
		return true;
	}

	return false;
}

bool Decoder::SeekByte(stream::SPos ofs)
{
	if (m_stream->SeekIn(StreamBegin, m_start + ofs, 0))
	{
		m_pos = m_start + ofs;
		return true;
	}

	return false;
}

bool Decoder::SeekSample(stream::SPos sample)
{
	return SeekByte(sample * (SPos)m_info.bytesPerSample * (SPos)m_info.channels);
}

bool Decoder::SeekTime(float seconds)
{
	return SeekByte((SPos)(seconds * m_info.bytesPerSecond));
}

///////////////////////////////////////////////////////////////////////////////

Encoder::Encoder() : 
m_init(false), 
m_stream(0), 
m_headerOfs(0),
m_channels(0),
m_rate(0),
m_bytesPerSample(0)
{
}

Encoder::~Encoder()
{
	if (m_init)
		Finalize();
}

bool Encoder::Initialize(
	stream::OutputStream &stream, 
	int channels,
	int rate,
	int bytesPerSample
)
{
	RAD_ASSERT(!m_init);

	m_stream = &stream;
	m_channels = channels;
	m_rate = rate;
	m_bytesPerSample = bytesPerSample;
	m_headerOfs = stream.OutPos();

	// write blank header first
	U8 garbage[44];
	if (stream.Write(garbage, 44, 0) != 44)
		return false;
	m_init = true;
	return true;
}

bool Encoder::Finalize()
{
	RAD_ASSERT(m_init&&m_stream);

	SPos endPos = m_stream->OutPos();

	if (!m_stream->SeekOut(StreamBegin, m_headerOfs, 0))
		return false;
	if (m_stream->Write("RIFF", 4, 0) != 4)
		return false;
	if (!m_stream->Write(SwapLittle((U32)(endPos-m_headerOfs-8))))
		return false;
	if (m_stream->Write("WAVE", 4, 0) != 4)
		return false;
	if (m_stream->Write("fmt ", 4, 0) != 4)
		return false;
	if (!m_stream->Write(SwapLittle((U32)16)))
		return false;
	if (!m_stream->Write(SwapLittle((U16)1)))
		return false;
	if (!m_stream->Write(SwapLittle((U16)m_channels)))
		return false;
	if (!m_stream->Write(SwapLittle((U32)m_rate)))
		return false;
	if (!m_stream->Write(SwapLittle((U32)(m_bytesPerSample*m_rate*m_channels))))
		return false;
	if (!m_stream->Write(SwapLittle((U16)(m_channels*m_bytesPerSample))))
		return false;
	if (!m_stream->Write(SwapLittle((U16)(m_bytesPerSample*8))))
		return false;
	if (m_stream->Write("data", 4, 0) != 4)
		return false;
	if (!m_stream->Write((U32)(endPos-m_headerOfs-44)))
		return false;

	m_init = false;
	m_stream = 0;

	return true;
}

bool Encoder::Encode(const void *samples, AddrSize numBytes)
{
	RAD_ASSERT(m_init&&m_stream);
	return m_stream->Write(samples, (SPos)numBytes, 0) == (SPos)numBytes;
}

} // wave
} // audio_codec
