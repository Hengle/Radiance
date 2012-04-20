// ReflectPrivateConstructor.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ReflectPrivateMapDef.h"
#include "ReflectPrivateAttribute.h"
#include "ReflectPrivateArgument.h"


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Constructor<TConstructorTraits>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits>
class Constructor :
public Class::CONSTRUCTOR
{
public:

	typedef TConstructorTraits              TraitsType;
	typedef typename TraitsType::ObjectType ObjectType;

	// ::reflect::Class::CONSTRUCTOR implementation

	virtual const Class *OwnerType() const;

	virtual void Call(
		void          *object,
		const IArgumentList &args
	) const;

	virtual bool Match(const ITypeList &args) const;

private:

	template <typename TConstructorTraits2, typename TAttributeTuple, typename TArgumentTuple>
	friend class ConstructorInitializer;

	Constructor(
		const ATTRIBUTEARRAY           &attributes,
		const IFunction::ARGUMENTARRAY &args
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindConstructorArgumentTraitsBase<TInitializer, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TInitializer, typename TBind>
struct BindConstructorArgumentTraitsBase
{
	typedef TInitializer                          InitType;
	typedef typename InitType::TraitsType         TraitsType;
	typedef typename InitType::AttributeTupleType AttributeTupleType;
	typedef typename InitType::ArgTupleType       ArgTupleType;
	typedef typename TraitsType::FunctionType     FunctionType;
	typedef typename TraitsType::ObjectType       ObjectType;

	typedef TBind                                    ArgInitType;
	typedef typename ArgInitType::QualifiedValueType ArgType;

	typedef ConstructorTraits<
		typename meta::PushArgument<FunctionType, ArgType>::Type,
		ObjectType
	> ResultTraitsType;

	typedef ConstructorInitializer<
		ResultTraitsType,
		AttributeTupleType,
		typename ArgTupleType::template AddTraits<TBind>::Type
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindRequiredArgumentTraits<
//	 ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
//	 TBind
// >
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TConstructorTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TBind
>
struct BindRequiredArgumentTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>:
public BindConstructorArgumentTraitsBase<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>
{
	typedef BindConstructorArgumentTraitsBase<
		ConstructorInitializer<
			TConstructorTraits, TAttributeTuple, TArgumentTuple
		>, TBind
	> SuperType;

	typedef typename SuperType::ResultType ResultType;
	typedef typename SuperType::InitType InitType;

	static ResultType BindArgument(
		const InitType &constructorInitializer,
		const TBind    &argumentInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindDefaultArgumentTraits<
//	 ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
//	 TBind
// >
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TConstructorTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TBind
>
struct BindDefaultArgumentTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	TBind
> :
public BindConstructorArgumentTraitsBase<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>
{
	typedef BindConstructorArgumentTraitsBase<
		ConstructorInitializer<
			TConstructorTraits, TAttributeTuple, TArgumentTuple
		>, TBind
	> SuperType;

	typedef typename SuperType::ResultType ResultType;
	typedef typename SuperType::InitType InitType;

	static ResultType BindArgument(
		const InitType &constructorInitializer,
		const TBind    &argumentInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindArgumentTraits<
//	 ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
//	 ArgumentInitializer<TArgumentTraits, TArgumentAttributeTuple>
// >
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TConstructorTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TArgumentTraits,
	typename TArgumentAttributeTuple
>
struct BindArgumentTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	ArgumentInitializer<TArgumentTraits, TArgumentAttributeTuple>
> :
public BindRequiredArgumentTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	ArgumentInitializer<TArgumentTraits, TArgumentAttributeTuple>
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindArgumentTraits<
//	 ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
//	 DefaultArgumentInitializer<TArgumentTraits, TArgumentAttributeTuple>
// >
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TConstructorTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TArgumentTraits,
	typename TArgumentAttributeTuple
>
struct BindArgumentTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	DefaultArgumentInitializer<TArgumentTraits, TArgumentAttributeTuple>
> :
public BindDefaultArgumentTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	DefaultArgumentInitializer<TArgumentTraits, TArgumentAttributeTuple>
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindAttributeTraits<
//	 ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
//	 TBind
// >
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TConstructorTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TBind
>
struct BindAttributeTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>
{
	typedef ConstructorInitializer<
		TConstructorTraits,
		TAttributeTuple,
		TArgumentTuple
	> InitType;

	typedef typename InitType::TraitsType         TraitsType;
	typedef typename InitType::AttributeTupleType AttributeTupleType;

	typedef ConstructorInitializer<
		TraitsType,
		typename AttributeTupleType::template AddTraits<TBind>::Type,
		TArgumentTuple
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindArgumentAttributeTraits<
//	 ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
//	 TBind
// >
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TConstructorTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TBind
>
struct BindArgumentAttributeTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>
{
	typedef ConstructorInitializer<
		TConstructorTraits,
		TAttributeTuple,
		TArgumentTuple
	> InitType;

	typedef typename InitType::TraitsType         TraitsType;
	typedef typename InitType::AttributeTupleType AttributeTupleType;
	typedef typename InitType::ArgTupleType       ArgTupleType;
	typedef typename ArgTupleType::HeadType       ArgInitType;
	typedef typename ArgTupleType::TailType       ArgTailType;

	typedef typename BindAttributeTraits<
		ArgInitType,
		TBind
	>::ResultType BindType;

	typedef ConstructorInitializer<
		TraitsType,
		AttributeTupleType,
		typename ArgTailType::template AddTraits<BindType>::Type
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits, typename TAttributeTuple, typename TArgumentTuple>
class ConstructorInitializer
{
public:

	typedef ConstructorInitializer<
		TConstructorTraits,
		TAttributeTuple,
		TArgumentTuple
	> SelfType;

	typedef TConstructorTraits              TraitsType;
	typedef TAttributeTuple                 AttributeTupleType;
	typedef TArgumentTuple                  ArgTupleType;
	typedef Constructor<TConstructorTraits> ConstructorType;

	ConstructorInitializer(
		const AttributeTupleType &attributes,
		const ArgTupleType       &arguments
	);

	template <int I>
	const Class::CONSTRUCTOR *Instance() const;

	template <typename T>
	typename BindArgumentTraits<SelfType, T>::ResultType BindArgument(const T &argumentInitializer) const;

	template <typename T>
	typename BindRequiredArgumentTraits<SelfType, T>::ResultType BindRequiredArgument(const T &argumentInitializer) const;

	template <typename T>
	typename BindDefaultArgumentTraits<SelfType, T>::ResultType BindDefaultArgument(const T &argumentInitializer) const;

	template <typename T>
	typename BindAttributeTraits<SelfType, T>::ResultType BindAttribute(const T &attributeInitializer) const;

	template <typename T>
	typename BindArgumentAttributeTraits<SelfType, T>::ResultType BindArgumentAttribute(const T &attributeInitializer) const;

private:

	const AttributeTupleType m_attributes;
	const ArgTupleType       m_arguments;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorTraits<TFunction, TOwner, IsStatic>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner>
struct ConstructorTraits :
public meta::FunctionTraits<TFunction>
{
	typedef ConstructorTraits<TFunction, TOwner>  SelfType;
	typedef meta::FunctionTraits<TFunction>  SuperTraitsType;
	typedef TOwner                                ObjectType;

	static ConstructorInitializer<SelfType> CreateInitializer();

	template <int I, typename TArgumentTuple>
	static const Class::CONSTRUCTOR::ARGUMENT **InitializeArguments(const TArgumentTuple &t);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecursiveCountConstructors<TConstructorTuple, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTuple, int I>
struct RecursiveCountConstructors
{
	typedef typename TConstructorTuple::TailType TailType;
	typedef typename TConstructorTuple::HeadType ValueType;
	typedef RecursiveCountConstructors<
		TailType,
		I - 1
	> RecurseType;

	static const int NUM_CONSTRUCTORS = RecurseType::NUM_CONSTRUCTORS + 1;
};

template <typename TConstructorTuple>
struct RecursiveCountConstructors<TConstructorTuple, -1>
{
	static const int NUM_CONSTRUCTORS = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::CountConstructors<TConstructorTuple>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTuple>
struct CountConstructors
{
	typedef RecursiveCountConstructors<TConstructorTuple, TConstructorTuple::SIZE - 1> CountType;

	static const int NUM_CONSTRUCTORS = CountType::NUM_CONSTRUCTORS;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseConstructors<TConstructorTuple, J, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTuple, int J, int I>
struct RecurseConstructors
{
	typedef typename TConstructorTuple::HeadType ValueType;

	static const int NEXT_J = J - 1;

	static void Initialize(const Class::CONSTRUCTOR **constructors, const TConstructorTuple &t);
};

template <typename TConstructorTuple, int J>
struct RecurseConstructors<TConstructorTuple, J, -1>
{
	static void Initialize(const Class::CONSTRUCTOR **constructors, const TConstructorTuple &t);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::CallConstructor<TMethod, T, TFunction, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TMethod,
	typename T,
	typename TFunction,
	int I = ::meta::FunctionTraits<TFunction>::NUM_ARGS
>
struct CallConstructor;

} // details
} // reflect


//////////////////////////////////////////////////////////////////////////////////////////
// CallConstructor<MethodType, T, TFunction , I> specializations
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_CALLMETHOD(_num, _args) \
	template <typename TMethod, typename T, typename TFunction> \
	struct CallConstructor<TMethod, T, TFunction, _num> \
	{ \
		static void Call( \
		  const TMethod *method, \
			void          *location, \
			const IArgumentList &args \
		) \
		{ \
			RAD_ASSERT(NULL != method); \
			RAD_ASSERT(NULL != location); \
			new (method->OwnerType(), location) T _args; \
		} \
	};

#include "ReflectPrivateCallMethod.inl"
#undef PRIVATE_RADREFLECT_CALLMETHOD
#include "ReflectPrivateConstructor.inl"
