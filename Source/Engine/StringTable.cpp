// StringTable.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "StringTable.h"
#include "Packages/PackagesDef.h"

#if defined(RAD_OPT_TOOLS)
	#include "COut.h"
	#include "Lua/LuaRuntime.h"
	#include <Runtime/Stream.h>
	#include <Runtime/Stream/STLStream.h>
	#include <Runtime/EndianStream.h>
	#include <Runtime/Container/ZoneVector.h>
	#include <Runtime/FileDef.h>
	#include <fstream>

	#define THIS "@this"
	#define LANG "@lang"

#endif

enum {
	StringTableId = RAD_FOURCC_LE('S', 'T', 'R', 'T'),
	StringTableVersion = 1
};

const char *StringTable::EN = "en";
const char *StringTable::FR = "fr";
const char *StringTable::IT = "it";
const char *StringTable::GR = "gr";
const char *StringTable::SP = "sp";
const char *StringTable::RU = "ru";
const char *StringTable::JP = "jp";
const char *StringTable::CH = "ch";

const char *StringTable::Langs[] = {
	StringTable::EN,
	StringTable::FR,
	StringTable::IT,
	StringTable::GR,
	StringTable::SP,
	StringTable::RU,
	StringTable::JP,
	StringTable::CH,
	0
};

const char *StringTable::LangTitles[] = {
	"English",
	"French",
	"Italian",
	"German",
	"Spanish",
	"Russian",
	"Japanese",
	"Chinese",
	0
};

int StringTable::Map(const char *lang) {
	RAD_ASSERT(lang);
	for (int i = 0; Langs[i]; ++i) {
		if (string::cmp(Langs[i], lang) == 0)
			return i;
	}
	return -1;
}

StringTable::Ref StringTable::New() {
	return Ref(new (ZEngine) StringTable());
}

const StringTable::Entry *StringTable::Find(const char *id) const {
	
	RAD_ASSERT(id);

	Entry::Map::const_iterator it = m_entries.find(String(id));
	if (it == m_entries.end())
		return 0;
	return &it->second;
}

const String *StringTable::Find(const char *id, LangId lang) const {

	const StringTable::Entry *entry = Find(id);
	if (entry) {
		RAD_ASSERT(lang);
		Entry::Strings::const_iterator it = entry->strings.find(lang);
		if (it != entry->strings.end()) {
			if (!it->second.empty())
				return &it->second;
		}
	}

	return 0;
}

#if defined(RAD_OPT_TOOLS)

int StringTable::Load(const char *name, const wchar_t *root, StringTable::Ref &_r, int *_loadMask) {

	StringTable::Ref r(new (ZEngine) StringTable());

	lua::State::Ref L(new (ZEngine) lua::State("StringTable"));

	luaL_Reg lr[] = {
		{ "Compile", lua_Compile },
		{ 0, 0 }
	};

	lua::RegisterGlobals(L->L, 0, lr);
	lua_pushlightuserdata(L->L, r.get());
	lua_setfield(L->L, LUA_REGISTRYINDEX, THIS);

	typedef lua::StreamLoader<8*Kilo, lua::StackMemTag> Loader;

	int loadMask = 0;
	if (_loadMask)
		*_loadMask = 0;

	for (int i = 0; i < LangId_MAX; ++i) {

		WString wpath;

		wpath.format(L"%s.%s", root, string::Widen(StringTable::Langs[i]).c_str());

		String spath(string::Shorten(wpath.c_str()));

		std::fstream f;

		f.open(spath.c_str(), std::ios_base::in);
		if (f.fail())
			continue;

		lua_pushinteger(L->L, i);
		lua_setfield(L->L, LUA_REGISTRYINDEX, LANG);

		typedef stream::basic_istream_adapter<char> InputStream;
		InputStream is(f);

		Loader loader(is);
		
		if (lua_load(L->L, &Loader::Read, &loader, spath.c_str())) {
			COut(C_Error) << "StringTable::Load(parse): " << lua_tostring(L->L, -1) << std::endl;
			r.reset();
			return pkg::SR_ScriptError;
		}

		if (lua_pcall(L->L, 0, 0, 0))
		{
			COut(C_Error) << "StringTable::Load(run): " << lua_tostring(L->L, -1) << std::endl;
			r.reset();
			return pkg::SR_ScriptError;
		}

		loadMask |= (1<<i);
	}

	if (!loadMask)
		return file::ErrorFileNotFound;

	_r = r;
	if (_loadMask)
		*_loadMask = loadMask;

	return pkg::SR_Success;
}

int StringTable::lua_Compile(lua_State *L) {

	lua::Variant::Map strings;
	ParseVariantTable(L, strings, true);

	lua_getfield(L, LUA_REGISTRYINDEX, THIS);
	StringTable *table = reinterpret_cast<StringTable*>(lua_touserdata(L, -1));
	lua_pop(L, 1);
	
	lua_getfield(L, LUA_REGISTRYINDEX, LANG);
	int langId = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	for (lua::Variant::Map::const_iterator it = strings.begin(); it != strings.end(); ++it) {
		const String &name = it->first;

		const String *value = static_cast<const String*>(it->second);
		if (!value)
			luaL_error(L, "StringTable::lua_Compile(): expected string value for string '%s' language '%s'", name.c_str(), StringTable::Langs[langId]);

		table->SetString(name.c_str(), (LangId)langId, value->c_str());
	}

	return 0;
}

void StringTable::SetString(const char *id, LangId lang, const char *value) {
	RAD_ASSERT(id);
	RAD_ASSERT(lang >= 0 && lang < LangId_MAX);

	String sid(id);

	Entry::Map::iterator it = m_entries.find(sid);

	if (value) {
		String sval(value);
		if (it == m_entries.end()) {
			Entry e;
			e.strings[lang] = sval;
			m_entries[sid] = e;
		} else {
			it->second.strings[lang] = sval;
		}
	} else if (it != m_entries.end()) {
		it->second.strings.erase(lang);
	}
}

bool StringTable::ChangeId(const char *src, const char *dst) {
	
	RAD_ASSERT(src);
	RAD_ASSERT(dst);

	String ssrc(src);

	Entry::Map::iterator it = m_entries.find(ssrc);
	if (it == m_entries.end())
		return false;

	String sdst(dst);
	if (m_entries.find(sdst) != m_entries.end())
		return false; // collision.

	Entry e;
	e.strings.swap(it->second.strings);

	m_entries.erase(it);
	m_entries[sdst] = e;

	return true;
}

void StringTable::DeleteId(const char *id) {
	RAD_ASSERT(id);
	String sid(id);
	m_entries.erase(sid);
}

bool StringTable::CreateId(const char *id) {
	RAD_ASSERT(id);
	String sid(id);
	Entry e;
	return m_entries.insert(Entry::Map::value_type(sid, e)).second;
}

bool StringTable::SaveText(const char *name, const wchar_t *path, int saveMask) const {

	for (int i = 0; i < LangId_MAX; ++i) {

		if (!(saveMask&(1<<i)))
			continue;

		WString wpath;
		wpath.format(L"%s.%s", path, string::Widen(StringTable::Langs[i]).c_str());

		String spath(string::Shorten(wpath.c_str()));

		std::fstream f;

		f.open(spath.c_str(), std::ios_base::out|std::ios_base::trunc);
		if (f.fail())
			return false;

		f << "-- " << name << "." << StringTable::Langs[i] << std::endl;
		f << "-- Radiance String Table" << std::endl << std::endl;

		f << "local table = {" << std::endl;

		int c = 0;

		for (Entry::Map::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {

			Entry::Strings::const_iterator string = it->second.strings.find((LangId)i);
			
			if (c)
				f << "," << std::endl;

			const String &name = it->first;
			
			f << "\t[\"" << name.c_str() << "\"] = \"";

			if (string != it->second.strings.end()) {
				const String &val = string->second;
				String mod;

				for (String::const_iterator x = val.begin(); x != val.end(); ++x) {
					if (*x == '\\') {
						mod += "\\\\";
					} else {
						mod += *x;
					}
				}

				f << mod;
			}
			f << "\"";

			++c;
		}

		f << std::endl << "}" << std::endl << std::endl << "Compile(table)" << std::endl;
		f << std::flush;
		f.close();
	}

	return true;
}

bool StringTable::SaveBin(stream::IOutputBuffer &ob) const {
	stream::LittleOutputStream os(ob);

	if (!os.Write((U32)StringTableId) || !os.Write((U32)StringTableVersion))
		return false;

	// count the number of strings.
	U32 numStrings = 0;
	for (Entry::Map::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
		if (!it->second.strings.empty())
			++numStrings;
	}

	if (!os.Write(numStrings))
		return false;

	for (Entry::Map::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
		const Entry &e = it->second;

		if (e.strings.empty())
			continue;

		const String &name = it->first;

		U16 langMask = 0;
		String strings[LangId_MAX];

		for (int i = 0; i < LangId_MAX; ++i) {
			Entry::Strings::const_iterator x = e.strings.find((LangId)i);
			if (x != e.strings.end()) {
				langMask |= (1<<i);
				strings[i] = x->second;
			}
		}

		if (!langMask)
			return false; // consistency failure.

		if (!os.Write(langMask))
			return false;

		if (!os.Write((U16)name.length()))
			return false;

		if (os.Write(name.c_str(), (stream::SPos)name.length(), 0) != (stream::SPos)name.length())
			return false;

		for (int i = 0; i < LangId_MAX; ++i) {
			if (langMask & (1<<i)) {
				if (!os.Write((U16)strings[i].length()))
					return false;
				if (os.Write(strings[i].c_str(), (stream::SPos)strings[i].length(), 0) != (stream::SPos)strings[i].length())
					return false;
			}
		}
	}

	return true;
}

#endif

#define CHECK_SIZE(_size) if (((bytes+(_size))-reinterpret_cast<const U8*>(data)) > (int)len) return pkg::SR_CorruptFile;

int StringTable::Load(const void *data, AddrSize len, Ref &_r) {

	const U8 *bytes = reinterpret_cast<const U8*>(data);

	CHECK_SIZE(sizeof(U32)*2);
	U32 id = *reinterpret_cast<const U32*>(bytes);
	U32 version = *reinterpret_cast<const U32*>(bytes+4);
	
	if (id != StringTableId)
		return pkg::SR_InvalidFormat;
	if (version != StringTableVersion)
		return pkg::SR_BadVersion;

	bytes += sizeof(U32)*2;

	CHECK_SIZE(sizeof(U32));
	U32 numStrings = *reinterpret_cast<const U32*>(bytes);
	bytes += 32;

	Ref r(New());

	for (U32 i = 0; i < numStrings; ++i ) {
		CHECK_SIZE(sizeof(U16)*2);
		U16 langMask = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);

		U16 len = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);
		CHECK_SIZE(len);

		const char *id = reinterpret_cast<const char *>(bytes);

		Entry e;
		
		for (int k = 0; k < LangId_MAX; ++k) {
			if (langMask & (1<<k)) {
				CHECK_SIZE(sizeof(U16));
				len = *reinterpret_cast<const U16*>(bytes);
				bytes += sizeof(U16);
				CHECK_SIZE(len);
				const char *sz = reinterpret_cast<const char*>(bytes);
				e.strings[(LangId)k] = String(sz);
				bytes += len;
			}
		}

		RAD_ASSERT(!e.strings.empty());
		r->m_entries[String(id)] = e;
	}

	_r = r;

	return pkg::SR_Success;
}


