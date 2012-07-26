// FontParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "FontParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

FontParser::FontParser() : m_loaded(false) {
}

FontParser::~FontParser() {
}

int FontParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (m_loaded && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (flags&P_Unload) {
		m_font.Destroy();
		m_mm.reset();
		m_loaded = false;
		return SR_Success;
	}

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked)
		return Load(time, engine, asset, flags);
#endif
	return LoadCooked(time, engine, asset, flags);
}

#if defined(RAD_OPT_TOOLS)
int FontParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!m_mm) {
		const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
		if (!s || s->empty)
			return SR_MetaError;
		m_mm = engine.sys->files->MapFile(s->c_str, ZAssets);
		
		if (!m_mm)
			return SR_FileNotFound;
	}

	if (!m_font.Create(m_mm->data, m_mm->size))
		return SR_InvalidFormat;

	m_loaded = true;
	return SR_Success;
}
#endif

int FontParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!m_mm) {
		String path(CStr("Cooked/"));
		path += CStr(asset->path);
		path += ".bin";

		m_mm = engine.sys->files->MapFile(path.c_str, ZAssets);
		if (!m_mm)
			return SR_FileNotFound;
	}

	if (!m_font.Create(m_mm->data, m_mm->size))
		return SR_InvalidFormat;

	m_loaded = true;
	return SR_Success;
}

void FontParser::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<FontParser>();
}

} // asset
