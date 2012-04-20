// MusicParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "MusicParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

MusicParser::MusicParser()
{
}

MusicParser::~MusicParser()
{
}

int MusicParser::Process(
	const xtime::TimeSlice &time, 
	Engine &engine, 
	const pkg::Asset::Ref &asset, 
	int flags
)
{
	if (flags&P_Unload)
	{
		m_file.Close();
		return SR_Success;
	}

	if (flags&(P_Load|P_Parse) && !m_file)
	{
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked)
			return Load(time, engine, asset, flags);
#endif
		return LoadCooked(time, engine, asset, flags);
	}

	return SR_Success;
}

#if defined(RAD_OPT_TOOLS)
int MusicParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	int media = file::AllMedia;
	int r = engine.sys->files->OpenFile(
		string::Widen(s->c_str()).c_str(),
		media,
		m_file,
		file::HIONotify()
	);

	return r;
}
#endif

int MusicParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	WString path(L"Cooked/");
	path += string::Widen(asset->path);
	path += L".bin";

	int media = file::AllMedia;
	int r = engine.sys->files->OpenFile(
		path.c_str(),
		media,
		m_file,
		file::HIONotify()
	);

	return r;
}

void MusicParser::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<MusicParser>();
}

} // asset
