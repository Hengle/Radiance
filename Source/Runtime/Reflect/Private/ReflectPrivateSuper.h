// ReflectPrivateSuper.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ReflectPrivateMapDef.h"
#include "ReflectPrivateAttribute.h"


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Super<TOwner, TValue>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TOwner, typename TValue>
class Super :
public Class::SUPER
{
public:

	// reflect::Class::SUPER implementation

	virtual const Class *OwnerType() const;
	virtual const Class *Type() const;

private:

	Super(
		const ATTRIBUTEARRAY &attributes,
		const NAME           &name,
		AddrSize             offset
	);

	template <typename TSuperTraits, typename TAttributeTuple>
	friend class SuperInitializer;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindAttributeTraits<SuperInitializer<TSuperTraits, TAttributeTuple>,	TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TSuperTraits,
	typename TAttributeTuple,
	typename TBind
>
struct BindAttributeTraits<
	SuperInitializer<TSuperTraits, TAttributeTuple>,
	TBind
>
{
	typedef SuperInitializer<TSuperTraits, TAttributeTuple> InitType;
	typedef typename InitType::TraitsType                   TraitsType;
	typedef typename InitType::AttributeTupleType           AttributeTupleType;

	typedef SuperInitializer<
		TraitsType,
		typename AttributeTupleType::template AddTraits<TBind>::Type
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::SuperInitializer<TSuperTraits>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TSuperTraits, typename TAttributeTuple>
class SuperInitializer
{
public:

	typedef SuperInitializer<TSuperTraits, TAttributeTuple> SelfType;
	typedef TSuperTraits                                    TraitsType;
	typedef TAttributeTuple                                 AttributeTupleType;
	typedef typename TraitsType::OwnerType                  OwnerType;
	typedef typename TraitsType::ValueType                  ValueType;	
	typedef Super<OwnerType, ValueType>                     SuperType;

	SuperInitializer(
		const AttributeTupleType &attributes,
		const NAME               &name,
		AddrSize                 offset
	);

	template <int I>
	const Class::SUPER *Instance() const;

	template <typename T>
	typename BindAttributeTraits<SelfType, T>::ResultType BindAttribute(const T &attributeInitializer) const;

private:

	AttributeTupleType m_attributes;
	NAME               m_name;
	AddrSize           m_offset;	

	friend struct SuperTraits<OwnerType, ValueType>;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::SuperTraits<TOwner, T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TOwner,	typename T>
struct SuperTraits
{
	typedef SuperTraits <TOwner, T> SelfType;
	typedef TOwner                  OwnerType;
	typedef T                       ValueType;

	static SuperInitializer<SelfType> CreateInitializer(
		const NAME &name,
		AddrSize   offset
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseSupers<TSuperTuple, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TSuperTuple, int I>
struct RecurseSupers
{
	static void Initialize(const Class::SUPER **supers, const TSuperTuple &s);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseSupers<TSuperTuple, -1>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TSuperTuple>
struct RecurseSupers<TSuperTuple, -1>
{
	static void Initialize(const Class::SUPER **supers, const TSuperTuple &s);
};

} // details
} // reflect


#include "ReflectPrivateSuper.inl"
