// InterfaceReflectMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if !defined(RAD_OPT_NO_REFLECTION)

#include "ReflectInterface.h"
#include "Interface.h"

//////////////////////////////////////////////////////////////////////////////////////////
//IInterface
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("IInterface", IInterface)

	RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACE(HInterface)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_QUERYINTERFACE
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	RADREFLECT_STATICMETHOD(const char*, IID)
	RADREFLECT_METHOD(const char*, CID)

	RADREFLECT_BEGIN_METHOD(void*)
		RADREFLECT_ARG("iid", const char*)
	RADREFLECT_END_METHOD(Interface)
		RADREFLECT_ATTRIBUTE(reflect::rtl_interop::QueryInterface, ())

	RADREFLECT_METHOD(UReg, Reference)
		RADREFLECT_RTLI_ATTRIBUTE_VISIBLE(false)
	RADREFLECT_METHOD(UReg, Release)
		RADREFLECT_RTLI_ATTRIBUTE_VISIBLE(false)

RADREFLECT_END(RADRT_API, IInterface)

//////////////////////////////////////////////////////////////////////////////////////////
// InterfaceHandle
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS_DESTRUCTOR("HInterface", HInterface, InterfaceHandle)

	RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACEHANDLE(IInterface)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)
	RADREFLECT_CONSTRUCTOR
		RADREFLECT_RTLI_ATTRIBUTE_VISIBLE(false)
	RADREFLECT_BEGIN_CONSTRUCTOR
		RADREFLECT_ARG("iface", void*)
	RADREFLECT_END_CONSTRUCTOR
	RADREFLECT_MEMBER_NAMED("Data", void*, m_pData)
	
RADREFLECT_END(RADRT_API, HInterface)

#endif // !defined(RAD_OPT_NO_REFLECTION)
