// ComponentManagerDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Interface.h"

// Quick docs on how this works.
// The ComponentManager supports dll's or static linking (or both).
//
// STATIC LIB
// ------------------
// Make a ComponentExports.h file: Declare your export function with 
// RAD_LIB_DECLARE_COMPONENTS_EXPORT_FN(mylibfn)
//
// Implement your code in a .cpp:
//
// RAD_LIB_BEGIN_COMPONENTS_EXPORT_FN(mylibfn)
//	RAD_EXPORT_COMPONENT(component1)
//	RAD_EXPORT_COMPONENT(component2)
// RAD_LIB_END_COMPONENTS_EXPORT
//
// In your .exe or .dll link in the static lib components via:
// RAD_LIB_REGISTER_COMPONENT_FN(mylibfn)
//
// Executable or Dynamic/Static Lib (depending on PML_OPT_DLL option)
// ------------------
// Make a ComponentsExport.h file: Declare your export function
// RAD_DECLARE_COMPONENTS_EXPORT_FN(mydllfn)
//
// Implement your code in a .cpp:
//
// RAD_BEGIN_COMPONENTS_EXPORT_FN(mydllfn)
//	RAD_EXPORT_COMPONENT(component1)
//	RAD_EXPORT_COMPONENT(component2)
// RAD_END_COMPONENTS_EXPORT
// 
// Add your DLL export function. These macros will compile out if not building a dll:
// RAD_BEGIN_COMPONENTS_EXPORT
//	RAD_CALL_COMPONENTS_EXPORT_FN(mydllfn)
// RAD_END_COMPONENTS_EXPORT
//
// If the components are part of your .exe add the following to your code
// path to register them in the component manager:
// RAD_REGISTER_COMPONENTS(componentManager, mydllfn)

#if defined(RAD_OPT_DLL)
	#define RAD_COMPONENT_MANAGER_DYNAMIC
#else
	#define RAD_COMPONENT_MANAGER_STATIC
#endif

///////////////////////////////////////////////////////////////////////////////
// Macros
///////////////////////////////////////////////////////////////////////////////

#if defined(RAD_COMPONENT_MANAGER_DYNAMIC)
	#define RAD_DECLARE_EXPORT_COMPONENTS(_name)
	#define RAD_BEGIN_EXPORT_COMPONENTS(_api, _name) RAD_DLL_BEGIN_EXPORT_COMPONENTS(_api)
	#define RAD_EXPORT_COMPONENT(_class) RAD_DLL_EXPORT_COMPONENT(_class)
	#define RAD_EXPORT_COMPONENT_NAMESPACE(_namespace, _class) RAD_DLL_EXPORT_COMPONENT_NAMESPACE(_namespace, _class)
	#define RAD_END_EXPORT_COMPONENTS RAD_DLL_END_EXPORT_COMPONENTS
	#define RAD_BEGIN_EXPORT_COMPONENTS_FN(_api, _name) RAD_DLL_BEGIN_COMPONENTS_EXPORT_FN(_api, _name)
	#define RAD_LINK_EXPORT_COMPONENTS_FN(_name) RAD_DLL_LINK_EXPORT_COMPONENTS_FN(_name)
	#define RAD_DECLARE_EXPORT_COMPONENTS_FN(_api, _name) RAD_DLL_DECLARE_EXPORT_COMPONENTS_FN(_api, _name)
	#define RAD_REGISTER_COMPONENTS(_com, _name) (_com)->LoadComponents(_name)
#else
	#define RAD_DECLARE_EXPORT_COMPONENTS(_name) RAD_LIB_DECLARE_EXPORT_COMPONENTS_FN(_name)
	#define RAD_BEGIN_EXPORT_COMPONENTS(_api, _name) RAD_LIB_BEGIN_EXPORT_COMPONENTS_FN(_name)
	#define RAD_END_EXPORT_COMPONENTS RAD_LIB_END_EXPORT_COMPONENTS
	#define RAD_LINK_EXPORT_COMPONENTS_FN(_name) RAD_LIB_LINK_EXPORT_COMPONENTS_FN(_name)
	#define RAD_EXPORT_COMPONENT(_class) RAD_LIB_EXPORT_COMPONENT(_class)
	#define RAD_EXPORT_COMPONENT_NAMESPACE(_namespace, _class) RAD_LIB_EXPORT_COMPONENT_NAMESPACE(_namespace, _class)
	#define RAD_BEGIN_EXPORT_COMPONENTS_FN(_api, _name) RAD_LIB_BEGIN_EXPORT_COMPONENTS_FN(_name)
	#define RAD_DECLARE_EXPORT_COMPONENTS_FN(_api, _name) RAD_LIB_DECLARE_EXPORT_COMPONENTS_FN(_name)
	#define RAD_REGISTER_COMPONENTS(_com, _name) RAD_LIB_LINK_EXPORT_COMPONENTS_FN(_name)
#endif

enum ComponentLoadFlags
{
	RAD_FLAG(CLF_Recursive),
	RAD_FLAG(CLF_NativePath),
	RAD_FLAG(CLF_DisplayErrors)
};

RAD_ZONE_DEC(RADRT_API, ZComMan);
RAD_DECLARE_EXCEPTION(MissingComponentException, exception);
RAD_DECLARE_INTERFACE(HComponentManager, IComponentManager);

#if !defined(RAD_OPT_NO_REFLECTION)

#if defined(RAD_COMPONENT_MANAGER_DYNAMIC)
RADREFLECT_DECLARE(RADRT_API, ComponentLoadFlags)
#endif

RADREFLECT_DECLARE_INTERFACE(RADRT_API, IComponentManager, HComponentManager)

#endif

///////////////////////////////////////////////////////////////////////////////
// DLL
///////////////////////////////////////////////////////////////////////////////

typedef void * (RAD_STDCALL *RadComponentsExportFnType)(int, int &, bool);
#define RAD_DLL_DECLARE_EXPORT_COMPONENTS(_api) extern "C" _api void *RAD_STDCALL __RAD_ComponentsExport(int, bool)
#define RAD_DLL_BEGIN_EXPORT_COMPONENTS(_api)\
	extern "C" _api void* RAD_STDCALL __RAD_ComponentsExport(int _idx, int &_ofs, bool _queryName) { void *_z = 0;
#define RAD_DLL_EXPORT_COMPONENT(_class) \
	if (_idx == _ofs++) { if (_queryName) { return (void*)(_class::ID); } return (::ZComMan) new _class(); }
#define RAD_DLL_EXPORT_COMPONENT_NAMESPACE(_namespace, _class) \
	if (_idx == _ofs++) { if (_queryName) { return (void*)(_namespace::_class::ID); } return new (::ZComMan) _namespace::_class(); }
#define RAD_DLL_END_COMPONENTS_EXPORT return 0; }
#define RAD_DLL_BEGIN_EXPORT_COMPONENTS_FN(_api, _name) _api void *_name(int _idx, int &_ofs, bool _queryName) { void *_z = 0;
#define RAD_DLL_LINK_EXPORT_COMPONENTS_FN(_name) if ((_z=_name(_idx, _ofs, _queryName))) { return _z; }
#define RAD_DLL_DECLARE_EXPORT_COMPONENTS_FN(_api, _name) _api void *_name(int _idx, int &_ofs, bool _queryName)

///////////////////////////////////////////////////////////////////////////////
// LIB
///////////////////////////////////////////////////////////////////////////////

#define RAD_LIB_END_EXPORT_COMPONENTS }
#define RAD_LIB_BEGIN_EXPORT_COMPONENTS_FN(_name) void _name() { 
#define RAD_LIB_CALL_EXPORT_COMPONENTS_FN(_name) _name();
#define RAD_LIB_DECLARE_EXPORT_COMPONENTS_FN(_name) void _name()
#define RAD_LIB_LINK_EXPORT_COMPONENTS_FN(_name) _name();
#define RAD_LIB_EXPORT_COMPONENT(_class) \
class _class##__rad_component_registrar : public ::IComponentRegistrar \
{\
public:\
	_class##__rad_component_registrar()\
	{\
		Register();\
	}\
	virtual const char *ID() const { return _class::ID; }\
	virtual void *New() const { return new (::ZComMan) _class(); }\
};\
static _class##__rad_component_registrar _class##__rad_component_instance;

#define RAD_LIB_EXPORT_COMPONENT_NAMESPACE(_namespace, _class) \
class _class##__rad_component_registrar : public ::IComponentRegistrar \
{\
public:\
	_class##__rad_component_registrar()\
	{\
		Register();\
	}\
	virtual const char *ID() const { return _namespace::_class::ID; }\
	virtual void *New() const { return new (::ZComMan) _namespace::_class(); }\
};\
static _class##__rad_component_registrar _class##__rad_component_instance;

