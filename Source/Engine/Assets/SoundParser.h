// SoundParser.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../FileSystem/FileSystem.h"
#include <Runtime/Stream.h>
#include <Runtime/AudioCodec/SoundHeader.h>
#if defined(RAD_OPT_TOOLS)
#include <Runtime/AudioCodec/Wave.h>
#endif
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS SoundParser : public pkg::Sink<SoundParser>
{
public:

	static void Register(Engine &engine);

	enum
	{
		SinkStage = pkg::SS_Parser,
		AssetType = AT_Sound
	};

	typedef boost::shared_ptr<SoundParser> Ref;

	SoundParser();
	virtual ~SoundParser();

	RAD_DECLARE_READONLY_PROPERTY(SoundParser, header, const audio_codec::SoundHeader*);
	RAD_DECLARE_READONLY_PROPERTY(SoundParser, data, const void*);

protected:

	RAD_DECLARE_GET(header, const audio_codec::SoundHeader*) { return m_data.bytes ? &m_header : 0; }
	RAD_DECLARE_GET(data, const void*) { return m_data.cvoid; }

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

#if defined(RAD_OPT_TOOLS)
	int Load(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);
#endif

	int LoadCooked(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

	union DataPair
	{
		U8 *bytes;
		const void *cvoid;
	};
	
	DataPair m_data;
	audio_codec::SoundHeader m_header;
	file::HBufferedAsyncIO m_buf;
	bool m_loaded;
#if defined(RAD_OPT_TOOLS)
	int m_decodeOfs;
	audio_codec::wave::Decoder m_decoder;
	file::HStreamInputBuffer m_ib;
	stream::InputStream m_is;
#endif
};

} // asset

#include <Runtime/PopPack.h>
