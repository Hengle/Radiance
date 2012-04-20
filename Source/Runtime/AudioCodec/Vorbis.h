// Vorbis.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#define OV_EXCLUDE_STATIC_CALLBACKS

#include "VorbisDef.h"
#include "../StreamDef.h"
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>
#include "../PushPack.h"

namespace audio_codec {
namespace ogg_vorbis {

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Comments
{
public:
	Comments(const Comments &c);

	RAD_DECLARE_READONLY_PROPERTY(Comments, num, int);
	const char *Comment(int n) const;

private:

	RAD_DECLARE_GET(num, int);

	friend class Decoder;
	Comments(vorbis_comment *c);
	vorbis_comment *m_c;
};

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Decoder
{
public:
	Decoder();
	~Decoder();

	// Initialize the ogg vorbis decoder.
	// Returns true if the stream is a vorbis stream.
	//
	// If partial is "true" the stream is only opened far enough to
	// test that it's a vorbis stream. If you do a parial open make
	// sure to call FinishInitialize() before using it.

	bool Initialize(stream::InputStream &stream, bool partial);
	bool FinishInitialize();

	void Finalize();
	
	// Output is PCM data. Use BitStreamInfo() to retrieve stream format.
	bool Decode(
		void *buffer, 
		AddrSize bufferSize,
		EndianMode endianMode,
		SampleType sampleType,
		DataType dataType,
		int *bitStream,
		AddrSize &bytesDecoded
	);

	bool SeekBytes(int ofs, bool xfade);
	bool SeekSample(int sample, bool xfade);
	bool SeekPage(int sample, bool xfade);
	bool SeekTime(float time, bool blend);
	bool SeekPage(float time, bool xfade);

	RAD_DECLARE_READONLY_PROPERTY(Decoder, bitRateInstant, int);
	RAD_DECLARE_READONLY_PROPERTY(Decoder, logicalBitStreamCount, int);
	RAD_DECLARE_READONLY_PROPERTY(Decoder, isSeekable, bool);
	RAD_DECLARE_READONLY_PROPERTY(Decoder, bytePos, int);
	RAD_DECLARE_READONLY_PROPERTY(Decoder, samplePos, int);
	RAD_DECLARE_READONLY_PROPERTY(Decoder, timePos, float);
	
	int BSSerialNumber(int bitStram = -1) const;
	int BSCompressedSize(int bitStream = -1) const;
	int BSNumSamples(int bitStream = -1) const;
	float BSDuration(int bitStream = -1) const; // in seconds
	bool BSInfo(BSI &info, int bitStream = -1) const;
	Comments BSComments(int bitStream = -1) const;

private:

	RAD_DECLARE_GET(bitRateInstant, int);
	RAD_DECLARE_GET(logicalBitStreamCount, int);
	RAD_DECLARE_GET(isSeekable, bool);
	RAD_DECLARE_GET(bytePos, int);
	RAD_DECLARE_GET(samplePos, int);
	RAD_DECLARE_GET(timePos, float);

	mutable OggVorbis_File m_ogg;
	stream::InputStream *m_stream;
	bool m_init, m_partial;

	static size_t VorbisRead(void* dst, size_t size, size_t nmemb, void* parm);
	static int VorbisSeek(void* parm, S64 offset, int whence);
	static int VorbisClose(void* parm);
	static long VorbisTell(void* parm);
};

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Encoder
{
public:

	static const float DefaultQuality;

	Encoder();
	~Encoder();

	bool Initialize(
		int serialNo,
		int numChannels,
		int rate,
		float qualityZeroToOne,
		stream::OutputStream &stream
	);

	// Finalize must be called to flush encoder data.
	void Finalize();

	// All comments must be added before any data is encoded!
	void AddComment(const char *comment);
	void AddComment(const char *tag, const char *comment); // adds "tag=comment"

	// Begins encoding a block of PCM data.
	// Write your 32 bit IEEE floating point PCM data into the returned
	// buffer and call EndEncode().
	
	// returns float[numChannels][numSamples]
	// Do not free the buffer.
	float **BeginEncode(int numSamples);

	// Pass the number of samples actually encoded (may differ than amount
	// passed to BeginEncode())
	bool EndEncode(int numSamples);

	// Use these routines to page out the finalized ogg data into the output
	// stream.
	//
	// You can use BuildOgg() to do this for your or you can call
	// BeginOgg() / WriteOgg() to do it a page at a time.
	bool BeginOgg();
	OggState WriteOgg(AddrSize *bytesWritten=0);

	// Builds the ogg stream all at once.
	bool BuildOgg();

private:

	struct VorbisVars
	{
		ogg_stream_state   *os;
		ogg_page           *og;
		ogg_packet         *op;
		vorbis_info        *vi;
		vorbis_comment     *vc;
		vorbis_dsp_state   *vd;
		vorbis_block       *vb;
	};

	VorbisVars m_vv;
	bool m_init;
	bool m_firstEncode;
	bool m_begin;
	stream::OutputStream* m_stream;
};

} // ogg_vorbis
} // audio_codec

#include "../PopPack.h"
