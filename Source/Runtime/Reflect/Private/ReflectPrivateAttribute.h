// ReflectPrivateAttribute.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ReflectPrivateMapDef.h"


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Attribute<TValue>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue>
class Attribute :
public ATTRIBUTE
{
public:

	// reflect::::ATTRIBUTE implementation

	virtual const Class *Type() const;
	virtual const void *Value() const;

protected:

	Attribute(
		const NAME   &name,
		const TValue &value
	);

private:

	TValue m_value;

	template <typename TAttributeTraits>
	friend class AttributeInitializer;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::AttributeInitializer<TAttributeTraits>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TAttributeTraits>
class AttributeInitializer
{
public:

	typedef TAttributeTraits               TraitsType;
	typedef typename TraitsType::ValueType ValueType;
	typedef Attribute<ValueType>           AttributeType;

	AttributeInitializer(
		const NAME      &name,
		const ValueType &value
	);

	template <typename TContainerTraits, int K, int J, int I>
	const ATTRIBUTE *Instance() const;

private:

	NAME      m_name;
	ValueType m_value;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::AttributeTraits<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct AttributeTraits
{
	typedef AttributeTraits<T> SelfType;
	typedef T                  ValueType;

	static AttributeInitializer<SelfType> CreateInitializer(
		const NAME      &name,
		const ValueType &value
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseAttributes<TContainerTraits, TAttributeTuple, K, J, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TContainerTraits, typename TAttributeTuple, int K, int J, int I>
struct RecurseAttributes
{
	static void Initialize(const ATTRIBUTE **attributes, const TAttributeTuple &t);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseAttributes<TContainerTraits, TAttributeTuple, K, J, -1>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TContainerTraits, typename TAttributeTuple, int K, int J>
struct RecurseAttributes<TContainerTraits, TAttributeTuple, K, J, -1>
{
	static void Initialize(const ATTRIBUTE **attributes, const TAttributeTuple &t);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::InitializeAttributes<TContainerTraits, TAttributeTuple, K, J>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TContainerTraits, typename TAttributeTuple, int K, int J>
ATTRIBUTEARRAY InitializeAttributes(const TAttributeTuple &t);

} // details
} // reflect


#include "ReflectPrivateAttribute.inl"
