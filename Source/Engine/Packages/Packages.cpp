// Packages.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Packages.h"
#include "../COut.h"
#include <Runtime/File.h>
#include <Runtime/DataCodec/LmpReader.h>
#include "../FileSystem/FileSystem.h"
#include "../Engine.h"
#include <iostream>
#undef max

namespace pkg {

RAD_ZONE_DEF(RADENG_API, ZPackages, "Packages", ZEngine);

///////////////////////////////////////////////////////////////////////////////

RADENG_API int RADENG_CALL PlatformIndex(int plat)
{
	if (plat)
	{
		for (int i = 0; i < P_NumTargets; ++i)
		{
			if ((P_FirstTarget<<i)&plat)
				return i;
		}
	}

	return -1;
}

SinkBase::Mutex SinkBase::s_m;
SinkBase::MutexPool SinkBase::s_mp(ZPackages, "pkg-sink-mutex-pool", 8);

SinkBase::Ref SinkBase::Cast(const Asset::Ref &asset, int stage)
{
	RAD_ASSERT(asset);
	Asset::SinkMap::const_iterator it = asset->m_sinks.find(stage);
	if (it != asset->m_sinks.end())
	{
		return it->second;
	}
	return SinkBase::Ref();
}

int SinkBase::_Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const AssetRef &asset,
		int flags
)
{
	int r;
	{
		Lock L(s_m);
		if (++m_c == 1)
		{
			RAD_ASSERT(!m_m);
			m_m = s_mp.Construct();
		}
	}

	RAD_ASSERT(m_m);
	{
		Lock L(*m_m); // serialize Sink
		r = Process(
			time,
			engine,
			asset,
			flags
		);
	}

	{
		Lock L(s_m);
		if (--m_c == 0)
		{
			s_mp.Destroy(m_m);
			m_m = 0;
		}
	}

	return r;
}

namespace details {

SinkBase::Ref SinkFactoryBase::Cast(const AssetRef &asset)
{
	return SinkBaseHelper::Cast(asset, Stage());
}

} // details

///////////////////////////////////////////////////////////////////////////////

Package::Ref Package::New(const PackageManRef &pm, const wchar_t *path, const char *name)
{
	return Ref(new (ZPackages) Package(pm, path, name));
}

Package::Package(const PackageMan::Ref &pm, const wchar_t *path, const char *name) :
m_name(name),
m_path(path),
m_pm(pm)
#if defined(RAD_OPT_PC_TOOLS)
, m_readOnly(false),
m_tickSize(0)
#endif
#if defined(RAD_OPT_TOOLS)
, m_cooked(false)
#endif
{
	RAD_ASSERT(pm);
}

Package::~Package()
{
}

Package::Entry::Ref Package::CreateEntry(const char *name, asset::Type type)
{
	RAD_ASSERT(name);
	details::WriteLock WL(m_m);

	if (m_dirSet.find(String(name).lower()) != m_dirSet.end())
		return Entry::Ref(); // already exists.

	PackageMan::Ref pm = m_pm.lock();
	RAD_ASSERT(pm);

	Entry::Ref ref(new (ZPackages) Entry(
		pm->m_nextId++, 
		name, 
		(m_name + RAD_PACKAGE_SEP_STR + name).c_str(), 
		type,
		shared_from_this()
#if defined(RAD_OPT_TOOLS)
		,pm->KeyDefsForType(type)
#endif
	));

	RAD_VERIFY(m_dir.insert(Entry::Map::value_type(ref->m_name, ref)).second);
	RAD_VERIFY(m_idDir.insert(Entry::IdMap::value_type(ref->m_id, ref)).second);
	RAD_VERIFY(m_dirSet.insert(StringIdMap::value_type(ref->m_name.lower(), ref->id)).second);
	pkgMan->MapId(ref->id, shared_from_this());

	return ref;
}

Asset::Ref Package::Asset(int id, Zone z) const
{
#if defined(RAD_OPT_PC_TOOLS)
	if (z == Z_Unique)
	{
		Entry::Ref entry = FindEntry(id);
		if (!entry)
			return Asset::Ref();
		Package *self = const_cast<Package*>(this);
		return pkg::Asset::New(z, entry);
	}
#endif

	RAD_ASSERT(z < Z_Max);
	if (z >= Z_Max)
		return Asset::Ref();

	details::ReadLock L(m_m);

	AssetIdWMap::const_iterator it = m_idAssets[z].find(id);

	if (it == m_idAssets[z].end())
	{
		details::UpgradeToWriteLock WL(L);

		it = m_idAssets[z].find(id);
		if (it == m_idAssets[z].end())
		{
			Entry::Ref entry = FindEntry(id);
			if (!entry)
				return Asset::Ref();

			Package *self = const_cast<Package*>(this);

			Asset::Ref r = pkg::Asset::New(z, entry);
			RAD_ASSERT(r);
			RAD_VERIFY(self->m_assets[z].insert(AssetWMap::value_type(entry->m_name, r)).second);
			RAD_VERIFY(self->m_idAssets[z].insert(AssetIdWMap::value_type(entry->m_id, r)).second);
			return r;
		}
	}

	Asset::Ref r(it->second.lock());
	RAD_VERIFY(r);
	return r;
}

Asset::Ref Package::Asset(const char *name, Zone z) const
{
	RAD_ASSERT(name);

#if defined(RAD_OPT_PC_TOOLS)
	if (z == Z_Unique)
	{
		Entry::Ref entry = FindEntry(name);
		if (!entry)
			return Asset::Ref();
		Package *self = const_cast<Package*>(this);
		return pkg::Asset::New(z, entry);
	}
#endif

	RAD_ASSERT(z < Z_Max);
	if (z >= Z_Max)
		return Asset::Ref();

	String _name(name);
	details::ReadLock L(m_m);

	AssetWMap::const_iterator it = m_assets[z].find(_name);

	if (it == m_assets[z].end())
	{
		details::UpgradeToWriteLock WL(L);

		it = m_assets[z].find(_name);
		if (it == m_assets[z].end())
		{
			Entry::Ref entry = FindEntry(name);
			if (!entry)
			{
				return Asset::Ref();
			}

			Package *self = const_cast<Package*>(this);

			Asset::Ref r = pkg::Asset::New(z, entry);
			RAD_ASSERT(r);
			RAD_VERIFY(self->m_assets[z].insert(AssetWMap::value_type(entry->m_name, r)).second);
			RAD_VERIFY(self->m_idAssets[z].insert(AssetIdWMap::value_type(entry->m_id, r)).second);
			return r;
		}
	}

	Asset::Ref r(it->second.lock());
	RAD_VERIFY(r);
	return r;
}

Asset::Map Package::RefedAssets(Zone z) const
{
#if defined(RAD_OPT_PC_TOOLS)
	if (z == Z_Unique)
		return Asset::Map();
#endif
	RAD_ASSERT(z < Z_Max);
	if (z >= Z_Max)
		return Asset::Map();

	Asset::Map assets;
	details::WriteLock L(m_m);
	for (AssetWMap::const_iterator it = m_assets[z].begin(); it != m_assets[z].end(); ++it)
	{
		Asset::Ref r(it->second.lock());
		if (r)
		{
			RAD_VERIFY(assets.insert(Asset::Map::value_type(it->first, r)).second);
		}
	}
	return assets;
}

///////////////////////////////////////////////////////////////////////////////

Asset::Ref Asset::New(
	Zone z,
	const Package::Entry::Ref &entry
)
{
	Ref r(new (ZPackages) Asset (z, entry));
	entry->m_pkg.lock()->m_pm.lock()->AllocSinks(r);
	return r;
}

Asset::Asset(
	Zone z,
	const Package::Entry::Ref &entry
) :
m_z(z),
m_entry(entry)
{
}

int Asset::Process(
	const xtime::TimeSlice &time,
	int flags
)
{
	return m_entry->m_pkg.lock()->m_pm.lock()->Process(
		time,
		shared_from_this(),
		flags
	);
}

///////////////////////////////////////////////////////////////////////////////

PackageMan::Ref PackageMan::New(Engine &engine, const char *pkgDir)
{
	return Ref(new (ZPackages) PackageMan(engine, pkgDir));
}

PackageMan::PackageMan(Engine &engine, const char *pkgDir) :
m_engine(engine),
m_pkgDir(pkgDir),
m_wpkgDir(string::Widen(pkgDir)),
m_nextId(0)
#if defined(RAD_OPT_PC_TOOLS)
, m_ui(0),
m_resavePackages(false)
#endif
{
}

PackageMan::~PackageMan()
{
}

bool PackageMan::Initialize()
{
	COut(C_Info) << "PackageMan: initializing packages..." << std::endl;

#if defined(RAD_OPT_TOOLS)
	if (!LoadKeyDefs())
	{
		return false;
	}
#endif

	COut(C_Info) << "PackageMan: finished..." << std::endl;

	return true;
}

Package::Ref PackageMan::ResolvePackage(const char *name, int flags)
{
	const String sname(name);
	details::ReadLock L(m_m);
	Package::Map::iterator it = m_packages.find(sname);
	if (it != m_packages.end())
	{
		return it->second;
	}

	details::UpgradeToWriteLock WL(L); // (+readers, writer)

	if (flags&P_Load)
	{
#if defined(RAD_OPT_TOOLS)

		// check to see if this is a case sensativity issue, like someone typed in
		// a bad letter
		String lowerName(sname.lower());
		if (m_packageDir.find(lowerName) != m_packageDir.end())
			return Package::Ref();

		WString wname(string::Widen(name) + L".pkg");
		WString wPath(m_wpkgDir + L"/" + wname);
		if (m_engine.sys->files->FileExists(wPath.c_str(), file::AllMedia))
		{
			EnumeratePackage(
#if defined(RAD_OPT_PC_TOOLS)
				tools::NullUIProgress,
#endif
				wPath, 
				wname, 
				0
			);
		}
		else
#endif
		{
			LoadBin(name, flags);
		}

		it = m_packages.find(sname);
		if (it != m_packages.end())
		{
			return it->second;
		}
	}

	return Package::Ref();
}

int PackageMan::ProcessAll(
	Zone z,
	const xtime::TimeSlice &time,
	int flags,
	bool ignoreErrors
)
{

#if defined(RAD_OPT_PC_TOOLS)
	if (z == Z_Unique)
		return SR_Success;
#endif

	const Package::Map &pkgs = packages;
	for (Package::Map::const_iterator it = pkgs.begin(); it != pkgs.end(); ++it)
	{
		const Package::Ref &pkg = it->second;
		for (int curZone = (z==Z_All)?Z_First:z; curZone < Z_Max; ++curZone)
		{
			Asset::Map assets(pkg->RefedAssets((Zone)curZone));
			for (Asset::Map::const_iterator it2 = assets.begin(); it2 != assets.end(); ++it2)
			{
				int r = it2->second->Process(
					time,
					flags
				);

				if (r < SR_Success && !ignoreErrors)
				{
					return r;
				}
			}

			if (z != Z_All)
				break;
		}
	}

	return SR_Success;
}

int PackageMan::Process(
	const xtime::TimeSlice &time,
	const Asset::Ref &asset,
	int flags
)
{
	RAD_ASSERT(!(flags&P_Unload));

	bool alloc = (flags==P_SAlloc) ? true : false;
	const SinkFactoryMap &factories = TypeSinks(asset->m_entry->type);

	for (SinkFactoryMap::const_iterator it = factories.begin(); it != factories.end(); ++it)
	{
		const details::SinkFactoryBase::Ref &f = it->second;
		SinkBase::Ref sink = f->Cast(asset);

		if (!sink)
		{
			sink = AllocSink(f, asset);
		}

		if (alloc || !sink) continue;

		int r = sink->_Process(
			time,
			m_engine,
			asset,
			flags
		);

		if (r != SR_Success)
		{
			return r;
		}
	}

	return SR_Success;
}

void PackageMan::Unbind(Binding *binding)
{
	Asset::IdWMap *assets = binding->m_f->assets;
	for (int i = 0; i < Z_Max; ++i)
	{
		while (!assets[i].empty())
		{
			Asset::Ref asset(assets[i].begin()->second);
			if (asset)
			{
				asset->m_sinks.erase(binding->m_f->Stage());
			}
			assets[i].erase(assets[i].begin());
		}
	}
	SinkFactoryMap &map = TypeSinks(binding->m_type);
	map.erase(binding->m_f->Stage());
}

PackageMan::SinkFactoryMap &PackageMan::TypeSinks(asset::Type type)
{
	TypeSinkFactoryMap::iterator it = m_sinkFactoryMap.find(type);

	if (it == m_sinkFactoryMap.end())
	{
		return m_sinkFactoryMap.insert(TypeSinkFactoryMap::value_type(type, SinkFactoryMap())).first->second;
	}

	return it->second;
}

SinkBase::Ref PackageMan::AllocSink(const details::SinkFactoryBase::Ref &f, const Asset::Ref &asset)
{
	SinkBase::Ref state = f->New();
	if (state)
	{
#if defined(RAD_OPT_PC_TOOLS)
		if (asset->zone != Z_Unique)
#endif
		{
			f->assets[asset->zone].insert(AssetIdWMap::value_type(asset->m_entry->id, asset));
		}
		RAD_VERIFY(asset->m_sinks.insert(
			Asset::SinkMap::value_type(f->Stage(), state)
		).second);
	}
	return state;
}

void PackageMan::AllocSinks(const Asset::Ref &asset)
{
	SinkFactoryMap &map = TypeSinks(asset->m_entry->type);

	for (SinkFactoryMap::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		AllocSink(it->second, asset);
	}
}

void PackageMan::LoadBin(
	const char *name,
	int loadFlags
)
{
	// load package lump
	WString path(L"Packages/");
	path += string::Widen(name);
	path += L".lump";

	pkg::Package::Ref pkg = pkg::Package::New(
		shared_from_this(),
		path.c_str(),
		name
	);

	int media = file::AllMedia;
	file::HStreamInputBuffer buf;
	file::Result r = m_engine.sys->files->OpenFileStream(
		path.c_str(),
		media,
		buf,
		file::HIONotify()
	);

	if (r != file::Success)
	{
		COut(C_Error) << "Unable to open '" << string::Shorten(path.c_str()) << "'!" << std::endl;
		return;
	}

	stream::InputStream is(buf->buffer);
	
	if (!pkg->m_lmpReader.LoadLumpInfo(LumpSig, LumpId, is, data_codec::lmp::LittleEndian))
	{
		COut(C_Error) << "'" << string::Shorten(path.c_str()) << "' is not a valid package lump!" << std::endl;
		return;
	}

	buf.Close();

	const data_codec::lmp::StreamReader::Lump *imports = 0;

	for (U32 i = 0; i < pkg->m_lmpReader.NumLumps(); ++i)
	{
		const data_codec::lmp::StreamReader::Lump *l = pkg->m_lmpReader.GetByIndex(i);
		if (!string::cmp("@imports", l->Name()))
		{
			imports = l;
		}
		else
		{
			const TagData *tag = (const TagData*)l->TagData();
			if (!tag)
			{
				COut(C_Error) << "'" << l->Name() << "' is missing tag data in package '" << name << "'" << std::endl;
				return;
			}

			Package::Entry::Ref entry = pkg->CreateEntry(l->Name(), (asset::Type)tag->type);
			entry->m_imports.reserve(tag->numImports);

#if defined(RAD_OPT_TOOLS)
			entry->m_cooked = true;
#endif

			for (U16 k = 0; k < tag->numImports; ++k)
			{
				Package::Entry::Import import((int)tag->imports[k]);
				entry->m_imports.push_back(import);
			}

			for (int k = 0; k < P_NumTargets+1; ++k)
			{
				if (tag->ofs[k] != 0)
				{
					entry->m_tags[k] = ((U8*)tag)+tag->ofs[k];
				}
			}
		}
	}

	if (!imports || !imports->TagData())
	{
		COut(C_Error) << "'" << string::Shorten(path.c_str()) << "' missing imports table!" << std::endl;
		return;
	}

	const U8 *tag = (const U8*)imports->TagData();
	U16 numImports = *reinterpret_cast<const U16*>(tag);
	tag += sizeof(U16);

	pkg->m_imports.resize(numImports);

	for (U16 i = 0; i < numImports; ++i)
	{
		U16 size = *reinterpret_cast<const U16*>(tag);
		tag += sizeof(U16);
		pkg->m_imports[i].m_path = (const char*)tag;
		tag += size;
	}

	m_packages.insert(Package::Map::value_type(String(name), pkg));
}

namespace {

void Split(const char *path, String &pkgName, String &assetName)
{
	char p[256];
	char n[256];

	p[0] = 0;
	n[0] = 0;

	char *w = p;

	while (*path)
	{
		if (*path != RAD_PACKAGE_SEP_CHAR)
		{
			*w++ = *path++;
		}
		else
		{
			path++;
			break;
		}
	}

	*w = 0;
	w = n;

	while (*path)
	{
		*w++ = *path++;
	}

	*w = 0;

	pkgName = p;
	assetName = n;
}

}

Asset::Ref PackageMan::Resolve(
	const char *path,
	Zone z,
	int flags
)
{
	if (!path || !path[0]) 
		return Asset::Ref();

	String pkgName, assetName;
	Split(path, pkgName, assetName);

	pkg::Package::Ref pkg = ResolvePackage(pkgName.c_str(), flags);
	Asset::Ref ref;

	if (pkg)
		ref = pkg->Asset(assetName.c_str(), z);

	return ref;
}

Package::Entry::Ref PackageMan::Resolve(
	const char *path,
	int flags
)
{
	if (!path || !path[0]) 
		return Package::Entry::Ref();

	String pkgName, assetName;
	Split(path, pkgName, assetName);

	pkg::Package::Ref pkg = ResolvePackage(pkgName.c_str(), flags);
	Package::Entry::Ref ref;

	if (pkg)
		ref = pkg->FindEntry(assetName.c_str());

	return ref;
}

int PackageMan::ResolveId(
	const char *path,
	int flags
)
{
	Package::Entry::Ref ref = Resolve(path, flags);
	return ref ? ref->id : -1;
}

} // pkg
