// ReflectPrivateDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ReflectPrivateUtil.h"
#include "../../Container/LWRedBlackNodeTree.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Private reflection helper macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADNULL_API

#define PRIVATE_RADREFLECT_NAMESPACE_BEGIN  namespace reflect {

#define PRIVATE_RADREFLECT_NAMESPACE_END }

#define PRIVATE_RADREFLECT_PRIVATE_NAMESPACE_BEGIN PRIVATE_RADREFLECT_NAMESPACE_BEGIN namespace details {

#define PRIVATE_RADREFLECT_PRIVATE_NAMESPACE_END PRIVATE_RADREFLECT_NAMESPACE_END }

#define PRIVATE_RADREFLECT_TYPE_SIG(_api, _type) \
	template <> _api const Class *RADRT_CALL Type<_type>()

//////////////////////////////////////////////////////////////////////////////////////////
// Private type and class reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_DECLARE(_api, _type) \
PRIVATE_RADREFLECT_NAMESPACE_BEGIN \
	PRIVATE_RADREFLECT_TYPE_SIG(_api, _type); \
	PRIVATE_RADREFLECT_TYPE_SIG(_api, _type const); \
PRIVATE_RADREFLECT_NAMESPACE_END

#define PRIVATE_RADREFLECT_EXPOSE_PRIVATES(_type) \
	friend struct ::reflect::details::TypeTraits<_type>;

//////////////////////////////////////////////////////////////////////////////////////////
// Private reflection helper macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_FRIENDS \
	template <typename T> friend struct ::reflect::details::TypeTraitsBase; \
	template <typename T> friend struct ::reflect::details::TypeTraits; \
	friend struct ::reflect::details::CLASSDESCRIPTOR;

#define PRIVATE_RADREFLECT_CLASS_BASE \
	::container::LWRedBlackNodeTree::Node

//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace reflect::details
//////////////////////////////////////////////////////////////////////////////////////////


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details forward declarations
//////////////////////////////////////////////////////////////////////////////////////////

struct CLASSDESCRIPTOR;

struct TypeTraitsShared;
template <typename T> struct TypeTraitsBase;
template <typename T> struct TypeTraits;

} // details
} // reflect

