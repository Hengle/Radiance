// ReflectPrivateArgument.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../StringBase.h"
#include "../../Meta/TypeTraits/QualifierTraits.h"


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Argument<TValue, IS_REFERENCE, IS_CONST>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue, bool IS_REFERENCE, bool IS_CONST>
class Argument :
public IFunction::ARGUMENT
{
public:

	// reflect::Class::METHODBASE::ARGUMENT implementation

	virtual bool IsConst() const;
	virtual bool IsReference() const;
	virtual const Class *Type() const;
	virtual const void *DefaultValue() const;

protected:

	Argument(
		const ATTRIBUTEARRAY &attributes,
		const NAME           &name
	);

	template <typename TArgumentTraits, typename TAttributeTuple>
	friend class ArgumentInitializer;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::DefaultArgument<TArgumentTraits>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue, bool IS_REFERENCE, bool IS_CONST>
class DefaultArgument :
public Argument<TValue, IS_REFERENCE, IS_CONST>
{
public:

	// Argument overrides

	virtual const void *DefaultValue() const;

private:

	DefaultArgument(
		const ATTRIBUTEARRAY &attributes,
		const NAME           &name,
		const TValue         &defaultVal
	);

	TValue m_default;

	template <typename TArgumentTraits, typename TAttributeTuple>
	friend class DefaultArgumentInitializer;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindAttributeTraits<ArgumentInitializer<TArgumentTraits, TAttributeTuple>,	TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TArgumentTraits,
	typename TAttributeTuple,
	typename TBind
>
struct BindAttributeTraits<
	ArgumentInitializer<TArgumentTraits, TAttributeTuple>,
	TBind
>
{
	typedef ArgumentInitializer<TArgumentTraits, TAttributeTuple> InitType;
	typedef typename InitType::TraitsType                         TraitsType;
	typedef typename InitType::AttributeTupleType                 AttributeTupleType;

	typedef ArgumentInitializer<
		TraitsType,
		typename AttributeTupleType::template AddTraits<TBind>::Type
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentInitializer<TArgumentTraits, TAttributeTuple>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TArgumentTraits, typename TAttributeTuple>
class ArgumentInitializer
{
public:

	typedef ArgumentInitializer<TArgumentTraits, TAttributeTuple> SelfType;
	typedef TArgumentTraits                                       TraitsType;
	typedef TAttributeTuple                                       AttributeTupleType;
	typedef typename TraitsType::QualifiedValueType               QualifiedValueType;
	typedef typename TraitsType::ValueType                        ValueType;

	static const bool IS_REFERENCE = TraitsType::IS_REFERENCE;
	static const bool IS_CONST     = TraitsType::IS_CONST;

	typedef Argument<ValueType, IS_REFERENCE, IS_CONST> ArgumentType;

	ArgumentInitializer(
		const AttributeTupleType &attributes,
		const NAME               &name
	);

	template <typename TMethodTraits, int J, int I>
	const IFunction::ARGUMENT *Instance() const;

	template <typename T>
	typename BindAttributeTraits<SelfType, T>::ResultType BindAttribute(const T &attributeInitializer) const;

protected:

	AttributeTupleType m_attributes;
	NAME               m_name;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindAttributeTraits<DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple>,	TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TArgumentTraits,
	typename TAttributeTuple,
	typename TBind
>
struct BindAttributeTraits<
	DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple>,
	TBind
>
{
	typedef DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple> InitType;
	typedef typename InitType::TraitsType                                TraitsType;
	typedef typename InitType::AttributeTupleType                        AttributeTupleType;

	typedef DefaultArgumentInitializer<
		TraitsType,
		typename AttributeTupleType::template AddTraits<TBind>::Type
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TArgumentTraits, typename TAttributeTuple>
class DefaultArgumentInitializer :
public ArgumentInitializer<TArgumentTraits, TAttributeTuple>
{
public:

	typedef ArgumentInitializer<TArgumentTraits, TAttributeTuple> SuperType;
	typedef typename SuperType::ValueType ValueType;
	typedef typename SuperType::AttributeTupleType AttributeTupleType;
	typedef DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple> SelfType;
	typedef DefaultArgument<ValueType, SuperType::IS_REFERENCE, SuperType::IS_CONST> ArgumentType;

	DefaultArgumentInitializer(
		const AttributeTupleType &attributes,
		const NAME               &name,
		const ValueType          &defaultVal
	);

	// We need per type per method per argument default arguments

	template <typename TMethodTraits, int J, int I>
	const IFunction::ARGUMENT *Instance() const;

	template <typename T>
	typename BindAttributeTraits<SelfType, T>::ResultType BindAttribute(const T &attributeInitializer) const;

private:

	const ValueType m_default;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentTraits<T, REF, C>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct ArgumentTraits
{
	typedef ArgumentTraits<T>                                 SelfType;
	typedef T                                                 QualifiedValueType;
	typedef typename meta::RemoveConstReference<T>::Type ValueType;

	static const bool IS_REFERENCE = meta::IsReference<T>::VALUE;
	static const bool IS_CONST     = meta::IsConst<T>::VALUE;

	static ArgumentInitializer<SelfType> CreateInitializer(const NAME &name);

	static DefaultArgumentInitializer<SelfType> CreateDefaultInitializer(
		const NAME      &name,
		const ValueType &defaultVal
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseArguments<TMethodTraits, J, TArgumentTuple, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, int J, typename TArgumentTuple, int I>
struct RecurseArguments
{
	static void Initialize(const IFunction::ARGUMENT **arguments, const TArgumentTuple &a);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseArguments<TMethodTraits, J, TArgumentTuple, -1>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, int J, typename TArgumentTuple>
struct RecurseArguments<TMethodTraits, J, TArgumentTuple, -1>
{
	static void Initialize(const IFunction::ARGUMENT **arguments, const TArgumentTuple &a);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtractValue<TMethod, T, TReturn, C>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T, bool TReturn, bool C>
struct ArgumentExtractValue;

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtractValue<TMethod, T, false, false>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
struct ArgumentExtractValue<TMethod, T, false, false>
{
	typedef T ValueType;

	static ValueType Value(
		const TMethod *method,
		const IArgumentList &args,
		int           i
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtractValue<TMethod, T, false, true>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
struct ArgumentExtractValue<TMethod, T, false, true>
{
	typedef const T ValueType;

	static ValueType Value(
		const TMethod *method,
		const IArgumentList &args,
		int           i
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtractValue<TMethod, T, true, false>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
struct ArgumentExtractValue<TMethod, T, true, false>
{
	typedef T &ValueType;

	static ValueType Value(
		const TMethod *method,
		const IArgumentList &args,
		int           i
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtractValue<TMethod, T, true, true>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
struct ArgumentExtractValue<TMethod, T, true, true>
{
	typedef const T &ValueType;

	static ValueType Value(
		const TMethod *method,
		const IArgumentList &args,
		int           i
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtract<TMethod, TFunction, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename TFunction, int I>
struct ArgumentExtract
{
	typedef TFunction                                                 FunctionType;
	typedef typename ::meta::Argument<FunctionType, I>::Type     ArgType;
	typedef typename ::meta::RemoveConstReference<ArgType>::Type BaseArgType;

	static const bool IS_REFERENCE = meta::IsReference<ArgType>::VALUE;
	static const bool IS_CONST     = meta::IsConst<ArgType>::VALUE;

	typedef ArgumentExtractValue<
		TMethod,
		BaseArgType,
		IS_REFERENCE,
		IS_CONST
	> ExtractType;

	typedef typename ExtractType::ValueType ValueType;

	static ValueType Value(
		const TMethod *method,
		const IArgumentList &args
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MatchArgument<TMethod, T, TReturn, C>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T, bool TReturn, bool C>
struct MatchArgument;

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MatchArgument<TMethod, T, false, false>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
struct MatchArgument<TMethod, T, false, false>
{
	static bool Match(
		const TMethod *method,
		const ITypeList &args,
		int           i
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MatchArgument<TMethod, T, false, true>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
struct MatchArgument<TMethod, T, false, true>
{
	static bool Match(
		const TMethod *method,
		const ITypeList &args,
		int           i
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MatchArgument<TMethod, T, true, false>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
struct MatchArgument<TMethod, T, true, false>
{
	static bool Match(
		const TMethod *method,
		const ITypeList &args,
		int           i
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MatchArgument<TMethod, T, true, true>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
struct MatchArgument<TMethod, T, true, true>
{
	static bool Match(
		const TMethod *method,
		const ITypeList &args,
		int           i
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentMatch<TMethod, TFunction, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename TFunction, int I>
struct ArgumentMatch
{
	typedef TFunction                                                 FunctionType;
	typedef typename ::meta::Argument<FunctionType, I>::Type     ArgType;
	typedef typename ::meta::RemoveConstReference<ArgType>::Type BaseArgType;

	static const bool IS_REFERENCE = meta::IsReference<ArgType>::VALUE;
	static const bool IS_CONST     = meta::IsConst<ArgType>::VALUE;

	typedef MatchArgument<
		TMethod,
		BaseArgType,
		IS_REFERENCE,
		IS_CONST
	> MatchType;

	static bool Match(
		const TMethod *method,
		const ITypeList &args
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MatchMethod<TMethod, TFunction, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TMethod,
	typename TFunction,
	int I            = ::meta::FunctionTraits<TFunction>::NUM_ARGS
>
struct MatchMethod;

} // details
} // reflect


//////////////////////////////////////////////////////////////////////////////////////////
// MatchMethod<TMethod, TFunction, I> specializations
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_MATCHMETHOD(_num, _args) \
	template <typename TMethod, typename TFunction> \
	struct MatchMethod<TMethod, TFunction, _num> \
	{ \
		static bool Match( \
		  const TMethod *method, \
		  const ITypeList &args \
		) \
		{ \
			return _args; \
		} \
	};

#include "ReflectPrivateMatchMethod.inl"
#undef PRIVATE_RADREFLECT_MATCHMETHOD

#include "ReflectPrivateArgument.inl"
