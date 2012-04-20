// ReflectPrivateClass.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ReflectPrivateMapDef.h"
#include "ReflectPrivateAttribute.h"
#include "ReflectPrivateSuper.h"
#include "ReflectPrivateMember.h"
#include "ReflectPrivateConstructor.h"
#include "ReflectPrivateMethod.h"
#include "../../PushPack.h"


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MEMBERS
//////////////////////////////////////////////////////////////////////////////////////////

struct MEMBERS
{
	Class::MEMBERARRAY         members;
	Class::STATICMEMBERARRAY   staticMembers;
	Class::STATICCONSTANTARRAY staticConstants;

	MEMBERS();
	MEMBERS(
		const Class::MEMBERARRAY         &m,
		const Class::STATICMEMBERARRAY   &s,
		const Class::STATICCONSTANTARRAY &c
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::METHODS
//////////////////////////////////////////////////////////////////////////////////////////

struct METHODS
{
	Class::MUTABLEMETHODARRAY methods;
	Class::CONSTMETHODARRAY   constMethods;
	Class::STATICMETHODARRAY  staticMethods;

	METHODS();
	METHODS(
		const Class::MUTABLEMETHODARRAY &m,
		const Class::CONSTMETHODARRAY   &c,
		const Class::STATICMETHODARRAY  &s
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::CLASSDESCRIPTOR
//////////////////////////////////////////////////////////////////////////////////////////

struct CLASSDESCRIPTOR
{
	ATTRIBUTEARRAY          attributes;
	NAME                    name;
	AddrSize                size;
	AddrSize                alignment;
	Class::SUPERARRAY       supers;
	MEMBERS                 members;
	Class::CONSTRUCTORARRAY constructors;
	METHODS                 methods;
	Class::DTOR             dtor;
	
	CLASSDESCRIPTOR();
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::CLASSDESCRIPTOR_STATE
//////////////////////////////////////////////////////////////////////////////////////////

struct CLASSDESCRIPTOR_STATE
{
	enum E
	{
		CLASS,
		SUPER,
		MEMBER,
		CONSTRUCTOR_BEGIN,
		CONSTRUCTOR_ARGUMENT,
		CONSTRUCTOR,
		METHOD_BEGIN,
		METHOD_ARGUMENT,
		METHOD
	};
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptorTraits<T, ...>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T,
	typename TAttributeTuple   = ::TupleTraits<>::Type,
	typename TSuperTuple       = ::TupleTraits<>::Type,
	typename TMemberTuple      = ::TupleTraits<>::Type,
	typename TConstructorTuple = ::TupleTraits<>::Type,
	typename TMethodTuple      = ::TupleTraits<>::Type,
	CLASSDESCRIPTOR_STATE::E State = CLASSDESCRIPTOR_STATE::CLASS
>
struct ClassDescriptorTraits
{
	typedef T                 ClassType;
	typedef TAttributeTuple   AttributeTupleType;
	typedef TSuperTuple       SuperTupleType;
	typedef TMemberTuple      MemberTupleType;
	typedef TConstructorTuple ConstructorTupleType;
	typedef TMethodTuple      MethodTupleType;

	static const CLASSDESCRIPTOR_STATE::E STATE = State;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindTraits<TTraits>
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(_x)\
	typedef typename _x::ClassType            ClassType;\
	typedef typename _x::AttributeTupleType   AttributeTupleType;\
	typedef typename _x::SuperTupleType       SuperTupleType;\
	typedef typename _x::MemberTupleType      MemberTupleType;\
	typedef typename _x::ConstructorTupleType ConstructorTupleType;\
	typedef typename _x::MethodTupleType      MethodTupleType;\
	static const CLASSDESCRIPTOR_STATE::E STATE = _x::STATE

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindSuperTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindSuperTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(TTraits);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			typename SuperTupleType::template AddTraits<TBind>::Type,
			MemberTupleType,
			ConstructorTupleType,
			MethodTupleType,
			CLASSDESCRIPTOR_STATE::SUPER
		>
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindMemberTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindMemberTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(TTraits);

	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			typename MemberTupleType::template AddTraits<TBind>::Type,
			ConstructorTupleType,
			MethodTupleType,
			CLASSDESCRIPTOR_STATE::MEMBER
		>
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BeginConstructorTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BeginConstructorTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(TTraits);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			MemberTupleType,
			typename ConstructorTupleType::template AddTraits<TBind>::Type,
			MethodTupleType,
			CLASSDESCRIPTOR_STATE::CONSTRUCTOR_BEGIN
		>
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::EndConstructorTraits<TTraits>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
struct EndConstructorTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(TTraits);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			MemberTupleType,
			ConstructorTupleType,
			MethodTupleType,
			CLASSDESCRIPTOR_STATE::CONSTRUCTOR
		>
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BeginMethodTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BeginMethodTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(TTraits);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			MemberTupleType,
			ConstructorTupleType,
			typename MethodTupleType::template AddTraits<TBind>::Type,
			CLASSDESCRIPTOR_STATE::METHOD_BEGIN
		>
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::EndMethodTraits<TTraits>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, bool EmptyTuple>
struct EndMethodTraitsBase
{
	typedef void         *MethodPtrType;
	typedef void          ResultType;
};

template <typename TTraits>
struct EndMethodTraitsBase<TTraits, false>
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(TTraits);
	
	typedef typename MethodTupleType::HeadType      MethodInitializerType;
	typedef typename MethodInitializerType::PtrType MethodPtrType;

	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			MemberTupleType,
			ConstructorTupleType,
			MethodTupleType,
			CLASSDESCRIPTOR_STATE::METHOD
		>
	> ResultType;
};
//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::EndMethodTraits<TTraits>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
struct EndMethodTraits :
public EndMethodTraitsBase<
	TTraits,
	::meta::SameTypes<
		typename TTraits::MethodTupleType,
		::TupleTraits<>::Type
	>::VALUE
>
{
	typedef EndMethodTraitsBase<
	TTraits,
		::meta::SameTypes<
			typename TTraits::MethodTupleType,
			::TupleTraits<>::Type
		>::VALUE
	> SuperType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Helper macro for specializing argument and attribute bind traits
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_SPECIALIZE_BINDTRAITS(_traits, _base, _state) \
template < \
	typename T, \
	typename AttributeTupleType, \
	typename TSuperTuple, \
	typename TMemberTuple, \
	typename TConstructorTuple, \
	typename TMethodTuple, \
	typename TBind \
> \
struct _traits< \
	ClassDescriptorTraits< \
		T, \
		AttributeTupleType, \
		TSuperTuple, \
		TMemberTuple, \
		TConstructorTuple, \
		TMethodTuple, \
		_state \
	>, \
	TBind \
> : \
public _base< \
	ClassDescriptorTraits< \
		T, \
		AttributeTupleType, \
		TSuperTuple, \
		TMemberTuple, \
		TConstructorTuple, \
		TMethodTuple, \
		_state \
	>, \
	TBind \
> \
{ \
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindConstructorArgumentTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindConstructorArgumentTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(TTraits);
	
	typedef typename ConstructorTupleType::HeadType               ConstructorInitializerType;
	typedef typename ConstructorTupleType::TailType               TailType;
	typedef BindArgumentTraits<ConstructorInitializerType, TBind> BindType;

	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			MemberTupleType,
			typename TailType::template AddTraits<typename BindType::ResultType>::Type,
			MethodTupleType,			
			CLASSDESCRIPTOR_STATE::CONSTRUCTOR_ARGUMENT
		>
	> ResultType;

	static ResultType BindArgument(
		const ClassDescriptor<TTraits> &descriptor,
		const TBind                    &argumentInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindMethodArgumentTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindMethodArgumentTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(TTraits);
	typedef typename MethodTupleType::HeadType               MethodInitializerType;
	typedef typename MethodTupleType::TailType               TailType;
	typedef BindArgumentTraits<MethodInitializerType, TBind> BindType;

	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			MemberTupleType,
			ConstructorTupleType,
			typename TailType::template AddTraits<typename BindType::ResultType>::Type,
			CLASSDESCRIPTOR_STATE::METHOD_ARGUMENT
		>
	> ResultType;

	static ResultType BindArgument(
		const ClassDescriptor<TTraits> &descriptor,
		const TBind                    &argumentInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindArgumentTraits<ClassDescriptorTraits<...>, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_SPECIALIZE_BINDARGUMENTTRAITS(_bind, _state) \
	PRIVATE_RADREFLECT_SPECIALIZE_BINDTRAITS(BindArgumentTraits, _bind, _state)

PRIVATE_RADREFLECT_SPECIALIZE_BINDARGUMENTTRAITS(
	BindConstructorArgumentTraits,
	CLASSDESCRIPTOR_STATE::CONSTRUCTOR_BEGIN
)

PRIVATE_RADREFLECT_SPECIALIZE_BINDARGUMENTTRAITS(
	BindConstructorArgumentTraits,
	CLASSDESCRIPTOR_STATE::CONSTRUCTOR_ARGUMENT
)

PRIVATE_RADREFLECT_SPECIALIZE_BINDARGUMENTTRAITS(
	BindMethodArgumentTraits,
	CLASSDESCRIPTOR_STATE::METHOD_BEGIN
)

PRIVATE_RADREFLECT_SPECIALIZE_BINDARGUMENTTRAITS(
	BindMethodArgumentTraits,
	CLASSDESCRIPTOR_STATE::METHOD_ARGUMENT
)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindAttributeTraitsBase<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_PRIVATECLASS_BINDATTRIBUTETRAITS(_traits, _bind, _tuple)\
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(_traits);\
	typedef _tuple __xxx_tuple;\
	typedef typename __xxx_tuple::HeadType  InitType;\
	typedef typename __xxx_tuple::TailType  TailType;\
	typedef typename BindAttributeTraits<InitType, _bind>::ResultType BindType

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindArgumentAttributeTraitsBase<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_PRIVATECLASS_BINDARGUMENTATTRIBUTETRAITS(_traits, _bind, _tuple)\
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(_traits);\
	typedef _tuple __xxx_tuple;\
	typedef typename __xxx_tuple::HeadType  InitType;\
	typedef typename __xxx_tuple::TailType  TailType;\
	typedef typename BindArgumentAttributeTraits<InitType, _bind>::ResultType BindType

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindClassAttributeTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindClassAttributeTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(TTraits);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			typename AttributeTupleType::template AddTraits<TBind>::Type,
			SuperTupleType,
			MemberTupleType,
			ConstructorTupleType,
			MethodTupleType,
			STATE
		>
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindSuperAttributeTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindSuperAttributeTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDATTRIBUTETRAITS(TTraits, TBind, typename TTraits::SuperTupleType);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			typename TailType::template AddTraits<BindType>::Type,
			MemberTupleType,
			ConstructorTupleType,
			MethodTupleType,
			STATE
		>
	> ResultType;

	static ResultType BindAttribute(
		const ClassDescriptor<TTraits> &descriptor,
		const TBind                    &attributeInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindMemberAttributeTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindMemberAttributeTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDATTRIBUTETRAITS(TTraits, TBind, typename TTraits::MemberTupleType);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			typename TailType::template AddTraits<BindType>::Type,
			ConstructorTupleType,
			MethodTupleType,
			STATE
		>
	> ResultType;

	static ResultType BindAttribute(
		const ClassDescriptor<TTraits> &descriptor,
		const TBind                    &attributeInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindConstructorAttributeTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindConstructorAttributeTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDATTRIBUTETRAITS(TTraits, TBind, typename TTraits::ConstructorTupleType);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			MemberTupleType,
			typename TailType::template AddTraits<BindType>::Type,
			MethodTupleType,
			STATE
		>
	> ResultType;

	static ResultType BindAttribute(
		const ClassDescriptor<TTraits> &descriptor,
		const TBind                    &attributeInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindConstructorArgumentAttributeTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindConstructorArgumentAttributeTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDARGUMENTATTRIBUTETRAITS(TTraits, TBind, typename TTraits::ConstructorTupleType);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			MemberTupleType,
			typename TailType::template AddTraits<BindType>::Type,
			MethodTupleType,
			STATE
		>
	> ResultType;

	static ResultType BindAttribute(
		const ClassDescriptor<TTraits> &descriptor,
		const TBind                    &attributeInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindConstructorAttributeTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindMethodAttributeTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDATTRIBUTETRAITS(TTraits, TBind, typename TTraits::MethodTupleType);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			MemberTupleType,
			ConstructorTupleType,
			typename TailType::template AddTraits<BindType>::Type,
			STATE
		>
	> ResultType;

	static ResultType BindAttribute(
		const ClassDescriptor<TTraits> &descriptor,
		const TBind                    &attributeInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindMethodArgumentAttributeTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
struct BindMethodArgumentAttributeTraits
{
	PRIVATE_RADREFLECT_PRIVATECLASS_BINDARGUMENTATTRIBUTETRAITS(TTraits, TBind, typename TTraits::MethodTupleType);
	
	typedef ClassDescriptor<
		ClassDescriptorTraits<
			ClassType,
			AttributeTupleType,
			SuperTupleType,
			MemberTupleType,
			ConstructorTupleType,
			typename TailType::template AddTraits<BindType>::Type,
			STATE
		>
	> ResultType;

	static ResultType BindAttribute(
		const ClassDescriptor<TTraits> &descriptor,
		const TBind                    &attributeInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindAttributeTraits<TTraits, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_SPECIALIZE_BINDATTRIBUTETRAITS(_bind, _state) \
	PRIVATE_RADREFLECT_SPECIALIZE_BINDTRAITS(BindAttributeTraits, _bind, _state)

PRIVATE_RADREFLECT_SPECIALIZE_BINDATTRIBUTETRAITS(
	BindSuperAttributeTraits,
	CLASSDESCRIPTOR_STATE::SUPER
)

PRIVATE_RADREFLECT_SPECIALIZE_BINDATTRIBUTETRAITS(
	BindMemberAttributeTraits,
	CLASSDESCRIPTOR_STATE::MEMBER
)

PRIVATE_RADREFLECT_SPECIALIZE_BINDATTRIBUTETRAITS(
	BindConstructorAttributeTraits,
	CLASSDESCRIPTOR_STATE::CONSTRUCTOR
)

PRIVATE_RADREFLECT_SPECIALIZE_BINDATTRIBUTETRAITS(
	BindConstructorArgumentAttributeTraits,
	CLASSDESCRIPTOR_STATE::CONSTRUCTOR_ARGUMENT
)

PRIVATE_RADREFLECT_SPECIALIZE_BINDATTRIBUTETRAITS(
	BindMethodAttributeTraits,
	CLASSDESCRIPTOR_STATE::METHOD
)

PRIVATE_RADREFLECT_SPECIALIZE_BINDATTRIBUTETRAITS(
	BindMethodArgumentAttributeTraits,
	CLASSDESCRIPTOR_STATE::METHOD_ARGUMENT
)

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
class ClassDescriptor
{
public:

	PRIVATE_RADREFLECT_PRIVATECLASS_BINDTRAITS(TTraits);
	
	// Constructors

	ClassDescriptor(const NAME &name);

	ClassDescriptor(
		const AttributeTupleType   &attributes,
		const NAME                 &name,		
		const SuperTupleType       &supers,
		const MemberTupleType      &members,
		const ConstructorTupleType &constructors,
		const MethodTupleType      &methods
	);

	// Get runtime descriptor

	CLASSDESCRIPTOR Descriptor() const;

	// Bind superclass

	template <typename T>
	typename BindSuperTraits<TTraits, T>::ResultType BindSuper(const T &superInitializer) const;

	// Bind member

	template <typename T>
	typename BindMemberTraits<TTraits, T>::ResultType BindMember(const T &memberInitializer) const;

	// Bind constructor

	template <typename T>
	typename BeginConstructorTraits<TTraits, T>::ResultType BeginConstructor(const T &constructorInitializer) const;

	typename EndConstructorTraits<TTraits>::ResultType EndConstructor() const;

	// Bind method

	template <typename T>
	typename BeginMethodTraits<TTraits, T>::ResultType BeginMethod(const T &methodInitializer) const;

	typename EndMethodTraits<TTraits>::ResultType EndMethod(
		const NAME                                       &name,
		typename EndMethodTraits<TTraits>::MethodPtrType method
	) const;

	// Bind argument

	template <typename T>
	typename BindArgumentTraits<TTraits, T>::ResultType BindArgument(const T &argumentInitializer) const;

	template <typename T>
	typename BindConstructorArgumentTraits<TTraits, T>::ResultType BindConstructorArgument(const T &argumentInitializer) const;

	template <typename T>
	typename BindMethodArgumentTraits<TTraits, T>::ResultType BindMethodArgument(const T &argumentInitializer) const;

	// Bind attribute

	template <typename T>
	typename BindAttributeTraits<TTraits, T>::ResultType BindAttribute(const T &attributeInitializer) const;

	template <typename T>
	typename BindClassAttributeTraits<TTraits, T>::ResultType BindClassAttribute(const T &attributeInitializer) const;

	template <typename T>
	typename BindSuperAttributeTraits<TTraits, T>::ResultType BindSuperAttribute(const T&attributeInitializer) const;

	template <typename T>
	typename BindMemberAttributeTraits<TTraits, T>::ResultType BindMemberAttribute(const T&attributeInitializer) const;

	template <typename T>
	typename BindConstructorAttributeTraits<TTraits, T>::ResultType BindConstructorAttribute(const T&attributeInitializer) const;

	template <typename T>
	typename BindConstructorArgumentAttributeTraits<TTraits, T>::ResultType BindConstructorArgumentAttribute(const T&attributeInitializer) const;

	template <typename T>
	typename BindMethodAttributeTraits<TTraits, T>::ResultType BindMethodAttribute(const T&attributeInitializer) const;

	template <typename T>
	typename BindMethodArgumentAttributeTraits<TTraits, T>::ResultType BindMethodArgumentAttribute(const T&attributeInitializer) const;

private:

	const Class::SUPER **InitializeSupers() const;
	const MEMBERS InitializeMembers() const;
	const Class::CONSTRUCTOR **InitializeConstructors() const;
	const METHODS InitializeMethods() const;

	AttributeTupleType   m_attributes;
	NAME                 m_name;
	SuperTupleType       m_supers;
	MemberTupleType      m_members;
	ConstructorTupleType m_constructors;
	MethodTupleType      m_methods;

};

} // details
} // reflect


#include "../../PopPack.h"
#include "ReflectPrivateClass.inl"
