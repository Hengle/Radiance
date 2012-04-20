// InterfaceDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntInterface.h"
#include "ReflectInterface.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Public interface declaration macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RAD_INTERFACE_API(_api, _interface) \
	PRIVATE_RAD_INTERFACE(_api, _interface)

#define RAD_INTERFACE(_interface) \
	RAD_INTERFACE_API(RADNULL_API, _interface)

#define RAD_DECLARE_INTERFACE(_handle, _interface) \
	PRIVATE_RAD_DECLARE_INTERFACE(_interface); \
	typedef ::InterfaceHandle<_interface> _handle

#define RAD_INTERFACE_BEGIN(_interface, _base, _iid) \
	PRIVATE_RAD_DECLARE_INTERFACE(_interface); \
	RAD_INTERFACE(_interface) : public _base { \
	public: \
		typedef _base SUPER; \
		typedef ::InterfaceHandle<_interface> HANDLE; \
		static const char *IID() { return #_iid; }

#define RAD_INTERFACE_BEGIN_API(_api, _interface, _base, _iid) \
	PRIVATE_RAD_DECLARE_INTERFACE(_interface); \
	RAD_INTERFACE_API(_api, _interface) : public _base { \
	public: \
		typedef _base SUPER; \
		typedef ::InterfaceHandle<_interface> HANDLE; \
		static const char *IID() { return #_iid; }

#define RAD_REFLECTED_INTERFACE_BEGIN(_interface, _base, _iid)\
	RAD_INTERFACE_BEGIN(_interface, _base, _iid)\
	private: RADREFLECT_EXPOSE_PRIVATES(_interface)\
	public:

#define RAD_REFLECTED_INTERFACE_BEGIN_API(_api, _interface, _base, _iid)\
	RAD_INTERFACE_BEGIN_API(_api, _interface, _base, _iid)\
	private: RADREFLECT_EXPOSE_PRIVATES(_interface)\
	public:

#define RAD_INTERFACE_END };

RAD_DECLARE_EXCEPTION(BadInterfaceVersionException, exception);

template <typename I> class InterfaceHandle;
RAD_DECLARE_INTERFACE(HInterface, IInterface);

#if !defined(RAD_OPT_NO_REFLECTION)

RADREFLECT_DECLARE(RADRT_API, ::IInterface)
RADREFLECT_DECLARE(RADRT_API, ::HInterface)

#endif // !defined(RAD_OPT_NO_REFLECTION)

