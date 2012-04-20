// ReflectPrivateMapDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Reflect.h"
#include "../../Meta/ICE/And.h"
#include "../../Meta/ICE/Or.h"
#include "../../Meta/ICE/Not.h"
#include "../../Meta/ICE/PredicatedAdd.h"
#include "../../Meta/TypeTraits/QualifierTraits.h"
#include "../../Meta/TypeTraits/TypeOf.h"
#include "../../Meta/TypeTraits/FunctionTraits.h"
#include "../../Meta/TypeTraits/ReferenceTraits.h"
#include "../../Meta/TypeTraits/MethodTraits.h"
#include "../../Meta/TypeTraits/TypeSelect.h"
#include "../../Tuple/MakeTuple.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Private type and class reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_BEGIN_HELPER(_type) \
	PRIVATE_RADREFLECT_PRIVATE_NAMESPACE_BEGIN \
	template <> \
	struct TypeTraits<_type> : \
	public ::reflect::details::TypeTraitsBase<_type> \
	{ \
		typedef TypeTraits<_type> SelfType;

#define PRIVATE_RADREFLECT_BEGIN_DESCRIPTOR(_name) \
		static ::reflect::details::CLASSDESCRIPTOR Descriptor() \
		{ \
			::reflect::details::CLASSDESCRIPTOR descriptor = ::reflect::details::ClassDescriptor< \
				ClassDescriptorTraits<TYPE> \
			>(::reflect::details::Name(_name, L##_name))

#define PRIVATE_RADREFLECT_BEGIN_NAMESPACE(_name, _namespace, _type) \
	PRIVATE_RADREFLECT_BEGIN_HELPER(_namespace::_type) \
		PRIVATE_RADREFLECT_BEGIN_DESCRIPTOR(_name)

#define PRIVATE_RADREFLECT_BEGIN(_name, _type) \
	PRIVATE_RADREFLECT_BEGIN_HELPER(_type) \
		PRIVATE_RADREFLECT_BEGIN_DESCRIPTOR(_name)

#define PRIVATE_RADREFLECT_BEGIN_CLASS_NAMESPACE(_name, _namespace, _type) \
	PRIVATE_RADREFLECT_BEGIN_HELPER(_namespace::_type) \
		static void Destruct(const Class *type, void *location) { reinterpret_cast<_namespace::_type *>(location)->~_type(); } \
		PRIVATE_RADREFLECT_BEGIN_DESCRIPTOR(_name)

#define PRIVATE_RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, _destructor) \
	PRIVATE_RADREFLECT_BEGIN_HELPER(_namespace::_type) \
		static void Destruct(const Class *type, void *location) { reinterpret_cast<_namespace::_type *>(location)->~_destructor(); } \
		PRIVATE_RADREFLECT_BEGIN_DESCRIPTOR(_name)

#define PRIVATE_RADREFLECT_BEGIN_CLASS(_name, _type) \
	PRIVATE_RADREFLECT_BEGIN_HELPER(_type) \
		static void Destruct(const Class *type, void *location) { reinterpret_cast<_type *>(location)->~_type(); } \
		PRIVATE_RADREFLECT_BEGIN_DESCRIPTOR(_name)

#define PRIVATE_RADREFLECT_BEGIN_CLASS_DESTRUCTOR(_name, _type, _destructor) \
	PRIVATE_RADREFLECT_BEGIN_HELPER(_type) \
		static void Destruct(const Class *type, void *location) { reinterpret_cast<_type *>(location)->~_destructor(); } \
		PRIVATE_RADREFLECT_BEGIN_DESCRIPTOR(_name)

// MPS 
//
// This is setup like this for thread safety.  Instead of just 
// constructing a static Class and returning it's address, we need to 
// lock to make sure one thread doesn't access it while another is still
// initializing (constructing) the Class.  This will only acquire the
// lock when the Class is not yet constructed, when it is constructed
// the result pointer is no longer NULL.
//
// There is a single lock for all types since critical sections may be
// a limited resource on some platforms.  This shouldn't be a problem
// since the construction only occurs once.  The only potential problem
// may be deadlock, but the client can get around that by accessing
// the Class info for a type causing deadlock in an initialization 
// function (before threads are created that are causing deadlock).

#define PRIVATE_RADREFLECT_END_NAMESPACE(_api, _namespace, _type) \
	PRIVATE_RADREFLECT_END(_api, _namespace::_type)

#define PRIVATE_RADREFLECT_END(_api, _type) \
			.Descriptor(); \
			descriptor.dtor = Destruct; \
			return descriptor; \
		} \
		static const ::reflect::Class *Type() \
		{ \
			static const ::reflect::Class * volatile result = NULL; \
			if (NULL == result) \
			{ \
				::reflect::details::TypeLock lock; \
				if (NULL == result) \
				{ \
					static const ::reflect::details::ClassType<SelfType> type; \
					result = &type; \
				} \
			} \
			RAD_ASSERT(NULL != result); \
			return (const ::reflect::Class *)result; \
		} \
		static const ::reflect::Class *ConstType() \
		{ \
			static const ::reflect::Class * volatile result = NULL; \
			if (NULL == result) \
			{ \
				::reflect::details::TypeLock lock; \
				if (NULL == result) \
				{ \
					static const ::reflect::details::ClassType<SelfType, true> type; \
					result = &type; \
				} \
			} \
			RAD_ASSERT(NULL != result); \
			return (const ::reflect::Class *)result; \
		} \
	private: \
		static const ::reflect::Class *s_class; \
	}; \
	const ::reflect::Class *TypeTraits<_type>::s_class = TypeTraits<_type>::Type(); \
	PRIVATE_RADREFLECT_PRIVATE_NAMESPACE_END \
	PRIVATE_RADREFLECT_NAMESPACE_BEGIN \
	PRIVATE_RADREFLECT_TYPE_SIG(_api, _type) \
	{ \
		return ::reflect::details::TypeTraits<_type>::Type(); \
	} \
	PRIVATE_RADREFLECT_TYPE_SIG(_api, _type const) \
	{ \
		return ::reflect::details::TypeTraits<_type>::ConstType(); \
	} \
	PRIVATE_RADREFLECT_NAMESPACE_END

#define PRIVATE_RADREFLECT_TYPE_NULL(_api, _type) \
	PRIVATE_RADREFLECT_NAMESPACE_BEGIN \
	PRIVATE_RADREFLECT_TYPE_SIG(_api, _type) \
	{ \
		return NULL; \
	} \
	PRIVATE_RADREFLECT_TYPE_SIG(_api, _type const) \
	{ \
		return NULL; \
	} \
	PRIVATE_RADREFLECT_NAMESPACE_END

//////////////////////////////////////////////////////////////////////////////////////////
// Private attribute reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_CLASS_ATTRIBUTE(_name, _type, _args) \
	.BindClassAttribute( \
		::reflect::details::AttributeTraits<_type>::CreateInitializer( \
			::reflect::details::Name(_name, L##_name), \
			_type _args \
		) \
	)

#define PRIVATE_RADREFLECT_ATTRIBUTE(_name, _type, _args) \
	.BindAttribute( \
		::reflect::details::AttributeTraits<_type>::CreateInitializer( \
			::reflect::details::Name(_name, L##_name), \
			_type _args \
		) \
	)

//////////////////////////////////////////////////////////////////////////////////////////
// Private supertype reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_SUPER(_name, _type) \
	.BindSuper( \
		::reflect::details::SuperTraits<TYPE, _type>::CreateInitializer( \
			::reflect::details::Name(_name, L##_name), \
			RAD_SUPER_OFFSET(TYPE, _type) \
		) \
	)

//////////////////////////////////////////////////////////////////////////////////////////
// Private member reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_MEMBER(_name, _type, _member) \
	.BindMember( \
		::reflect::details::MemberTraits<TYPE, _type>::CreateInitializer( \
			::reflect::details::Name(_name, L##_name), \
			RAD_MEMBER_OFFSET(TYPE, _member) \
		) \
	)

#define PRIVATE_RADREFLECT_STATICMEMBER(_name, _type, _member) \
	.BindMember( \
		::reflect::details::MemberTraits<TYPE, _type, true>::CreateInitializer( \
			::reflect::details::Name(_name, L##_name), \
			&TYPE::_member \
		) \
	)

//////////////////////////////////////////////////////////////////////////////////////////
// Private constructor reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_BEGIN_CONSTRUCTOR \
	.BeginConstructor( \
		::reflect::details::ConstructorTraits< \
			void (), TYPE \
		>::CreateInitializer() \
	)

#define PRIVATE_RADREFLECT_END_CONSTRUCTOR \
	.EndConstructor()

//////////////////////////////////////////////////////////////////////////////////////////
// Private argument reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_ARG(_name, _type) \
	.BindArgument( \
		::reflect::details::ArgumentTraits<_type>::CreateInitializer( \
			::reflect::details::Name(_name, L##_name) \
		) \
	)

#define PRIVATE_RADREFLECT_DEFAULT_ARG(_name, _type, _default) \
	.BindArgument( \
		::reflect::details::ArgumentTraits<_type>::CreateDefaultInitializer( \
			::reflect::details::Name(_name, L##_name), \
			_default \
		) \
	)

//////////////////////////////////////////////////////////////////////////////////////////
// Private destructor reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_DESTRUCTOR \
	.BindDestructor( \
		::reflect::details::DestructorTraits< \
			TYPE \
		>::CreateInitializer() \
	)

//////////////////////////////////////////////////////////////////////////////////////////
// Private method reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_BEGIN_CONSTMETHOD(_return) \
	.BeginMethod( \
		::reflect::details::MethodTraits< \
			_return (), TYPE const \
		>::CreateInitializer() \
	)
				
#define PRIVATE_RADREFLECT_BEGIN_METHOD(_return) \
	.BeginMethod( \
		::reflect::details::MethodTraits< \
			_return (), TYPE \
		>::CreateInitializer() \
	)

#define PRIVATE_RADREFLECT_BEGIN_STATICMETHOD(_return) \
	.BeginMethod( \
		::reflect::details::MethodTraits< \
			_return (), TYPE, true \
		>::CreateInitializer() \
	)

#define PRIVATE_RADREFLECT_BEGIN_STATICMETHOD_CALLTYPE(_return, _calltype) \
	.BeginMethod( \
		::reflect::details::MethodTraits< \
			_return _calltype (), TYPE, true \
		>::CreateInitializer() \
	)

#define PRIVATE_RADREFLECT_END_METHOD(_name, _method) \
	.EndMethod( \
		::reflect::details::Name(_name, L##_name), \
		&TYPE::_method \
	)

//////////////////////////////////////////////////////////////////////////////////////////
// Private serialization reflection macros
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_SERIALIZE_STREAM(_reader, _writer) \
		static stream::InputStream &ReadStream( \
			const Class                *type, \
			::stream::InputStream &is, \
			void                       *object \
		) \
		{ \
			return _reader(type, is, object); \
		} \
		static stream::OutputStream &WriteStream( \
			const Class                 *type, \
			::stream::OutputStream &os, \
			const void                  *object \
		) \
		{ \
			return _writer(type, os, object); \
		}

#define PRIVATE_RADREFLECT_SERIALIZE_XMLDOM(_reader, _writer) \
		static bool ReadXmlDom( \
			const Class                     *type, \
			const ::xml::dom::HElement &element, \
			void                            *object \
		) \
		{ \
			return _reader(type, element, object); \
		} \
		static bool WriteXmlDom( \
			const Class               *type, \
			::xml::dom::HElement &element, \
			const void                *object \
		) \
		{ \
			return _writer(type, element, object); \
		}

#define PRIVATE_RADREFLECT_SERIALIZE_STREAM_STANDARD(_type) \
	PRIVATE_RADREFLECT_SERIALIZE_STREAM( \
		::reflect::details::ReadStream<_type>, \
		::reflect::details::WriteStream<_type> \
	)

#define PRIVATE_RADREFLECT_SERIALIZE_XMLDOM_STANDARD(_type) \
	PRIVATE_RADREFLECT_SERIALIZE_XMLDOM( \
		::reflect::details::ReadXmlDom<_type>, \
		::reflect::details::WriteXmlDom<_type> \
	)

//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace reflect::details
//////////////////////////////////////////////////////////////////////////////////////////


namespace reflect {
namespace details {

template <typename TTraits, bool IsConst = false>
class ClassType;

template <typename TTraits> class ClassDescriptor;

template <typename TAttributeTraits> class AttributeInitializer;
template <typename T> struct AttributeTraits;

template <
	typename TSuperTraits, 
	typename TAttributeTuple = ::TupleTraits<>::Type
> class SuperInitializer;

template <typename TOwner, typename T> struct SuperTraits;

template <
	typename TMemberTraits,
	typename TAttributeTuple = ::TupleTraits<>::Type
> class MemberInitializer;

template <typename TOwner, typename T, bool IsStatic> struct MemberTraits;

template <
	typename TConstructorTraits,
	typename TAttributeTuple = ::TupleTraits<>::Type,
	typename TArgTuple       = ::TupleTraits<>::Type
> class ConstructorInitializer;

template <typename TFunction, typename TOwner> struct ConstructorTraits;

template <
	typename TMethodTraits,
	typename TAttributeTuple = ::TupleTraits<>::Type,
	typename TArgTuple       = ::TupleTraits<>::Type
> class MethodInitializer;

template <typename TFunction, typename TOwner, bool IsStatic> struct MethodTraits;

template <
	typename TArgTraits,
	typename TAttributeTuple = ::TupleTraits<>::Type
> class ArgumentInitializer;

template <
	typename TArgTraits,
	typename TAttributeTuple = ::TupleTraits<>::Type
> class DefaultArgumentInitializer;

template <typename T> struct ArgumentTraits;

template <typename TInitializer, typename TBind> struct BindRequiredArgumentTraits;
template <typename TInitializer, typename TBind> struct BindDefaultArgumentTraits;
template <typename TInitializer, typename TBind> struct BindArgumentTraits;
template <typename TInitializer, typename TBind> struct BindAttributeTraits;
template <typename TInitializer, typename TBind> struct BindArgumentAttributeTraits;

} // details
} // reflect

