// Wave.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "WaveDef.h"
#include "SoundHeader.h"
#include "../StreamDef.h"
#include "../PushPack.h"

namespace audio_codec {
namespace wave {

class RADRT_CLASS Decoder
{
public:
	Decoder();
	~Decoder();

	RAD_DECLARE_READONLY_PROPERTY(Decoder, header, const SoundHeader*);

	bool Initialize(stream::InputStream &stream);
	void Finalize();

	bool Decode(void *buffer, AddrSize bufSize, AddrSize &bytesDecoded);

	bool SeekByte(stream::SPos ofs);
	bool SeekSample(stream::SPos sample);
	bool SeekTime(float seconds);

private:

	RAD_DECLARE_GET(header, const SoundHeader*) { return &m_info; }

	int SeekTag(const char *tag);

	SoundHeader m_info;
	stream::SPos m_start, m_pos;
	stream::InputStream *m_stream;
	bool m_init;
};

class RADRT_CLASS Encoder
{
public:
	Encoder();
	~Encoder();

	bool Initialize(
		stream::OutputStream &stream, 
		int channels,
		int rate,
		int bytesPerSample
	);

	// Finalize must be called to flush encoder data.
	bool Finalize();

	bool Encode(const void *samples, AddrSize numBytes);

private:

	int m_channels;
	int m_rate;
	int m_bytesPerSample;
	stream::OutputStream *m_stream;
	stream::SPos m_headerOfs;
	bool m_init;
};

} // wave
} // audio_codec

#include "../PopPack.h"
