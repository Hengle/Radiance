// InterfaceBuilder.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Thread/Interlocked.h"
#include "../StringBase.h"
#include "Interface.h"
#include "../Base/RefCount.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Interface builder macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RAD_IMPLEMENT_IINTERFACE(_inc_ref_callback_, _dec_ref_callback_, _zero_refs_callback_, _component_) \
	static const char *ID; \
	const  char *CID() { return ID; } \
	const void *Component() { return _component_; } \
	::UReg Reference()   { ::UReg x = (::UReg)IncrementRefCount(); _inc_ref_callback_; return x; } \
	::UReg Release()     { ::UReg x = (::UReg)DecrementRefCount(); if(x) {_dec_ref_callback_;} return x; } \
	void OnZeroReferences() { if (!_zero_refs_callback_) { delete this; } }

// _interface_class_ defines the class path to take to get to IInterface
#define RAD_INTERFACE_MAP_BEGIN(_interface_class_) \
	void *Interface(const char *iid) \
	{ \
		void *temp; \
		if (0 == ::string::cmp(::IInterface::IID(), iid)) \
		{ \
			temp = 0; \
			Reference(); \
			return static_cast<IInterface*>(static_cast< _interface_class_*>(this)); \
		}

#define RAD_INTERFACE_MAP_ADD(_class_) \
		else if(0 == ::string::cmp(_class_::IID(), iid)) \
		{ \
			temp = 0; \
			Reference(); \
			return static_cast< _class_*>(this); \
		}

#define RAD_INTERFACE_MAP_ADD_NO_REF(_class_) \
		else if(0 == ::string::cmp(_class_::IID(), iid)) \
		{ \
			temp = 0; \
			return static_cast< _class_*>(this); \
		}

#define RAD_INTERFACE_MAP_ADD_CUSTOM(_class_, _return_) \
		else if(0 == ::string::cmp(_class_::IID(), iid)) \
		{ \
			temp = 0; \
			Reference(); \
			return _return_; \
		}

#define RAD_INTERFACE_MAP_ADD_CUSTOM_NO_REF(_class_, _return_) \
		else if(0 == ::string::cmp(_class_::IID(), iid)) \
		{ \
			temp = 0; \
			return _return_; \
		}

#define RAD_INTERFACE_MAP_BASE_CLASS(_class_) \
		else if(0 != (temp=_class_::Interface(iid))) \
		{ \
			return temp; \
		}

#define RAD_INTERFACE_MAP_END \
		return NULL; \
	}

//////////////////////////////////////////////////////////////////////////////////////////
// Component helper macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RAD_IMPLEMENT_COMPONENT(_component_, _id_) \
	const char* _component_::ID = #_id_

template <typename T>
inline T *InterfaceComponent(const HInterface &hface)
{
	RAD_ASSERT(hface);
	T *t = static_cast<T*>(const_cast<void*>(hface->Component()));
	RAD_ASSERT(t);
	return t;
}

