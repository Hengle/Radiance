// Packages.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace pkg {

///////////////////////////////////////////////////////////////////////////////

inline Binding::Binding(const details::SinkFactoryBase::Ref &f, asset::Type type, const PackageManRef &ref) :
m_f(f),
m_type(type),
m_pm(ref)
{
}

#if defined(RAD_OPT_PC_TOOLS)
inline Binding::Binding(const details::CookerFactoryBase::Ref &f, asset::Type type, const PackageManRef &ref) :
m_cf(f),
m_type(type),
m_pm(ref)
{
}
#endif

inline Binding::~Binding()
{
	PackageMan::Ref ref = m_pm.lock();
	if (ref)
	{
		ref->Unbind(this);
	}
}

///////////////////////////////////////////////////////////////////////////////

class SinkBaseHelper
{
public:

	static SinkBase::Ref Cast(const Asset::Ref &asset, int stage)
	{
		return SinkBase::Cast(asset, stage);
	}
};

template <typename T>
inline boost::shared_ptr<T> Sink<T>::Cast(const AssetRef &asset)
{
	if (!asset || asset->type != T::AssetType)
	{
		return boost::shared_ptr<T>();
	}
	return boost::static_pointer_cast<T>(SinkBaseHelper::Cast(asset, T::SinkStage));
}

///////////////////////////////////////////////////////////////////////////////

inline const StringIdMap &Package::RAD_IMPLEMENT_GET(dir)
{
	return m_dirSet;
}

inline Package::Entry::Entry(
	int id,
	const char *name,
	const char *path,
	asset::Type type,
	const Package::Ref &pkg
#if defined(RAD_OPT_TOOLS)
	, const KeyDef::MapRef &defs
#endif
) :
m_id(id),
m_name(name),
m_path(path),
m_type(type),
m_pkg(pkg)
#if defined(RAD_OPT_TOOLS)
, m_cooked(false),
m_defs(defs)
#endif
{
	for (int i = 0; i < P_NumTargets+1; ++i)
		m_tags[i] = 0;
}

inline Package::Entry::~Entry()
{
}

inline const void *Package::Entry::TagData(int plat)
{
	plat &= P_AllTargets;

	if (!plat)
		return m_tags[0];

	int num = PlatformIndex(plat);
	if (num < 0 || num >= P_NumTargets)
		return 0;

	++num;

	if (plat == P_TargetIPad && !m_tags[num])
		return TagData(P_TargetIPhone);

	return m_tags[num] ? m_tags[num] : m_tags[0];
}

inline Package::Entry::Ref Package::FindEntry(const char *name) const
{
	RAD_ASSERT(name);
	Entry::Map::const_iterator it = m_dir.find(String(name));
	if (it == m_dir.end()) return Entry::Ref();
	return it->second;
}

inline Package::Entry::Ref Package::FindEntry(int id) const
{
	Entry::IdMap::const_iterator it = m_idDir.find(id);
	if (it == m_idDir.end()) return Entry::Ref();
	return it->second;
}

inline Asset::Ref Package::Entry::Asset(Zone z) const
{
	return m_pkg.lock()->Asset(m_id, z);
}

inline const char *Package::RAD_IMPLEMENT_GET(name)
{
	return m_name.c_str;
}

inline const char *Package::RAD_IMPLEMENT_GET(path)
{
	return m_path.c_str;
}

inline Package::Import::Import()
{
}

inline Package::Import::Import(const char *path) : m_path(path)
{
}

inline const char *Package::Import::RAD_IMPLEMENT_GET(path)
{
	return m_path.c_str;
}

inline void Package::SetName(const Entry::Ref &entry, const char *name)
{
	m_dir.erase(entry->m_name);
	entry->m_name = name;
	entry->m_path = m_name + ":" + name;
	RAD_VERIFY(m_dir.insert(Entry::Map::value_type(entry->m_name, entry)).second);
}

inline Package::Entry::Import::Import(int index) : m_index(index)
{
}

#if defined(RAD_OPT_TOOLS)

inline Package::Entry::Import::Import(const char *name, const Package::Import::Ref &ref) :
m_name(name),
m_ref(ref),
m_refCount(1)
{

}

template <typename T>
const T *Package::Entry::KeyValue(const char *path, int plat) const
{
	String sPath(path);

	plat &= P_AllTargets|P_NoDefaultKey;

	{
		int p = plat; // FindKey() modifies this.
		KeyVal::Ref key = FindKey(path, p);
		if (key)
			return static_cast<const T*>(key->val);
	}

	if (!(plat&P_NoDefaultKey) && m_defs)
	{
		KeyDef::Map::const_iterator it = m_defs->find(sPath);
		if (it != m_defs->end())
		{
			return static_cast<const T*>(it->second->val);
		}
	}

	return 0;
}

template <typename T>
int Package::Entry::MatchTargetKeys(const char *path, int plat, int allplats) const
{
	int matched = 0;
	plat &= P_AllTargets;
	allplats &= P_AllTargets;

	if (plat)
	{ // pick first target if multiples passed in
		for (int p = P_FirstTarget; p <= P_LastTarget; p <<= 1)
		{
			if (plat&p)
			{
				plat = p;
				break;
			}
		}
	}

	matched |= plat&allplats; // always matches this

	const T *target = KeyValue<T>(path, plat);

	for (int i = P_FirstTarget; i <= P_LastTarget; i <<= 1)
	{
		if (plat&i)
			continue;
		if (!(i&allplats))
			continue;
		const T *test = KeyValue<T>(path, i);

		if ((target && test && *test == *target) || (!target && !test))
			matched |= i;
	}

	return matched;
}

inline void Package::Entry::UpdateModifiedTime()
{
	m_modifiedTime = xtime::TimeDate::Now(xtime::TimeDate::universal_time_tag());
}

inline bool Package::Entry::RAD_IMPLEMENT_GET(cooked)
{
	return m_pkg.lock()->cooked;
}

inline const xtime::TimeDate *Package::Entry::RAD_IMPLEMENT_GET(modifiedTime)
{
	return &m_modifiedTime;
}

#if defined(RAD_OPT_PC_TOOLS)
inline void Package::Entry::Delete()
{
    Package::Ref pkg = m_pkg.lock();
    if (pkg)
    {
        pkg->Delete(m_id);
    }
}

inline bool Package::Entry::Rename(const char *name)
{
    bool r = false;
    Package::Ref pkg = m_pkg.lock();
    if (pkg)
    {
        r = pkg->Rename(m_id, name);
    }
    return r;
}
#endif

inline const KeyVal::Map &Package::Entry::Keys() const
{
	return m_keys;
}

inline const Package::Entry::Import *Package::Entry::Resolve(const char *name) const
{
	Import::Map::const_iterator it = m_importMap.find(String(name));
	if (it == m_importMap.end())
	{
		return 0;
	}
	return it->second.get();
}

#endif // defined(RAD_OPT_TOOLS)

inline asset::Type Package::Entry::RAD_IMPLEMENT_GET(type)
{
	return m_type;
}

inline const char *Package::Entry::RAD_IMPLEMENT_GET(path)
{
	return m_path.c_str;
}

inline const char *Package::Entry::RAD_IMPLEMENT_GET(name)
{
	return m_name.c_str;
}

inline Package::Ref Package::Entry::RAD_IMPLEMENT_GET(pkg)
{
	return m_pkg.lock();
}

inline int Package::Entry::RAD_IMPLEMENT_GET(id)
{
	return m_id;
}

inline int Package::Entry::RAD_IMPLEMENT_GET(numImports)
{
	return (int)m_imports.size();
}

inline const Package::Entry::Import *Package::Entry::Resolve(int index) const
{
	RAD_ASSERT(index >= 0 && index < (int)m_imports.size());
	return &m_imports[index];
}

inline Asset::Ref Package::Entry::Resolve(
	const Import &import,
	Zone z,
	int flags
) const
{
	Package::Ref pkg = this->pkg;

#if defined(RAD_OPT_TOOLS)
	if (!cooked)
	{
		return pkg->pkgMan->Resolve(import.m_ref->path, z, flags);
	}
#endif
	return pkg->pkgMan->Resolve(
		pkg->m_imports[import.m_index].path,
		z,
		flags
	);
}

inline int Package::Entry::ResolveId(
	const Import &import,
	int flags
) const
{
	Package::Ref pkg = this->pkg;

#if defined(RAD_OPT_TOOLS)
	if (!cooked)
	{
		return pkg->pkgMan->ResolveId(import.m_ref->path, flags);
	}
#endif
	return pkg->pkgMan->ResolveId(
		pkg->m_imports[import.m_index].path,
		flags
	);
}

inline void Package::UnlinkAsset(pkg::Asset *asset)
{
	RAD_ASSERT(asset);
#if defined(RAD_OPT_PC_TOOLS)
	if (asset->m_z == Z_Unique)
		return;
#endif
	RAD_ASSERT(asset->m_z < Z_Max);
	if (asset->m_z >= Z_Max)
		return;
	details::WriteLock WL(m_m);
	m_assets[asset->m_z].erase(asset->m_entry->m_name);
	m_idAssets[asset->m_z].erase(asset->m_entry->m_id);
}

inline bool Package::Contains(int id) const
{
	return m_idDir.find(id) != m_idDir.end();
}

///////////////////////////////////////////////////////////////////////////////

inline Asset::~Asset()
{
	Package::Ref _pkg(pkg);

	if (_pkg.get() && _pkg->pkgMan.get())
	{
		Engine *engine = &pkg->pkgMan->m_engine;

		for (SinkMap::const_reverse_iterator it = m_sinks.rbegin(); it != m_sinks.rend(); ++it)
		{
			it->second->_Process(
				xtime::TimeSlice::Infinite,
				*engine,
				Asset::Ref(),
				P_Unload
			);
		}

		_pkg->UnlinkAsset(this);
	}
}

inline asset::Type Asset::RAD_IMPLEMENT_GET(type)
{
	return m_entry->m_type;
}

inline const char *Asset::RAD_IMPLEMENT_GET(path)
{
	return m_entry->m_path.c_str;
}

inline const char *Asset::RAD_IMPLEMENT_GET(name)
{
	return m_entry->m_name.c_str;
}

inline Package::Ref Asset::RAD_IMPLEMENT_GET(pkg)
{
	return m_entry->m_pkg.lock();
}

inline int Asset::RAD_IMPLEMENT_GET(numImports)
{
	return m_entry->numImports;
}

inline int Asset::RAD_IMPLEMENT_GET(id)
{
	return m_entry->m_id;
}

inline Package::Entry::Ref Asset::RAD_IMPLEMENT_GET(entry)
{
	return m_entry;
}

inline Zone Asset::RAD_IMPLEMENT_GET(zone)
{
	return m_z;
}
#if defined(RAD_OPT_TOOLS)
inline bool Asset::RAD_IMPLEMENT_GET(cooked)
{
	return m_entry->m_cooked;
}
inline Cooker::Ref Asset::AllocateIntermediateCooker()
{
	return pkg->pkgMan->AllocateIntermediateCooker(shared_from_this());
}
#endif

#if defined(RAD_OPT_PC_TOOLS)
inline Cooker::Ref Asset::CookerForAsset()
{
	return pkg->pkgMan->CookerForAsset(shared_from_this());
}
#endif

///////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_TOOLS)
inline KeyDef::Ref PackageMan::DefaultKeyDef() const
{
	return m_defaultKeyDef;
}
#endif

template <typename T>
inline SinkBase::Ref PackageMan::SinkFactory<T>::New()
{
	return SinkBase::Ref(new (ZPackages) T());
}

template <typename T>
inline int PackageMan::SinkFactory<T>::Stage() const
{
	return T::SinkStage;
}

template <typename T>
Binding::Ref PackageMan::Bind()
{
	SinkFactoryMap &map = TypeSinks((asset::Type)T::AssetType);
	details::SinkFactoryBase::Ref ref(new (ZPackages) SinkFactory<T>());
	std::pair<SinkFactoryMap::iterator, bool> pair = 
		map.insert(
			SinkFactoryMap::value_type(
				T::SinkStage, 
				ref
			)
		);
	RAD_VERIFY_MSG(pair.second, "A sink with the requested stage already exists!");
	return Binding::Ref(new (ZPackages) Binding(ref, (asset::Type)T::AssetType, shared_from_this()));
}

#if defined(RAD_OPT_TOOLS)

template <typename T>
inline Cooker::Ref PackageMan::CookerFactory<T>::New()
{
	return Cooker::Ref(new (ZPackages) T());
}

template <typename T>
Binding::Ref PackageMan::BindCooker()
{
	details::CookerFactoryBase::Ref ref(new (ZPackages) CookerFactory<T>());
	std::pair<TypeCookerFactoryMap::iterator, bool> pair = 
		m_cookerFactoryMap.insert(
			TypeCookerFactoryMap::value_type(
				(asset::Type)T::AssetType, 
				ref
			)
		);
	RAD_VERIFY_MSG(pair.second, "A cooker with the requested stage already exists!");
	return Binding::Ref(new (ZPackages) Binding(ref, (asset::Type)T::AssetType, shared_from_this()));
}

#endif

inline void PackageMan::MapId(int id, const Package::Ref &pkg)
{
	m_idDir[id] = pkg;
}

inline void PackageMan::UnmapId(int id)
{
	m_idDir.erase(id);
}

inline Asset::Ref PackageMan::Asset(int id, Zone z) const
{
	m_m.lock_shared();

	IdPackageWMap::const_iterator it = m_idDir.find(id);
	if (it == m_idDir.end())
	{
		m_m.unlock_shared();
		return Asset::Ref();
	}

	Package::Ref pkg = it->second.lock();
	m_m.unlock_shared();

	if (!pkg)
	{
		return Asset::Ref();
	}

	return pkg->Asset(id, z);
}

inline Package::Entry::Ref PackageMan::FindEntry(int id) const
{
	m_m.lock_shared();

	IdPackageWMap::const_iterator it = m_idDir.find(id);
	if (it == m_idDir.end())
	{
		m_m.unlock_shared();
		return Package::Entry::Ref();
	}

	Package::Ref pkg = it->second.lock();
	m_m.unlock_shared();

	if (!pkg)
	{
		return Package::Entry::Ref();
	}

	return pkg->FindEntry(id);
}

} // pkg
