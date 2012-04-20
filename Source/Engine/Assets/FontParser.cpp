// FontParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "FontParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

FontParser::FontParser() : m_loaded(false)
{
}

FontParser::~FontParser()
{
}

int FontParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (m_loaded && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (flags&P_Unload)
	{
		m_font.Destroy();
		m_buf.Close();
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
)
{
	if (!m_buf)
	{
		const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
		if (!s || s->empty())
			return SR_MetaError;

		WString path(string::Widen(s->c_str()));

		int media = file::AllMedia;
		int r = engine.sys->files->LoadFile(
			path.c_str(),
			media,
			m_buf,
			file::HIONotify()
		);

		if (r < SR_Success)
			return r;
	}

	if (m_buf->result == SR_Pending)
	{
		if (time.infinite)
			m_buf->WaitForCompletion();
		else
			return SR_Pending;
	}

	if (m_buf->result < SR_Success)
		return m_buf->result;

	if (!m_font.Create(m_buf->data->ptr, m_buf->data->size))
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
)
{
	if (!m_buf)
	{
		WString path(L"Cooked/");
		path += string::Widen(asset->path);
		path += L".bin";

		int media = file::AllMedia;
		int r = engine.sys->files->LoadFile(
			path.c_str(),
			media,
			m_buf,
			file::HIONotify()
		);

		if (r < SR_Success)
			return r;
	}

	if (m_buf->result == SR_Pending)
	{
		if (time.infinite)
			m_buf->WaitForCompletion();
		else
			return SR_Pending;
	}

	if (m_buf->result < SR_Success)
		return m_buf->result;

	if (!m_font.Create(m_buf->data->ptr, m_buf->data->size))
		return SR_InvalidFormat;

	m_loaded = true;
	return SR_Success;
}

void FontParser::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<FontParser>();
}

} // asset
