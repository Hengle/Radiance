// StringTable.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "StringTable.h"
#include "Packages/PackagesDef.h"

#if defined(RAD_OPT_TOOLS)
	#include "COut.h"
	#include <Runtime/Stream.h>
	#include <Runtime/Stream/STLStream.h>
	#include <Runtime/EndianStream.h>
	#include <Runtime/Container/ZoneVector.h>
	#include <Runtime/FileDef.h>
	#include <fstream>
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
		Entry::Strings::const_iterator it = entry->strings.find(lang);
		if (it != entry->strings.end()) {
			if (!it->second.empty)
				return &it->second;
		}
	}

	return 0;
}

#if defined(RAD_OPT_TOOLS)

int StringTable::Load(const char *name, const char *root, StringTable::Ref &_r, int *_loadMask) {

	StringTable::Ref r(new (ZEngine) StringTable());

	int loadMask = 0;
	if (_loadMask)
		*_loadMask = 0;

	for (int i = 0; i < LangId_MAX; ++i) {

		String path;
		path.Printf("%s.%s.csv", root, StringTable::Langs[i]);

		std::fstream f;

		f.open(path.c_str, std::ios_base::in);
		if (f.fail())
			continue;

		String id, value;

		if (ReadCSVLine(f, id, value)) {
			loadMask |= (1<<i);
			do {
				if (!value.empty || (i == LangId_EN))
					r->SetString(id.c_str, (LangId)i, value.c_str);
			} while (ReadCSVLine(f, id, value));
		}

		f.close();
	}

	if (!loadMask)
		return pkg::SR_FileNotFound;

	_r = r;
	if (_loadMask)
		*_loadMask = loadMask;

	return pkg::SR_Success;
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

bool StringTable::SaveText(const char *name, const char *path, int saveMask) const {

	for (int i = 0; i < LangId_MAX; ++i) {

		if (!(saveMask&(1<<i)))
			continue;

		String spath;
		spath.Printf("%s.%s.csv", path, StringTable::Langs[i]);

		std::fstream f;

		f.open(spath.c_str, std::ios_base::out|std::ios_base::trunc|std::ios_base::binary);
		if (f.fail())
			return false;

		for (Entry::Map::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {

			Entry::Strings::const_iterator string = it->second.strings.find((LangId)i);
			
			WriteCSVToken(f, it->first);
			f << ",";

			if (string != it->second.strings.end()) {
				const String &val = string->second;
				if (!val.empty)
					WriteCSVToken(f, val);
			}

			f << "\n";
		}

		f.close();
	}

	return true;
}

void StringTable::WriteCSVToken(std::ostream &f, const String &_str) {
	f << "\"";

	String str(_str);
	string::UTF32Buf utf = str.ToUTF32();

	String mod;
	const String kQuotes(CStr("\"\""));

	string::UTF32Buf::const_iterator begin = utf.begin;
	string::UTF32Buf::const_iterator end = utf.end;

	for (string::UTF32Buf::const_iterator x = begin; x < end; ++x) {
		if (*x == '\"') {
			mod += kQuotes;
		} else {
			char c[5] = {0, 0, 0, 0, 0};
			string::utf32to8(c, x, 1);
			mod += String(c, string::RefTag);
		}
	}

	f << mod << "\"";
}

bool StringTable::ReadCSVLine(std::istream &is, String &id, String &value) {
	return ReadCSVToken(is, id) && ReadCSVToken(is, value);
}

bool StringTable::ReadCSVToken(std::istream &is, String &token) {
	char c;
	is.read(&c, 1);

	if (is.eof())
		return false;

	bool quoted = c == '\"';
	bool terminated = !quoted;
	int qcount = 0;

	token.Clear();

	if (!quoted) {
		if (c == '\n' || c == ',')
			return true; // blank
		token += c;
	}

	for (;;) {
		is.read(&c, 1);
		if (is.eof())
			break;
		
		if (c == '\n' || c == ',') {
			if (quoted) {
				if (qcount)
					break;
			} else {
				break;
			}
		}
		
		if (c == '\"') {
			++qcount;
			if (qcount == 2) {
				token += '\"';
				qcount = 0;
			}
		} else {
			token += c;
		}
	}

	return !token.empty;
}

bool StringTable::SaveBin(stream::IOutputBuffer &ob) const {
	stream::LittleOutputStream os(ob);

	if (!os.Write((U32)StringTableId) || !os.Write((U32)StringTableVersion))
		return false;

	// count the number of strings.
	U32 numStrings = 0;
	for (Entry::Map::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
		if (!it->second.strings.empty()) {
			// Only count non-blank strings.
			for (Entry::Strings::const_iterator x = it->second.strings.begin(); x != it->second.strings.end(); ++x) {
				if (!x->second.empty) {
					++numStrings;
					break;
				}
			}
		}
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
				if (!x->second.empty) {
					langMask |= (1<<i);
					strings[i] = x->second;
				}
			}
		}

		if (!langMask)
			continue; // all blanks or no valid languages.

		if (!os.Write(langMask))
			return false;

		if (!os.Write((U16)(name.numBytes.get()+1)))
			return false;

		if (os.Write(name.c_str.get(), (stream::SPos)name.numBytes.get()+1, 0) != (stream::SPos)name.numBytes.get()+1)
			return false;

		for (int i = 0; i < LangId_MAX; ++i) {
			if (langMask & (1<<i)) {
				if (!os.Write((U16)(strings[i].numBytes.get()+1)))
					return false;
				if (os.Write(strings[i].c_str.get(), (stream::SPos)strings[i].numBytes.get()+1, 0) != (stream::SPos)strings[i].numBytes.get()+1)
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
	bytes += sizeof(U32);

	Ref r(New());

	for (U32 i = 0; i < numStrings; ++i ) {
		CHECK_SIZE(sizeof(U16)*2);
		U16 langMask = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);

		U16 size = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);
		CHECK_SIZE(size);

		const char *id = reinterpret_cast<const char *>(bytes);
		bytes += size;

		Entry e;
		
		for (int k = 0; k < LangId_MAX; ++k) {
			if (langMask & (1<<k)) {
				CHECK_SIZE(sizeof(U16));
				size = *reinterpret_cast<const U16*>(bytes);
				bytes += sizeof(U16);
				CHECK_SIZE(size);
				const char *sz = reinterpret_cast<const char*>(bytes);
				e.strings[(LangId)k] = String(sz);
				bytes += size;
			}
		}

		RAD_ASSERT(!e.strings.empty());
		r->m_entries[String(id)] = e;
	}

	_r = r;

	return pkg::SR_Success;
}


