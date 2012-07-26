// StringTableParser.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "StringTableParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

StringTableParser::StringTableParser() {
}

StringTableParser::~StringTableParser() {
}

int StringTableParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Unload|P_Parse)))
		return SR_Success;

	if (m_stringTable && (flags&(P_Load|P_Parse)))
		return SR_Success;

	if (flags&P_Unload) {
		m_stringTable.reset();
		m_mm.reset();
		return SR_Success;
	}

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked)
		return Load(time, engine, asset, flags);
#endif
	return LoadCooked(time, engine, asset, flags);
}

#if defined(RAD_OPT_TOOLS)
int StringTableParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {

	const String *s = asset->entry->KeyValue<String>("Source.Root", P_TARGET_FLAGS(flags));
	if (!s)
		return pkg::SR_MetaError;

	String native;
	if (!engine.sys->files->ExpandToNativePath(s->c_str, native, ~file::kFileMask_PakFiles))
		return SR_ErrorGeneric;

	int r = StringTable::Load(asset->path, native.c_str, m_stringTable);

	if ((flags&P_Create) && (r == SR_FileNotFound)) // this is OK for this asset just make a new table.
		m_stringTable = StringTable::New();

	if (!m_stringTable)
		return r;

	return SR_Success;
}
#endif

#if defined(RAD_OPT_PC_TOOLS)

int StringTableParser::Save(
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!m_stringTable || (asset->type != AT_StringTable))
		return SR_ErrorGeneric;

	const String *s = asset->entry->KeyValue<String>("Source.Root", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	String native;
	if (!engine.sys->files->ExpandToNativePath(s->c_str, native, ~file::kFileMask_PakFiles))
		return SR_ErrorGeneric;

	if (!m_stringTable->SaveText(asset->path, native.c_str))
		return SR_IOError;

	return SR_Success;
}

#endif

int StringTableParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!m_mm) {
		String path(CStr("Cooked/"));
		path += CStr(asset->path);
		path += ".bin";

		m_mm = engine.sys->files->MapFile(path.c_str, ZStringTables);
		if (!m_mm)
			return SR_FileNotFound;
	}

	int r = StringTable::Load(m_mm->data, m_mm->size, m_stringTable);
	m_mm.reset();
	return r;
}

void StringTableParser::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<StringTableParser>();
}

} // asset
