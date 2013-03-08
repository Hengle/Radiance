/*! \file Packages.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup packages
*/

/*! \defgroup packages Package System
	The Radiance package management is a key-value description mechanism for describing
	asset meta-data, and allowing for implicit or explicit dependancies between assets.

	One of the primary differences between the design of this package system versus others
	is that it is not designed or intended to be a container for binary data, except in
	cooked form. Package files are lua scripts which describe the contents of the package
	and can be automatically merged by most version control systems, or merged with
	text merge tools in the case of conflicts. This allows users to modify packages and
	assets without having to coordinate to avoid overwriting modifications to binary files.
	
	This means that most if not all asset types actually use meta-data to
	address their binary data. For example the texture asset has a key called Source.File 
	which contains the Base directory relative path to the texture file. The asset also contains 
	fields to control resizing and compression settings to apply when the asset is loaded or 
	cooked.

	The Package system provides all of the basic systems for creating and editing asset and
	package data. In addition the system provides an asynchronous processing framework designed
	to facilitate loading and using assets in the engine.
*/

#pragma once

#include "PackagesDef.h"
#include "PackageDetails.h"
#include "../Lua/LuaRuntime.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Time/Time.h>
#include <Runtime/Stream.h>
#include <Runtime/File.h>
#include <Runtime/DataCodec/LmpReader.h>
#include <Runtime/ReflectDef.h>
#include <Runtime/Base/ObjectPool.h>

#if defined(RAD_OPT_PC_TOOLS)
	#include "../Renderer/Renderer.h"
	#include "../Tools/Progress.h"
	#include "../Persistence.h"
	#include <Runtime/Event.h>
	#include <Runtime/Time.h>
	#include <Runtime/DataCodec/LmpDef.h>
	#include <Runtime/Thread/Thread.h>
	#include <stdio.h>
namespace tools {
namespace editor {
	class GLWidget;
} // editor
} // tools
#endif

#if defined(RAD_OPT_TOOLS)
	#include <iostream>
#endif

#include <Runtime/PushPack.h>

class Engine;

namespace pkg {

//! Returns the zero-based index of the specified platform bit.
/*! \param plat The platform bitflag of the requested index. 
	\return zero-based index of the platform or -1 if the bitflags did not contain a valid platform bit. 
	\sa pkg::P_TargetPC, pkg::P_TargetIPhone, pkg::P_TargetIPad, pkg::P_TargetXBox360, and pkg::P_TargetPS3. */
RADENG_API int RADENG_CALL PlatformIndex(int plat);

#if defined(RAD_OPT_TOOLS)

//! Returns the platform bit for the specified platform.
/*! \param name Null terminated string containing the requested platforms textual name.
	\return bitflags for the requested platform or zero if the platform does not exist. 
	\sa pkg::P_TargetPC, pkg::P_TargetIPhone, pkg::P_TargetIPad, pkg::P_TargetXBox360, and pkg::P_TargetPS3. */
RADENG_API int RADENG_CALL PlatformFlagsForName(const char *name);

//! Returns the null terminated string of the specified platform name.
/*! \param flags Platform flag.
	\return Null terminated string of the specified platform name. 
	\sa pkg::P_TargetPC, pkg::P_TargetIPhone, pkg::P_TargetIPad, pkg::P_TargetXBox360, and pkg::P_TargetPS3. */
RADENG_API const char *RADENG_CALL PlatformNameForFlags(int flags);

//! Stores a variant key-value for use in a key/value pair.
/*! A KeyValue is used to describe a specific value for a meta-data key	in an assets meta-data registry. 
	The value is stored as a lua::Variant. There may be multiple KeyValue's for the same key with
	each one containing a value for a specific plaform. A KeyValue contains platform flags which
	define the platform it applies to. If there are platform specific versions of a key value
	they will be stored as distinct keys, postfixed with the platforms name in the assets
	meta-data registry. 
	
	A KeyValue also refers to its definition via a KeyDef. A KeyDef registry is defined per
	asset-type in a lua file contained in Base/Packages in a *.key file. A KeyDef also defines
	the user-interface semantics of a KeyValue. 
	
	\sa pkg::KeyDef 
*/
struct KeyVal {
	typedef boost::shared_ptr<KeyVal> Ref;
	typedef zone_map<String, Ref, ZPackagesT>::type Map;

	//! Contructs an empty key.
	KeyVal() : flags(0) {}

	//! The name of the key without any path. 
	/*! If this is a platform specific key-value then this value will be things like "PC", or "iOS" and
		therefore not very descriptive. The path value of a key should be used to identify it. */
	String name;

	//! The fully qualified name of the key including its path.
	String path;

	//! A reference to the keys definition. 
	/*! This may be NULL in which case the key value will be treated as an unformatted string. */
	KeyDefRef def;

	//! Contains the value of the key in a variant. 
	/*!	Variants do not have deep copy construction semantics.
		\sa pkg::KeyVal::Clone() */
	lua::Variant val;

	//! Contains platform flags, or 0 for generic (all platforms).
	/*! \sa pkg::P_TargetPC, pkg::P_TargetIPhone, pkg::P_TargetIPad, pkg::P_TargetXBox360, and pkg::P_TargetPS3. */
	int flags;

	//! Clones the key value into a new key value.
	/*! The variant value contained in a KeyVal does not specify deep copy construction semantics. 
		Therefore it is generally undesirable to copy construct one. If a copy of a key value
		is needed this function should be used.

		Additionally the clone can be specialized into a platform specific version of the
		original by specifying a platform flag. The name and path of the clone will be
		modified to address the new platform and the flags field of the key object will
		contain the specified platform flag. */
	Ref Clone(int flags);
};

//! Defines the attributes of a KeyValue.
struct KeyDef : boost::enable_shared_from_this<KeyDef> {
	
	struct Pair {
		typedef zone_map<String, Pair, ZPackagesT>::type Map;
		String name;
		lua::Variant val;
	};

	typedef boost::shared_ptr<KeyDef> Ref;
	typedef boost::weak_ptr<KeyDef> WRef;
	typedef zone_map<String, Ref, ZPackagesT>::type Map;
	typedef boost::shared_ptr<Map> MapRef;

	KeyVal::Ref CreateKey(int flags);

	KeyDef() : style(0), flags(0), childFlags(0)
#if defined(RAD_OPT_PC_TOOLS)
	, editor(0)
#endif
	{
	}

	int Type() const {
		return (style&K_TypeMask);
	}

	Pair::Map pairs;
	KeyDef::Map children;
	lua::Variant val;
	String name;
	String path;
	int style;
	int flags;
	int childFlags;

#if defined(RAD_OPT_PC_TOOLS)
	const reflect::Class *editor;
#endif
};

#endif

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Binding : public boost::noncopyable {
public:
	typedef boost::shared_ptr<Binding> Ref;
	virtual ~Binding();

	RAD_DECLARE_READONLY_PROPERTY(Binding, pkgMan, PackageManRef);

private:

	friend class PackageMan;

	Binding(
		const details::SinkFactoryBase::Ref &f, 
		asset::Type type, 
		const PackageManRef &ref
	);

#if defined(RAD_OPT_PC_TOOLS)
	Binding(
		const details::CookerFactoryBase::Ref &f, 
		asset::Type type, 
		const PackageManRef &ref
	);
#endif

	RAD_DECLARE_GET(pkgMan, PackageManRef) { return m_pm.lock(); }

#if defined(RAD_OPT_PC_TOOLS)
	details::CookerFactoryBase::Ref m_cf;
#endif
	details::SinkFactoryBase::Ref m_f;
	asset::Type m_type;
	PackageManWRef m_pm;
};

///////////////////////////////////////////////////////////////////////////////

// NOTE: The package system guarantees that access to Sink objects via Process()
// are atomic (i.e. more than one thread will never be in a sinks Process call)

class SinkBaseHelper;
class SinkBase {
public:

	SinkBase() : m_c(0), m_m(0) {}
	virtual ~SinkBase() {}

protected:

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const AssetRef &asset,
		int flags
	) = 0;

private:

	friend class SinkBaseHelper;
	friend class PackageMan;
	friend class Asset;

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	struct SinkMutex {
		SinkMutex * volatile next;
		Mutex m;
	};

	typedef ObjectPool<SinkMutex> MutexPool;

	void AcquireMutex();
	void ReleaseMutex();
	
	int _Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const AssetRef &asset,
		int flags
	);

	static SinkBase *Cast(const AssetRef &asset, int sid);

	volatile int m_c;
	SinkMutex * volatile m_m;
	
	static Mutex s_m;
	static MutexPool s_mp;
	static SinkMutex * volatile s_head;
	static volatile int s_c;
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
class Sink : public SinkBase {
public:
	static T *Cast(const AssetRef &asset);

	void *operator new(size_t size);
	void operator delete(void *p);

private:

	typedef ThreadSafeMemoryPool Pool;

	static Pool &GetPool();

};

#if defined(RAD_OPT_TOOLS)

///////////////////////////////////////////////////////////////////////////////

class BinFile {
public:
	typedef boost::shared_ptr<BinFile> Ref;

	BinFile(FILE *fp) : m_ib(fp), m_ob(fp) {}
	~BinFile()
	{
		if (m_ib.fp)
			fclose(m_ib.fp);
	}

	RAD_DECLARE_READONLY_PROPERTY(BinFile, ib, stream::IInputBuffer&);
	RAD_DECLARE_READONLY_PROPERTY(BinFile, ob, stream::IOutputBuffer&);

private:

	RAD_DECLARE_GET(ib, stream::IInputBuffer&) { 
		return m_ib; 
	}

	RAD_DECLARE_GET(ob, stream::IOutputBuffer&) { 
		return m_ob; 
	}

	mutable file::FILEInputBuffer m_ib;
	mutable file::FILEOutputBuffer m_ob;
};

///////////////////////////////////////////////////////////////////////////////

class Cooker {
public:
	typedef CookerRef Ref;
	typedef zone_vector<Ref, ZPackagesT>::type Vec;

	Cooker(int version) : m_version(version), m_languages(0) {}
	virtual ~Cooker() {}

	virtual CookStatus Status(int flags) = 0;
	int Cook(int flags);

	bool HasTag();
	BinFile::Ref OpenRead(const char *path);
	BinFile::Ref OpenTagRead();

	file::MMapping::Ref MapFile(
		const char *path, 
		::Zone &zone
	);

	file::MMapping::Ref LoadTag();

protected:

	virtual int Compile(int flags) = 0;

	void ResetPakFile();
	String FilePath(const char *path);
	bool FileTime(const char *path, xtime::TimeDate &time);
	bool CopyOutputFile(const char *src, const char *dst);
	bool CopyOutputBinFile(const char *src);
	BinFile::Ref OpenWrite(const char *path);
	BinFile::Ref OpenTagWrite();
	int AddImport(const char *path);
	void UpdateModifiedTime(int target);

	// Compares: -1 == source is newer, 1 == cook is newer, 0 == same

	int CompareVersion(int target, bool updateIfNewer=true);
	int CompareModifiedTime(int target, bool updateIfNewer=true);
	int CompareCachedFileTime(int target, const char *key, const char *path, bool updateIfNewer=true, bool optional=false);
	int CompareCachedFileTimeKey(int target, const char *key, const char *localized = 0, bool updateIfNewer=true, bool optional=false);
	int CompareCachedStringKey(int target, const char *key);
	int CompareCachedString(int target, const char *key, const char *string);
	int CompareCachedLocalizeKey(int target, const char *key);
	const char *TargetString(int target, const char *key);
	void SetTargetString(int target, const char *key, const char *string);
	bool ModifiedTime(int target, xtime::TimeDate &td) const;
	bool TimeForKey(int target, const char *key, xtime::TimeDate &td) const;

	static String TargetPath(int target);
	static String LocalizedString(int languages);
	
	RAD_DECLARE_PROPERTY(Cooker, pakfile, const char*, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Cooker, engine, Engine*);
	RAD_DECLARE_READONLY_PROPERTY(Cooker, cout, std::ostream&);
	RAD_DECLARE_READONLY_PROPERTY(Cooker, asset, const AssetRef&);
	RAD_DECLARE_READONLY_PROPERTY(Cooker, assetPath, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Cooker, assetName, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Cooker, globals, Persistence::KeyValue::Map*);
	RAD_DECLARE_READONLY_PROPERTY(Cooker, version, int);
	RAD_DECLARE_READONLY_PROPERTY(Cooker, languages, int);

private:

	friend class PackageMan;

	RAD_DECLARE_GET(engine, Engine*);
	
	RAD_DECLARE_GET(cout, std::ostream&) { 
		return *m_cout; 
	}
	
	RAD_DECLARE_GET(asset, const AssetRef&) { 
		return m_asset; 
	}
	
	RAD_DECLARE_GET(assetPath, const char*) { 
		return m_assetPath.c_str; 
	}
	
	RAD_DECLARE_GET(assetName, const char*) { 
		return m_assetName.c_str; 
	}
	
	RAD_DECLARE_GET(globals, Persistence::KeyValue::Map*) { 
		return m_globals->keys; 
	}
	
	RAD_DECLARE_GET(version, int) { 
		return m_version; 
	}
	
	RAD_DECLARE_GET(languages, int) {
		return m_languages;
	}

	RAD_DECLARE_GET(pakfile, const char*) {
		return m_pakfile.c_str;
	}

	RAD_DECLARE_SET(pakfile, const char*);

	//! After compiling this must be called to save cooker state.
	void SaveState();

	String TagPath();

	struct Import {
		String path;
	};

	typedef zone_vector<Import, ZPackagesT>::type ImportVec;

	enum {
		ImportsTag = RAD_FOURCC('I', 'M', 'P', 'T')
	};

	void LoadGlobals();
	void LoadImports();
	void SaveImports();
	void SaveGlobals();

	std::ostream *m_cout;
	AssetRef m_asset;
	ImportVec m_imports;
	PackageMan *m_pkgMan;
	PackageRef m_pkg;
	String m_assetPath;
	String m_assetName;
	asset::Type m_type;
	Persistence::Ref m_globals;
	String m_globalsPath;
	String m_tagsPath;
	String m_filePath;
	String m_basePath;
	String m_pakfile;
	String m_originalPakfile;
	bool m_cooking;
	int m_languages;
	int m_version;
};

#endif


///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Package : public boost::enable_shared_from_this<Package>, public boost::noncopyable {
public:
	typedef PackageRef Ref;
	typedef PackageWRef WRef;
	typedef PackageVec Vec;
	typedef PackageMap Map;
	typedef zone_map<String, AssetRef, ZPackagesT>::type AssetMap;

	virtual ~Package();

    AssetRef Asset(const char *name, Zone z) const;
	AssetRef Asset(int id, Zone z) const;
	bool Contains(int id) const;
	AssetMap RefedAssets(Zone z) const;

	RAD_DECLARE_READONLY_PROPERTY(Package, dir, const StringIdMap&);
	RAD_DECLARE_READONLY_PROPERTY(Package, name, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Package, path, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Package, pkgMan, PackageManRef);

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_READONLY_PROPERTY(Package, cooked, bool);
#endif

#if defined(RAD_OPT_PC_TOOLS)
	// Delete/Rename/Save are not thread safe.
    void Delete();
    bool Rename(const char *name);
    bool Save() const;
	RAD_DECLARE_READONLY_PROPERTY(Package, readOnly, bool);
#endif

	class RADENG_CLASS Import
#if defined(RAD_OPT_TOOLS)
		: public boost::enable_shared_from_this<Import>
#endif
	{
	public:

		Import();

#if defined(RAD_OPT_TOOLS)
		typedef boost::shared_ptr<Import> Ref;
		typedef zone_vector<Ref, ZPackagesT>::type Vec;
		~Import();
#endif

		RAD_DECLARE_READONLY_PROPERTY(Import, path, const char *);

	private:

		friend class Package;
		friend class PackageMan;

		Import(const char *path);

		RAD_DECLARE_GET(path, const char*);

#if defined(RAD_OPT_TOOLS)

		static Import::Ref New(
			const Package::Ref &pkg,
			const char *path
		);

		PackageWRef m_pkg;
#endif
		String m_path;
	};

	class Entry {
#if defined(RAD_OPT_PC_TOOLS)
		RAD_EVENT_CLASS(EventNoAccess)
#endif
	public:
		typedef boost::shared_ptr<Entry> Ref;
		typedef zone_map<String, Ref, ZPackagesT>::type Map;
		typedef zone_map<int, Ref, ZPackagesT>::type IdMap;
		typedef zone_vector<Ref, ZPackagesT>::type Vec;

		~Entry();

		RAD_DECLARE_READONLY_PROPERTY(Entry, name, const char*);
		RAD_DECLARE_READONLY_PROPERTY(Entry, path, const char*);
		RAD_DECLARE_READONLY_PROPERTY(Entry, type, asset::Type);
		RAD_DECLARE_READONLY_PROPERTY(Entry, pkg, PackageRef);
		RAD_DECLARE_READONLY_PROPERTY(Entry, numImports, int);
		RAD_DECLARE_READONLY_PROPERTY(Entry, id, int);
		RAD_DECLARE_READONLY_PROPERTY(Entry, tagData, const void*);

		AssetRef Asset(Zone z) const;
				
#if defined(RAD_OPT_TOOLS)

		RAD_DECLARE_READONLY_PROPERTY(Entry, cooked, bool);
		RAD_DECLARE_READONLY_PROPERTY(Entry, modifiedTime, const xtime::TimeDate*);

		void UpdateModifiedTime();
		
		KeyDef::Ref FindKeyDef(
			const String &name,
			const String &path,
			bool allowDefault = true
		) const;

		static String TrimKeyName(const String &name);
		void AddImport(const char *name, const char *path);
		bool AddKey(const KeyVal::Ref &key, bool applyDefault);

		// will not add a key with no definition
		// -1 = not added, no definition
		//  0 = not added, duplicate key
		//  1 = added
		int TryAddKey(const KeyVal::Ref &key, bool applyDefault);

		// plat: on input specified platform, on output specifies the platform if found or zero
		// if the default path was found.
		KeyVal::Ref FindKey(const char *path, int &plat) const;

		// NOTE: P_Exact means match path exactly, this is also
		// implied for any platforms specified. P_Prefix means
		// remove any path that has the supplied prefix.
		KeyVal::Ref RemoveKey(const char *path, int flags = P_Prefix);

		void UnmapImport(const KeyVal::Ref &key);
		void MapImport(const KeyVal::Ref &key);

		const KeyVal::Map &Keys() const;

		template <typename T>
		const T *KeyValue(const char *path, int plat) const;

		template <typename T>
		int MatchTargetKeys(const char *path, int plat, int allplats) const;

#endif

#if defined(RAD_OPT_PC_TOOLS)

		void Delete();
		bool Rename(const char *name);

		struct KeyChangedEventData {
			Package::Entry::Ref origin;
			KeyVal::Ref key;
			// NOTE: path does not contain full path, it contains the
			// normalized path. i.e. changing Texture.Width.XBox would result
			// in a KeyChangedEvent that contains Texture.Width for path,
			// while the key->path would contain Texture.Width.Xbox
			String path;
			int flags;
		};

		typedef Event<KeyChangedEventData, EventNoAccess> KeyChangeEvent;
		KeyChangeEvent OnKeyChange;
#endif

		class Import {
		public:

#if defined(RAD_OPT_TOOLS)
			typedef boost::shared_ptr<Import> Ref;
			typedef zone_map<String, Ref, ZPackagesT>::type Map;
#endif
			typedef zone_vector<Import, ZPackagesT>::type Vec;

		private:

			friend class Entry;
			friend class Asset;
			friend class PackageMan;

#if defined(RAD_OPT_TOOLS)
			Import(const char *name, const Package::Import::Ref &ref);
			Package::Import::Ref m_ref;
			String m_name;
			int m_refCount;
#endif
			Import(int index);
			int m_index;
		};

		const Import *Resolve(int index) const;

#if defined(RAD_OPT_TOOLS)
		const Import *Resolve(const char *name) const;
#endif

		AssetRef Resolve(
			const Import &import,
			Zone z,
			int flags = P_Load
		) const;

		int ResolveId(
			const Import &import,
			int flags = P_Load
		) const;

	private:

		friend class Asset;
		friend class Package;
		friend class PackageMan;
		friend class Import;

		Entry(
			int id,
			const char *name,
			const char *path,
			asset::Type type,
			const Package::Ref &pkg
#if defined(RAD_OPT_TOOLS)
			, const KeyDef::MapRef &defs
#endif
		);

		RAD_DECLARE_GET(name, const char*);
		RAD_DECLARE_GET(path, const char*);
		RAD_DECLARE_GET(type, asset::Type);
		RAD_DECLARE_GET(pkg, PackageRef);
		RAD_DECLARE_GET(numImports, int);
		RAD_DECLARE_GET(id, int);
		RAD_DECLARE_GET(tagData, const void*);

		int m_id;
		String m_name;
		String m_path;
		asset::Type m_type;
		Import::Vec m_imports;
		PackageWRef m_pkg;
		void *m_tag;

#if defined(RAD_OPT_TOOLS)
		RAD_DECLARE_GET(cooked, bool);
		RAD_DECLARE_GET(modifiedTime, const xtime::TimeDate*);
		bool m_cooked;
		KeyVal::Map m_keys;
		KeyDef::MapRef m_defs;
		Import::Map m_importMap;
		xtime::TimeDate m_modifiedTime;
#endif
	};

	Entry::Ref CreateEntry(const char *name, asset::Type type);
	Entry::Ref FindEntry(const char *name) const;
	Entry::Ref FindEntry(int id) const;

#if defined(RAD_OPT_TOOLS)
	Entry::Ref Clone(const Entry::Ref &source, const char *name);
#endif

private:

	friend class Asset;
	friend class PackageMan;
	friend class Import;

#if defined(RAD_OPT_TOOLS)
	typedef zone_map<String, Import*, ZPackagesT>::type ImportMap;
	Import::Ref NewImport(const char *path);
#endif
	typedef zone_vector<Import, ZPackagesT>::type ImportVec;

	static Ref New(const PackageManRef &pm, const char *path, const char *name);
	void SetName(const Entry::Ref &entry, const char *name);

	void UnlinkAsset(pkg::Asset *asset);

	Package(const PackageManRef &pm, const char *path, const char *name);

	RAD_DECLARE_GET(dir, const StringIdMap&);
	RAD_DECLARE_GET(name, const char*);
	RAD_DECLARE_GET(path, const char*);

	RAD_DECLARE_GET(pkgMan, PackageManRef) { 
		return m_pm.lock(); 
	}

#if defined(RAD_OPT_PC_TOOLS)
	// Delete, Rename, UpdateImports, RemoveFileBacking are not thread safe.
    void Delete(int id);
	bool Rename(int id, const char *name);
	void UpdateImports(const char *src, const char *dst);
	void RemoveFileBacking(const char *path);
	
	RAD_DECLARE_GET(readOnly, bool) { 
		return m_readOnly; 
	}
	
	bool m_readOnly;
	int  m_tickSize;
#endif

#if defined(RAD_OPT_TOOLS)
	bool m_cooked;
	
	RAD_DECLARE_GET(cooked, bool) { 
		return m_cooked; 
	}

	ImportMap m_importMap;
#endif

	data_codec::lmp::StreamReader m_lmpReader;
	AssetWMap m_assets[Z_Max];
	AssetIdWMap m_idAssets[Z_Max];
	ImportVec m_imports;
	Entry::Map m_dir;
	Entry::IdMap m_idDir;
	StringIdMap m_dirSet; // case-insensitive
	String m_name;
	String m_path;
	PackageManWRef m_pm;
	mutable details::SharedMutex m_m;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Asset : 
	public boost::enable_shared_from_this<Asset>, 
	public boost::noncopyable {
public:
	typedef AssetRef Ref;
	typedef AssetWRef WRef;
	typedef AssetVec Vec;
	typedef AssetMap Map;
	typedef AssetIdWMap IdWMap;

	virtual ~Asset();

	RAD_DECLARE_READONLY_PROPERTY(Asset, name, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Asset, path, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Asset, type, asset::Type);
	RAD_DECLARE_READONLY_PROPERTY(Asset, pkg, PackageRef);
	RAD_DECLARE_READONLY_PROPERTY(Asset, numImports, int);
	RAD_DECLARE_READONLY_PROPERTY(Asset, id, int);
	RAD_DECLARE_READONLY_PROPERTY(Asset, entry, Package::Entry::Ref);
	RAD_DECLARE_READONLY_PROPERTY(Asset, zone, Zone);

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_READONLY_PROPERTY(Asset, cooked, bool);

	//! Allocates a cooker object that writes into intermediate directory.
	Cooker::Ref AllocateIntermediateCooker();
#endif

	int Process(
		const xtime::TimeSlice &time,
		int flags,
		int maxStage = SS_Max
	);

private:

	friend class Package;
	friend class SinkBase;
	friend class SinkFactoryBase;
	friend class PackageMan;

	typedef zone_map<int, SinkBase*, ZPackagesT>::type SinkMap;

	static Ref New(
		Zone z,
		const Package::Entry::Ref &entry
	);

	Asset(
		Zone z,
		const Package::Entry::Ref &entry
	);

	RAD_DECLARE_GET(type, asset::Type);
	RAD_DECLARE_GET(path, const char*);
	RAD_DECLARE_GET(name, const char*);
	RAD_DECLARE_GET(pkg, PackageRef);
	RAD_DECLARE_GET(numImports, int);
	RAD_DECLARE_GET(id, int);
	RAD_DECLARE_GET(entry, Package::Entry::Ref);
	RAD_DECLARE_GET(zone, Zone);

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_GET(cooked, bool);
#endif

	Zone m_z;
	SinkMap m_sinks;
	Package::Entry::Ref m_entry;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS PackageMan :
	public boost::enable_shared_from_this<PackageMan>,
	public boost::noncopyable {
public:
	typedef boost::shared_ptr<PackageMan> Ref;
	typedef boost::weak_ptr<PackageMan> WRef;

	static Ref New(Engine &engine, const char *pkgDir);

	virtual ~PackageMan();

	bool Initialize();

	template <typename T>
	Binding::Ref Bind();

	RAD_DECLARE_READONLY_PROPERTY(PackageMan, packages, const Package::Map&);

	Asset::Ref Asset(int id, Zone z) const;
	Package::Entry::Ref FindEntry(int id) const;

#if defined(RAD_OPT_PC_TOOLS)
	void SaveAll();
	bool Rename(int id, const char *name);
	void Delete(int id);
	Package::Ref CreatePackage(const char *name);
	void SavePackages(const IdVec &ids);
	void SavePackages(const PackageMap &pkgs);
	PackageMap GatherPackages(const IdVec &ids);
	bool DiscoverPackages(tools::UIProgress &ui);
	int Cook(
		int flags,
		int languages,
		int compression,
		int numThreads,
		tools::editor::GLWidget &glContext,
		std::ostream &out
	);
	void CancelCook();
	void ResetCancelCook();
#endif

#if defined(RAD_OPT_TOOLS)
	template <typename T>
	Binding::Ref BindCooker();

	KeyDef::MapRef KeyDefsForType(asset::Type type);
	KeyDef::Ref DefaultKeyDef() const;
#endif

	Asset::Ref Resolve(
		const char *path,
		Zone z,
		int flags = P_Load
	);

	Package::Entry::Ref Resolve(
		const char *path,
		int flags = P_Load
	);

	int ResolveId(
		const char *path,
		int flags = P_Load
	);

	Package::Ref ResolvePackage(
		const char *name,
		int flags = 0
	);

	int ProcessAll(
		Zone z,
		const xtime::TimeSlice &time,
		int flags,
		bool ignoreErrors
	);

private:

	enum {
		LumpSig = RAD_FOURCC_LE('p', 'l', 'm', 'p'),
		LumpId  = 0x55bddecf
	};

	RAD_DECLARE_GET(packages, const Package::Map&) { return m_packages; }

	template <typename T>
	struct SinkFactory : public details::SinkFactoryBase {
		virtual SinkBase *New();
		virtual int Stage() const;
	};

	friend class Binding;
	friend class Asset;
	friend class Package;

	typedef zone_map<int, details::SinkFactoryBase::Ref, ZPackagesT>::type SinkFactoryMap; // for order
	typedef zone_map<asset::Type, SinkFactoryMap, ZPackagesT>::type TypeSinkFactoryMap;

	PackageMan(
		Engine &engine,
		const char *pkgDir
	);

	int Process(
		const xtime::TimeSlice &time,
		const Asset::Ref &asset,
		int flags,
		int maxStage
	);

	void Unbind(Binding *binding);
	SinkFactoryMap &TypeSinks(asset::Type type);
	SinkBase *AllocSink(const details::SinkFactoryBase::Ref &f, const Asset::Ref &asset);
	void AllocSinks(const Asset::Ref &asset);

	void LoadBin(
		const char *name, 
		int loadFlags
	);

#if defined(RAD_OPT_TOOLS)
	friend class Cooker;
	typedef zone_map<asset::Type, details::CookerFactoryBase::Ref, ZPackagesT>::type TypeCookerFactoryMap;

	template <typename T>
	struct CookerFactory : public details::CookerFactoryBase {
		virtual Cooker::Ref New();
	};

	lua::State::Ref InitLua();

	void EnumeratePackage(
#if defined(RAD_OPT_PC_TOOLS)
		tools::UIProgress &ui,
#endif
		const String &path,
		const String &filename,
		int size
	);

	static int lua_Add(lua_State *L);

	void ParseEntry(
		pkg::Package *pkg,
		const String &path,
		const Package::Entry::Ref &entry,
		const KeyVal::Ref &parent,
		const lua::Variant::Map &map
	);

	typedef zone_map<asset::Type, KeyDef::MapRef, ZPackagesT>::type TypeToKeyDefsMap;
	bool LoadKeyDefs();
	bool LoadKeyDefs(const String &path, const String &filename);
	void ParseKeyDefs(lua_State *L, const String &filename, asset::Type type);
	void ParseKeyDefs(
		const String &filename, 
		const String &path, 
		const KeyDef::MapRef &keyMap, 
		const KeyDef::Ref &parent, 
		const lua::Variant::Map &map
	);

	TypeToKeyDefsMap m_keyDefs;
	KeyDef::Ref m_defaultKeyDef;
#endif

	struct TagData {
		U32 ofs;
		U16 type;
		U16 numImports;
		U16 imports[1]; // variable sized
	};

#if defined(RAD_OPT_TOOLS)
	bool MakeIntermediateDirs();
	Cooker::Ref AllocateIntermediateCooker(const Asset::Ref &asset);
	TypeCookerFactoryMap m_cookerFactoryMap;
#endif

#if defined(RAD_OPT_PC_TOOLS)
	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	tools::UIProgress *m_ui;
	// Delete, Rename, UpdateImports are not thread safe.
    void Delete(const Package::Ref &pkg);
    bool Rename(const Package::Ref &pkg, const char *name);
    void UpdateImports(const char *src, const char *dst);
	int PackageSize(const String &path);

	struct CookCommand {
		CookCommand *next;
		int flags;
		int result;
		String name;
		Cooker::Ref cooker;
	};

	struct CookThread : public thread::Thread {
		typedef boost::shared_ptr<CookThread> Ref;
		typedef zone_vector<Ref, ZPackagesT>::type Vec;

		CookThread(
			int threadNum,
			PackageMan *pkgMan,
			std::ostream &out,
			tools::editor::GLWidget &glw
		);

		virtual ~CookThread();

		virtual int ThreadProc();

		int threadNum;
		PackageMan *pkgMan;
		std::ostream *out;
		tools::editor::GLWidget *glw;
	};

	typedef zone_map<int, Cooker::Ref, ZPackagesT>::type CookerMap;

	struct CookPakFile {
		typedef boost::shared_ptr<CookPakFile> Ref;
		typedef zone_map<String, Ref, ZPackagesT>::type Map;
		typedef zone_map<int, Ref, ZPackagesT>::type IdMap;
		String name;
		StringVec roots;
		CookerMap cookers;
	};

	struct CookState {
		typedef boost::shared_ptr<CookState> Ref;
		CookerMap cookers;
		CookPakFile::Map pakfiles;
		CookPakFile::IdMap assetPakFiles;
		std::set<int> cooked;
		std::ostream *cout;
		int flags;
		int ptargets;
		int languages;
		String targetPath;
		CookCommand *pending;
		CookCommand *complete;
		thread::Gate waiting;
		thread::Gate finished;
		volatile int numPending;
		volatile bool done;
		volatile int error;
		int numThreads;
		Mutex mutex;
		Mutex ioMutex;
	};

	struct CookPackage {
		typedef zone_map<pkg::Package*, CookPackage, ZPackagesT>::type Map;
		Cooker::Vec cookers;
		Package::Ref pkg;
		StringVec imports;

		int AddImport(const char *path);
	};

	Cooker::Ref CookerForAsset(const Asset::Ref &asset, const CookPakFile::Ref &pakfile);

	int CookPakFileSources(
		const CookPakFile::Ref &pakfile,
		int flags,
		std::ostream &out
	);

	void CreateCookThreads(tools::editor::GLWidget &glContext, std::ostream &out);
	void ResetCooker(const Cooker::Ref &cooker);

	bool MakeBuildDirs(int flags, std::ostream &out);
	bool MakePackageDirs(const char *prefix);
	int BuildPackageData();
	int BuildManifest(int compression);
	int BuildPakFiles(int flags, int compression);
	int BuildPakFile(const char *name, int compression);
	int PakDirectory(
		const char *path,
		const char *prefix,
		int compression, 
		data_codec::lmp::Writer &lumpWriter
	);
	int LoadCookTxt(int flags, std::ostream &out);

	static int LuaChunkWrite(lua_State *L, const void *p, size_t size, void *ud);
	
	int CompileScripts();

	bool LoadTagFile(const Cooker::Ref &cooker, void *&data, AddrSize &size);
	bool CreateTagData(const Cooker::Ref &cooker, TagData *&data, AddrSize &size);

	volatile bool m_cancelCook;
	CookState::Ref m_cookState;
	CookThread::Vec m_cookThreads;
	bool m_resavePackages;
	friend struct CookThread;
#endif

	void MapId(int id, const Package::Ref &pkg);
	void UnmapId(int id);

	details::UInterlocked m_nextId;
	mutable details::SharedMutex m_m;
	TypeSinkFactoryMap m_sinkFactoryMap;
	Package::Map m_packages;
	IdPackageWMap m_idDir;
	String m_pkgDir;
	Engine &m_engine;

#if defined(RAD_OPT_TOOLS)
	StringSet m_packageDir;
#endif
};

} // pkg

#include <Runtime/PopPack.h>

#include "Packages.inl"

