// StringTable.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/StreamDef.h>

#if defined(RAD_OPT_TOOLS)
	#include "Lua/LuaRuntimeDef.h"
#endif

#include <Runtime/PushPack.h>

//! String table. All strings are UTF8 encoded.
class RADENG_CLASS StringTable : public boost::noncopyable {
public:

	typedef boost::shared_ptr<StringTable> Ref;

	//! This enum is serialize don't change the order without modifying the StringTable
	//! cooker version.
	enum LangId {
		LangId_EN,
		LangId_FR,
		LangId_IT,
		LangId_GR,
		LangId_SP,
		LangId_RU,
		LangId_JP,
		LangId_CH,
		LangId_MAX,
		LangId_First = LangId_EN
	};

	enum {
		RAD_FLAG(LangFlag_EN),
		RAD_FLAG(LangFlag_FR),
		RAD_FLAG(LangFlag_IT),
		RAD_FLAG(LangFlag_GR),
		RAD_FLAG(LangFlag_SP),
		RAD_FLAG(LangFlag_RU),
		RAD_FLAG(LangFlag_JP),
		RAD_FLAG(LangFlag_CH),
		LangFlag_ALL = 0xffffffff
	};

	static const char *EN;
	static const char *FR;
	static const char *IT;
	static const char *GR;
	static const char *SP;
	static const char *RU;
	static const char *JP;
	static const char *CH;
	static const char *Langs[];
	static const char *LangTitles[];

	static int Map(const char *lang);
	
	struct Entry {
		typedef zone_map<String, Entry, ZEngineT>::type Map;
		typedef zone_map<LangId, String, ZEngineT>::type Strings;

		Strings strings;
	};
	
	//! Finds the specified string table entry.
	const Entry  *Find(const char *id) const;
	//! Finds the specificed string in the string table.
	const String *Find(const char *id, LangId lang) const;

	//! Loads string table binary data. Data is not referenced after it's loaded and can be freed.
	//! Returns pkg::SR_* codes.
	static int Load(const void *data, AddrSize len, Ref &r);

	RAD_DECLARE_READONLY_PROPERTY(StringTable, entries, const Entry::Map*);

#if defined(RAD_OPT_TOOLS)

	static Ref New();

	//! Loads string table text data.
	static int Load(const char *name, const char *root, Ref &r, int *loadMask = 0);

	void SetString(const char *id, LangId lang, const char *value);
	bool ChangeId(const char *src, const char *dst);
	void DeleteId(const char *id);
	bool CreateId(const char *id);

	bool SaveText(const char *name, const char *root, int saveMask=LangFlag_ALL) const;
	bool SaveBin(stream::IOutputBuffer &ob) const;

#endif

private:

	RAD_DECLARE_GET(entries, const Entry::Map*) { 
		return &m_entries; 
	}

#if defined(RAD_OPT_TOOLS)
	static int lua_Compile(lua_State *L);
#else
	static Ref New();
#endif

	StringTable() {}

	Entry::Map m_entries;
};

#include <Runtime/PopPack.h>
