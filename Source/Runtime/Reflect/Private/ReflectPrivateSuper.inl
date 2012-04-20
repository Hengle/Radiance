// ReflectPrivateSuper.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.


//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace reflect
//////////////////////////////////////////////////////////////////////////////////////////

namespace reflect {

//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace reflect::details
//////////////////////////////////////////////////////////////////////////////////////////

namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Super<TOwner, TValue>::OwnerType()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TOwner, typename TValue>
const Class *Super<TOwner, TValue>::OwnerType() const
{
	return reflect::Type<TOwner>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Super<TBase, TOwner, TValue>::Type()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TOwner, typename TValue>
const Class *Super<TOwner, TValue>::Type() const
{
	return reflect::Type<TValue>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Super<TBase, TOwner, TValue>::Super()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TOwner, typename TValue>
inline Super<TOwner, TValue>::Super(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	AddrSize             offset
) :
Class::SUPER(attributes, name, offset)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::SuperInitializer<TSuperTraits>::SuperInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TSuperTraits, typename TAttributeTuple>
inline SuperInitializer<TSuperTraits, TAttributeTuple>::SuperInitializer(
	const AttributeTupleType &attributes,
	const NAME               &name,
	AddrSize                 offset
) :
m_attributes(attributes),
m_name(name),
m_offset(offset)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::SuperInitializer<TSuperTraits, TAttributeTuple>::Instance<I>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TSuperTraits, typename TAttributeTuple>
template <int I>
inline const Class::SUPER *SuperInitializer<TSuperTraits, TAttributeTuple>::Instance() const
{
	static SuperType super(
		InitializeAttributes<TraitsType, AttributeTupleType, -1, I>(m_attributes),
		m_name,
		m_offset
	);
	return &super;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::SuperInitializer<TSuperTraits, TAttributeTuple>::BindAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TSuperTraits, typename TAttributeTuple>
template <typename T>
inline typename BindAttributeTraits<
		SuperInitializer<TSuperTraits, TAttributeTuple>, T
>::ResultType SuperInitializer<TSuperTraits, TAttributeTuple>::BindAttribute(
	const T &attributeInitializer
) const
{
	return typename BindAttributeTraits<SelfType, T>::ResultType(
		m_attributes + attributeInitializer,
		m_name,
		m_offset
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::SuperTraits<TOwner, T>::CreateInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TOwner, typename T>
inline SuperInitializer<SuperTraits<TOwner, T> > SuperTraits<TOwner, T>::CreateInitializer(
	const NAME &name,
	AddrSize   offset
)
{
	return SuperInitializer<SelfType>(
		::MakeTuple(),
		name,
		offset
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseSupers<TSuperTuple, I>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TSuperTuple, int I>
inline void RecurseSupers<TSuperTuple, I>::Initialize(const Class::SUPER **supers, const TSuperTuple &t)
{
	typedef typename TSuperTuple::HeadType HeadType;
	supers[I] = t.Head().HeadType::template Instance<I>();
	RecurseSupers<typename TSuperTuple::TailType, I - 1>::Initialize(supers, t.Tail());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseSupers<TSuperTuple, -1>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TSuperTuple>
inline void RecurseSupers<TSuperTuple, -1>::Initialize(const Class::SUPER **supers, const TSuperTuple &t)
{
}

} // details
} // reflect

