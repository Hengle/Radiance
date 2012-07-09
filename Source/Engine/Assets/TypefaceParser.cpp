// TypefaceParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "TypefaceParser.h"
#include "FontParser.h"
#include "MaterialParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

TypefaceParser::TypefaceParser() : 
m_font(0),
m_mat(0),
m_width(0),
m_height(0)
{
}

TypefaceParser::~TypefaceParser()
{
}

int TypefaceParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (valid && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (flags&P_Unload)
	{
		m_fontRef.reset();
		m_matRef.reset();
		m_font = 0;
		m_mat = 0;
		m_width = 0; 
		m_height = 0;
		return SR_Success;
	}

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked)
		return Load(time, engine, asset, flags);
#endif
	return LoadCooked(time, engine, asset, flags);
}

#if defined(RAD_OPT_TOOLS)
int TypefaceParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!m_fontRef)
	{
		const int *i = asset->entry->KeyValue<int>("Typeface.Width", P_TARGET_FLAGS(flags));
		if (!i)
			return SR_MetaError;
		m_width = *i;

		i = asset->entry->KeyValue<int>("Typeface.Height", P_TARGET_FLAGS(flags));
		if (!i)
			return SR_MetaError;
		m_height = *i;

		if (m_width < 1 || m_height < 1)
			return SR_MetaError;

		const String *s = asset->entry->KeyValue<String>("Source.Font", P_TARGET_FLAGS(flags));
		if (!s || s->empty)
			return SR_MetaError;

		m_fontRef = engine.sys->packages->Resolve(s->c_str, asset->zone);
		if (!m_fontRef)
			return SR_MissingFile;
	}

	if (!m_font)
	{
		int r = m_fontRef->Process(time, flags);
		if (r != SR_Success)
			return r;
		FontParser::Ref parser = FontParser::Cast(m_fontRef);
		if (!parser || !parser->font.get())
			return SR_MetaError;
		m_font = parser->font;
	}

	if (!m_matRef)
	{
		const String *s = asset->entry->KeyValue<String>("Source.Material", P_TARGET_FLAGS(flags));
		if (!s || s->empty)
			return SR_MetaError;

		m_matRef = engine.sys->packages->Resolve(s->c_str, asset->zone);
		if (!m_matRef)
			return SR_MissingFile;
	}

	if (!m_mat)
	{
		int r = m_matRef->Process(time, flags);
		if (r != SR_Success)
			return r;
		MaterialParser::Ref parser = MaterialParser::Cast(m_matRef);
		if (!parser || !parser->valid || !parser->procedural)
			return SR_MetaError;
		m_mat = parser->material;
	}

	return SR_Success;
}
#endif

int TypefaceParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!m_fontRef)
	{
		const U16 *tags = (const U16*)asset->entry->TagData(P_TARGET_FLAGS(flags));

		m_width = tags[0];
		m_height = tags[1];

		const Package::Entry::Import *i = asset->entry->Resolve(0);
		if (!i)
			return SR_ParseError;

		m_fontRef = asset->entry->Resolve(*i, asset->zone);
		if (!m_fontRef)
			return SR_MissingFile;
	}

	if (!m_font)
	{
		int r = m_fontRef->Process(time, flags);
		if (r != SR_Success)
			return r;
		FontParser::Ref parser = FontParser::Cast(m_fontRef);
		if (!parser || !parser->font.get())
			return SR_MetaError;
		m_font = parser->font;
	}

	if (!m_matRef)
	{
		const Package::Entry::Import *i = asset->entry->Resolve(1);
		if (!i)
			return SR_ParseError;

		m_matRef = asset->entry->Resolve(*i, asset->zone);
		if (!m_matRef)
			return SR_MissingFile;
	}

	if (!m_mat)
	{
		int r = m_matRef->Process(time, flags);
		if (r != SR_Success)
			return r;
		MaterialParser::Ref parser = MaterialParser::Cast(m_matRef);
		if (!parser || !parser->valid || !parser->procedural)
			return SR_MetaError;
		m_mat = parser->material;
	}

	return SR_Success;
}

void TypefaceParser::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<TypefaceParser>();
}

} // asset
