// ComponentManager.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "ComponentManager.h"
#include "ComponentBuilder.h"
#include "../String.h"
#include "../Container/ZoneHashMap.h"
#include "../Container/ZoneVector.h"
#include "../Base/ObjectPool.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#if defined(RAD_OPT_WINX)
#define SEARCH_EXT ".dll"
#define COMPONENT_EXPORT_FN "___RAD_ComponentsExport@8"
#elif defined(RAD_OPT_LINUX) || defined(RAD_OPT_APPLE)
#define SEARCH_EXT ".so"
#define COMPONENT_EXPORT_FN "_Z22__RAD_ComponentsExportiRib"
#else
#error RAD_ERROR_UNSUP_PLAT
#endif

#include "../File.h"
#include "../SharedLibrary.h"

using namespace file;
using namespace string;

RAD_ZONE_DEF(RADRT_API, ZComMan, "Component Manager", ZRuntime);

namespace details {

typedef boost::mutex Mutex;
typedef boost::lock_guard<Mutex> Lock;

struct ComponentManager : 
	public IComponentManager,
	private AtomicRefCount
{

	typedef zone_vector<SharedLibrary*, ZComManT>::type SharedLibraryVec;
	typedef ObjectPool<SharedLibrary> SharedLibraryPool;

	struct SharedComponentFactory
	{
		RadComponentsExportFnType fn;
		int idx;
	};
	
	SharedLibraryVec m_sharedLibraries;
	SharedLibraryPool m_sharedLibraryPool;

	struct StaticComponentFactory
	{
		IComponentRegistrar *registrar;
	};

	typedef zone_hash_map<
		String, 
		SharedComponentFactory,
		ZComManT
	>::type SharedComponentFactoryMap;

	typedef zone_hash_map<
		String, 
		StaticComponentFactory,
		ZComManT
	>::type StaticComponentFactoryMap;
	
	typedef zone_hash_map<
		String, 
		HInterface,
		ZComManT
	>::type ComponentInstanceMap;

	#define LOCK() Lock ___x(m_m)

	Mutex m_m;
	SharedComponentFactoryMap m_factories;
	static StaticComponentFactoryMap s_factories;
	ComponentInstanceMap m_instances;

	ComponentManager()
	{
		m_sharedLibraryPool.Create(ZComMan, "component-man-shared-lib-pool", 8);
	}

	virtual ~ComponentManager()
	{
		//
		// This ensures that we release our handle to any components *before* the dll's are detached.
		//
		ReleaseCachedComponents();
	}

	virtual void ReleaseCachedComponents()
	{
		m_instances.clear();
	}

	virtual void ReleaseSharedLibraries()
	{
		ReleaseCachedComponents();
		for (SharedLibraryVec::iterator it = m_sharedLibraries.begin(); it != m_sharedLibraries.end(); ++it)
		{
			m_sharedLibraryPool.Destroy(*it);
		}
		m_sharedLibraries.clear();
	}

	virtual void LoadComponents(RadComponentsExportFnType fn)
	{
		RegisterComponents(fn);
	}

	virtual void LoadComponents(const char *path, ComponentLoadFlags flags)
	{
		Search search;
		RAD_DEBUG_ONLY(bool _b = EnforcePortablePathsEnabled(); EnforcePortablePaths(false));

		bool recursive = (flags & CLF_Recursive) ? true : false;
		char nativePath[MaxFilePathLen+1];

		if (flags & CLF_NativePath)
		{
			cpy(nativePath, path);
		}
		else
		{
			ExpandToNativePath(path, nativePath, MaxFilePathLen+1);
		}
		
		if (search.Open(
			nativePath, 
			SEARCH_EXT, 
			SearchFlags(((recursive)?Recursive:0)|FileNames|NativePath)))
		{
			char filename[MaxFilePathLen+1];
			while (search.NextFile(filename, MaxFilePathLen+1))
			{
				char buff[MaxFilePathLen+1];
				cpy(buff, nativePath);
				cat(buff, NativePathSeparator);
				cat(buff, filename);

				SharedLibrary *so = m_sharedLibraryPool.Construct();
				RAD_VERIFY(so);
				if (so->Load(buff, (flags & CLF_DisplayErrors) ? true : false))
				{
					RegisterComponents(so);
				}
				else
				{
					m_sharedLibraryPool.Destroy(so);
				}
			}
		}
		
		RAD_DEBUG_ONLY(EnforcePortablePaths(_b));
	}

	void RegisterComponents(SharedLibrary *so)
	{
		int count = 0;
		RadComponentsExportFnType fn = (RadComponentsExportFnType)so->ProcAddress(COMPONENT_EXPORT_FN);
		if (fn)
		{
			count = RegisterComponents(fn);
		}

		if (0 == count)
		{
			m_sharedLibraryPool.Destroy(so);
		}
		else
		{
			m_sharedLibraries.push_back(so);
		}
	}

	int RegisterComponents(RadComponentsExportFnType fn)
	{
		int count = 0;
		for (int i = 0;; ++i)
		{
			int ofs = 0;
			const char *name = (const char*)fn(i, ofs, true);
			if (!name) { break; }
			SharedComponentFactory f;
			f.fn = fn;
			f.idx = i;
			{
				LOCK();
				std::pair<SharedComponentFactoryMap::iterator, bool> r = m_factories.insert(SharedComponentFactoryMap::value_type(String(name), f));
				if (r.second) { ++count; }
			}
		}
		return count;
	}

	static void StaticRegisterComponent(IComponentRegistrar *registrar)
	{
//		LOCK();
		StaticComponentFactory f;
		f.registrar = registrar;
		s_factories.insert(StaticComponentFactoryMap::value_type(String(registrar->ID()), f));
	}

	virtual HInterface Create(const char *cid, bool cacheInstance)
	{
		if (cacheInstance)
		{
			HInterface x = Find(cid);
			if (x) { return x; }
		}

		LOCK();

		String scid(cid);
		{
			StaticComponentFactoryMap::const_iterator it = s_factories.find(scid);
			if (it != s_factories.end())
			{
				HInterface x(it->second.registrar->New());
				if (x && cacheInstance)
				{
					m_instances.insert(ComponentInstanceMap::value_type(scid, x));
				}
				return x;
			}
		}

		{
			SharedComponentFactoryMap::const_iterator it = m_factories.find(scid);
			if (it != m_factories.end())
			{
				int ofs = 0;
				HInterface x(it->second.fn(it->second.idx, ofs, false));

				if (x && cacheInstance)
				{
					m_instances.insert(ComponentInstanceMap::value_type(scid, x));
				}

				return x;
			}
		}

		return 0;
	}

	virtual HInterface Find(const char *cid)
	{
		String scid(cid);

		LOCK();

		ComponentInstanceMap::const_iterator it = m_instances.find(scid);
		if (it != m_instances.end())
		{
			return it->second;
		}
		
		return 0;
	}

	void ForEachLibFn(const char *nativeFnSymbol, Callback *cb)
	{
		RAD_ASSERT(cb&&nativeFnSymbol);
		LOCK();

		for (SharedLibraryVec::const_iterator it = m_sharedLibraries.begin(); it != m_sharedLibraries.end(); ++it)
		{
			void *f = (*it)->ProcAddress(nativeFnSymbol);
			if (f)
			{
				cb->Call(f);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, NULL, this)
	RAD_INTERFACE_MAP_BEGIN(ComponentManager)
		RAD_INTERFACE_MAP_ADD(IComponentManager)
	RAD_INTERFACE_MAP_END
};

ComponentManager::StaticComponentFactoryMap ComponentManager::s_factories;

RAD_IMPLEMENT_COMPONENT(ComponentManager, ComponentManager);

} // details

HComponentManager IComponentManager::Instance()
{
	static HComponentManager s;
	if (!s) { s = new (ZComMan) ::details::ComponentManager(); }
	return s;
}

void IComponentManager::StaticRegisterComponent(IComponentRegistrar *registrar)
{
	::details::ComponentManager::StaticRegisterComponent(registrar);
}


