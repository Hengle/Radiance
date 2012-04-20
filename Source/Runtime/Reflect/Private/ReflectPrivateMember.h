// ReflectPrivateMember.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ReflectPrivateMapDef.h"
#include "ReflectPrivateAttribute.h"


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Member<TBase, TOwner, TValue, TLocation>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TBase, typename TOwner, typename TValue, typename TLocation>
class Member :
public TBase
{
public:

	// reflect::IOwned implementation

	virtual const Class *OwnerType() const;

	// reflect::ITyped implementation

	virtual const Class *Type() const;

private:

	Member(
		const ATTRIBUTEARRAY &attributes,
		const NAME           &name,
		TLocation            location
	);

	template <typename TMemberTraits, typename TAttributeTuple>
	friend class MemberInitializer;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindAttributeTraits<MemberInitializer<TMemberTraits, TAttributeTuple>,	TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TMemberTraits,
	typename TAttributeTuple,
	typename TBind
>
struct BindAttributeTraits<
	MemberInitializer<TMemberTraits, TAttributeTuple>,
	TBind
>
{
	typedef MemberInitializer<TMemberTraits, TAttributeTuple> InitType;
	typedef typename InitType::TraitsType                     TraitsType;
	typedef typename InitType::AttributeTupleType             AttributeTupleType;

	typedef MemberInitializer<
		TraitsType,
		typename AttributeTupleType::template AddTraits<TBind>::Type
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MemberInitializer<TMemberTraits, TAttributeTuple>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTraits, typename TAttributeTuple>
class MemberInitializer
{
public:

	typedef MemberInitializer<TMemberTraits, TAttributeTuple> SelfType;
	typedef TMemberTraits                                     TraitsType;
	typedef TAttributeTuple                                   AttributeTupleType;
	typedef typename TraitsType::OwnerType                    OwnerType;
	typedef typename TraitsType::ValueType                    ValueType;
	typedef typename TraitsType::LocationType                 LocationType;
	typedef typename TraitsType::MemberType                   MemberBaseType;
	typedef Member<
		MemberBaseType,
		OwnerType,
		ValueType,
		LocationType
	> MemberType;

	const static bool IS_CONST  = TraitsType::IS_CONST;
	const static bool IS_STATIC = TraitsType::IS_STATIC;

	MemberInitializer(
		const AttributeTupleType &attributes,
		const NAME               &name,
		LocationType             location
	);

	template <int I>
	const MemberBaseType *Instance() const;

	template <typename T>
	typename BindAttributeTraits<SelfType, T>::ResultType BindAttribute(const T &attributeInitializer) const;

private:

	AttributeTupleType m_attributes;
	NAME               m_name;
	LocationType       m_location;

	friend struct MemberTraits<OwnerType, ValueType, IS_STATIC>;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MemberSelect<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct MemberConstSelect
{
	typedef void     *AddressType;
	typedef AddrSize OffsetType;

	static const bool IS_CONST  = false;
};

template <typename T>
struct MemberConstSelect<T const>
{
	typedef const void     *AddressType;
	typedef const AddrSize OffsetType;
	
	static const bool IS_CONST  = true;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MemberTraits<TOwner, T, IsStatic>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TOwner, typename T, bool IsStatic = false>
struct MemberTraits
{
	typedef MemberConstSelect<T> ConstSelectType;
	
	typedef meta::TypeSelect<
		IsStatic,
		typename ConstSelectType::AddressType,
		typename ConstSelectType::OffsetType
	> LocationSelectType;

	typedef meta::TypeSelect<
		IsStatic,
		typename meta::TypeSelect<
			ConstSelectType::IS_CONST,
			reflect::Class::STATICCONSTANT,
			reflect::Class::STATICMEMBER
		>::Type,
		reflect::Class::MEMBER
	> MemberSelectType;

	typedef MemberTraits<TOwner, T, IsStatic> SelfType;	
	typedef TOwner                            OwnerType;
	typedef T                                 ValueType;
	typedef typename LocationSelectType::Type LocationType;
	typedef typename MemberSelectType::Type   MemberType;
	
	static const bool IS_CONST  = ConstSelectType::IS_CONST;
	static const bool IS_STATIC = IsStatic;

	static MemberInitializer<SelfType> CreateInitializer(
		const NAME   &name,
		LocationType location
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecursiveCountMembers<TMemberTuple, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple, int I>
struct RecursiveCountMembers
{
	typedef typename TMemberTuple::TailType TailType;
	typedef typename TMemberTuple::HeadType ValueType;
	typedef RecursiveCountMembers<
		TailType,
		I - 1
	> RecurseType;

	static const int NUM_MEMBERS = meta::PredicatedAdd<
		!ValueType::IS_STATIC,
		RecurseType::NUM_MEMBERS,
		1
	>::VALUE;

	static const int NUM_STATICMEMBERS = meta::PredicatedAdd<
		meta::And<
			!ValueType::IS_CONST,
			ValueType::IS_STATIC
		>::VALUE,
		RecurseType::NUM_STATICMEMBERS,
		1
	>::VALUE;

		static const int NUM_STATICCONSTANTS = meta::PredicatedAdd<
		meta::And<
			ValueType::IS_CONST,
			ValueType::IS_STATIC
		>::VALUE,
		RecurseType::NUM_STATICCONSTANTS,
		1
	>::VALUE;
};

template <typename TMemberTuple>
struct RecursiveCountMembers<TMemberTuple, -1>
{
	static const int NUM_MEMBERS         = 0;
	static const int NUM_STATICMEMBERS   = 0;
	static const int NUM_STATICCONSTANTS = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::CountMembers<TMemberTuple>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple>
struct CountMembers
{
	typedef RecursiveCountMembers<TMemberTuple, TMemberTuple::SIZE - 1> CountType;

	static const int NUM_MEMBERS         = CountType::NUM_MEMBERS;
	static const int NUM_STATICMEMBERS   = CountType::NUM_STATICMEMBERS;
	static const int NUM_STATICCONSTANTS = CountType::NUM_STATICCONSTANTS;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseMembers<TMemberTuple, J, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple, int J, int I>
struct RecurseMembers
{
	typedef typename TMemberTuple::HeadType ValueType;

	static const bool MATCH = !ValueType::IS_STATIC;
	static const int NEXT_J = meta::PredicatedAdd<MATCH, J, -1>::VALUE;

	static void Initialize(const Class::MEMBER **members, const TMemberTuple &m);
};

template <typename TMemberTuple, int J>
struct RecurseMembers<TMemberTuple, J, -1>
{
	static void Initialize(const Class::MEMBER **members, const TMemberTuple &m);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseStaticMembers<TMemberTuple, J, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple, int J, int I>
struct RecurseStaticMembers
{
	typedef typename TMemberTuple::HeadType ValueType;

	static const bool MATCH = meta::And<
		!ValueType::IS_CONST,
		ValueType::IS_STATIC
	>::VALUE;

	static const int NEXT_J = meta::PredicatedAdd<MATCH, J, -1>::VALUE;

	static void Initialize(const Class::STATICMEMBER **members, const TMemberTuple &m);
};

template <typename TMemberTuple, int J>
struct RecurseStaticMembers<TMemberTuple, J, -1>
{
	static void Initialize(const Class::STATICMEMBER **members, const TMemberTuple &m);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseStaticConstants<TMemberTuple, J, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple, int J, int I>
struct RecurseStaticConstants
{
	typedef typename TMemberTuple::HeadType ValueType;

	static const bool MATCH = meta::And<
		ValueType::IS_CONST,
		ValueType::IS_STATIC
	>::VALUE;		
	
	static const int NEXT_J = meta::PredicatedAdd<MATCH, J, -1>::VALUE;

	static void Initialize(const Class::STATICCONSTANT **members, const TMemberTuple &m);
};

template <typename TMemberTuple, int J>
struct RecurseStaticConstants<TMemberTuple, J, -1>
{
	static void Initialize(const Class::STATICCONSTANT **members, const TMemberTuple &m);
};

} // details
} // reflect


#include "ReflectPrivateMember.inl"
