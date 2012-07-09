// RTLInteropMap.cpp
// Radiance Type Library Interop Reflection Map.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if !defined(RAD_OPT_NO_REFLECTION)

#include "ReflectMap.h"
#include "RTLInterop.h"

using namespace reflect::rtl_interop;

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::ManagedTypeInterop
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("reflect::rtl_interop::ManagedTypeInterop", ManagedTypeInterop)
RADREFLECT_END(RADRT_API, ManagedTypeInterop)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::Out
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("reflect::rtl_interop::Out", Out)
RADREFLECT_END(RADRT_API, Out)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::Visible
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("reflect::rtl_interop::Visible", Visible)
RADREFLECT_END(RADRT_API, Visible)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::NoSuper
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("reflect::rtl_interop::NoSuper", NoSuper)
RADREFLECT_END(RADRT_API, NoSuper)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::Abstract
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("reflect::rtl_interop::Abstract", Abstract)
RADREFLECT_END(RADRT_API, Abstract)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::PropertyGet
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("reflect::rtl_interop::PropertyGet", PropertyGet)
RADREFLECT_END(RADRT_API, PropertyGet)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::PropertySet
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("reflect::rtl_interop::PropertySet", PropertySet)
RADREFLECT_END(RADRT_API, PropertySet)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::Interface
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("reflect::rtl_interop::Interface", Interface)
RADREFLECT_END(RADRT_API, Interface)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::InterfaceHandle
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("reflect::rtl_interop::InterfaceHandle", InterfaceHandle)
RADREFLECT_END(RADRT_API, InterfaceHandle)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::Interface
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("reflect::rtl_interop::QueryInterface", QueryInterface)
RADREFLECT_END(RADRT_API, QueryInterface)

#endif // !defined(RAD_OPT_NO_REFLECTION)
