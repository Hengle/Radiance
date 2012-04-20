// ReflectPrivateMethod.h
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
// reflect::details::MethodBase<TMethodTraits>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits>
class MethodBase :
public TMethodTraits::MethodType
{
public:

	typedef TMethodTraits                   TraitsType;
	typedef typename TraitsType::ObjectType ObjectType;
	typedef typename TraitsType::ReturnType ReturnValueType;
	typedef typename TraitsType::PtrType    PtrType;

	// reflect::Class::METHOD implementation

	virtual const Class *OwnerType() const;
	virtual const Class *ReturnType() const;
	virtual bool Match(const ITypeList &args) const;

protected:

	MethodBase(
		const ATTRIBUTEARRAY           &attributes,
		const NAME                     &name,
		PtrType                        ptr,
		const IFunction::ARGUMENTARRAY &args
	);

	PtrType m_ptr;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Method<TMethodTraits>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits>
class Method :
public MethodBase<TMethodTraits>
{
public:

	typedef MethodBase<TMethodTraits> SuperType;
	typedef typename SuperType::TraitsType TraitsType;
	typedef typename SuperType::PtrType PtrType;
	typedef typename TraitsType::VoidType VoidType;
	typedef typename SuperType::ObjectType ObjectType;

	// SuperType implementation
	// (either reflect::Class::CONSTMETHOD or
	// reflect::Class::MUTABLEMETHOD)

	virtual void Call(
		const Reflected     &result,
		VoidType      *object,
		const IArgumentList &args
	) const;

private:

	template <typename TMethodTraits2, typename TAttributeTuple, typename TArgumentTuple>
	friend class MethodInitializer;

	Method(
		const ATTRIBUTEARRAY           &attributes,
		const NAME                     &name,
		PtrType                        ptr,
		const IFunction::ARGUMENTARRAY &args
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Method<MethodTraits<TFunction, TOwner, true>>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner>
class Method<MethodTraits<TFunction, TOwner, true> > :
public MethodBase<MethodTraits<TFunction, TOwner, true> >
{
public:

	typedef MethodBase<MethodTraits<TFunction, TOwner, true> > SuperType;
	typedef typename SuperType::PtrType PtrType;
	typedef typename SuperType::ObjectType ObjectType;
	typedef typename SuperType::TraitsType TraitsType;

	// reflect::Class::STATICMETHOD implementation

	virtual void Call(
		const Reflected     &result,
		const IArgumentList &args
	) const;

private:

	template <typename TMethodTraits, typename TAttributeTuple, typename TArgumentTuple>
	friend class MethodInitializer;

	Method(
		const ATTRIBUTEARRAY           &attributes,
		const NAME                     &name,
		PtrType                        ptr,
		const IFunction::ARGUMENTARRAY &args
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindMethodArgumentTraitsBase<TInitializer, TBind>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TInitializer, typename TBind>
struct BindMethodArgumentTraitsBase
{
	typedef TInitializer                          InitType;
	typedef typename InitType::TraitsType         TraitsType;
	typedef typename InitType::AttributeTupleType AttributeTupleType;
	typedef typename InitType::ArgTupleType       ArgTupleType;
	typedef typename TraitsType::FunctionType     FunctionType;
	typedef typename TraitsType::ObjectType       ObjectType;

	typedef TBind                                    ArgInitType;
	typedef typename ArgInitType::QualifiedValueType ArgType;

	static const bool IS_STATIC = TraitsType::IS_STATIC;

	typedef MethodTraits<
		typename meta::PushArgument<FunctionType, ArgType>::Type,
		ObjectType,
		IS_STATIC
	> ResultTraitsType;

	typedef MethodInitializer<
		ResultTraitsType,
		AttributeTupleType,
		typename ArgTupleType::template AddTraits<TBind>::Type
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindRequiredArgumentTraits<
//	 MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
//	 TBind
// >
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TMethodTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TBind
>
struct BindRequiredArgumentTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>:
public BindMethodArgumentTraitsBase<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>
{
	typedef  BindMethodArgumentTraitsBase<
			MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
		TBind
	> SuperType;

	typedef typename SuperType::ResultType ResultType;
	typedef typename SuperType::InitType InitType;

	static ResultType BindArgument(
		const InitType &methodInitializer,
		const TBind    &argumentInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindDefaultArgumentTraits<
//	 MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
//	 TBind
// >
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TMethodTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TBind
>
struct BindDefaultArgumentTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	TBind
> :
public BindMethodArgumentTraitsBase<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>
{
	typedef  BindMethodArgumentTraitsBase<
			MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
		TBind
	> SuperType;

	typedef typename SuperType::ResultType ResultType;
	typedef typename SuperType::InitType InitType;

	static ResultType BindArgument(
		const InitType &methodInitializer,
		const TBind    &argumentInitializer
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindArgumentTraits<MethodInitializer<MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>, T>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TMethodTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TArgumentTraits,
	typename TArgumentAttributeTuple
>
struct BindArgumentTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	ArgumentInitializer<TArgumentTraits, TArgumentAttributeTuple>
> :
public BindRequiredArgumentTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	ArgumentInitializer<TArgumentTraits, TArgumentAttributeTuple>
>
{
};

template <
	typename TMethodTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TArgumentTraits,
	typename TArgumentAttributeTuple
>
struct BindArgumentTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	DefaultArgumentInitializer<TArgumentTraits, TArgumentAttributeTuple>
> :
public BindDefaultArgumentTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	DefaultArgumentInitializer<TArgumentTraits, TArgumentAttributeTuple>
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindAttributeTraits<
//	 MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
//	 TBind
// >
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TMemberTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TBind
>
struct BindAttributeTraits<
	MethodInitializer<TMemberTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>
{
	typedef MethodInitializer<
		TMemberTraits,
		TAttributeTuple,
		TArgumentTuple
	> InitType;

	typedef typename InitType::TraitsType         TraitsType;
	typedef typename InitType::AttributeTupleType AttributeTupleType;

	typedef MethodInitializer<
		TraitsType,
		typename AttributeTupleType::template AddTraits<TBind>::Type,
		TArgumentTuple
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindArgumentAttributeTraits<
//	 MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
//	 TBind
// >
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TMethodTraits,
	typename TAttributeTuple,
	typename TArgumentTuple,
	typename TBind
>
struct BindArgumentAttributeTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>
{
	typedef MethodInitializer<
		TMethodTraits,
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

	typedef MethodInitializer<
		TraitsType,
		AttributeTupleType,
		typename ArgTailType::template AddTraits<BindType>::Type
	> ResultType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, typename TAttributeTuple, typename TArgumentTuple>
class MethodInitializer
{
public:

	typedef MethodInitializer<
		TMethodTraits,
		TAttributeTuple,
		TArgumentTuple
	> SelfType;

	typedef TMethodTraits                   TraitsType;
	typedef TAttributeTuple                 AttributeTupleType;
	typedef TArgumentTuple                  ArgTupleType;
	typedef typename TraitsType::MethodType MethodBaseType;
	typedef typename TraitsType::PtrType    PtrType;
	typedef Method<TMethodTraits>           MethodType;

	static const bool IS_CONST  = TraitsType::IS_CONST;
	static const bool IS_STATIC = TraitsType::IS_STATIC;

	MethodInitializer(
		const AttributeTupleType &attributes,
		const NAME               &name,
		PtrType                  ptr,
		const ArgTupleType       &a
	);

	template <int I>
	const MethodBaseType *Instance() const;

	template <typename T>
	typename BindArgumentTraits<SelfType, T>::ResultType BindArgument(const T &argumentInitializer) const;

	template <typename T>
	typename BindRequiredArgumentTraits<SelfType, T>::ResultType BindRequiredArgument(const T &argumentInitializer) const;

	template <typename T>
	typename BindDefaultArgumentTraits<SelfType, T>::ResultType BindDefaultArgument(const T &argumentInitializer) const;

	SelfType BindMethod(
		const NAME &name,
		PtrType    ptr
	) const;

	template <typename T>
	typename BindAttributeTraits<SelfType, T>::ResultType BindAttribute(const T &attributeInitializer) const;

	template <typename T>
	typename BindArgumentAttributeTraits<SelfType, T>::ResultType BindArgumentAttribute(const T &attributeInitializer) const;

private:

	AttributeTupleType m_attributes;
	NAME               m_name;
	PtrType            m_ptr;
	ArgTupleType       m_arguments;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodSelect<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct MethodSelect
{
	typedef void                 VoidType;
	typedef Class::MUTABLEMETHOD MethodType;

	static const bool IS_CONST  = false;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodSelect<T const>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct MethodSelect<T const>
{
	typedef const void         VoidType;
	typedef Class::CONSTMETHOD MethodType;

	static const bool IS_CONST  = true;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodTraitsBase<TFunction, TOwner, false>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner, bool IsStatic>
struct MethodTraitsBase :
public meta::MethodTraits<TFunction, TOwner>,
public MethodSelect<TOwner>
{
	typedef meta::MethodTraits<TFunction, TOwner> SuperTraitsType;
	typedef typename SuperTraitsType::PtrType          PtrType;

	static const bool IS_STATIC = false;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodTraitsBase<TFunction, TOwner, true>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner>
struct MethodTraitsBase<TFunction, TOwner, true> :
public meta::FunctionTraits<TFunction>
{
	typedef meta::FunctionTraits<TFunction> SuperTraitsType;
	typedef TOwner                               ObjectType;
	typedef Class::STATICMETHOD                  MethodType;
	typedef typename SuperTraitsType::PtrType    PtrType;

	static const bool IS_CONST  = false;
	static const bool IS_STATIC = true;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodTraits<TFunction, TOwner, IsStatic>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner, bool IsStatic = false>
struct MethodTraits :
public MethodTraitsBase<TFunction, TOwner, IsStatic>
{
	typedef MethodTraits <TFunction, TOwner, IsStatic> SelfType;

	static MethodInitializer<SelfType, TupleTraits<>::Type> CreateInitializer();

	template <int I, typename TArgumentTuple>
	static const Class::METHOD::ARGUMENT **InitializeArguments(const TArgumentTuple &t);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecursiveCountMethods<TMethodTuple, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple, int I>
struct RecursiveCountMethods
{
	typedef typename TMethodTuple::TailType TailType;
	typedef typename TMethodTuple::HeadType ValueType;
	typedef RecursiveCountMethods<
		TailType,
		I - 1
	> RecurseType;

	static const int NUM_METHODS = meta::PredicatedAdd<
		!meta::Or<
			ValueType::IS_CONST,
			ValueType::IS_STATIC
		>::VALUE,
		RecurseType::NUM_METHODS,
		1
	>::VALUE;

	static const int NUM_CONSTMETHODS = meta::PredicatedAdd<
		ValueType::IS_CONST,
		RecurseType::NUM_CONSTMETHODS,
		1
	>::VALUE;

	static const int NUM_STATICMETHODS = meta::PredicatedAdd<
		ValueType::IS_STATIC,
		RecurseType::NUM_STATICMETHODS,
		1
	>::VALUE;
};

template <typename TMethodTuple>
struct RecursiveCountMethods<TMethodTuple, -1>
{
	static const int NUM_METHODS       = 0;
	static const int NUM_CONSTMETHODS  = 0;
	static const int NUM_STATICMETHODS = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::CountMethods<TMethodTuple>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple>
struct CountMethods
{
	typedef RecursiveCountMethods<TMethodTuple, TMethodTuple::SIZE - 1> CountType;

	static const int NUM_METHODS       = CountType::NUM_METHODS;
	static const int NUM_CONSTMETHODS  = CountType::NUM_CONSTMETHODS;
	static const int NUM_STATICMETHODS = CountType::NUM_STATICMETHODS;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseMethods<TMethodTuple, J, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple, int J, int I>
struct RecurseMethods
{
	typedef typename TMethodTuple::HeadType ValueType;

	static const bool MATCH = !meta::Or<
		ValueType::IS_CONST,
		ValueType::IS_STATIC
	>::VALUE;

	static const int NEXT_J = meta::PredicatedAdd<MATCH, J, -1>::VALUE;

	static void Initialize(const Class::MUTABLEMETHOD **methods, const TMethodTuple &t);
};

template <typename TMethodTuple, int J>
struct RecurseMethods<TMethodTuple, J, -1>
{
	static void Initialize(const Class::MUTABLEMETHOD **methods, const TMethodTuple &t);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseConstMethods<TMethodTuple, J, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple, int J, int I>
struct RecurseConstMethods
{
	typedef typename TMethodTuple::HeadType ValueType;

	static const bool MATCH = ValueType::IS_CONST;
	static const int NEXT_J = meta::PredicatedAdd<MATCH, J, -1>::VALUE;

	static void Initialize(const Class::CONSTMETHOD **methods, const TMethodTuple &t);
};

template <typename TMethodTuple, int J>
struct RecurseConstMethods<TMethodTuple, J, -1>
{
	static void Initialize(const Class::CONSTMETHOD **methods, const TMethodTuple &t);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseStaticMethods<TMethodTuple, J, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple, int J, int I>
struct RecurseStaticMethods
{
	typedef typename TMethodTuple::HeadType ValueType;

	static const bool MATCH = ValueType::IS_STATIC;
	static const int NEXT_J = meta::PredicatedAdd<MATCH, J, -1>::VALUE;

	static void Initialize(const Class::STATICMETHOD **methods, const TMethodTuple &t);
};

template <typename TMethodTuple, int J>
struct RecurseStaticMethods<TMethodTuple, J, -1>
{
	static void Initialize(const Class::STATICMETHOD **methods, const TMethodTuple &t);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodCaller<T, TMethodPtr, TFunction, TReturn>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T,
	typename TMethodPtr,
	typename TFunction,
	typename TReturn = typename meta::RemoveReference<
		typename meta::FunctionTraits<TFunction>::ReturnType
	>::Type
>
struct MethodCaller;

template <typename T, typename TMethodPtr, typename TFunction, typename TReturn>
struct MethodCaller
{
	static void Call(
		const Class::METHOD *method,
		T                   &object,
		const Reflected     &result,
		TMethodPtr          ptr,
		const IArgumentList &args
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodCaller<T, TMethodPtr, TFunction, void>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename TMethodPtr, typename TFunction>
struct MethodCaller<T, TMethodPtr, TFunction, void>
{
	static void Call(
		const Class::METHOD *method,
		T                   &object,
		const Reflected     &result,
		TMethodPtr           ptr,
		const IArgumentList &args
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodCaller<void, TMethodPtr, TFunction, TReturn>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodPtr, typename TFunction, typename TReturn>
struct MethodCaller<void, TMethodPtr, TFunction, TReturn>
{
	static void Call(
		const Class::METHOD *method,
		const Reflected     &result,
		TMethodPtr           ptr,
		const IArgumentList &args
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodCaller<void, TMethodPtr, TFunction, void>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodPtr, typename TFunction>
struct MethodCaller<void, TMethodPtr, TFunction, void>
{
	static void Call(
		const Class::METHOD *method,
		const Reflected     &result,
		TMethodPtr           ptr,
		const IArgumentList &args
	);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::CallMethod<TMethod, T, TMethodPtr, TFunction, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename TMethod,
	typename T,
	typename TMethodPtr,
	typename TFunction,
	typename TReturn = typename meta::FunctionTraits<TFunction>::ReturnType,
	int I            = ::meta::FunctionTraits<TFunction>::NUM_ARGS
>
struct CallMethod;

} // details
} // reflect


//////////////////////////////////////////////////////////////////////////////////////////
// CallMethod<TMethod, void, TMethodPtr, TFunction, I> specializations
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_CALLMETHOD(_num, _args) \
	template <typename TMethod, typename T, typename TMethodPtr, typename TFunction, typename TReturn> \
	struct CallMethod<TMethod, T, TMethodPtr, TFunction, TReturn, _num> \
	{ \
		static TReturn Call( \
		  const TMethod *method, \
			T             &object, \
			TMethodPtr    ptr, \
			const IArgumentList &args \
		) \
		{ \
			return (object.*ptr)_args; \
		} \
	};

#include "ReflectPrivateCallMethod.inl"
#undef PRIVATE_RADREFLECT_CALLMETHOD

//////////////////////////////////////////////////////////////////////////////////////////
// CallMethod<TMethod, void, TMethodPtr, TFunction, I> specializations
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_RADREFLECT_CALLMETHOD(_num, _args) \
	template <typename TMethod, typename TMethodPtr, typename TFunction, typename TReturn> \
	struct CallMethod<TMethod, void, TMethodPtr, TFunction, TReturn, _num> \
	{ \
		static TReturn Call( \
		  const TMethod *method, \
			TMethodPtr    ptr, \
			const IArgumentList &args \
		) \
		{ \
			return (*ptr)_args; \
		} \
	};

#include "ReflectPrivateCallMethod.inl"
#undef PRIVATE_RADREFLECT_CALLMETHOD

#include "ReflectPrivateMethod.inl"
