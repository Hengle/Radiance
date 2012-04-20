// Persistence.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "World/Keys.h"
#include <Runtime/StreamDef.h>
#include <Runtime/PushPack.h>
#include <Runtime/Container/ZoneVector.h>

class RADENG_CLASS CloudFile
{
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

class RADENG_CLASS CloudStorage
{
public:
	
	static bool Enabled();

	static CloudFile::Vec Resolve(const char *name);
	static void ResolveConflict(const CloudFile::Ref &version);
	static bool StartDownloadingLatestVersion(const char *name);
	static CloudFile::Status FileStatus(const char *name);

};

class RADENG_CLASS Persistence
{
public:
	typedef boost::shared_ptr<Persistence> Ref;

	static Ref Load(const char *name);
	static Ref Load(stream::InputStream &is);
	static Ref New(const char *name);

	bool Read(const char *name);
	bool Read(stream::InputStream &is);

	bool Save();
	bool Save(const char *name);
	bool Save(stream::OutputStream &os);

	RAD_DECLARE_READONLY_PROPERTY(Persistence, keys, world::Keys*);

private:

	Persistence(const char *name)
	{
		if (name)
			m_name = name;
	}

	RAD_DECLARE_GET(keys, world::Keys*) { return &const_cast<Persistence*>(this)->m_keys; }

	String m_name;
	world::Keys m_keys;
};

#include <Runtime/PopPack.h>
