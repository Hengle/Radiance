// ReflectInterface.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../ReflectMap.h"
#include "../ReflectDef.h"
#include "../Reflect/RTLInterop.h"
#include "../Reflect/Attributes.h"

//////////////////////////////////////////////////////////////////////////////////////////
//Interface Reflection Macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_DECLARE_INTERFACE(_api, _ifaceName, _handleType)\
	RADREFLECT_DECLARE(_api, _ifaceName)\
	RADREFLECT_DECLARE(_api, _handleType)

#define RADREFLECT_BEGIN_INTERFACE(_ifaceName, _ifaceType, _superType)\
	RADREFLECT_BEGIN_CLASS(_ifaceName, _ifaceType)\
		RADREFLECT_SUPER(_superType)

#define RADREFLECT_BEGIN_INTERFACE_NAMESPACE(_ifaceName, _namespace, _ifaceType, _superType)\
	RADREFLECT_BEGIN_CLASS_NAMESPACE(_ifaceName, _namespace, _ifaceType)\
		RADREFLECT_SUPER(_superType)

#define RADREFLECT_END_INTERFACE_NAMESPACE(_api, _namespace, _ifaceType, _handleName, _handleType)\
		RADREFLECT_STATICMETHOD(const char*, IID)\
		RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACE_NAMESPACE(_namespace, _handleType)\
	RADREFLECT_END_NAMESPACE(_api, _namespace, _ifaceType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_handleName, _namespace, _handleType, InterfaceHandle)\
		RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACEHANDLE_NAMESPACE(_namespace, _ifaceType)\
		RADREFLECT_CONSTRUCTOR\
			RADREFLECT_RTLI_ATTRIBUTE_VISIBLE(false)\
		RADREFLECT_BEGIN_CONSTRUCTOR\
			RADREFLECT_ARG("iface", void*)\
		RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_END_NAMESPACE(_api, _namespace, _handleType)

#define RADREFLECT_END_INTERFACE(_api, _ifaceType, _handleName, _handleType)\
		RADREFLECT_STATICMETHOD(const char*, IID)\
		RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACE(_handleType)\
	RADREFLECT_END(_api, _ifaceType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR(_handleName, _handleType, InterfaceHandle)\
		RADREFLECT_RTLI_CLASS_ATTRIBUTE_INTERFACEHANDLE(_ifaceType)\
		RADREFLECT_CONSTRUCTOR\
			RADREFLECT_RTLI_ATTRIBUTE_VISIBLE(false)\
		RADREFLECT_BEGIN_CONSTRUCTOR\
			RADREFLECT_ARG("iface", void*)\
		RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_END(_api, _handleType)


