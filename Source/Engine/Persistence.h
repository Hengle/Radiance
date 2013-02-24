// Persistence.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <Runtime/StreamDef.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

class RADENG_CLASS CloudFile {
public:
	enum Status
	{
		Ready,
		Downloading,
		Error
	};

	typedef boost::shared_ptr<CloudFile> Ref;
	typedef zone_vector<Ref, ZEngineT>::type Vec;

	RAD_DECLARE_READONLY_PROPERTY(CloudFile, ib, stream::IInputBuffer&);
	RAD_DECLARE_READONLY_PROPERTY(CloudFile, ob, stream::IOutputBuffer&);

protected:

	virtual RAD_DECLARE_GET(ib, stream::IInputBuffer&) = 0;
	virtual RAD_DECLARE_GET(ob, stream::IOutputBuffer&) = 0;
};

class RADENG_CLASS CloudStorage {
public:
	
	static bool Enabled();

	static CloudFile::Vec Resolve(const char *name);
	static void ResolveConflict(const CloudFile::Ref &version);
	static bool StartDownloadingLatestVersion(const char *name);
	static CloudFile::Status FileStatus(const char *name);

};

class RADENG_CLASS Persistence {
public:
	typedef boost::shared_ptr<Persistence> Ref;

	struct KeyValue {
		typedef zone_map<String, KeyValue, ZWorldT>::type Map;

		KeyValue() : mVal(0) {
		}
	
		KeyValue(const KeyValue &kv) : mVal(0), sVal(kv.sVal) {
			if (kv.mVal)
				mVal = new (ZWorld) Map(*kv.mVal);
		}

		~KeyValue() {
			if (mVal)
				delete mVal;
		}
		
		String sVal;
		Map *mVal;
	};

	KeyValue *KeyForPath(const char *path);
	const KeyValue *KeyForPath(const char *path) const;

	int IntForKey(const char *path, int def=-1) const;
	bool BoolForKey(const char *path, bool def=false) const;
	float FloatForKey(const char *path, float def=0.0f) const;
	const char *StringForKey(const char *path, const char *def = 0) const;
	Color4 Color4ForKey(const char *path, const Color4 &def = Color4::Zero) const;
	Vec3 Vec3ForKey(const char *path, const Vec3 &def = Vec3::Zero) const;

	static Ref Load(const char *name);
	static Ref Load(stream::InputStream &is);
	static Ref New(const char *name);

	Ref Clone();

	bool Read(const char *name);
	bool Read(stream::InputStream &is);

	bool Save();
	bool Save(const char *name);
	bool Save(stream::OutputStream &os);

	RAD_DECLARE_READONLY_PROPERTY(Persistence, keys, KeyValue::Map*);

private:

	Persistence(const char *name) {
		if (name)
			m_name = name;
	}

	RAD_DECLARE_GET(keys, KeyValue::Map*) { return &const_cast<Persistence*>(this)->m_keys; }

	String m_name;
	KeyValue::Map m_keys;
};

#include <Runtime/PopPack.h>
