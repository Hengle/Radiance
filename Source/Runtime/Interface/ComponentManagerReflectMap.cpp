// ComponentManagerReflectMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if !defined(RAD_OPT_NO_REFLECTION)

#include "ReflectInterface.h"
#include "ComponentManager.h"

//////////////////////////////////////////////////////////////////////////////////////////
// ComponentLoadFlags
//////////////////////////////////////////////////////////////////////////////////////////
#if defined(RAD_COMPONENT_MANAGER_DYNAMIC)
RADREFLECT_BEGIN_ENUM("ComponentLoadFlags", ComponentLoadFlags)
	RADREFLECT_ENUM_FLAGS_ATTRIBUTE
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)
	RADREFLECT_ENUM_VALUE_NAMED("Recursive", CLF_Recursive)
	RADREFLECT_ENUM_VALUE_NAMED("NativePath", CLF_NativePath)
	RADREFLECT_ENUM_VALUE_NAMED("DisplayErrors", CLF_DisplayErrors)
RADREFLECT_END(RADRT_API, ComponentLoadFlags)
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// IComponentManager
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_INTERFACE("IComponentManager", 
									 IComponentManager,
									 IInterface)
	
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	RADREFLECT_STATICMETHOD(HComponentManager, Instance)

	RADREFLECT_METHOD(void, ReleaseCachedComponents)
	
#if defined(RAD_COMPONENT_MANAGER_DYNAMIC)
	RADREFLECT_METHOD(void, ReleaseSharedLibaries)
	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_ARG("path", const wchar_t*)
		RADREFLECT_DEFAULT_ARG("flags", ComponentLoadFlags, CLF_Recursive|CLF_NativePath)
	RADREFLECT_END_METHOD(LoadComponents)
#endif

	RADREFLECT_BEGIN_METHOD(HInterface)
		RADREFLECT_ARG("cid", const char*)
		RADREFLECT_DEFAULT_ARG("cachedInstance", bool, true)
	RADREFLECT_END_METHOD(Create)

	RADREFLECT_BEGIN_METHOD(HInterface)
		RADREFLECT_ARG("cid", const char*)
	RADREFLECT_END_METHOD(Find)
		
RADREFLECT_END_INTERFACE(RADRT_API, 
	IComponentManager,
	"HComponentManager",
	HComponentManager
)

#endif // !defined(RAD_OPT_NO_REFLECTION)
