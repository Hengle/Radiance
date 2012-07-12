// PackageTools.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if defined(RAD_OPT_TOOLS)

#include "Packages.h"
#include "../COut.h"
#include "../Lua/LuaRuntime.h"
#include <Runtime/File.h>
#include <Runtime/Stream/STLStream.h>
#include <Runtime/Container/ZoneSet.h>
#include "../FileSystem/FileSystem.h"
#include "../Engine.h"
#include <iostream>
#include <fstream>

extern "C" {
#include <Lua/lualib.h>
#if LUA_VERSION_NUM >= 502
#define LUALIB_API LUAMOD_API
#endif
LUALIB_API int luaopen_bit(lua_State *L);
}

#include <Runtime/PushSystemMacros.h>

namespace pkg {

///////////////////////////////////////////////////////////////////////////////

namespace
{
struct PlatNameFlags
{
	const char *name;
	int flags;
};

PlatNameFlags s_PlatNames[] =
{
	{ "PC", P_TargetPC },
	{ "IPhone", P_TargetIPhone },
	{ "IPad", P_TargetIPad },
	{ "XBox360", P_TargetXBox360 },
	{ "PS3", P_TargetPS3 },
	{ 0, 0 }
};
}

RADENG_API int RADENG_CALL PlatformFlagsForName(const char *name)
{
	for (int i = 0;; ++i)
	{
		if (!s_PlatNames[i].name) break;
		if (!string::cmp(name, s_PlatNames[i].name))
		{
			return s_PlatNames[i].flags;
		}
	}

	return 0;
}

RADENG_API const char *RADENG_CALL PlatformNameForFlags(int flags)
{
	if (flags)
	{
		for (int i = 0;; ++i)
		{
			if (!s_PlatNames[i].name)
				break;
			if (s_PlatNames[i].flags&flags)
			{
				return s_PlatNames[i].name;
			}
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

KeyVal::Ref KeyVal::Clone(int flags)
{
	Ref key(new (ZPackages) KeyVal(*this));
	if (flags)
	{
		key->flags |= flags;
		const char *sz = PlatformNameForFlags(flags);

		if (sz)
		{
			key->path += ".";
			key->path += sz;
			key->name += ".";
			key->name += sz;
		}
	}

	key->val = val.Clone();
	return key;
}

KeyVal::Ref KeyDef::CreateKey(int flags)
{
	KeyVal::Ref key(new (ZPackages) KeyVal());
	key->def = shared_from_this();
	key->val = val.Clone();
	key->path = path;
	key->name = name;
	key->flags = flags;

	if (flags)
	{
		const char *sz = PlatformNameForFlags(flags);
		if (sz)
		{
			key->path += ".";
			key->path += sz;
			key->name += ".";
			key->name += sz;
		}
	}

	return key;
}

///////////////////////////////////////////////////////////////////////////////

Package::Import::Ref Package::Import::New(const Package::Ref &pkg, const char *path)
{
	Import::Ref import(new (ZPackages) Import(path));
	import->m_pkg = pkg;
	RAD_VERIFY(pkg->m_importMap.insert(ImportMap::value_type(import->m_path, import.get())).second);
	return import;
}

Package::Import::~Import()
{
	Package::Ref pkg = m_pkg.lock();
	if (pkg)
	{
		pkg->m_importMap.erase(m_path);
	}
}

Package::Import::Ref Package::NewImport(const char *path)
{
	ImportMap::const_iterator it = m_importMap.find(String(path));
	if (it != m_importMap.end())
	{
		return it->second->shared_from_this();
	}
	return Import::New(shared_from_this(), path);
}

bool Package::Entry::AddKey(const KeyVal::Ref &key, bool applyDefault)
{
	std::pair<KeyVal::Map::iterator, bool> p =
		m_keys.insert(KeyVal::Map::value_type(key->path, key));

	if (p.second) // inserted
	{
		if (!key->def) // lookup def
		{
			key->def = FindKeyDef(key->name, key->path);
		}

		if (!key->val.Valid() && applyDefault)
		{
			key->val = key->def->val;
		}

		MapImport(key);
	}

	return p.second;
}

KeyVal::Ref Package::Entry::FindKey(const char *path, int &plat) const
{
	String sPath(path);

	plat &= P_AllTargets;

	if (plat)
	{
		String x = sPath + "." + PlatformNameForFlags(plat);
		KeyVal::Map::const_iterator it = m_keys.find(x);
		if (it != m_keys.end())
			return it->second;
		
		plat = 0;
	}

	KeyVal::Map::const_iterator it = m_keys.find(sPath);
	if (it != m_keys.end())
		return it->second;

	return KeyVal::Ref();
}

KeyVal::Ref Package::Entry::RemoveKey(const char *path, int flags)
{
	KeyVal::Ref r;
	String sPath(path);
	
	flags &= P_AllTargets|P_Exact|P_Prefix;

	if (flags&P_Exact)
		flags &= ~P_Prefix;
	
	if (flags&P_AllTargets) // any targets set?
	{ // remove flagsforms
		for (int t = P_FirstTarget; t <= P_LastTarget; ++t)
		{
			if (!(flags&t))
				continue; // not set.

			String _k(sPath);
			_k += ".";
			_k += PlatformNameForFlags(t);

			KeyVal::Map::iterator it = m_keys.find(_k);

			if (it != m_keys.end())
			{
				r = it->second;
				UnmapImport(r);
				m_keys.erase(it);
			}
		}
	}

	if (flags&P_Exact)
	{
		KeyVal::Map::iterator it = m_keys.find(sPath);
		if (it != m_keys.end())
		{
			r = it->second;
			UnmapImport(r);
			m_keys.erase(it);
		}
	}
	else if(flags&P_Prefix)
	{
		for (KeyVal::Map::iterator it = m_keys.begin(); it != m_keys.end();)
		{
			if (sPath.NCompare(it->second->path, sPath.length) == 0)
			{
				KeyVal::Map::iterator next = it; ++next;
				r = it->second;
				UnmapImport(r);
				m_keys.erase(it);
				it = next;
			}
			else
			{
				++it;
			}
		}
	}

	return r;
}

void Package::Entry::UnmapImport(const KeyVal::Ref &key)
{
	if (key->def && key->def->Type() == K_Import)
	{
		Import::Map::iterator it = m_importMap.find(key->path);
		if (it != m_importMap.end())
		{
			if (--it->second->m_refCount == 0)
			{
				m_importMap.erase(it);
			}
		}
	}
}

void Package::Entry::MapImport(const KeyVal::Ref &key)
{
	if (key->def && key->def->Type() == K_Import)
	{
		const String *s = static_cast<const String*>(key->val);
		if (s && !s->empty)
		{
			AddImport(key->path.c_str, s->c_str);
		}
	}
}

KeyDef::Ref Package::Entry::FindKeyDef(
	const String &name,
	const String &path
) const
{
	KeyDef::Ref def;
	KeyDef::Map::iterator itDef = m_defs->find(path);

	if (itDef == m_defs->end())
	{
		int plat = PlatformFlagsForName(name.c_str);
		if (plat != 0) // platform flag
		{
			String trimmed = TrimKeyName(path);
			if (!trimmed.empty)
			{
				itDef = m_defs->find(trimmed);
				if (itDef != m_defs->end())
				{
					def = itDef->second;
				}
			}
		}
	}
	else
	{
		def = itDef->second;
	}

	if (!def)
	{
		def = pkg->pkgMan->DefaultKeyDef();
	}

	return def;
}

String Package::Entry::TrimKeyName(const String &name)
{
	if (name.empty) 
		return name;

	int period = name.length;

	while (period-- > 1)
	{
		if (name[period] == '.') break;
	}

	return name.Left(period);
}

void Package::Entry::AddImport(const char *name, const char *path)
{
	Import::Map::iterator it = m_importMap.find(String(name));

	if (it == m_importMap.end())
	{
		Import::Ref r(new (ZPackages) Import(name, pkg->NewImport(path)));
		RAD_VERIFY(m_importMap.insert(Import::Map::value_type(r->m_name, r)).second);
	}
	else
	{
		++it->second->m_refCount;
	}
}

#if defined(RAD_OPT_PC_TOOLS)

void Package::Delete()
{
	// remove from pkgMan first before we wipe our directory.
	pkgMan->Delete(shared_from_this());

    for (Entry::IdMap::iterator it = m_idDir.begin(); it != m_idDir.end(); ++it)
    {
        pkgMan->UpdateImports(it->second->m_path.c_str, 0);
    }

    m_idDir.clear();
    m_dirSet.clear();
    m_dir.clear();

	RemoveFileBacking(m_path.c_str);
}

void Package::Delete(int id)
{
    Entry::IdMap::iterator it = m_idDir.find(id);
    if (it != m_idDir.end())
    {
        Entry::Ref ref = it->second;
        pkgMan->UpdateImports(ref->m_path.c_str, 0);
        m_dir.erase(ref->m_name);
        m_dirSet.erase(String(ref->m_name).Lower());
        m_idDir.erase(it);
    }
}

bool Package::Rename(const char *name)
{
	if (!pkgMan->Rename(shared_from_this(), name))
	{
		return false;
	}

	RemoveFileBacking(m_path.c_str);

	for (Entry::IdMap::const_iterator it = m_idDir.begin(); it != m_idDir.end(); ++it)
	{
		Entry::Ref ref = it->second;
		String oldPath = ref->m_path;
		ref->m_path = m_name + RAD_PACKAGE_SEP_STR + ref->m_name;
		pkgMan->UpdateImports(oldPath.c_str, ref->m_path.c_str);
	}

	Save();

	return true;
}

bool Package::Rename(int id, const char *name)
{
	String oldPath;
	String newPath;

	RAD_ASSERT(name);
	{
		StringIdMap::const_iterator it = m_dirSet.find(String(name).Lower());
		if (it != m_dirSet.end() && it->second != id)
		{ // already something with that name.
			return false;
		}
	}

	Entry::IdMap::iterator it = m_idDir.find(id);
	if (it == m_idDir.end())
	{
		return false;
	}

	Entry::Ref ref = it->second;
	String oldName = ref->m_name;
	oldPath = ref->m_path;
	ref->m_name = name;
	ref->m_path = m_name + RAD_PACKAGE_SEP_STR + name;
	newPath = ref->m_path;
	m_dir.erase(oldName);
	m_dir[ref->m_name] = ref;
	m_dirSet.erase(oldName.Lower());
	m_dirSet[ref->m_name.Lower()] = id;

	for (int i = 0; i < Z_Max; ++i)
	{
		AssetWMap::iterator it = m_assets[i].find(oldName);
		if (it != m_assets[i].end())
		{
			m_assets[i][ref->m_name] = it->second;
			m_assets[i].erase(it);
		}
	}

    pkgMan->UpdateImports(oldPath.c_str, newPath.c_str);
    return true;
}

namespace {
// Package::Save() support

typedef zone_set<String, ZPackagesT>::type PathMap;

void _InitPath(std::ostream &out, const String &path, PathMap &map)
{
	PathMap::const_iterator it = map.find(path);
	if (it != map.end())
		return;

	out << "\tt." << path << " = {}" << std::endl;

	map.insert(path);
}

void InitPath(std::ostream &out, const String &path, PathMap &map)
{
	String frag;
	for (String::const_iterator it = path.begin; it != path.end; ++it)
	{
		if (*it == '.') // flush
		{
			_InitPath(out, frag, map);
		}
		frag += *it;
	}
}

void WriteVariant(std::ostream &out, const String &path, const lua::Variant &v)
{
	const String *s = static_cast<const String*>(v);
	if (s)
	{
		out << "\tt." << path << " = \"" << *s << "\"" << std::endl;
		return;
	}

	const int *i = static_cast<const int*>(v);
	if (i)
	{
		out << "\tt." << path << " = " << *i << std::endl;
		return;
	}

	const bool *b = static_cast<const bool*>(v);
	if (b)
	{
		out << "\tt." << path << " = " << (*b ? "true" : "false") << std::endl;
		return;
	}
}

} // namespace

bool Package::Save() const
{
	details::WriteLock L(m_m);

	std::fstream f;
	String path;

	{
		path = CStr("9:/") + pkgMan->m_engine.sys->files->hddRoot.get();
		char native[file::MaxFilePathLen+1];
		if (!file::ExpandToNativePath(path.c_str, native, file::MaxFilePathLen+1))
			return false;
		path = native;
	}

	f.open(
		(path + "/" + m_path).c_str, 
		std::ios_base::out|std::ios_base::trunc
	);

	if (f.bad())
		return false;

	f << "-- " << m_name << std::endl;
	f << "-- Radiance Package File" << std::endl << std::endl;

	for (Entry::Map::const_iterator it = m_dir.begin(); it != m_dir.end(); ++it)
	{
		const Entry::Ref &ref = it->second;

		PathMap paths;

		f << "f = function()" << std::endl;
		f << "\tlocal t = {}" << std::endl;
		f << "\tt.type = \"" << asset::TypeString(ref->type) << "\"" << std::endl;
		f << "\tt.name = \"" << ref->name.get() << "\"" << std::endl;
		f << "\tt.modifiedTime = \"" << ref->modifiedTime->ToString() << "\"" << std::endl;

		for (KeyVal::Map::const_iterator kit = ref->m_keys.begin(); kit != ref->m_keys.end(); ++kit)
		{
			const KeyVal::Ref &key = kit->second;
			if (!key->def)
				continue;
			if (!key->val.Valid())
				continue;

			InitPath(f, key->path, paths);	
			WriteVariant(f, key->path, key->val);
		}

		f << "\treturn t" << std::endl;
		f << "end" << std::endl;
		f << "add(f())" << std::endl << std::endl;
	}

	f << std::flush;
	f.close();

	f.open(
		(path + "/" + m_path + ".idx").c_str,
		std::ios_base::out|std::ios_base::trunc
	);

	if (f.bad())
		return false;

	f << m_dir.size() << std::endl;
	f.close();

	return true;
}

void Package::UpdateImports(const char *src, const char *dst)
{
    RAD_ASSERT(src);
	
	if (!dst) // disable removing of imports on delete.
		return;

	String ssrc(src);

	
	{
		ImportMap::iterator it = m_importMap.find(ssrc);
		if (it != m_importMap.end())
		{
			Import *i = it->second;
			m_importMap.erase(it);
			if (dst)
			{
				i->m_path = dst;
				m_importMap[ssrc] = i;
			}
		}
	}

	for (Entry::Map::const_iterator it = m_dir.begin(); it != m_dir.end(); ++it)
	{
		bool modified = false;
		const Entry::Ref &entry = it->second;
		for (KeyVal::Map::const_iterator it2 = entry->m_keys.begin(); it2 != entry->m_keys.end(); ++it2)
		{
			const KeyVal::Ref &key = it2->second;
			if (key->def && (key->def->style&K_Import))
			{
				// imports have already been fixed up so just change keyvalue
				String *s = static_cast<String*>(key->val);
				if (s && *s == src)
				{
					*s = dst;
					modified = true;
				}
			}
		}

		if (modified)
			entry->UpdateModifiedTime();
	}
}

void Package::RemoveFileBacking(const char *path)
{
	PackageMan::Ref pm(m_pm.lock());
	pm->m_engine.sys->files->DeleteFile(path ? path : m_path.c_str.get(), file::AllMedia);
	pm->m_engine.sys->files->DeleteFile((String(path ? CStr(path) : m_path) + ".idx").c_str, file::AllMedia);
}

Package::Entry::Ref Package::Clone(const Entry::Ref &source, const char *name)
{
	Entry::Ref clone = CreateEntry(name, source->type);
	if (!clone)
		return Entry::Ref(); // duplicate error.

	const KeyVal::Map &keys = source->Keys();

	for (KeyVal::Map::const_iterator it = keys.begin(); it != keys.end(); ++it)
	{
		const KeyVal::Ref &s = it->second;
		KeyVal::Ref d = s->Clone(0);

		clone->AddKey(d, false);
		if (d->def && (d->def->style&K_Import))
			clone->MapImport(d);
	}

	return clone;
}

#endif // defined(RAD_OPT_PC_TOOLS)

///////////////////////////////////////////////////////////////////////////////

#define PACKAGEMAN_KEY "@pkgm"
#define PACKAGE_KEY "@pkg"

lua::State::Ref PackageMan::InitLua()
{
	lua::State::Ref state(new (ZPackages) lua::State("Packages"));
	lua_State *L = state->L;
#define MAP(_x) \
	lua_pushnumber(L, (lua_Number)_x); \
	lua_setglobal(L, #_x)

	MAP(K_TypeMask);
	MAP(K_Variant);
	MAP(K_File);
	MAP(K_CheckBoxes);
	MAP(K_List);
	MAP(K_Unsigned);
	MAP(K_MultiLine);
	MAP(K_Color);
	MAP(K_EditorOnly);
	MAP(K_Hidden);
	MAP(K_ReadOnly);
	MAP(K_Import);
	MAP(K_Global);
	MAP(P_TargetPC);
	MAP(P_TargetIPhone);
	MAP(P_TargetIPad);
	MAP(P_TargetConsole);
	MAP(P_TargetXBox360);
	MAP(P_TargetPS3);
	MAP(P_AllTargets);
#undef MAP

	luaL_Reg r[] =
	{
		{ "add", lua_Add },
		{ 0, 0 }
	};

	lua::RegisterGlobals(L, 0, r);
	lua_pushlightuserdata(L, this);
	lua_setfield(L, LUA_REGISTRYINDEX, PACKAGEMAN_KEY);
	luaopen_bit(L);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, "bit");
#endif
	luaopen_string(L);
#if LUA_VERSION_NUM >= 502
	lua_setglobal(L, LUA_STRLIBNAME);
#endif

	return state;
}

int PackageMan::lua_Add(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, PACKAGE_KEY);
	pkg::Package *pkg = (pkg::Package*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	RAD_VERIFY(pkg);

	if (!pkg)
		return 0;

	PackageMan::Ref self = pkg->m_pm.lock();
	RAD_ASSERT(self);

	lua::Variant::Map map;
	lua::ParseVariantTable(L, map, true);

	lua::Variant::Map::iterator it = map.find(CStr("name"));
	const String *pstr = 0;
	String name;

	if (it == map.end() || !(pstr=static_cast<const String*>(it->second)))
	{
		luaL_error(L, "Asset is missing 'name' field, (Function %s, File %s, Line %d)",
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
		return 0; // Shutup CA
	}

	name = *pstr;
	map.erase(it); // not a key.

	it = map.find(CStr("type"));
	String stype;

	if (it == map.end() || !(pstr=static_cast<const String*>(it->second)))
	{
		luaL_error(L, "Asset (%s) is missing 'type' field, (Function %s, File %s, Line %d)",
			name.c_str.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
		return 0; // Shutup CA
	}

	stype = *pstr;
	map.erase(it); // not a key.

	asset::Type type = asset::TypeFromName(stype.c_str);
	if (type == asset::AT_Max)
	{
		luaL_error(L, "Asset (%s) has invalid type '%s', (Function %s, File %s, Line %d)",
			name.c_str.get(),
			stype.c_str.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	it = map.find(CStr("modifiedTime"));
	xtime::TimeDate modifiedTime;

	if (it != map.end() && (pstr=static_cast<const String*>(it->second)))
	{
		modifiedTime = xtime::TimeDate::FromString(pstr->c_str);
		map.erase(it); // not a key.
	}
	else
	{
		modifiedTime = xtime::TimeDate::Now(xtime::TimeDate::universal_time_tag());
		self->m_resavePackages = true; // resave times.
	}

	Package::Entry::Ref entry = pkg->CreateEntry(name.c_str, type);
	if (!entry)
	{
		luaL_error(L, "Duplicate asset (%s), (Function %s, File %s, Line %d)",
			name.c_str.get(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	entry->m_modifiedTime = modifiedTime;

	self->ParseEntry(pkg, String(), entry, KeyVal::Ref(), map);

	lua_pop(L, 1);

#if defined(RAD_OPT_PC_TOOLS)
	if (pkg->m_tickSize > 0)
	{
		--pkg->m_tickSize;
		self->m_ui->Step();
		self->m_ui->Refresh();
	}
#endif

	return 0;
}

void PackageMan::ParseEntry(
	pkg::Package *pkg,
	const String &path,
	const Package::Entry::Ref &entry,
	const KeyVal::Ref &parent,
	const lua::Variant::Map &map
)
{
	for (lua::Variant::Map::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		String fullName;

		if (path.empty)
		{
			fullName = it->first;
		}
		else
		{
			fullName = path + "." + it->first;
		}

		KeyVal::Ref key(new (ZPackages) KeyVal());
		key->name = it->first;
		key->path = fullName;

		const lua::Variant::Map *sub = static_cast<const lua::Variant::Map*>(it->second);
		if (sub)
		{
			ParseEntry(pkg, fullName, entry, key, *sub);
		}
		else
		{ // assign value.
			entry->AddKey(key, true);

			if (static_cast<const lua_Number*>(it->second) != 0)
			{ // convert to ints
				int *x = (int*)::reflect::SafeAllocate(ZPackages, ::reflect::Type<int>());
				reflect::SharedReflected::Ref number(
					new (ZPackages) reflect::SharedReflected(
						reflect::Reflect(*x)
					)
				);
				*x = (int)*static_cast<const lua_Number*>(it->second);
				key->val = lua::Variant(number);
			}
			else
			{
				key->val = it->second;
			}

			if (key->val.Class() != key->def->val.Class())
			{
				COut(C_Error) << "Key (" << fullName << ") does not match definition. Asset (" <<
						entry->name.get() << ", " << asset::TypeString(entry->type) << "), Package '" <<
						pkg->name.get() << "' (" << pkg->path.get() << ")" << std::endl;
				key->def = DefaultKeyDef();
				key->val = key->def->val;
				continue;
			}
		}
	}
}

bool PackageMan::LoadKeyDefs()
{
	COut(C_Info) << "PackageMan: loading asset definitions..." << std::endl;

	for (int i = 0; i < asset::AT_Max; ++i)
	{
		KeyDefsForType((asset::Type)i); // allocate;
	}

	m_defaultKeyDef.reset(new (ZPackages) KeyDef());
	{
		KeyDef::Pair p;
		p.name = CStr("value");
		p.val = lua::Variant(
			reflect::SharedReflected::Ref(
				new (ZPackages) reflect::SharedReflected(
					reflect::Reflected::New<void ()>(
						ZPackages,
						reflect::Type<String>(),
						reflect::NullArgs()
					)
				)
			)
		);
		RAD_ASSERT(p.val.Valid());
		m_defaultKeyDef->pairs.insert(KeyDef::Pair::Map::value_type(p.name, p));
		m_defaultKeyDef->val = p.val;
		m_defaultKeyDef->style = K_Variant;
	}

	file::HSearch s = m_engine.sys->files->OpenSearch(
		m_pkgDir.c_str,
		".keys",
		file::AllMedia
	);

	if (!s)
	{
		COut(C_ErrMsgBox) << "PackageMan::LoadKeyDefs(): unable to open "
			<< m_pkgDir << " directory!" << std::endl;
		return false;
	}

	String filename;
	while (s->NextFile(filename))
	{
		LoadKeyDefs(m_pkgDir + "/" + filename, filename);
	}

	return true;
}

bool PackageMan::LoadKeyDefs(const String &path, const String &filename)
{
	asset::Type type;
	{
		char x[file::MaxFilePathLen+1];
		file::SetFileExt(filename.c_str, 0, x, file::MaxFilePathLen+1);
		type = asset::TypeFromName(x);
		if (type == asset::AT_Max)
		{
			COut(C_ErrMsgBox) << "PackageMan: '" << x
				<< "' is not a valid asset type (source: "
				<< filename << ")" << std::endl;
			return false;
		}

		COut(C_Info) << "PackageMan: loading '" <<
			path << "'..." << std::endl;
	}

	int media = file::AllMedia;
	file::HBufferedAsyncIO buf;
	bool r = m_engine.sys->files->LoadFile(
			path.c_str,
			media,
			buf,
			file::HIONotify()
	) >= file::Success;

	r = r && buf;

	if (r)
	{
		buf->WaitForCompletion();
		r = buf->result == file::Success;
	}

	if (!r)
	{
		COut(C_ErrMsgBox) << "PackageMan: failed to load '" << path << "'" << std::endl;
		return false;
	}

	lua::State::Ref L = InitLua();
	RAD_VERIFY(L);

	if (luaL_loadbuffer(
		L->L,
		(const char*)buf->data->ptr.get(),
		buf->data->size,
		path.c_str
	) != 0)
	{
		COut(C_ErrMsgBox) << "PackageMan: " << lua_tostring(L->L, -1) << std::endl;
		return false;
	}

	if (lua_pcall(L->L, 0, 1, 0))
	{
		COut(C_ErrMsgBox) << "PackageMan: " << lua_tostring(L->L, -1) << std::endl;
		return false;
	}

	ParseKeyDefs(L->L, path, type);
	lua_pop(L->L, 1);
	return true;
}

void PackageMan::ParseKeyDefs(lua_State *L, const String &filename, asset::Type type)
{
	lua::Variant::Map map;
	if (lua::ParseVariantTable(L, map, false))
	{
		ParseKeyDefs(filename, String(), KeyDefsForType(type), KeyDef::Ref(), map);
	}
}

void PackageMan::ParseKeyDefs(const String &filename, const String &path, const KeyDef::MapRef &keyMap, const KeyDef::Ref &parent, const lua::Variant::Map &map)
{
	for (lua::Variant::Map::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		if (it->first == "style")
		{
			if (!parent) 
				continue;

			const lua_Number *style = static_cast<const lua_Number*>(it->second);
			if (style)
			{
				parent->style = (int)*style;
			}
			else
			{
				COut(C_Error) << "Expected integer value: " <<
					path << ".style, file:" << filename << std::endl;
				parent->style = 0;
			}
			continue;
		}
		else if (it->first == "editor")
		{
#if defined(RAD_OPT_PC_TOOLS)
			// don't bind editor in non pc-tools builds
			if (!parent)
				continue;

			const String *seditor = static_cast<const String*>(it->second);
			if (seditor)
			{
				parent->editor = reflect::Class::Find<char>(seditor->c_str);
				if (!parent->editor)
				{
					COut(C_Error) << "Unable to find editor class: " <<
						*seditor << "for keydef: " << parent->path <<
						", file: " << filename << std::endl;
				}
			}
			else
			{
				COut(C_Error) << "Expected string value: " <<
					path << ".editor, file: " << filename << std::endl;
			}
#endif
			continue;
		}
		else if(it->first == String(it->first).Lower()) // all lowercase
		{
			if (!parent) 
				continue;

			KeyDef::Pair pair;
			pair.name = it->first;

			if (static_cast<const lua_Number*>(it->second) != 0)
			{ // convert to ints
				int *x = (int*)::reflect::SafeAllocate(ZPackages, ::reflect::Type<int>());
				reflect::SharedReflected::Ref number(
					new (ZPackages) reflect::SharedReflected(
						reflect::Reflect(*x)
					)
				);
				*x = (int)*static_cast<const lua_Number*>(it->second);
				pair.val = lua::Variant(number);
			}
			else
			{
				pair.val = it->second;
			}

			parent->pairs.insert(KeyDef::Pair::Map::value_type(pair.name, pair));

			if (it->first == "value")
			{
				parent->val = pair.val;
			}
			continue;
		}

		KeyDef::Ref key(new (ZTools) KeyDef());
		key->name = it->first;
		key->flags = 0;

		if (path.empty)
		{
			key->path = key->name;	
		}
		else
		{
			key->path = path + "." + key->name;
			key->flags = PlatformFlagsForName(key->name.c_str);
		}

		key->style = K_Variant;

		if (parent)
			parent->childFlags |= key->flags;

		const lua::Variant::Map *children = static_cast<const lua::Variant::Map*>(it->second);
		if (children)
		{
			ParseKeyDefs(filename, key->path, keyMap, key, *children);
		}
		else
		{
			COut(C_Error) << "Expected table: " <<
				path << "." << key->name << ", file:" << filename << std::endl;

		}

		RAD_VERIFY(keyMap->insert(KeyDef::Map::value_type(key->path, key)).second);
		if (parent)
		{
			RAD_VERIFY(parent->children.insert(KeyDef::Map::value_type(key->name, key)).second);
		}
	}
}

KeyDef::MapRef PackageMan::KeyDefsForType(asset::Type type)
{
	TypeToKeyDefsMap::iterator it = m_keyDefs.find(type);
	if (it == m_keyDefs.end())
	{
		std::pair<TypeToKeyDefsMap::iterator, bool> pair = m_keyDefs.insert(TypeToKeyDefsMap::value_type(
			type,
			KeyDef::MapRef(new KeyDef::Map())
		));
		RAD_VERIFY(pair.second);
		it = pair.first;
	}

	return it->second;
}

void PackageMan::EnumeratePackage(
#if defined(RAD_OPT_PC_TOOLS)
	tools::UIProgress &ui,
#endif
	const String &path,
	const String &filename,
	int size
)
{
	COut(C_Info) << "PackageMan: loading '" << path << "'..." << std::endl;
	
#if defined(RAD_OPT_PC_TOOLS)
	ui.title = (CStr("Discovering Packages... ") + path + " (fetching)").c_str;
	ui.Refresh();
#endif

	file::HFile file;
	file::HStreamInputBuffer stream = m_engine.sys->files->SafeCreateStreamBuffer(32*Kilo);

	if (!stream || m_engine.sys->files->OpenFile(
			path.c_str,
			file::AllMedia,
			file,
			file::HIONotify()
		) < file::Success)
	{
		COut(C_ErrMsgBox) << "PackageMan: failed to load '" << path << "'" << std::endl;
#if defined(RAD_OPT_PC_TOOLS)
		ui.totalProgress = ui.totalProgress.get() + size;
		ui.Refresh();
#endif
		return;
	}

	stream->Bind(file);
	lua::State::Ref L = InitLua();

	{
		typedef lua::StreamLoader<8*Kilo, lua::StackMemTag> Loader;
		stream::InputStream is(stream->buffer);

		Loader loader(is);

#if LUA_VERSION_NUM >= 502
		if (lua_load(
				L->L,
				&Loader::Read,
				&loader,
				path.c_str,
				0
			)
		)
#else
		if (lua_load(
				L->L,
				&Loader::Read,
				&loader,
				path.c_str
			)
		)
#endif
		{
			COut(C_ErrMsgBox) << "PackageMan(parse): " << lua_tostring(L->L, -1) << std::endl;
			return;
		}
	}

	char name[file::MaxFilePathLen+1];
	file::SetFileExt(filename.c_str, 0, name, file::MaxFilePathLen+1);

	pkg::Package::Ref pkg = pkg::Package::New(
		shared_from_this(),
		path.c_str,
		name
	);

#if defined(RAD_OPT_PC_TOOLS)
	pkg->m_readOnly = (file->media&(file::Paks|file::CDDVD)) != 0;
	pkg->m_tickSize = size;
#endif
	
	lua_pushlightuserdata(L->L, pkg.get());
	lua_setfield(L->L, LUA_REGISTRYINDEX, PACKAGE_KEY);

#if defined(RAD_OPT_PC_TOOLS)
	m_ui = &ui;
	ui.title = (String("Discovering Packages... ") + path + " (parsing)").c_str.get();
	ui.Refresh();
#endif
	
	if (lua_pcall(L->L, 0, 1, 0))
	{
		COut(C_ErrMsgBox) << "PackageMan(run): " << lua_tostring(L->L, -1) << std::endl;
		return;
	}

	String lowerName = CStr(name);
	lowerName.Lower();

	if (!m_packageDir.insert(lowerName).second)
	{
		COut(C_ErrMsgBox) << "Duplicate package names (" << name << ") path: " <<
			path << std::endl;
	}

	m_packages.insert(Package::Map::value_type(String(name), pkg));
}

#if defined(RAD_OPT_PC_TOOLS)
bool PackageMan::DiscoverPackages(tools::UIProgress &ui)
{
	COut(C_Info) << "PackageMan: discovering packages..." << std::endl;
	ui.title = "Discovering Packages...";
	ui.total = 0;
	ui.totalProgress = 0;
	ui.Refresh();

	file::HSearch s = m_engine.sys->files->OpenSearch(
		m_pkgDir.c_str,
		".pkg",
		file::AllMedia
	);

	if (!s)
	{
		COut(C_ErrMsgBox) << "PackageMan::DiscoverPackages(): unable to open " << m_pkgDir << " directory!" << std::endl;
		return false;
	}

	typedef zone_vector<std::pair<String, int>, ZPackagesT>::type Vec;
	Vec files;
	String filename;
	int totalSize = 0;

	while (s->NextFile(filename))
	{
		int size = PackageSize(m_pkgDir + "/" + filename);
		size = std::max(1, size);
		totalSize += size;
		files.push_back(Vec::value_type(filename, size));
	}

	ui.total = totalSize;
	ui.Refresh();

	totalSize = 0;
	for (Vec::const_iterator it = files.begin(); it != files.end(); ++it)
	{
		const String &x = it->first;
		{
			details::WriteLock L(m_m);
			EnumeratePackage(ui, m_pkgDir + "/" + x, x, it->second);
		}
		totalSize += it->second;
		ui.totalProgress = totalSize;
		ui.Refresh();
	}

	if (m_resavePackages)
	{
		m_resavePackages = false;
		SaveAll();
	}

	MakeIntermediateDirs();

	return true;
}

int PackageMan::PackageSize(const String &path)
{
	file::HStreamInputBuffer ibuf = m_engine.sys->files->SafeCreateStreamBuffer(32);
	file::HFile file;

	if (m_engine.sys->files->OpenFile(
		(path + ".idx").c_str,
		file::AllMedia,
		file,
		file::HIONotify()
	) != file::Success)
	{
		return 0;
	}

	ibuf->Bind(file);
	stream::streambuf buf(&ibuf->buffer.get(), 0);
	std::istream stream(&buf);

	int numFiles;
	stream >> numFiles;
	bool good = stream.good();

	return good ? numFiles : 0;
}

void PackageMan::Delete(const Package::Ref &pkg)
{
	details::WriteLock L(m_m);
	for (Package::Entry::IdMap::iterator it = pkg->m_idDir.begin(); it != pkg->m_idDir.end(); ++it)
	{
		UnmapId(it->second->id);
	}
	m_packages.erase(pkg->m_name);
	m_packageDir.erase(String(pkg->m_name).Lower());
}

bool PackageMan::Rename(int id, const char *name)
{
	IdPackageWMap::const_iterator it = m_idDir.find(id);
	if (it != m_idDir.end())
	{
		Package::Ref ref = it->second.lock();
		if (ref)
		{
			return ref->Rename(id, name);
		}
	}

	return false;
}

void PackageMan::Delete(int id)
{
	IdPackageWMap::const_iterator it = m_idDir.find(id);
	if (it != m_idDir.end())
	{
		Package::Ref ref = it->second.lock();
		if (ref)
		{
			ref->Delete(id);
		}
	}
}

Package::Ref PackageMan::CreatePackage(const char *name)
{
	pkg::Package::Ref pkg = pkg::Package::New(
		shared_from_this(),
		(m_pkgDir + "/" + name + ".pkg").c_str,
		name
	);

	String sname(CStr(name));

	if (!m_packageDir.insert(String(sname).Lower()).second)
		return Package::Ref();

	m_packages.insert(Package::Map::value_type(sname, pkg));
	MakeIntermediateDirs();
	return pkg;
}

bool PackageMan::Rename(const Package::Ref &pkg, const char *name)
{
	details::WriteLock L(m_m);

	String sname(name);
	String lowerName(sname);
	lowerName.Lower();
	
	if (!m_packageDir.insert(lowerName).second)
	{ // exists
		return false;
	}

	Package::Map::iterator it = m_packages.find(pkg->m_name);
	if (it != m_packages.end())
	{
		m_packages.erase(it);
	}

	lowerName = pkg->m_name.Lower();
	m_packageDir.erase(lowerName);

	m_packages[sname] = pkg;
	pkg->m_name = sname;
	pkg->m_path = m_pkgDir + "/" + name + ".pkg";

    return true;
}

void PackageMan::UpdateImports(const char *src, const char *dst)
{
	for (Package::Map::const_iterator it = m_packages.begin(); it != m_packages.end(); ++it)
	{
		it->second->UpdateImports(src, dst);
	}
}

void PackageMan::SaveAll()
{
	details::WriteLock L(m_m);
	for (Package::Map::const_iterator it = m_packages.begin(); it != m_packages.end(); ++it)
	{
		it->second->Save();
	}
}

void PackageMan::SavePackages(const PackageMap &pkgs)
{
	for (PackageMap::const_iterator it = pkgs.begin(); it != pkgs.end(); ++it)
	{
		it->second->Save();
	}
}

void PackageMan::SavePackages(const IdVec &ids)
{
	SavePackages(GatherPackages(ids));
}

PackageMap PackageMan::GatherPackages(const IdVec &ids)
{
	PackageMap pkgs;
	for (IdVec::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		Package::Entry::Ref ref = FindEntry(*it);
		if (ref)
		{
			pkgs[String(ref->pkg->name.get())] = ref->pkg;
		}
	}

	return pkgs;
}

#endif

} // pkg

#endif // RAD_OPT_TOOLS

