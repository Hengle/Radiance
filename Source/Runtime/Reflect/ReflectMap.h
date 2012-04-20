// ReflectMap.h
// Macros for creating a reflection map
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

// let include chain know we're here (disables some necessary warnings, see Reflect/win/WinOpts.h
#define __RADREFLECT_REFLECTMAP_H__ 

#include "../Base.h"
#include "Private/ReflectPrivateMap.h"
#include "RTLInterop.h"
#include "Attributes.h"

#include "../PushPack.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Type and class reflection macros
//////////////////////////////////////////////////////////////////////////////////////////
//
// RADREFLECT_BEGIN begins a type reflection block.
// RADREFLECT_BEGIN_CLASS begins a class reflection block.
// RADREFLECT_END ends a type or class reflection block.
//
// RADREFLECT_TYPE defines a reflection block that reflects
// a simple type.
//
// RADREFLECT_TYPE_NULL defines a reflection block that specifies
// no reflect data (Type<>() returns NULL).  The void type is
// is reflected as NULL.
//
// Type and class reflection blocks must be defined in the
// global namespace.
//
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_BEGIN_NAMESPACE(_name, _namespace, _type) \
	PRIVATE_RADREFLECT_BEGIN_NAMESPACE(_name, _namespace, _type)

#define RADREFLECT_BEGIN(_name, _type) \
	PRIVATE_RADREFLECT_BEGIN(_name, _type)

#define RADREFLECT_BEGIN_CLASS_NAMESPACE(_name, _namespace, _type) \
	PRIVATE_RADREFLECT_BEGIN_CLASS_NAMESPACE(_name, _namespace, _type) \

#define RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, _destructor) \
	PRIVATE_RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, _destructor) \

#define RADREFLECT_BEGIN_CLASS(_name, _type) \
	PRIVATE_RADREFLECT_BEGIN_CLASS(_name, _type) \

#define RADREFLECT_BEGIN_CLASS_DESTRUCTOR(_name, _type, _destructor) \
	PRIVATE_RADREFLECT_BEGIN_CLASS_DESTRUCTOR(_name, _type, _destructor) \

#define RADREFLECT_END_NAMESPACE(_api, _namespace, _type) \
	PRIVATE_RADREFLECT_END_NAMESPACE(_api, _namespace, _type)

#define RADREFLECT_END(_api, _type) \
	PRIVATE_RADREFLECT_END(_api, _type)

#define RADREFLECT_CLASS_NAMESPACE(_api, _name, _namespace, _type) \
	RADREFLECT_BEGIN_CLASS_NAMESPACE(_name, _namespace, _type) \
	RADREFLECT_END_NAMESPACE(_api, _namespace, _type)

#define RADREFLECT_CLASS(_api, _name, _type) \
	RADREFLECT_BEGIN_CLASS(_name, _type) \
	RADREFLECT_END(_api, _type)

#define RADREFLECT_TYPE_NAMESPACE(_api, _name, _namespace, _type) \
	RADREFLECT_BEGIN_NAMESPACE(_name, _namespace, _type) \
	RADREFLECT_END_NAMESPACE(_api, _namespace, _type)

#define RADREFLECT_TYPE(_api, _name, _type) \
	RADREFLECT_BEGIN(_name, _type) \
	RADREFLECT_END(_api, _type)

#define RADREFLECT_TYPE_NULL_NAMESPACE(_api, _namespace, _type) \
	PRIVATE_RADREFLECT_TYPE_NULL(_api, _namespace, _type)

#define RADREFLECT_TYPE_NULL(_api, _type) \
	PRIVATE_RADREFLECT_TYPE_NULL(_api, _type)

//////////////////////////////////////////////////////////////////////////////////////////
// Attribute reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_CLASS_ATTRIBUTE_NAMED(_name, _type, _args) \
	PRIVATE_RADREFLECT_CLASS_ATTRIBUTE(_name, _type, _args)

#define RADREFLECT_CLASS_ATTRIBUTE(_type, _args) \
	PRIVATE_RADREFLECT_CLASS_ATTRIBUTE(#_type, _type, _args)

#define RADREFLECT_ATTRIBUTE_NAMED(_name, _type, _args) \
	PRIVATE_RADREFLECT_ATTRIBUTE(_name, _type, _args)

#define RADREFLECT_ATTRIBUTE(_type, _args) \
	PRIVATE_RADREFLECT_ATTRIBUTE(#_type, _type, _args)

//////////////////////////////////////////////////////////////////////////////////////////
// Superclass reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_SUPER_NAMED(_name, _type) \
	PRIVATE_RADREFLECT_SUPER(_name, _type)

#define RADREFLECT_SUPER(_type) \
	RADREFLECT_SUPER_NAMED(#_type, _type)

//////////////////////////////////////////////////////////////////////////////////////////
// Member reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_MEMBER_NAMED(_name, _type, _member) \
	PRIVATE_RADREFLECT_MEMBER(_name, _type, _member)

#define RADREFLECT_MEMBER(_type, _member) \
	RADREFLECT_MEMBER_NAMED(#_member, _type, _member)

#define RADREFLECT_STATICMEMBER_NAMED(_name, _type, _member) \
	PRIVATE_RADREFLECT_STATICMEMBER(_name, _type, _member)

#define RADREFLECT_STATICMEMBER(_type, _member) \
	RADREFLECT_STATICMEMBER_NAMED(#_member, _type, _member)

//////////////////////////////////////////////////////////////////////////////////////////
// Constructor reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_BEGIN_CONSTRUCTOR \
	PRIVATE_RADREFLECT_BEGIN_CONSTRUCTOR

#define RADREFLECT_END_CONSTRUCTOR \
	PRIVATE_RADREFLECT_END_CONSTRUCTOR

#define RADREFLECT_CONSTRUCTOR \
	RADREFLECT_BEGIN_CONSTRUCTOR \
	RADREFLECT_END_CONSTRUCTOR

//////////////////////////////////////////////////////////////////////////////////////////
// Method reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_BEGIN_CONSTMETHOD(_return) \
	PRIVATE_RADREFLECT_BEGIN_CONSTMETHOD(_return)

#define RADREFLECT_BEGIN_METHOD(_return) \
	PRIVATE_RADREFLECT_BEGIN_METHOD(_return)

#define RADREFLECT_BEGIN_STATICMETHOD(_return) \
	PRIVATE_RADREFLECT_BEGIN_STATICMETHOD(_return)

#define RADREFLECT_BEGIN_STATICMETHOD_CALLTYPE(_return, _calltype) \
	PRIVATE_RADREFLECT_BEGIN_STATICMETHOD_CALLTYPE(_return, _calltype)

#define RADREFLECT_END_METHOD_NAMED(_name, _method) \
	PRIVATE_RADREFLECT_END_METHOD(_name, _method)

#define RADREFLECT_END_METHOD(_method) \
	RADREFLECT_END_METHOD_NAMED(#_method, _method)

//////////////////////////////////////////////////////////////////////////////////////////
// Method reflection shortcut macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_CONSTMETHOD_NAMED(_name, _return, _method) \
	RADREFLECT_BEGIN_CONSTMETHOD(_return) \
	RADREFLECT_END_METHOD_NAMED(_name, _method)

#define RADREFLECT_CONSTMETHOD(_return, _method) \
	RADREFLECT_BEGIN_CONSTMETHOD(_return) \
	RADREFLECT_END_METHOD(_method)

#define RADREFLECT_METHOD_NAMED(_name, _return, _method) \
	RADREFLECT_BEGIN_METHOD(_return) \
	RADREFLECT_END_METHOD_NAMED(_name, _method)

#define RADREFLECT_METHOD(_return, _method) \
	RADREFLECT_BEGIN_METHOD(_return) \
	RADREFLECT_END_METHOD(_method)

#define RADREFLECT_STATICMETHOD_NAMED(_name, _return, _method) \
	RADREFLECT_BEGIN_STATICMETHOD(_return) \
	RADREFLECT_END_METHOD_NAMED(_name, _method)

#define RADREFLECT_STATICMETHOD(_return, _method) \
	RADREFLECT_BEGIN_STATICMETHOD(_return) \
	RADREFLECT_END_METHOD(_method)

//////////////////////////////////////////////////////////////////////////////////////////
// Argument reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_ARG(_name, _type) \
	PRIVATE_RADREFLECT_ARG(_name, _type)

#define RADREFLECT_DEFAULT_ARG(_name, _type, _default) \
	PRIVATE_RADREFLECT_DEFAULT_ARG(_name, _type, _default)

//////////////////////////////////////////////////////////////////////////////////////////
// Property reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_PROPERTY_GET_METHOD(_name, _method, _get_type)\
	RADREFLECT_CONSTMETHOD(_get_type, _method)\
		RADREFLECT_RTLI_ATTRIBUTE_PROPERTYGET(_name)

#define RADREFLECT_PROPERTY_SET_METHOD(_name, _method, _set_type)\
	RADREFLECT_BEGIN_METHOD(void)\
		RADREFLECT_ARG("value", _set_type)\
	RADREFLECT_END_METHOD(_method)\
		RADREFLECT_RTLI_ATTRIBUTE_PROPERTYSET(_name)

#define RADREFLECT_PROPERTY(_name, _get_type, _set_type) \
	RADREFLECT_PROPERTY_GET_METHOD(#_name, __get_##_name, _get_type)\
	RADREFLECT_PROPERTY_SET_METHOD(#_name, __set_##_name, _set_type)

#define RADREFLECT_PROPERTY_GET_NAMED(_name, _field, _get_type) \
	RADREFLECT_PROPERTY_GET_METHOD(_name, __get_##_field, _get_type)

#define RADREFLECT_PROPERTY_SET_NAMED(_name, _field, _set_type) \
	RADREFLECT_PROPERTY_SET_METHOD(_name, __set_##_field, _set_type)

#define RADREFLECT_PROPERTY_GET(_name, _get_type) \
	RADREFLECT_PROPERTY_GET_METHOD(#_name, __get_##_name, _get_type)

#define RADREFLECT_PROPERTY_SET(_name, _set_type) \
	RADREFLECT_PROPERTY_SET_METHOD(#_name, __set_##_name, _set_type)

//////////////////////////////////////////////////////////////////////////////////////////
// Enum reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_BEGIN_ENUM(_name, _type)\
	RADREFLECT_BEGIN(_name, _type)\
		RADREFLECT_CLASS_ATTRIBUTE(::reflect::Enum, ())

#define RADREFLECT_BEGIN_ENUM_NAMESPACE(_name, _namespace, _type)\
	RADREFLECT_BEGIN_NAMESPACE(_name, _namespace, _type)\
		RADREFLECT_CLASS_ATTRIBUTE(::reflect::Enum, ())

#define RADREFLECT_ENUM_VALUE(_value)\
	RADREFLECT_CLASS_ATTRIBUTE_NAMED(#_value, ::reflect::EnumValue, (_value))

#define RADREFLECT_NAMESPACED_ENUM_VALUE(_namespace, _value)\
	RADREFLECT_CLASS_ATTRIBUTE_NAMED(#_value, ::reflect::EnumValue, (_namespace::_value))

#define RADREFLECT_ENUM_VALUE_NAMED(_name, _value)\
	RADREFLECT_CLASS_ATTRIBUTE_NAMED(_name, ::reflect::EnumValue, (_value))

#define RADREFLECT_ENUM_FLAGS_ATTRIBUTE\
	RADREFLECT_CLASS_ATTRIBUTE(::reflect::EnumFlags, ())

//////////////////////////////////////////////////////////////////////////////////////////
// Serialization reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_SERIALIZE_STREAM(_reader, _writer) \
	PRIVATE_RADREFLECT_SERIALIZE_STREAM(_reader, _writer)

#define RADREFLECT_SERIALIZE_XMLDOM(_reader, _writer) \
	PRIVATE_RADREFLECT_SERIALIZE_XMLDOM(_reader, _writer)

#define RADREFLECT_SERIALIZE_STREAM_STANDARD(_type) \
	PRIVATE_RADREFLECT_SERIALIZE_STREAM_STANDARD(_type)

#define RADREFLECT_SERIALIZE_XMLDOM_STANDARD(_type) \
	PRIVATE_RADREFLECT_SERIALIZE_XMLDOM_STANDARD(_type)

#include "../PopPack.h"
