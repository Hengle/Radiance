// Persistence.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Persistence.h"
#include "App.h"
#include "Engine.h"
#include <Runtime/Stream.h>
#include <Runtime/File.h>
#include <stdio.h>

#if defined(RAD_OPT_IOS)
FILE *__IOS_OpenPersistence(const char *name, const char *mode);
#endif

namespace {

enum
{
	Tag = RAD_FOURCC_LE('P', 'E', 'R', 'S'),
	Version = 1
};

bool LoadStorage(stream::InputStream &is, world::Keys &keys)
{
	bool r = false;
	keys.pairs.clear();

	U32 tag, ver;
	if (is.Read(&tag) && is.Read(&ver))
	{
		if (tag == Tag && ver <= Version)
		{
			U32 numPairs;
			if (is.Read(&numPairs))
			{
				char key[1024];
				char val[1024];

				U32 i;
				U32 len;

				for (i = 0; i < numPairs; ++i)
				{
					if (!is.Read(&len))
						break;
					if (len >= 1024)
						break;
					if (is.Read(key, (stream::SPos)len, 0) != (stream::SPos)len)
						break;
					key[len] = 0;

					if (!is.Read(&len))
						break;
					if (len >= 1024)
						break;
					if (is.Read(val, (stream::SPos)len, 0) != (stream::SPos)len)
						break;
					val[len] = 0;

					keys.pairs[String(key)] = String(val);
				}

				r = i == numPairs;
			}
		}
	}

	return r;
}

bool LoadStorage(const char *name, world::Keys &keys)
{
	FILE *fp = 0;
#if defined(RAD_OPT_IOS)
	fp = __IOS_OpenPersistence(name, "rb");
#else
	WString path(L"9:/");
	path += string::Widen(name);
	wchar_t nativePath[file::MaxFilePathLen+1];
	if (file::ExpandToNativePath(path.c_str(), nativePath, file::MaxFilePathLen+1))
	{
		fp = file::wfopen(nativePath, L"rb");
	}
#endif

	if (!fp)
		return false;

	file::stream::InputBuffer ib(fp);
	stream::InputStream is(ib);

	bool r = LoadStorage(is, keys);
	fclose(fp);
	return r;
}

bool SaveStorage(stream::OutputStream &os, const world::Keys &keys)
{
	bool r = false;
	if (os.Write((U32)Tag) && os.Write((U32)Version))
	{
		if (os.Write((U32)keys.pairs.size()))
		{
			world::Keys::Pairs::const_iterator it;
			for (it = keys.pairs.begin(); it != keys.pairs.end(); ++it)
			{
				if (!os.Write((U32)it->first.length()))
					break;
				if (os.Write(it->first.c_str(), (stream::SPos)it->first.length(), 0) != (stream::SPos)it->first.length())
					break;
				if (!os.Write((U32)it->second.length()))
					break;
				if (os.Write(it->second.c_str(), (stream::SPos)it->second.length(), 0) != (stream::SPos)it->second.length())
					break;
			}

			r = it == keys.pairs.end();
		}
	}

	return r;
}

bool SaveStorage(const char *name, const world::Keys &keys)
{
	FILE *fp = 0;
#if defined(RAD_OPT_IOS)
	fp = __IOS_OpenPersistence(name, "wb");
#else
	WString path(L"9:/");
	path += string::Widen(name);
	wchar_t nativePath[file::MaxFilePathLen+1];
	if (file::ExpandToNativePath(path.c_str(), nativePath, file::MaxFilePathLen+1))
	{
		fp = file::wfopen(nativePath, L"wb");
	}
#endif

	if (!fp)
		return false;

	file::stream::OutputBuffer ob(fp);
	stream::OutputStream os(ob);

	bool r = SaveStorage(os, keys);
	fclose(fp);
	return r;
}

} // namespace

Persistence::Ref Persistence::Load(const char *name)
{
	Ref r(new (ZEngine) Persistence(name));
	r->Read(name);
	return r;
}

Persistence::Ref Persistence::Load(stream::InputStream &is)
{
	Ref r(new (ZEngine) Persistence(0));
	r->Read(is);
	return r;
}

Persistence::Ref Persistence::New(const char *name)
{
	RAD_ASSERT(name);
	return Ref(new (ZEngine) Persistence(name));
}

bool Persistence::Read(const char *name)
{
	if (!name)
		return false;

	bool r = LoadStorage(name, m_keys);

	if (!r)
		m_keys.pairs.clear();
	
	return r;
}

bool Persistence::Read(stream::InputStream &is)
{
	bool r = LoadStorage(is, m_keys);

	if (!r)
		m_keys.pairs.clear();
	
	return r;
}

bool Persistence::Save()
{
	if (!m_name.empty())
		return SaveStorage(m_name.c_str(), m_keys);
	return true;
}

bool Persistence::Save(const char *name)
{
	RAD_ASSERT(name);
	m_name = name;
	return Save();
}

bool Persistence::Save(stream::OutputStream &os)
{
	return SaveStorage(os, m_keys);
}

