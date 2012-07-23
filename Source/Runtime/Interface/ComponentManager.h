// ComponentManager.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ComponentManagerDef.h"
#include "../FileDef.h"

class IComponentRegistrar;

RAD_REFLECTED_INTERFACE_BEGIN_API(RADRT_API, IComponentManager, IInterface, IComponentManager)

	// Methods

	static HComponentManager Instance();

	// load directly from function pointer.
	virtual void LoadComponents(RadComponentsExportFnType fn) = 0;

	// load from dll's.
	virtual void LoadComponents(
		const char *path,
		const file::FileSystemRef &fs,
		ComponentLoadFlags flags = ComponentLoadFlags(CLF_Recursive|CLF_NativePath)
	) = 0;

	virtual void ReleaseSharedLibraries() = 0;

	virtual void ReleaseCachedComponents() = 0;
	virtual HInterface Create(const char *cid, bool cacheInstance = false) = 0;
	virtual HInterface Find(const char *cid) = 0;

	class Callback
	{
	public:
		virtual void Call(void *fn) = 0;
	};

	virtual void ForEachLibFn(const char *nativeFnSymbol, Callback *cb) = 0;
	
private:
	friend class IComponentRegistrar;
	static void StaticRegisterComponent(IComponentRegistrar *registrar);

RAD_INTERFACE_END

class IComponentRegistrar
{
public:
	virtual const char *ID() const = 0;
	virtual void *New() const = 0;
protected:
	IComponentRegistrar() {}
	void Register()
	{
		IComponentManager::StaticRegisterComponent(this);
	}
};


