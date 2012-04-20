// ReflectPrivateAttribute.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Attribute<TValue>::Type()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue>
const Class *Attribute<TValue>::Type() const
{
	return reflect::Type<TValue>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Attribute<TValue>::Type()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue>
const void *Attribute<TValue>::Value() const
{
	return static_cast<const void *>(&m_value);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Attribute<TValue>::Attribute()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue>
inline Attribute<TValue>::Attribute(
	const NAME   &name,
	const TValue &value
) :
ATTRIBUTE(name),
m_value(value)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::AttributeInitializer<TAttributeTraits>::AttributeInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TAttributeTraits>
inline AttributeInitializer<TAttributeTraits>::AttributeInitializer(
	const NAME      &name,
	const ValueType &value
) :
m_name(name),
m_value(value)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::AttributeInitializer<TAttributeTraits>::Instance<TContainerTraits, K, J, I>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TAttributeTraits>
template <typename TContainerTraits, int K, int J, int I>
inline const ATTRIBUTE *AttributeInitializer<TAttributeTraits>::Instance() const
{
	static AttributeType attribute(m_name, m_value);
	return &attribute;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::AttributeTraits<T>::CreateInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline AttributeInitializer<AttributeTraits<T> > AttributeTraits<T>::CreateInitializer(
	const NAME      &name,
	const ValueType &value
)
{
	return AttributeInitializer<SelfType>(name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseAttributes<TContainerTraits, TAttributeTuple, K, J, I>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TContainerTraits, typename TAttributeTuple, int K, int J, int I>
inline void RecurseAttributes<TContainerTraits, TAttributeTuple, K, J, I>::Initialize(const ATTRIBUTE **attributes, const TAttributeTuple &t)
{
// GCC makes me do this, *sigh*.
	typedef typename TAttributeTuple::HeadType HeadType;
	attributes[I] = t.Head().HeadType::template Instance<TContainerTraits, K, J, I>();
	RecurseAttributes<TContainerTraits, typename TAttributeTuple::TailType, K, J, I - 1>::Initialize(attributes, t.Tail());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseAttributes<TContainerTraits, TAttributeTuple, K, J, -1>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TContainerTraits, typename TAttributeTuple, int K, int J>
inline void RecurseAttributes<TContainerTraits, TAttributeTuple, K, J, -1>::Initialize(const ATTRIBUTE **attributes, const TAttributeTuple &t)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::InitializeAttributes<TContainerTraits, TAttributeTuple, K, J>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TContainerTraits, typename TAttributeTuple, int K, int J>
inline ATTRIBUTEARRAY InitializeAttributes(const TAttributeTuple &t)
{
	static const ATTRIBUTE *attributes[TAttributeTuple::SIZE + 1];
	attributes[TAttributeTuple::SIZE] = NULL;
	RecurseAttributes<TContainerTraits, TAttributeTuple, K, J, TAttributeTuple::SIZE - 1>::Initialize(attributes, t);
	return ATTRIBUTEARRAY(attributes);
}

} // details
} // reflect

