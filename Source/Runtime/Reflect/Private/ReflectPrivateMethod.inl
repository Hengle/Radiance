// ReflectPrivateMethod.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodBase<TMethodTraits>::OwnerType()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits>
const Class *MethodBase<TMethodTraits>::OwnerType() const
{
	return reflect::Type<ObjectType>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodBase<TMethodTraits>::ReturnType()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits>
const Class *MethodBase<TMethodTraits>::ReturnType() const
{
	return reflect::Type<ReturnValueType>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodBase<TMethodTraits>::Match()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits>
bool MethodBase<TMethodTraits>::Match(const ITypeList &args) const
{
	if (this->NumArguments() < args.Size()) { return false; }
	return MatchMethod<typename TraitsType::MethodType, typename TraitsType::FunctionType>::Match(
		this,
		args
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodBase<TMethodTraits>::MethodBase()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits>
inline MethodBase<TMethodTraits>::MethodBase(
	const ATTRIBUTEARRAY           &attributes,
	const NAME                     &name,
	PtrType                        ptr,
	const IFunction::ARGUMENTARRAY &args
) :
TMethodTraits::MethodType(attributes, name, args),
m_ptr(ptr)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MutableMethod<TMethodTraits>::Call()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits>
void Method<TMethodTraits>::Call(
	const Reflected     &result,
	VoidType            *object,
	const IArgumentList &args
) const
{
	RAD_ASSERT(NULL != this->m_ptr);
	RAD_ASSERT(NULL != object);
	ObjectType *obj = (ObjectType *)object;
	if (this->NumArguments() < args.Size()) { ThrowInvalidArgumentException(this->NumArguments()); }
	MethodCaller<ObjectType, PtrType, typename TraitsType::FunctionType>::Call(
		this,
		*obj,
		result,
		this->m_ptr,
		args
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Method<TMethodTraits>::Method()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits>
inline Method<TMethodTraits>::Method(
	const ATTRIBUTEARRAY           &attributes,
	const NAME                     &name,
	PtrType                        ptr,
	const IFunction::ARGUMENTARRAY &args
) :
MethodBase<TMethodTraits>(attributes, name, ptr, args)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Method<MethodTraits<TFunction, TOwner, true>, I::Call()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner>
void Method<MethodTraits<TFunction, TOwner, true> >::Call(
	const Reflected     &result,
	const IArgumentList &args
) const
{
	RAD_ASSERT(NULL != this->m_ptr);
	if (this->NumArguments() < args.Size()) { ThrowInvalidArgumentException(this->NumArguments()); }
	MethodCaller<void, PtrType, typename TraitsType::FunctionType>::Call(
		this,
		result,
		this->m_ptr,
		args
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Method<MethodTraits<TFunction, TOwner, true>>::Method()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner>
inline Method<MethodTraits<TFunction, TOwner, true> >::Method(
	const ATTRIBUTEARRAY           &attributes,
	const NAME                     &name,
	PtrType                        ptr,
	const IFunction::ARGUMENTARRAY &args
) :
MethodBase<MethodTraits<TFunction, TOwner, true> >(attributes, name, ptr, args)
{
}

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
inline typename BindRequiredArgumentTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>::ResultType BindRequiredArgumentTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>::BindArgument(
	const InitType &methodInitializer,
	const TBind    &argumentInitializer
)
{
	return methodInitializer.BindRequiredArgument(argumentInitializer);
}

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
inline typename BindDefaultArgumentTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>::ResultType BindDefaultArgumentTraits<
	MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>::BindArgument(
	const InitType &methodInitializer,
	const TBind    &argumentInitializer
)
{
	return methodInitializer.BindDefaultArgument(argumentInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::Method()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, typename TAttributeTuple, typename TArgumentTuple>
inline MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::MethodInitializer(
	const AttributeTupleType &attributes,
	const NAME               &name,
	PtrType                  ptr,
	const ArgTupleType       &a
) :
m_attributes(attributes),
m_name(name),
m_ptr(ptr),
m_arguments(a)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::Instance<I>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, typename TAttributeTuple, typename TArgumentTuple>
template <int I>
inline const typename TMethodTraits::MethodType *MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::Instance() const
{
	static MethodType method(
		InitializeAttributes<TraitsType, AttributeTupleType, -1, I>(m_attributes),
		m_name,
		m_ptr,
		TraitsType::template InitializeArguments<I, TArgumentTuple>(m_arguments)
	);
	return &method;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodInitializer<TMethodTraits, TArgumentTuple>::BindArgument<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, typename TAttributeTuple, typename TArgumentTuple>
template <typename T>
inline typename BindArgumentTraits<
		MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>, T
>::ResultType MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::BindArgument(
	const T &argumentInitializer
) const
{
	return BindArgumentTraits<SelfType, T>::BindArgument(*this, argumentInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodInitializer<TMethodTraits, TArgumentTuple>::BindRequiredArgument<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, typename TAttributeTuple, typename TArgumentTuple>
template <typename T>
inline typename BindRequiredArgumentTraits<
		MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>, T
>::ResultType MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::BindRequiredArgument(
	const T &argumentInitializer
) const
{
	return typename BindRequiredArgumentTraits<SelfType, T>::ResultType(
		m_attributes,
		m_name,
		NULL,
		m_arguments + argumentInitializer
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::BindDefaultArgument<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, typename TAttributeTuple, typename TArgumentTuple>
template <typename T>
inline typename BindDefaultArgumentTraits<
		MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>, T
>::ResultType MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::BindDefaultArgument(
	const T &argumentInitializer
) const
{
	return typename BindDefaultArgumentTraits<SelfType, T>::ResultType(
		m_attributes,
		m_name,
		NULL,
		m_arguments + argumentInitializer
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::BindMethod()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, typename TAttributeTuple, typename TArgumentTuple>
inline MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple> MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::BindMethod(
	const NAME &name,
	PtrType    ptr
) const
{
	return SelfType(m_attributes, name, ptr, m_arguments);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodInitializer<TMemberTraits, TAttributeTuple, TArgumentTuple>::BindAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, typename TAttributeTuple, typename TArgumentTuple>
template <typename T>
inline typename BindAttributeTraits<
		MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>, T
>::ResultType MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::BindAttribute(
	const T &attributeInitializer
) const
{
	return typename BindAttributeTraits<SelfType, T>::ResultType(
		m_attributes + attributeInitializer,
		m_name,
		m_ptr,
		m_arguments
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::BindArgumentAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, typename TAttributeTuple, typename TArgumentTuple>
template <typename T>
inline typename BindArgumentAttributeTraits<
		MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>, T
>::ResultType MethodInitializer<TMethodTraits, TAttributeTuple, TArgumentTuple>::BindArgumentAttribute(
	const T &attributeInitializer
) const
{
	return typename BindArgumentAttributeTraits<SelfType, T>::ResultType(
		m_attributes,
		m_name,
		m_ptr,
		m_arguments.Tail() + m_arguments.Head().BindAttribute(attributeInitializer)
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodTraits<TFunction, TOwner, IsStatic>::CreateInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner, bool IsStatic>
inline MethodInitializer<
	MethodTraits<TFunction, TOwner, IsStatic>,
	TupleTraits<>::Type
> MethodTraits<TFunction, TOwner, IsStatic>::CreateInitializer()
{
	return MethodInitializer<SelfType, TupleTraits<>::Type>(
		::MakeTuple(),
		NAME(),
		NULL,
		MakeTuple()
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodTraits<TFunction, TOwner, IsStatic>::InitializeArguments<I, TArgumentTuple>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner, bool IsStatic>
template <int I, typename TArgumentTuple>
inline const Class::METHOD::ARGUMENT **MethodTraits<TFunction, TOwner, IsStatic>::InitializeArguments(const TArgumentTuple &a)
{
	static const Class::METHOD::ARGUMENT *arguments[TArgumentTuple::SIZE + 1];
	arguments[TArgumentTuple::SIZE] = NULL;
	RecurseArguments<SelfType, I, TArgumentTuple, TArgumentTuple::SIZE - 1>::Initialize(arguments, a);
	return arguments;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseMethods<TMethodTuple, J, I>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple, int J, int I>
inline void RecurseMethods<TMethodTuple, J, I>::Initialize(const Class::MUTABLEMETHOD **methods, const TMethodTuple &t)
{
	GetInstance<MATCH, Class::MUTABLEMETHOD, TMethodTuple, J>::Execute(methods, t);
	RecurseMethods<typename TMethodTuple::TailType, NEXT_J, I - 1>::Initialize(methods, t.Tail());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseMethods<TMethodTuple, J, -1>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple, int J>
inline void RecurseMethods<TMethodTuple, J, -1>::Initialize(const Class::MUTABLEMETHOD **methods, const TMethodTuple &t)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseConstMethods<TMethodTuple, J, I>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple, int J, int I>
inline void RecurseConstMethods<TMethodTuple, J, I>::Initialize(const Class::CONSTMETHOD **methods, const TMethodTuple &t)
{
	GetInstance<MATCH, Class::CONSTMETHOD, TMethodTuple, J>::Execute(methods, t);
	RecurseConstMethods<typename TMethodTuple::TailType, NEXT_J, I - 1>::Initialize(methods, t.Tail());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseConstMethods<TMethodTuple, J, -1>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple, int J>
inline void RecurseConstMethods<TMethodTuple, J, -1>::Initialize(const Class::CONSTMETHOD **methods, const TMethodTuple &t)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseStaticMethods<TMethodTuple, J, I>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple, int J, int I>
inline void RecurseStaticMethods<TMethodTuple, J, I>::Initialize(const Class::STATICMETHOD **methods, const TMethodTuple &t)
{
	GetInstance<MATCH, Class::STATICMETHOD, TMethodTuple, J>::Execute(methods, t);
	RecurseStaticMethods<typename TMethodTuple::TailType, NEXT_J, I - 1>::Initialize(methods, t.Tail());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseStaticMethods<TMethodTuple, J, -1>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTuple, int J>
inline void RecurseStaticMethods<TMethodTuple, J, -1>::Initialize(const Class::STATICMETHOD **methods, const TMethodTuple &t)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodCaller<T, TMethodPtr, TFunction, TReturn>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename TMethodPtr, typename TFunction, typename TReturn>
inline void MethodCaller<T, TMethodPtr, TFunction, TReturn>::Call(
	const Class::METHOD  *method,
	T                    &object,
	const Reflected      &result,
	TMethodPtr            ptr,
	const IArgumentList  &args
)
{
	TReturn *res = result ? (TReturn *)result : 0;
	if (NULL == res)
	{
		CallMethod<Class::METHOD, T, TMethodPtr, TFunction>::Call(
			method,
			object,
			ptr,
			args
		);
	}
	else
	{
		*res = CallMethod<Class::METHOD, T, TMethodPtr, TFunction>::Call(
			method,
			object,
			ptr,
			args
		);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodCaller<T, TMethodPtr, TFunction, void>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename TMethodPtr, typename TFunction>
inline void MethodCaller<T, TMethodPtr, TFunction, void>::Call(
	const Class::METHOD  *method,
	T                    &object,
	const Reflected      &result,
	TMethodPtr            ptr,
	const IArgumentList  &args
)
{
	CallMethod<Class::METHOD, T, TMethodPtr, TFunction>::Call(
		method,
		object,
		ptr,
		args
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodCaller<void, TMethodPtr, TFunction, TReturn>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodPtr, typename TFunction, typename TReturn>
inline void MethodCaller<void, TMethodPtr, TFunction, TReturn>::Call(
	const Class::METHOD  *method,
	const Reflected      &result,
	TMethodPtr            ptr,
	const IArgumentList  &args
)
{
	TReturn *res = result ? (TReturn *)result : 0;
	if (NULL == res)
	{
		CallMethod<Class::METHOD, void, TMethodPtr, TFunction>::Call(
			method,
			ptr,
			args
		);
	}
	else
	{
		*res = CallMethod<Class::METHOD, void, TMethodPtr, TFunction>::Call(
			method,
			ptr,
			args
		);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MethodCaller<void, TMethodPtr, TArgumentTuple, void>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodPtr, typename TFunction>
inline void MethodCaller<void, TMethodPtr, TFunction, void>::Call(
	const Class::METHOD *method,
	const Reflected     &result,
	TMethodPtr           ptr,
	const IArgumentList &args
)
{
	CallMethod<Class::METHOD, void, TMethodPtr, TFunction>::Call(
		method,
		ptr,
		args
	);
}

} // details
} // reflect

