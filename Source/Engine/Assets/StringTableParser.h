/*! \file StringTableParser.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../StringTable.h"
#include <Runtime/File.h>
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS StringTableParser : public pkg::Sink<StringTableParser>  {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_StringTable
	};

	StringTableParser();
	virtual ~StringTableParser();
	
#if defined(RAD_OPT_PC_TOOLS)
	static int Save(
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);
	RAD_DECLARE_READONLY_PROPERTY(StringTableParser, stringTable, StringTable*);
#else
	RAD_DECLARE_READONLY_PROPERTY(StringTableParser, stringTable, const StringTable*);
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

#if defined(RAD_OPT_PC_TOOLS)
	RAD_DECLARE_GET(stringTable, StringTable*) {
		return m_stringTable.get();
	}
#else
	RAD_DECLARE_GET(stringTable, const StringTable*) {
		return m_stringTable.get();
	}
#endif

	file::MMapping::Ref m_mm;
	StringTable::Ref m_stringTable;

};

} // assets

#include <Runtime/PopPack.h>
