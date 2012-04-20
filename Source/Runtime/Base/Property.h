/*! \file Property.h
	\author Joe Riedel
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup properties
*/

/*! \defgroup properties Property System
	Radiance Property System description.
*/

#pragma once

#if !defined(__PRIVATE_RAD_OPT_INTBASE__)
	#error "Do not include this file directly, please include Base.h instead"
#endif

#if defined(_DOXYGEN)

#define RAD_DECLARE_PROPERTY(_class, name, get_type, set_type) GetSetProperty<get_type, set_type> name
#define RAD_DECLARE_PROPERTY_EX(_ctype, _class, name, get_type, set_type) GetSetProperty<get_type, set_type> name
#define RAD_DECLARE_PROPERTY_FOR_INCOMPLETE_TYPE(_class, name, get_type, set_type) GetSetProperty<get_type, set_type> name
#define RAD_DECLARE_PROPERTY_FOR_INCOMPLETE_TYPE_EX(_ctype, _class, name, get_type, set_type) GetSetProperty<get_type, set_type> name 
#define RAD_DECLARE_READONLY_PROPERTY(_class, name, get_type) GetProperty<get_type> name
#define RAD_DECLARE_READONLY_PROPERTY_EX(_ctype, _class, name, get_type) GetProperty<get_type> name 
#define RAD_DECLARE_READONLY_PROPERTY_FOR_INCOMPLETE_TYPE(_class, name, get_type) GetProperty<get_type> name 
#define RAD_DECLARE_READONLY_PROPERTY_FOR_INCOMPLETE_TYPE_EX(_ctype, _class, name, get_type) GetProperty<get_type> name
#define RAD_DECLARE_WRITEONLY_PROPERTY(_class, name, set_type) GetProperty<get_type> name
#define RAD_DECLARE_WRITEONLY_PROPERTY_EX(_ctype, _class, name, set_type) SetProperty<set_type> name
#define RAD_DECLARE_WRITEONLY_PROPERTY_FOR_INCOMPLETE_TYPE(_class, name, set_type) SetProperty<set_type> name
#define RAD_DECLARE_WRITEONLY_PROPERTY_FOR_INCOMPLETE_TYPE_EX(_ctype, _class, name, set_type) SetProperty<set_type> name
#define RAD_IMPLEMENT_READONLY_PROPERTY(_class, name, get_type) 
#define RAD_IMPLEMENT_WRITEONLY_PROPERTY(_class, name, set_type)
#define RAD_DECLARE_GET(name, get_type)(name, type) get_type get_##name()
#define RAD_DECLARE_SET(name, set_type)(name, type) void set_##name(set_type)
#define RAD_DECLARE_GETSET(name, get_type, set_type) get_type get##name(void); void set_##name(set_type)
#define RAD_IMPLEMENT_GET(name) get_##name()
#define RAD_IMPLEMENT_SET(name) set_##name

//! Property Field
/*! Defines a Property, which may have a get and set accessor, or both. 
	These properties behave like assignable fields (similiar to C# properties).
	\ingroup properties
	\sa properties
	\sa RAD_DECLARE_PROPERTY
	\sa RAD_DECLARE_PROPERTY_EX
	\sa RAD_DECLARE_PROPERTY_FOR_INCOMPLETE_TYPE
	\sa RAD_DECLARE_PROPERTY_FOR_INCOMPLETE_TYPE_EX
	\sa RAD_DECLARE_READONLY_PROPERTY
	\sa RAD_DECLARE_READONLY_PROPERTY_EX
	\sa RAD_DECLARE_READONLY_PROPERTY_FOR_INCOMPLETE_TYPE
	\sa RAD_DECLARE_READONLY_PROPERTY_FOR_INCOMPLETE_TYPE_EX
	\sa RAD_DECLARE_WRITEONLY_PROPERTY
	\sa RAD_DECLARE_WRITEONLY_PROPERTY_EX
	\sa RAD_DECLARE_WRITEONLY_PROPERTY_FOR_INCOMPLETE_TYPE
	\sa RAD_DECLARE_WRITEONLY_PROPERTY_FOR_INCOMPLETE_TYPE_EX
	\sa RAD_IMPLEMENT_READONLY_PROPERTY
	\sa RAD_IMPLEMENT_WRITEONLY_PROPERTY
	\sa RAD_DECLARE_GET
	\sa RAD_DECLARE_SET
	\sa RAD_DECLARE_GETSET
	\sa RAD_IMPLEMENT_GET
	\sa RAD_IMPLEMENT_SET
*/

struct RADProperty {};

//! Read Only Property Field
template <typename T>
struct GetProperty : public RADProperty {};

//! Write Only Property Field
template <typename T>
struct SetProperty : public RADProperty {};

//! Read & Write Property Field
template <typename T, typename X>
struct GetSetProperty : public RADProperty {};

#else

#include "PrivateProperty.h"

#define RAD_DECLARE_PROPERTY(_class, name, get_type, set_type) \
	RAD_DECLARE_PROPERTY_EX(class, _class, name, get_type, set_type)

#define RAD_DECLARE_PROPERTY_EX(_ctype, _class, name, get_type, set_type) \
	RAD_PRIVATE_PROPERTY_DECLARE_FRIENDS(name); \
	RAD_PRIVATE_PROPERTY_DECLARE_GETSET_CLASS(_ctype, _class, name, get_type, set_type); \
	RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) name

#define RAD_DECLARE_PROPERTY_FOR_INCOMPLETE_TYPE(_class, name, get_type, set_type) \
	RAD_DECLARE_PROPERTY_FOR_INCOMPLETE_TYPE_EX(class, _class, name, get_type, set_type)

#define RAD_DECLARE_PROPERTY_FOR_INCOMPLETE_TYPE_EX(_ctype, _class, name, get_type, set_type) \
	RAD_PRIVATE_PROPERTY_DECLARE_FRIENDS(name); \
	RAD_PRIVATE_PROPERTY_DECLARE_GETSET_CLASS_FOR_INCOMPLETE_TYPE(_ctype, _class, name, get_type, set_type); \
	RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) name

#define RAD_DECLARE_READONLY_PROPERTY(_class, name, get_type) \
	RAD_DECLARE_READONLY_PROPERTY_EX(class, _class, name, get_type)

#define RAD_DECLARE_READONLY_PROPERTY_EX(_ctype, _class, name, get_type) \
	RAD_PRIVATE_PROPERTY_DECLARE_FRIENDS(name); \
	RAD_PRIVATE_PROPERTY_DECLARE_GET_CLASS(_ctype, _class, name, get_type); \
	RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) name

#define RAD_DECLARE_READONLY_PROPERTY_FOR_INCOMPLETE_TYPE(_class, name, get_type) \
	RAD_DECLARE_READONLY_PROPERTY_FOR_INCOMPLETE_TYPE_EX(class, _class, name, get_type)

#define RAD_DECLARE_READONLY_PROPERTY_FOR_INCOMPLETE_TYPE_EX(_ctype, _class, name, get_type) \
	RAD_PRIVATE_PROPERTY_DECLARE_FRIENDS(name); \
	RAD_PRIVATE_PROPERTY_DECLARE_GET_CLASS_FOR_INCOMPLETE_TYPE(_ctype, _class, name, get_type); \
	RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) name

#define RAD_DECLARE_WRITEONLY_PROPERTY(_class, name, set_type) \
	RAD_DECLARE_WRITEONLY_PROPERTY_EX(class, _class, name, set_type)

#define RAD_DECLARE_WRITEONLY_PROPERTY_EX(_ctype, _class, name, set_type) \
	RAD_PRIVATE_PROPERTY_DECLARE_FRIENDS(name); \
	RAD_PRIVATE_PROPERTY_DECLARE_SET_CLASS(_ctype, _class, name, set_type); \
	RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) name

#define RAD_DECLARE_WRITEONLY_PROPERTY_FOR_INCOMPLETE_TYPE(_class, name, set_type) \
	RAD_DECLARE_WRITEONLY_PROPERTY_FOR_INCOMPLETE_TYPE_EX(class, _class, name, set_type)

#define RAD_DECLARE_WRITEONLY_PROPERTY_FOR_INCOMPLETE_TYPE_EX(_ctype, _class, name, set_type) \
	RAD_PRIVATE_PROPERTY_DECLARE_FRIENDS(name); \
	RAD_PRIVATE_PROPERTY_DECLARE_SET_CLASS_FOR_INCOMPLETE_TYPE(_ctype, _class, name, set_type); \
	RAD_PRIVATE_PROPERTY_GETSET_CLASS_NAME(name) name

#define RAD_IMPLEMENT_READONLY_PROPERTY(_class, name, get_type) \
	RAD_PRIVATE_PROPERTY_IMPLEMENT_GET_CLASS(_class, name, get_type)

#define RAD_IMPLEMENT_WRITEONLY_PROPERTY(_class, name, set_type) \
	RAD_PRIVATE_PROPERTY_IMPLEMENT_SET_CLASS(_class, name, set_type)

#define RAD_DECLARE_GET(name, type) \
	RAD_PRIVATE_PROPERTY_DECLARE_GET(name, type)

#define RAD_DECLARE_SET(name, type) \
	RAD_PRIVATE_PROPERTY_DECLARE_SET(name, type)

#define RAD_DECLARE_GETSET(name, get_type, set_type) \
	RAD_DECLARE_GET(name, get_type); \
	RAD_DECLARE_SET(name, set_type)

#define RAD_IMPLEMENT_GET(name) \
	RAD_PRIVATE_PROPERTY_IMPLEMENT_GET(name)
#define RAD_IMPLEMENT_SET(name) \
	RAD_PRIVATE_PROPERTY_IMPLEMENT_SET(name)

#endif
