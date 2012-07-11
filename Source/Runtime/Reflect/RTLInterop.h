// RTLInterop.h
// Radiance Type Library Interop types.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ReflectDef.h"
#include "../PushPack.h"


namespace reflect {
namespace rtl_interop {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::ManagedTypeInterop
//////////////////////////////////////////////////////////////////////////////////////////

class ManagedTypeInterop
{
	RADREFLECT_EXPOSE_PRIVATES(ManagedTypeInterop)

public:

	ManagedTypeInterop() : m_marshal(0), m_to(0), m_from(0) {}
	ManagedTypeInterop(const char *marshalType, const char *to, const char *from) : m_marshal(marshalType), m_to(to), m_from(from) {}

	const char *MarshalType() const { return m_marshal; }
	const char *To() const { return m_to; }
	const char *From() const { return m_from; }

private:

	const char *m_marshal, *m_to, *m_from;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::Out
//////////////////////////////////////////////////////////////////////////////////////////

class Out {};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::Visible
//////////////////////////////////////////////////////////////////////////////////////////

class Visible
{
	RADREFLECT_EXPOSE_PRIVATES(Visible)

public:

	Visible() : m_visible(true) {}
	Visible(bool visible) : m_visible(visible) {}

	operator bool () const { return m_visible; }

private:

	bool m_visible;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::NoSuper
//////////////////////////////////////////////////////////////////////////////////////////

class NoSuper {};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::Abstract
//////////////////////////////////////////////////////////////////////////////////////////

class Abstract {};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::PropertyGet
//////////////////////////////////////////////////////////////////////////////////////////

class PropertyGet {};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::PropertySet
//////////////////////////////////////////////////////////////////////////////////////////

class PropertySet {};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::Interface
//////////////////////////////////////////////////////////////////////////////////////////

// The interface stores no data. If it did this would cause a
// __gnu_cxx::recursive_init_error on G++ since both types
// construct each-other at static construction time.
// I actually don't understand how/why this worked on MSVC, it seems
// impossible...

class RTLInterface
{
	RADREFLECT_EXPOSE_PRIVATES(RTLInterface)
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::InterfaceHandle
//////////////////////////////////////////////////////////////////////////////////////////

class RTLInterfaceHandle
{
	RADREFLECT_EXPOSE_PRIVATES(RTLInterfaceHandle)

public:

	explicit RTLInterfaceHandle(const reflect::Class *ifaceType) : m_iface(ifaceType) {}
	const reflect::Class *InterfaceType() const { return m_iface; }

private:

	const reflect::Class *m_iface;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::QueryInterface
//////////////////////////////////////////////////////////////////////////////////////////

class QueryInterface {};

#if defined(RAD_OPT_CLR)
#pragma managed(push, on)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::ExtractNativeType()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename X>
inline T &ExtractNativeType(X ^x)
{
	return *static_cast<T*>(*(x->__RTLI_ReflectedObject));
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::rtl_interop::CreateManagedType()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename X>
inline T ^CreateManagedType(const X &x)
{
	T ^t = nullptr;
	if (x)
	{
		X *px = memory::SafeNew<X>(RAD_ALLOC_SIG_VOID);
		*px = x;
		t = gcnew T(&reflect::Reflect(*px), true);
	}
	return t;
}

template <typename T, typename X>
inline T ^CreateManagedType(X *x)
{
	T ^t = nullptr;
	if (x)
	{
		t = gcnew T(&reflect::Reflect(*x), true);
	}
	return t;
}

#pragma managed(pop)
#endif

} // rtl_iterop
} // reflect


//////////////////////////////////////////////////////////////////////////////////////////
// Interop helper macros
//////////////////////////////////////////////////////////////////////////////////////////

#if !defined(RAD_OPT_DISABLE_RTLI)

#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_MANAGEDTYPEINTEROP(_type, _to, _from)\
	RADREFLECT_CLASS_ATTRIBUTE(::reflect::rtl_interop::ManagedTypeInterop, (_type, _to, _from))

#define RADREFLECT_RTLI_ATTRIBUTE_VISIBLE(_visible) \
	RADREFLECT_ATTRIBUTE(::reflect::rtl_interop::Visible, (_visible))

#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(_visible) \
	RADREFLECT_CLASS_ATTRIBUTE(::reflect::rtl_interop::Visible, (_visible))

#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACE_NAMESPACE(_namespace, _handleType) \
	RADREFLECT_CLASS_ATTRIBUTE(::reflect::rtl_interop::RTLInterface, ())

#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACE(_handleType)\
	RADREFLECT_CLASS_ATTRIBUTE(::reflect::rtl_interop::RTLInterface, ())

#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACEHANDLE_NAMESPACE(_namespace, _ifaceType)\
	RADREFLECT_CLASS_ATTRIBUTE(::reflect::rtl_interop::RTLInterfaceHandle, (::reflect::Type<_namespace::_ifaceType>()))

#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACEHANDLE(_ifaceType)\
	RADREFLECT_CLASS_ATTRIBUTE(::reflect::rtl_interop::RTLInterfaceHandle, (::reflect::Type<_ifaceType>()))

#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_QUERYINTERFACE\
	RADREFLECT_CLASS_ATTRIBUTE(::reflect::rtl_interop::QueryInterface, ())

#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_ABSTRACT\
	RADREFLECT_CLASS_ATTRIBUTE(::reflect::rtl_interop::Abstract, ())

#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_NOSUPER\
	RADREFLECT_CLASS_ATTRIBUTE(::reflect::rtl_interop::NoSuper, ())

#define RADREFLECT_RTLI_ATTRIBUTE_OUT\
	RADREFLECT_ATTRIBUTE(::reflect::rtl_interop::Out, ())

#define RADREFLECT_RTLI_ATTRIBUTE_PROPERTYGET(_name)\
	RADREFLECT_ATTRIBUTE_NAMED(_name, ::reflect::rtl_interop::PropertyGet, ())

#define RADREFLECT_RTLI_ATTRIBUTE_PROPERTYSET(_name)\
	RADREFLECT_ATTRIBUTE_NAMED(_name, ::reflect::rtl_interop::PropertySet, ())

#if !defined(RAD_OPT_NO_REFLECTION)
RADREFLECT_DECLARE(RADRT_API, reflect::rtl_interop::ManagedTypeInterop)
RADREFLECT_DECLARE(RADRT_API, reflect::rtl_interop::Out)
RADREFLECT_DECLARE(RADRT_API, reflect::rtl_interop::Abstract)
RADREFLECT_DECLARE(RADRT_API, reflect::rtl_interop::Visible)
RADREFLECT_DECLARE(RADRT_API, reflect::rtl_interop::NoSuper)
RADREFLECT_DECLARE(RADRT_API, reflect::rtl_interop::PropertyGet)
RADREFLECT_DECLARE(RADRT_API, reflect::rtl_interop::PropertySet)
RADREFLECT_DECLARE(RADRT_API, reflect::rtl_interop::RTLInterface)
RADREFLECT_DECLARE(RADRT_API, reflect::rtl_interop::RTLInterfaceHandle)
RADREFLECT_DECLARE(RADRT_API, reflect::rtl_interop::QueryInterface)
#endif // !defined(RAD_OPT_NO_REFLECTION)

#else

#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_MANAGEDTYPEINTEROP(_type, _to, _from)
#define RADREFLECT_RTLI_ATTRIBUTE_VISIBLE(_visible)
#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(_visible)
#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACE_NAMESPACE(_namespace, _handleType)
#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACE(_handleType)
#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACEHANDLE_NAMESPACE(_namespace, _ifaceType)
#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACEHANDLE(_ifaceType)
#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_QUERYINTERFACE
#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_ABSTRACT
#define RADREFLECT_RTLI_CLASS_ATTRIBUTE_NOSUPER
#define RADREFLECT_RTLI_ATTRIBUTE_OUT
#define RADREFLECT_RTLI_ATTRIBUTE_PROPERTYGET(_name)
#define RADREFLECT_RTLI_ATTRIBUTE_PROPERTYSET(_name)

#endif

#include "../PopPack.h"
