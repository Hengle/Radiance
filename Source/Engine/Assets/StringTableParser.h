// StringTableParser.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../FileSystem/FileSystem.h"
#include "../StringTable.h"
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS StringTableParser : public pkg::Sink<StringTableParser> 
#if defined(RAD_OPT_PC_TOOLS)
	, public pkg::Document 
#endif 
{
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_StringTable
	};

	typedef boost::shared_ptr<StringTableParser> Ref;

	StringTableParser();
	virtual ~StringTableParser();

	RAD_DECLARE_READONLY_PROPERTY(StringTableParser, stringTable, const StringTable*);

#if defined(RAD_OPT_PC_TOOLS)
	virtual int Create(
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);
	virtual int Save(
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);
	RAD_DECLARE_READONLY_PROPERTY(StringTableParser, mutableStringTable, StringTable*);
#endif

protected:

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

	RAD_DECLARE_GET(stringTable, const StringTable*) {
		return m_stringTable.get();
	}

#if defined(RAD_OPT_PC_TOOLS)
	RAD_DECLARE_GET(mutableStringTable, StringTable*) {
		return m_stringTable.get();
	}
#endif

	file::HBufferedAsyncIO m_buf;
	StringTable::Ref m_stringTable;

};

} // assets

#include <Runtime/PopPack.h>
