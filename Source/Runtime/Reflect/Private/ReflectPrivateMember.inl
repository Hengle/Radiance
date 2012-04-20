// ReflectPrivateMember.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Member<TBase, TOwner, TValue, TLocation>::OwnerType()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TBase, typename TOwner, typename TValue, typename TLocation>
const Class *Member<TBase, TOwner, TValue, TLocation>::OwnerType() const
{
	return reflect::Type<TOwner>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Member<TBase, TOwner, TValue, TLocation>::Type()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TBase, typename TOwner, typename TValue, typename TLocation>
const Class *Member<TBase, TOwner, TValue, TLocation>::Type() const
{
	return reflect::Type<TValue>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Member<TBase, TOwner, TValue, TLocation>::Member()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TBase, typename TOwner, typename TValue, typename TLocation>
inline Member<TBase, TOwner, TValue, TLocation>::Member(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	TLocation            location
) :
TBase(attributes, name, location)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MemberInitializer<TMemberTraits, TAttributeTuple>::MemberInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTraits, typename TAttributeTuple>
inline MemberInitializer<TMemberTraits, TAttributeTuple>::MemberInitializer(
	const AttributeTupleType &attributes,
	const NAME               &name,
	LocationType             location
) :
m_attributes(attributes),
m_name(name),
m_location(location)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MemberInitializer<TMemberTraits, TAttributeTuple>::Instance<I>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTraits, typename TAttributeTuple>
template <int I>
inline const typename TMemberTraits::MemberType *MemberInitializer<TMemberTraits, TAttributeTuple>::Instance() const
{
	static MemberType member(
		InitializeAttributes<TraitsType, AttributeTupleType, -1, I>(m_attributes),
		m_name,
		m_location
	);
	return &member;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MemberInitializer<TMemberTraits, TAttributeTuple>::BindAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTraits, typename TAttributeTuple>
template <typename T>
inline typename BindAttributeTraits<
		MemberInitializer<TMemberTraits, TAttributeTuple>, T
>::ResultType MemberInitializer<TMemberTraits, TAttributeTuple>::BindAttribute(
	const T &attributeInitializer
) const
{
	return typename BindAttributeTraits<SelfType, T>::ResultType(
		m_attributes + attributeInitializer,
		m_name,
		m_location
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MemberTraits<TOwner, T, IsStatic>::CreateInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TOwner, typename T, bool IsStatic>
inline MemberInitializer<MemberTraits<TOwner, T, IsStatic> > MemberTraits<TOwner, T, IsStatic>::CreateInitializer(
	const NAME    &name,
	LocationType  location
)
{
	return MemberInitializer<SelfType>(
		::MakeTuple(),
		name,
		location
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseMembers<TMemberTuple, J, I>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple, int J, int I>
inline void RecurseMembers<TMemberTuple, J, I>::Initialize(const Class::MEMBER **members, const TMemberTuple &t)
{
	GetInstance<MATCH, Class::MEMBER, TMemberTuple, J>::Execute(members, t);
	RecurseMembers<typename TMemberTuple::TailType, NEXT_J, I - 1>::Initialize(members, t.Tail());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseMembers<TMemberTuple, -1>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple, int J>
inline void RecurseMembers<TMemberTuple, J, -1>::Initialize(const Class::MEMBER **members, const TMemberTuple &t)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseStaticMembers<TMemberTuple, J, I>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple, int J, int I>
inline void RecurseStaticMembers<TMemberTuple, J, I>::Initialize(const Class::STATICMEMBER **members, const TMemberTuple &t)
{
	GetInstance<MATCH, Class::STATICMEMBER, TMemberTuple, J>::Execute(members, t);
	RecurseStaticMembers<typename TMemberTuple::TailType, NEXT_J, I - 1>::Initialize(members, t.Tail());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseStaticMembers<TMemberTuple, J, -1>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple, int J>
inline void RecurseStaticMembers<TMemberTuple, J, -1>::Initialize(const Class::STATICMEMBER **members, const TMemberTuple &t)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseStaticConstants<TMemberTuple, J, I>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple, int J, int I>
inline void RecurseStaticConstants<TMemberTuple, J, I>::Initialize(const Class::STATICCONSTANT **members, const TMemberTuple &t)
{
	GetInstance<MATCH, Class::STATICCONSTANT, TMemberTuple, J>::Execute(members, t);
	RecurseStaticConstants<typename TMemberTuple::TailType, NEXT_J, I - 1>::Initialize(members, t.Tail());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseStaticConstants<TMemberTuple, J, -1>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMemberTuple, int J>
inline void RecurseStaticConstants<TMemberTuple, J, -1>::Initialize(const Class::STATICCONSTANT **members, const TMemberTuple &t)
{
}

} // details
} // reflect

