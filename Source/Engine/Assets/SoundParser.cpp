// SoundParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SoundParser.h"
#include "../Engine.h"
#include <algorithm>
#undef min
#undef max

using namespace pkg;

namespace asset {

SoundParser::SoundParser() : m_loaded(false) {
	m_data.bytes = 0;
}

SoundParser::~SoundParser() {
}

int SoundParser::Process(
	const xtime::TimeSlice &time, 
	Engine &engine, 
	const pkg::Asset::Ref &asset, 
	int flags
) {
	if (flags&(P_Unload|P_Trim)) {
#if defined(RAD_OPT_TOOLS)
		if (m_ib)
			zone_free(m_data.bytes);
		m_ib.reset();
		m_decoder.Finalize();
		m_decodeOfs = 0;
#endif

		m_data.bytes = 0;
		m_mm.reset();
		return SR_Success;
	}

	if (flags&(P_Load|P_Parse) && !m_loaded) {
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked)
			return Load(time, engine, asset, flags);
#endif
		return LoadCooked(time, engine, asset, flags);
	}

	return SR_Success;
}

#if defined(RAD_OPT_TOOLS)
int SoundParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!m_ib) {
		const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		m_ib = engine.sys->files->OpenInputBuffer(s->c_str, ZSound);
		if (!m_ib)
			return SR_FileNotFound;

		m_is.SetBuffer(*m_ib);
		if (!m_decoder.Initialize(m_is)) {
			m_ib.reset();
			return SR_InvalidFormat;
		}

		m_header = *m_decoder.header.get();
		m_data.bytes = (U8*)safe_zone_malloc(ZSound, m_header.numBytes);
		m_decodeOfs = 0;
	}

	enum { kBlockSize = 64*Kilo };

	while (m_decodeOfs < m_header.numBytes && time.remaining) {
		AddrSize reqBytes = (AddrSize)std::min<int>(kBlockSize, (m_header.numBytes-m_decodeOfs));
		AddrSize bytesDecoded;

		if (!m_decoder.Decode(m_data.bytes+m_decodeOfs, reqBytes, bytesDecoded) || (bytesDecoded != reqBytes)) {
			zone_free(m_data.bytes);
			m_data.bytes = 0;
			m_decoder.Finalize();
			m_ib.reset();
			return SR_InvalidFormat;
		}

		m_decodeOfs += (int)reqBytes;
	}

	m_loaded = (m_decodeOfs >= m_header.numBytes);

	return m_loaded ? SR_Success : SR_Pending;
}
#endif

int SoundParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!m_mm) {
		String path(CStr("Cooked/"));
		path += CStr(asset->path);
		path += ".bin";

		m_mm = engine.sys->files->MapFile(path.c_str, ZSound);
		if (!m_mm)
			return SR_FileNotFound;
	}

	const int HeaderSize = sizeof(audio_codec::SoundHeader);

	if (m_mm->size < HeaderSize)
		return SR_CorruptFile;
	m_header = *reinterpret_cast<const audio_codec::SoundHeader*>(m_mm->data.get());
	m_data.cvoid = reinterpret_cast<const U8*>(m_mm->data.get()) + HeaderSize;

	if ((HeaderSize+m_header.numBytes) > (int)m_mm->size.get())
		return SR_CorruptFile;

	m_loaded = true;
	return SR_Success;
}

void SoundParser::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<SoundParser>();
}

} // asset
