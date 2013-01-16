/*! \file CVars.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "CVars.h"
#include "App.h"
#include "Engine.h"
#include <Runtime/File.h>
#include <Runtime/Stream.h>
#include <Runtime/Endian.h>

enum {
	kCVarFileId = RAD_FOURCC('r', 'c', 'v', 'r'),
	kCVarFileVersion = 1
};

CVarZone &CVarZone::Globals() {
	static CVarZone s_globals;
	return s_globals;
}

void CVarZone::Open(const char *path) {
	m_open = true;
	
	if (!path) {
		m_path.Clear();
		return;
	}

	m_path = path;

	file::MMFileInputBuffer::Ref ib = App::Get()->engine->sys->files->OpenInputBuffer(
		path,
		ZEngine,
		Kilo
	);
	
	if (!ib)
		return;

	stream::InputStream is(*ib);
	Load(is);
}

void CVarZone::Close() {
	Flush();
	m_open = false;

#if defined(RAD_OPT_DEBUG)
	for (CVarMap::iterator it = m_cvars.begin(); it != m_cvars.end(); ++it) {
		it->second->m_zone = 0;
	}
#endif
}

void CVarZone::Flush() {

	if (m_path.empty)
		return;

	FILE *fp = App::Get()->engine->sys->files->fopen(m_path.c_str, "wb");
	if (!fp)
		return;

	file::FILEOutputBuffer ob(fp);
	stream::OutputStream os(ob);
	Save(os);
}

CVar *CVarZone::Find(const char *name) const {
	CVarMap::const_iterator it = m_cvars.find(CStr(name));
	if (it != m_cvars.end())
		return it->second;
	return 0;
}

CVarVec CVarZone::StartsWith(const char *name) const {
	CVarVec v;

	for (CVarMap::const_iterator it = m_cvars.begin(); it != m_cvars.end(); ++it) {
		if (it->first.StrStr(name) == 0)
			v.push_back(it->second);
	}

	return v;
}

void CVarZone::Load(stream::InputStream &is) {
	U32 id;
	U32 version;

	if (!(is.Read(&id)&&is.Read(&version)))
		return;

	if (id != kCVarFileId ||
		version != kCVarFileVersion) {
			return;
	}

	U32 cvarCount;
	if (!is.Read(&cvarCount))
		return;

	String name;
	U32 size;

	for (U32 i = 0; i < cvarCount; ++i) {
		if (!is.Read(&name))
			return;
		if (!is.Read(&size))
			return;

		CVar *cvar = Find(name.c_str);
		if (cvar && cvar->m_save) {
#if defined(RAD_OPT_DEBUG)
			stream::SPos startPos = is.InPos();
#endif
			if (!cvar->Read(is))
				return;

#if defined(RAD_OPT_DEBUG)
			stream::SPos endPos = is.InPos();
			// if you get this a CVar did not serialize all its data
			RAD_ASSERT((endPos-startPos) == (stream::SPos)size);
#endif
		} else {
			if (!is.SeekIn(stream::StreamCur, size, 0))
				return;
		}
	}
}

void CVarZone::Save(stream::OutputStream &os) {
	if (!os.Write((U32)kCVarFileId))
		return;
	if (!os.Write((U32)kCVarFileVersion))
		return;

	if (!os.Write((U32)m_cvarSaveCount))
		return;

	int numSaved = 0;
	for (CVarMap::const_iterator it = m_cvars.begin(); it != m_cvars.end(); ++it) {
		const CVar *cvar = it->second;

		if (!cvar->m_save)
			continue;

		if (!os.Write(cvar->m_name))
			return;

		stream::SPos startPos = os.OutPos();

		if (!os.Write((U32)0)) // place-holder
			return;

		if (!cvar->Write(os))
			return;

		stream::SPos endPos = os.OutPos();

		// write size.
		U32 size = (endPos-startPos) - sizeof(U32);

		if (!os.SeekOut(stream::StreamBegin, startPos, 0))
			return;

		if (!os.Write(size))
			return;

		if (!os.SeekOut(stream::StreamBegin, endPos, 0))
			return;

		++numSaved;
	}

	RAD_ASSERT(numSaved == m_cvarSaveCount);
}

void CVarZone::Register(CVar *cvar) {
	// you cannot register global cvars dynamically! please declare them as static variables.
	RAD_ASSERT((this != &Globals()) || !m_open);

#if defined(RAD_OPT_DEBUG)
	if (this != &Globals()) {
		RAD_ASSERT_MSG(!cvar->m_save, "Only Global CVars can be marked for serialize!");
		RAD_ASSERT_MSG(Globals().Find(cvar->m_name.c_str) == 0, "A CVar with this name already exists!");
	}
#endif

#if defined(RAD_OPT_DEBUG)
	std::pair<CVarMap::iterator, bool> r = 
#endif
		m_cvars.insert(CVarMap::value_type(cvar->m_name, cvar));
	RAD_ASSERT_MSG(r.second, "A CVar with this name already exists!");

	if (cvar->m_save)
		++m_cvarSaveCount;
}

///////////////////////////////////////////////////////////////////////////////

bool CVarString::Read(stream::InputStream &is) {
	return is.Read(&m_value);
}

bool CVarString::Write(stream::OutputStream &os) const {
	return os.Write(m_value);
}

///////////////////////////////////////////////////////////////////////////////

bool CVarInt::Read(stream::InputStream &is) {
	return is.Read(&m_value);
}

bool CVarInt::Write(stream::OutputStream &os) const {
	return os.Write(m_value);
}

///////////////////////////////////////////////////////////////////////////////

bool CVarBool::Read(stream::InputStream &is) {
	U8 b;
	if (!is.Read(&b))
		return false;
	m_value = b ? true : false;
	return true;
}

bool CVarBool::Write(stream::OutputStream &os) const {
	U8 b = m_value ? 1 : 0;
	return os.Write(b);
}

bool CVarBool::Parse(const char *value) {
	const String kValue(CStr(value));
	if (kValue == CStr("0")) {
		m_value = false;
		if (saves)
			zone->Flush();
		return true;
	} else if (kValue == CStr("1")) {
		m_value = true;
		if (saves)
			zone->Flush();
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool CVarFloat::Read(stream::InputStream &is) {
	return is.Read(&m_value);
}

bool CVarFloat::Write(stream::OutputStream &os) const {
	return os.Write(m_value);
}
