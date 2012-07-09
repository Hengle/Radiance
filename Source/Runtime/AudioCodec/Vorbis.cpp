// Vorbis.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Vorbis.h"
#include "../Stream.h"
#include <errno.h>

using namespace stream;

namespace audio_codec {
namespace ogg_vorbis {

///////////////////////////////////////////////////////////////////////////////

Decoder::Decoder() :
m_stream(0),
m_init(false),
m_partial(false)
{
}

Decoder::~Decoder()
{
	if (m_init)
		Finalize();
}

bool Decoder::Initialize(stream::InputStream &stream, bool partial)
{
	RAD_ASSERT(!m_init);

	m_stream = &stream;

	ov_callbacks cbs = {VorbisRead, VorbisSeek, VorbisClose, VorbisTell};

	if (partial)
	{
		m_partial = true;
		if (ov_test_callbacks(this, &m_ogg, 0, 0, cbs))
			return false;

		return true;
	}

	RAD_ASSERT(!m_partial);
	m_init = ov_open_callbacks(this, &m_ogg, 0, 0, cbs) == 0;
	return m_init;
}

bool Decoder::FinishInitialize()
{
	RAD_ASSERT(m_partial);
	RAD_ASSERT(!m_init);

	m_init = ov_test_open(&m_ogg) == 0;
	return m_init;
}

void Decoder::Finalize()
{
	RAD_ASSERT(m_init);
	ov_clear(&m_ogg);
	m_init = false;
}

// Output is PCM data. Use BitStreamInfo() to retrieve stream format.
bool Decoder::Decode(
	void *buffer, 
	AddrSize bufferSize,
	EndianMode endianMode,
	SampleType sampleType,
	DataType dataType,
	int *bitStream,
	AddrSize &bytesDecoded
)
{
	RAD_ASSERT(m_init);

	int bs;
	long r = ov_read(
		&m_ogg,
		(char*)buffer,
		(int)bufferSize,
		(endianMode == EM_Big) ? 1 : 0,
		(sampleType == ST_16Bit) ? 2 : 1,
		(dataType == DT_Signed) ? 1 : 0,
		&bs
	);

	bytesDecoded = 0;

	if (r > 0)
	{
		if (bitStream)
			*bitStream = bs;
		bytesDecoded = (AddrSize)r;
	}

	return r > 0;
}

bool Decoder::SeekBytes(int ofs, bool xfade)
{
	RAD_ASSERT(m_init);
	int r;
	if (xfade)
		r = ov_raw_seek_lap(&m_ogg, (ogg_int64_t)ofs);
	else
		r = ov_raw_seek(&m_ogg, (ogg_int64_t)ofs);
	return r == 0;
}

bool Decoder::SeekSample(int sample, bool xfade)
{
	RAD_ASSERT(m_init);
	int r;
	if (xfade)
		r = ov_pcm_seek_lap(&m_ogg, (ogg_int64_t)sample);
	else
		r = ov_pcm_seek(&m_ogg, (ogg_int64_t)sample);
	return r == 0;
}

bool Decoder::SeekPage(int sample, bool xfade)
{
	RAD_ASSERT(m_init);
	int r;
	if (xfade)
		r = ov_pcm_seek_page_lap(&m_ogg, (ogg_int64_t)sample);
	else
		r = ov_pcm_seek_page(&m_ogg, (ogg_int64_t)sample);
	return r == 0;
}

bool Decoder::SeekTime(float time, bool xfade)
{
	RAD_ASSERT(m_init);
	int r;
	if (xfade)
		r = ov_time_seek_lap(&m_ogg, (double)time);
	else
		r = ov_time_seek(&m_ogg, (double)time);
	return r == 0;
}

bool Decoder::SeekPage(float time, bool xfade)
{
	RAD_ASSERT(m_init);
	int r;
	if (xfade)
		r = ov_time_seek_page_lap(&m_ogg, (double)time);
	else
		r = ov_time_seek_page(&m_ogg, (double)time);
	return r == 0;
}

int Decoder::RAD_IMPLEMENT_GET(bitRateInstant)
{
	RAD_ASSERT(m_init);
	return (int)ov_bitrate_instant(&m_ogg);
}

int Decoder::RAD_IMPLEMENT_GET(logicalBitStreamCount)
{
	RAD_ASSERT(m_init);
	return (int)ov_streams(&m_ogg);
}

bool Decoder::RAD_IMPLEMENT_GET(isSeekable)
{
	RAD_ASSERT(m_init);
	return ov_seekable(&m_ogg) != 0;
}

int Decoder::RAD_IMPLEMENT_GET(bytePos)
{
	RAD_ASSERT(m_init);
	return (int)ov_raw_tell(&m_ogg);
}

int Decoder::RAD_IMPLEMENT_GET(samplePos)
{
	RAD_ASSERT(m_init);
	return (int)ov_pcm_tell(&m_ogg);
}

float Decoder::RAD_IMPLEMENT_GET(timePos)
{
	RAD_ASSERT(m_init);
	return (float)ov_time_tell(&m_ogg);
}

int Decoder::BSSerialNumber(int bitStream) const
{
	RAD_ASSERT(m_init);
	return (int)ov_serialnumber(&m_ogg, bitStream);
}

int Decoder::BSCompressedSize(int bitStream) const
{
	RAD_ASSERT(m_init);
	return (int)ov_raw_total(&m_ogg, bitStream);
}

int Decoder::BSNumSamples(int bitStream) const
{
	RAD_ASSERT(m_init);
	return (int)ov_pcm_total(&m_ogg, bitStream);
}

float Decoder::BSDuration(int bitStream) const
{
	RAD_ASSERT(m_init);
	return (float)ov_time_total(&m_ogg, bitStream);
}

bool Decoder::BSInfo(BSI &info, int bitStream) const
{
	RAD_ASSERT(m_init);
	vorbis_info *vi = ov_info(&m_ogg, bitStream);
	if (vi)
	{
		info.version = (int)vi->version;
		info.rate = (int)vi->rate;
		info.brLower = (int)vi->bitrate_lower;
		info.brNominal = (int)vi->bitrate_nominal;
		info.brUpper = (int)vi->bitrate_upper;
		info.brWindow = (int)vi->bitrate_window;
		info.channels = (int)vi->channels;
		return true;
	}

	return false;
}

Comments Decoder::BSComments(int bitStream) const
{
	return Comments(ov_comment(&m_ogg, bitStream));
}

size_t Decoder::VorbisRead(void* buffer, size_t size, size_t nmemb, void* parm)
{
	Decoder* vd = (Decoder*)parm;
	UReg errorCode;
	SPos read;
	read = (SPos)(size*nmemb);
	read = vd->m_stream->Read(buffer, read, &errorCode);
	if (errorCode == Success ||
		errorCode == ErrorUnderflow)
	{
		errno = 0;
	}
	else
	{
		errno = -1;
		read = 0;
	}

	return (size_t)read;
}

int Decoder::VorbisSeek(void* parm, S64 offset, int whence)
{
	Decoder* vd = (Decoder*)parm;

	switch (whence)
	{
	case 2: // SEEK_END
		vd->m_stream->SeekIn(StreamEnd, (SPos)-offset, 0);
		break;
	case 1: // SEEK_CUR
		vd->m_stream->SeekIn(StreamCur, (SPos)offset, 0);
		break;
	case 0: // SEEK_SET
		vd->m_stream->SeekIn(StreamBegin, (SPos)offset, 0);
		break;
	}

	return 0; // vorbis doesn't check this, i dunno why.
}

int Decoder::VorbisClose(void* parm)
{
	return 0;
}

long Decoder::VorbisTell(void* parm)
{
	Decoder* vd = (Decoder*)parm;
	return (long)vd->m_stream->InPos();
}

///////////////////////////////////////////////////////////////////////////////

Comments::Comments(vorbis_comment *c) : m_c(c)
{
}

Comments::Comments(const Comments &c) : m_c(c.m_c)
{
}

int Comments::RAD_IMPLEMENT_GET(num)
{
	return m_c ? m_c->comments : 0;
}

const char *Comments::Comment(int n) const
{
	RAD_ASSERT(!m_c || n < m_c->comments);
	return m_c ? m_c->user_comments[n] : 0;
}

///////////////////////////////////////////////////////////////////////////////

const float Encoder::DefaultQuality = 0.1f;

Encoder::Encoder() :
m_init(false),
m_firstEncode(false),
m_begin(false),
m_stream(0)
{
}

Encoder::~Encoder()
{
	if (m_init)
		Finalize();
}

bool Encoder::Initialize(
	int serialNo,
	int numChannels,
	int rate,
	float qualityZeroToOne,
	stream::OutputStream &stream
)
{
	RAD_ASSERT(!m_init);

	m_stream = &stream;
	memset(&m_vv, 0, sizeof(m_vv));
	m_vv.vi = new vorbis_info;

	vorbis_info_init(m_vv.vi);
	if (!vorbis_encode_init_vbr(m_vv.vi, numChannels, rate, qualityZeroToOne))
	{
		m_vv.os = new ogg_stream_state;
		m_vv.og = new ogg_page;
		m_vv.op = new ogg_packet;
		m_vv.vc = new vorbis_comment;
		m_vv.vd = new vorbis_dsp_state;
		m_vv.vb = new vorbis_block;

		vorbis_comment_init(m_vv.vc);
		vorbis_analysis_init(m_vv.vd, m_vv.vi);
		vorbis_block_init(m_vv.vd, m_vv.vb);
		ogg_stream_init(m_vv.os, serialNo);

		m_init = m_firstEncode = true;
		m_begin = false;

		return true;
	}
	
	vorbis_info_clear(m_vv.vi);
	delete m_vv.vi;
	m_stream = 0;
	return false;
}

void Encoder::Finalize()
{
	RAD_ASSERT(m_init);
	ogg_stream_clear(m_vv.os);
	vorbis_block_clear(m_vv.vb);
	vorbis_dsp_clear(m_vv.vd);
	vorbis_comment_clear(m_vv.vc);
	vorbis_info_clear(m_vv.vi);

	delete m_vv.og;
	delete m_vv.vb;
	delete m_vv.vd;
	delete m_vv.vc;
	delete m_vv.vi;
	delete m_vv.os;
	delete m_vv.op;
	m_init = false;
}

void Encoder::AddComment(const char *comment)
{
	RAD_ASSERT(m_init);
	RAD_ASSERT(m_firstEncode);
	RAD_ASSERT(comment);
	vorbis_comment_add(m_vv.vc, comment);
}

void Encoder::AddComment(const char *tag, const char *comment)
{
	RAD_ASSERT(m_init);
	RAD_ASSERT(m_firstEncode);
	RAD_ASSERT(tag);
	RAD_ASSERT(comment);
	vorbis_comment_add_tag(m_vv.vc, tag, comment);
}

float **Encoder::BeginEncode(int numSamples)
{
	RAD_ASSERT(m_init);
	RAD_ASSERT(numSamples>0);
	RAD_ASSERT(m_stream);
	RAD_ASSERT(!m_begin);

	if (m_firstEncode)
	{
		//
		// Build the vorbis header
		//
		// Vorbis streams begin with three headers; the initial header (with
		// most of the codec setup parameters) which is mandated by the Ogg
		// bitstream spec.  The second header holds any comment fields.  The
		// third header holds the bitstream codebook.  We merely need to
		// make the headers, then pass them to libvorbis one at a time;
		// libvorbis handles the additional Ogg bitstream constraints
		//

		ogg_packet header;
		ogg_packet header_comm;
		ogg_packet header_code;

		vorbis_analysis_headerout(m_vv.vd, m_vv.vc, &header, &header_comm, &header_code);
		ogg_stream_packetin(m_vv.os, &header);
		ogg_stream_packetin(m_vv.os, &header_comm);
		ogg_stream_packetin(m_vv.os, &header_code);

		// This ensures the actual
		// audio data will start on a new page, as per spec

		while (ogg_stream_flush(m_vv.os, m_vv.og))
		{
			if ((m_stream->Write(m_vv.og->header, (SPos)m_vv.og->header_len, 0) != (SPos)m_vv.og->header_len) ||
				(m_stream->Write(m_vv.og->body, (SPos)m_vv.og->body_len, 0) != (SPos)m_vv.og->body_len))
			{
				return 0;
			}

			m_firstEncode = false;
		}
	}

	m_begin = true;
	return vorbis_analysis_buffer(m_vv.vd, numSamples);
}

bool Encoder::EndEncode(int numSamples)
{
	RAD_ASSERT(m_init);
	RAD_ASSERT(numSamples>0);
	RAD_ASSERT(m_begin);
	m_begin = false;
	return vorbis_analysis_wrote(m_vv.vd, numSamples) == 0;
}

bool Encoder::BeginOgg()
{
	RAD_ASSERT(m_init);
	RAD_ASSERT(!m_begin);
	vorbis_analysis_wrote(m_vv.vd, 0);
	return true;
}

OggState Encoder::WriteOgg(AddrSize *bytesWritten)
{
	if (bytesWritten)
		*bytesWritten = 0;

	if (vorbis_analysis_blockout(m_vv.vd, m_vv.vb)==1)
	{
		vorbis_analysis(m_vv.vb, 0);
		vorbis_bitrate_addblock(m_vv.vb);

		while (vorbis_bitrate_flushpacket(m_vv.vd, m_vv.op))
		{
			ogg_stream_packetin(m_vv.os, m_vv.op);
			while (ogg_stream_pageout(m_vv.os, m_vv.og))
			{
				if ((m_stream->Write(m_vv.og->header, (SPos)m_vv.og->header_len, 0) != (SPos)m_vv.og->header_len) ||
					(m_stream->Write(m_vv.og->body, (SPos)m_vv.og->body_len, 0) != (SPos)m_vv.og->body_len))
				{
					return OS_Error;
				}

				if (ogg_page_eos(m_vv.og))
					return OS_Error;

				if (bytesWritten)
					*bytesWritten += m_vv.og->header_len + m_vv.og->body_len;
			}
		}

		return OS_MoreData;
	}

	return OS_Done;
}

bool Encoder::BuildOgg()
{
	if (!BeginOgg())
		return false;
	OggState s;
	while ((s = WriteOgg(0)) == OS_MoreData) {}
	return s == OS_Done;
}

} // ogg_vorbis
} // audio_codec
