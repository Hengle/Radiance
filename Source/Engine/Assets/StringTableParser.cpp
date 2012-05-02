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

	const String *s = asset->entry->KeyValue<String>("Source.Root", P_TARGET_FLAGS(flags));
	if (!s)
		return pkg::SR_MetaError;

	wchar_t path[256];
	wchar_t native[256];
	string::cpy(path, L"9:/");
	string::cat(path, engine.sys->files->hddRoot.get());
	string::cat(path, L"/");
	string::cat(path, string::Widen(s->c_str()).c_str());
	if (!file::ExpandToNativePath(path, native, 256))
		return SR_ErrorGeneric;

	int r = StringTable::Load(asset->path, native, m_stringTable);

	if ((flags&P_Create) && (r == file::ErrorFileNotFound)) // this is OK for this asset just make a new table.
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

	wchar_t path[256];
	wchar_t native[256];
	string::cpy(path, L"9:/");
	string::cat(path, engine.sys->files->hddRoot.get());
	string::cat(path, L"/");
	string::cat(path, string::Widen(s->c_str()).c_str());
	if (!file::ExpandToNativePath(path, native, 256))
		return SR_ErrorGeneric;

	if (!m_stringTable->SaveText(asset->path, native))
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
