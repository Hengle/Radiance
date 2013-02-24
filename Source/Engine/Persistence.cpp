// Persistence.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Persistence.h"
#include "App.h"
#include "Engine.h"
#include <Runtime/Stream.h>
#include <Runtime/File.h>
#include <stdio.h>

#if defined(RAD_OPT_APPLE)
FILE *AppleOpenPersistence(const char *name, const char *mode);
#endif

namespace {

enum {
	Tag = RAD_FOURCC_LE('P', 'E', 'R', 'S'),
	Version = 2,
	kType_String = 1,
	kType_Map = 2
};

bool LoadArray(stream::InputStream &is, Persistence::KeyValue::Map &keys) {
	U32 numKeys;
	if (!is.Read(&numKeys))
		return false;
	for (U32 i = 0; i < numKeys; ++i) {
		char key[1024];

		U32 len;
		if (!is.Read(&len))
			return false;
		if (len >= 1023)
			return false;
		if (is.Read(key, (stream::SPos)len, 0) != (stream::SPos)len)
			return false;
		key[len] = 0;

		U8 type;
		if (!is.Read(&type))
			return false;

		Persistence::KeyValue val;

		if (type == kType_String) {
			char value[1023];
			if (!is.Read(&len))
				return false;
			if (len >= 1023)
				return false;
			if (is.Read(value, (stream::SPos)len, 0) != (stream::SPos)len)
				return false;
			value[len] = 0;
			val.sVal = value;
		} else {
			RAD_ASSERT(type == kType_Map);
			val.mVal = new (ZWorld) Persistence::KeyValue::Map();
			if (!LoadArray(is, *val.mVal))
				return false;
		}

		// avoid deep copy constructor.
		Persistence::KeyValue &kv = keys[String(key)];
		kv.sVal = val.sVal;
		kv.mVal = val.mVal;
		val.mVal = 0; // don't free
	}

	return true;
}

bool LoadStorage(stream::InputStream &is, Persistence::KeyValue::Map &keys) {
	bool r = false;
	keys.clear();

	U32 tag, ver;
	if (is.Read(&tag) && is.Read(&ver)) {
		if ((tag == Tag) && (ver <= Version)) {
			if (ver > 1) {
				r = LoadArray(is, keys);
			} else {
				U32 numPairs;
				if (is.Read(&numPairs)) {
					char key[1024];
					char val[1024];

					U32 i;
					U32 len;

					for (i = 0; i < numPairs; ++i) {
						if (!is.Read(&len))
							break;
						if (len >= 1023)
							break;
						if (is.Read(key, (stream::SPos)len, 0) != (stream::SPos)len)
							break;
						key[len] = 0;

						if (!is.Read(&len))
							break;
						if (len >= 1023)
							break;
						if (is.Read(val, (stream::SPos)len, 0) != (stream::SPos)len)
							break;
						val[len] = 0;

						keys[String(key)].sVal = String(val);
					}

					r = i == numPairs;
				}
			}
		}
	}

	return r;
}

bool LoadStorage(const char *name, Persistence::KeyValue::Map &keys) {
	FILE *fp = 0;
#if defined(RAD_OPT_APPLE)
	fp = AppleOpenPersistence(name, "rb");
#else
	String path(CStr("@r:/") + name);
	fp = App::Get()->engine->sys->files->fopen(path.c_str, "rb");
#endif

	if (!fp)
		return false;

	file::FILEInputBuffer ib(fp);
	stream::InputStream is(ib);

	bool r = LoadStorage(is, keys);
	fclose(fp);
	return r;
}

bool SaveArray(stream::OutputStream &os, const Persistence::KeyValue::Map &keys) {
	if (!os.Write((U32)keys.size()))
		return false;
	for (Persistence::KeyValue::Map::const_iterator it = keys.begin(); it != keys.end(); ++it) {
		RAD_ASSERT(it->first.length < 1024);
		if (!os.Write((U32)it->first.length.get()))
			return false;
		if (os.Write(it->first.c_str.get(), (stream::SPos)it->first.length.get(), 0) != (stream::SPos)it->first.length.get())
			return false;

		if (it->second.mVal) {
			if (!os.Write((U8)kType_Map))
				return false;
			if (!SaveArray(os, *it->second.mVal))
				return false;
		} else {
			if (!os.Write((U8)kType_String))
				return false;
			if (!os.Write((U32)it->second.sVal.length.get()))
				return false;
			if (os.Write(it->second.sVal.c_str.get(), (stream::SPos)it->second.sVal.length.get(), 0) != (stream::SPos)it->second.sVal.length.get())
				return false;
		}
	}

	return true;
}

bool SaveStorage(stream::OutputStream &os, const Persistence::KeyValue::Map &keys) {
	bool r = false;
	if (os.Write((U32)Tag) && os.Write((U32)Version)) {
		r = SaveArray(os, keys);
	}

	return r;
}

bool SaveStorage(const char *name, const Persistence::KeyValue::Map &keys) {
	FILE *fp = 0;
#if defined(RAD_OPT_APPLE)
	fp = AppleOpenPersistence(name, "wb");
#else
	String path(CStr("@r:/") + name);
	fp = App::Get()->engine->sys->files->fopen(path.c_str, "wb");
#endif

	if (!fp)
		return false;

	file::FILEOutputBuffer ob(fp);
	stream::OutputStream os(ob);

	bool r = SaveStorage(os, keys);
	fclose(fp);
	return r;
}

} // namespace

Persistence::KeyValue *Persistence::KeyForPath(const char *path) {
	String x;
	KeyValue::Map *map = &m_keys;

	for (const char *z = path; *z; ++z) {
		if (*z == '/') {
			if (!x.empty) {
				KeyValue::Map::iterator it = map->find(x);
				if (it == map->end())
					return 0;
				map = it->second.mVal;
				if (!map)
					return 0;
			}
			x.Clear();
		} else {
			x += *z;
		}
	}

	if (!x.empty) {
		KeyValue::Map::iterator it = map->find(x);
		if (it == map->end())
			return 0;
		return &it->second;
	}

	return 0;
}

const Persistence::KeyValue *Persistence::KeyForPath(const char *path) const {
	String x;
	const KeyValue::Map *map = &m_keys;

	for (const char *z = path; *z; ++z) {
		if (*z == '/') {
			if (!x.empty) {
				KeyValue::Map::const_iterator it = map->find(x);
				if (it == map->end())
					return 0;
				map = it->second.mVal;
				if (!map)
					return 0;
			}
			x.Clear();
		} else {
			x += *z;
		}
	}

	if (!x.empty) {
		KeyValue::Map::const_iterator it = map->find(x);
		if (it == map->end())
			return 0;
		return &it->second;
	}

	return 0;
}

int Persistence::IntForKey(const char *path, int def) const {
	const KeyValue *kv = KeyForPath(path);
	if (!kv || kv->mVal)
		return def;
	int r;
#define CAWARN_DISABLE 6031 // return value ignored
#include <Runtime/PushCAWarnings.h>
	sscanf(kv->sVal.c_str, "%d", &r);
#include <Runtime/PopCAWarnings.h>
	return r;
}

bool Persistence::BoolForKey(const char *path, bool def) const {
	const KeyValue *kv = KeyForPath(path);
	if (!kv || kv->mVal)
		return def;
	return kv->sVal == "true";
}

float Persistence::FloatForKey(const char *path, float def) const {
	const KeyValue *kv = KeyForPath(path);
	if (!kv || kv->mVal)
		return def;
	float r;
#define CAWARN_DISABLE 6031 // return value ignored
#include <Runtime/PushCAWarnings.h>
	sscanf(kv->sVal.c_str, "%f", &r);
#include <Runtime/PopCAWarnings.h>
	return r;
}

const char *Persistence::StringForKey(const char *path, const char *def) const {
	const KeyValue *kv = KeyForPath(path);
	if (!kv || kv->mVal)
		return def;
	return kv->sVal.c_str;
}

Color4 Persistence::Color4ForKey(const char *path, const Color4 &def) const {
	const KeyValue *kv = KeyForPath(path);
	if (!kv || kv->mVal)
		return def;
	int r, g, b, a;
#define CAWARN_DISABLE 6031 // return value ignored
#include <Runtime/PushCAWarnings.h>
	sscanf(kv->sVal.c_str, "%d %d %d %d", &r, &g, &b, &a);
#include <Runtime/PopCAWarnings.h>
	return Color4(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

Vec3 Persistence::Vec3ForKey(const char *path, const Vec3 &def) const {
	const KeyValue *kv = KeyForPath(path);
	if (!kv || kv->mVal)
		return def;
	float x, y, z;
#define CAWARN_DISABLE 6031 // return value ignored
#include <Runtime/PushCAWarnings.h>
	sscanf(kv->sVal.c_str, "%f %f %f", &x, &y, &z);
#include <Runtime/PopCAWarnings.h>
	return Vec3(x, y, z);
}

Persistence::Ref Persistence::Load(const char *name) {
	Ref r(new (ZEngine) Persistence(name));
	r->Read(name);
	return r;
}

Persistence::Ref Persistence::Load(stream::InputStream &is) {
	Ref r(new (ZEngine) Persistence(0));
	r->Read(is);
	return r;
}

Persistence::Ref Persistence::New(const char *name) {
	RAD_ASSERT(name);
	return Ref(new (ZEngine) Persistence(name));
}

Persistence::Ref Persistence::Clone() {
	Persistence::Ref r(new Persistence(0));
	r->m_keys = m_keys;
	return r;
}

bool Persistence::Read(const char *name) {
	if (!name)
		return false;

	bool r = LoadStorage(name, m_keys);

	if (!r)
		m_keys.clear();
	
	return r;
}

bool Persistence::Read(stream::InputStream &is) {
	bool r = LoadStorage(is, m_keys);

	if (!r)
		m_keys.clear();
	
	return r;
}

bool Persistence::Save() {
	if (!m_name.empty)
		return SaveStorage(m_name.c_str, m_keys);
	return true;
}

bool Persistence::Save(const char *name) {
	RAD_ASSERT(name);
	m_name = name;
	return Save();
}

bool Persistence::Save(stream::OutputStream &os) {
	return SaveStorage(os, m_keys);
}

