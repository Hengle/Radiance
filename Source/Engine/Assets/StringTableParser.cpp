// StringTableParser.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

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
		m_buf.Close();
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

	const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return pkg::SR_MetaError;

	file::HStreamInputBuffer buf;
	int media = file::AllMedia;
	int r = engine.sys->files->OpenFileStream(
		string::Widen(s->c_str()).c_str(),
		media,
		buf,
		file::HIONotify()
	);

	if (r != SR_Success)
		return r;

	m_stringTable = StringTable::Load(buf->buffer, asset->path);
	if (!m_stringTable)
		return SR_CorruptFile;

	return SR_Success;
}
#endif

#if defined(RAD_OPT_PC_TOOLS)
// pkg::Document interface
int StringTableParser::Create(
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	m_stringTable = StringTable::New();
	m_buf.Close();
	return pkg::SR_Success;
}

int StringTableParser::Save(
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!m_stringTable || (asset->type != AT_StringTable))
		return SR_ErrorGeneric;

	const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	if (!m_stringTable->SaveText(asset->path, s->c_str()))
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
	if (!m_buf) {
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

		if (r == SR_Pending) {
			if (!time.infinite)
				return r;
			m_buf->WaitForCompletion();
		} else if (r != SR_Success) {
			return r;
		}
	}

	if (m_buf->result != SR_Success)
		return m_buf->result;

	int r = StringTable::Load(m_buf->data->ptr, m_buf->data->size, m_stringTable);
	m_buf.Close();
	return r;
}

void StringTableParser::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<StringTableParser>();
}

} // asset
