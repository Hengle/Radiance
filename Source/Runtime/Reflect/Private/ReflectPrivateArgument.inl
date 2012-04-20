// ReflectPrivateArgument.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Argument<TValue, IS_REFERENCE, IS_CONST>::IsConst()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue, bool IS_REFERENCE, bool IS_CONST>
bool Argument<TValue, IS_REFERENCE, IS_CONST>::IsConst() const
{
	return IS_CONST;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Argument<TValue, IS_REFERENCE, IS_CONST>::IsReference()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue, bool IS_REFERENCE, bool IS_CONST>
bool Argument<TValue, IS_REFERENCE, IS_CONST>::IsReference() const
{
	return IS_REFERENCE;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Argument<TValue, IS_REFERENCE, IS_CONST>::Type()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue, bool IS_REFERENCE, bool IS_CONST>
const Class *Argument<TValue, IS_REFERENCE, IS_CONST>::Type() const
{
	return reflect::Type<TValue>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Argument<TValue, IS_REFERENCE, IS_CONST>::DefaultValue()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue, bool IS_REFERENCE, bool IS_CONST>
const void *Argument<TValue, IS_REFERENCE, IS_CONST>::DefaultValue() const
{
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Argument<TArgumentTraits>::Argument()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue, bool IS_REFERENCE, bool IS_CONST>
inline Argument<TValue, IS_REFERENCE, IS_CONST>::Argument(
	const ATTRIBUTEARRAY &attributes,
	const NAME &name
) :
IFunction::ARGUMENT(attributes, name)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::DefaultArgument<TArgumentTraits>::DefaultValue()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue, bool IS_REFERENCE, bool IS_CONST>
const void *DefaultArgument<TValue, IS_REFERENCE, IS_CONST>::DefaultValue() const
{
	return static_cast<const void *>(&m_default);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::DefaultArgument<TArgumentTraits>::DefaultArgument()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TValue, bool IS_REFERENCE, bool IS_CONST>
inline DefaultArgument<TValue, IS_REFERENCE, IS_CONST>::DefaultArgument(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	const TValue         &defaultVal
) :
Argument<TValue, IS_REFERENCE, IS_CONST>(attributes, name),
m_default(defaultVal)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentInitializer<TArgumentTraits, TAttributeTuple>::Instance<TMethodTraits, J, I>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TArgumentTraits, typename TAttributeTuple>
template <typename TMethodTraits, int J, int I>
inline const IFunction::ARGUMENT *ArgumentInitializer<TArgumentTraits, TAttributeTuple>::Instance() const
{
	static ArgumentType argument(
		InitializeAttributes<TMethodTraits, AttributeTupleType, J, I>(m_attributes),
		m_name
	);
	return &argument;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentInitializer<TArgumentTraits, TAttributeTuple>::BindAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TArgumentTraits, typename TAttributeTuple>
template <typename T>
inline typename BindAttributeTraits<
		ArgumentInitializer<TArgumentTraits, TAttributeTuple>, T
>::ResultType ArgumentInitializer<TArgumentTraits, TAttributeTuple>::BindAttribute(
	const T &attributeInitializer
) const
{
	return typename BindAttributeTraits<SelfType, T>::ResultType(
		m_attributes + attributeInitializer,
		m_name
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentInitializer<TArgumentTraits, TAttributeTuple>::ArgumentInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TArgumentTraits, typename TAttributeTuple>
inline ArgumentInitializer<TArgumentTraits, TAttributeTuple>::ArgumentInitializer(
	const AttributeTupleType &attributes,
	const NAME               &name
) :
m_attributes(attributes),
m_name(name)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple>::Instance<TMethodTraits, J, I>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TArgumentTraits, typename TAttributeTuple>
template <typename TMethodTraits, int J, int I>
inline const IFunction::ARGUMENT *DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple>::Instance() const
{
	static ArgumentType argument(
		InitializeAttributes<TMethodTraits, AttributeTupleType, J, I>(this->m_attributes),
		this->m_name,
		this->m_default
	);
	return &argument;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentInitializer<TArgumentTraits, TAttributeTuple>::BindAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TArgumentTraits, typename TAttributeTuple>
template <typename T>
inline typename BindAttributeTraits<
		DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple>, T
>::ResultType DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple>::BindAttribute(
	const T &attributeInitializer
) const
{
	return typename BindAttributeTraits<SelfType, T>::ResultType(
		this->m_attributes + attributeInitializer,
		this->m_name,
		this->m_default
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple>::DefaultArgumentInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TArgumentTraits, typename TAttributeTuple>
inline DefaultArgumentInitializer<TArgumentTraits, TAttributeTuple>::DefaultArgumentInitializer(
	const AttributeTupleType &attributes,
	const NAME               &name,
	const ValueType          &defaultVal
) :
SuperType(attributes, name),
m_default(defaultVal)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentTraits<T>::CreateInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline ArgumentInitializer<ArgumentTraits<T> > ArgumentTraits<T>::CreateInitializer(const NAME &name)
{
	return ArgumentInitializer<SelfType>(MakeTuple(), name);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentTraits<T>::CreateDefaultInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline DefaultArgumentInitializer<ArgumentTraits<T> > ArgumentTraits<T>::CreateDefaultInitializer(
	const NAME      &name,
	const ValueType &defaultVal
)
{
	return DefaultArgumentInitializer<SelfType>(MakeTuple(), name, defaultVal);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseArguments<TMethodTraits, J, TArgumentTuple, I>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, int J, typename TArgumentTuple, int I>
inline void RecurseArguments<TMethodTraits, J, TArgumentTuple, I>::Initialize(const IFunction::ARGUMENT **arguments, const TArgumentTuple &t)
{
	typedef typename TArgumentTuple::HeadType HeadType;
	arguments[I] = t.Head().HeadType::template Instance<TMethodTraits, J, I>();
	RecurseArguments<TMethodTraits, J, typename TArgumentTuple::TailType, I - 1>::Initialize(arguments, t.Tail());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseArguments<TMethodTraits, J, TArgumentTuple, -1>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethodTraits, int J, typename TArgumentTuple>
inline void RecurseArguments<TMethodTraits, J, TArgumentTuple, -1>::Initialize(const IFunction::ARGUMENT **arguments, const TArgumentTuple &t)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtractValue<TMethod, T, false, false>::Value()
//////////////////////////////////////////////////////////////////////////////////////////
//
// value
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
inline T ArgumentExtractValue<TMethod, T, false, false>::Value(
	const TMethod *method,
	const IArgumentList &args,
	int           i
)
{
	RAD_ASSERT(NULL != method);
	RAD_ASSERT(i < method->NumArguments());
	if (i < args.Size())
	{
		Reflected arg = args.Argument(i);
		T *result = (T *)arg;
		RAD_ASSERT(result);
		return *result;
	}
	else
	{
		const IFunction::ARGUMENT *arg = method->Argument(i);
		RAD_ASSERT(arg != NULL);
		const T *result = static_cast<const T *>(arg->DefaultValue());
		RAD_ASSERT(result);
		return *result;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtractValue<TMethod, T, false, true>::Value()
//////////////////////////////////////////////////////////////////////////////////////////
//
// const value
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
inline const T ArgumentExtractValue<TMethod, T, false, true>::Value(
	const TMethod *method,
	const IArgumentList &args,
	int           i
)
{
	RAD_ASSERT(NULL != method);
	RAD_ASSERT(i < method->NumArguments());
	if (i < args.Size())
	{
		ConstReflected arg = args.ConstArgument(i);
		const T *result = (const T *)arg;
		RAD_ASSERT(result);
		return *result;
	}
	else
	{
		const IFunction::ARGUMENT *arg = method->Argument(i);
		RAD_ASSERT(arg != NULL);
		const T *result = static_cast<const T *>(arg->DefaultValue());
		RAD_ASSERT(result);
		return *result;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtractValue<TMethod, T, true, false>::Value()
//////////////////////////////////////////////////////////////////////////////////////////
//
// reference
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
inline T &ArgumentExtractValue<TMethod, T, true, false>::Value(
	const TMethod *method,
	const IArgumentList &args,
	int           i
)
{
	RAD_ASSERT(NULL != method);
	RAD_ASSERT(i < method->NumArguments());
	if (i < args.Size())
	{
		Reflected arg = args.Argument(i);
		T *result = (T *)arg;
		RAD_ASSERT(result);
		return *result;
	}
	else
	{
		method->ThrowMissingArgumentException(i);
		T *result = NULL;
		return *result;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtractValue<TMethod, T, true, true>::Value()
//////////////////////////////////////////////////////////////////////////////////////////
//
// const reference
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
inline const T &ArgumentExtractValue<TMethod, T, true, true>::Value(
	const TMethod *method,
	const IArgumentList &args,
	int           i
)
{
	RAD_ASSERT(NULL != method);
	RAD_ASSERT(i < method->NumArguments());
	if (i < args.Size())
	{
		ConstReflected arg = args.ConstArgument(i);
		const T *result = (const T *)arg;
		RAD_ASSERT(result);
		return *result;
	}
	else
	{
		method->ThrowMissingArgumentException(i);
		const T *result = NULL;
		return *result;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentExtract<TMethod, TFunction, I>::Value()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename TFunction, int I>
inline typename ArgumentExtract<TMethod, TFunction, I>::ValueType ArgumentExtract<TMethod, TFunction, I>::Value(
	const TMethod *method,
	const IArgumentList &args
)
{
	return ExtractType::Value(method, args, I);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MatchArgument<TMethod, T, false, false>::Match()
//////////////////////////////////////////////////////////////////////////////////////////
//
// value
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
inline bool MatchArgument<TMethod, T, false, false>::Match(
	const TMethod *method,
	const ITypeList &args,
	int           i
)
{
	RAD_ASSERT(NULL != method);
	RAD_ASSERT(i < method->NumArguments());
	if (i < args.Size())
	{
		return args.Type(i)->ConstType()->IsA(Type<T>()->ConstType());
	}
	else
	{
		const IFunction::ARGUMENT *arg = method->Argument(i);
		RAD_ASSERT(arg != NULL);
		return NULL != arg->DefaultValue();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MatchArgument<TMethod, T, false, true>::Match()
//////////////////////////////////////////////////////////////////////////////////////////
//
// const value
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
inline bool MatchArgument<TMethod, T, false, true>::Match(
	const TMethod *method,
	const ITypeList &args,
	int           i
)
{
	RAD_ASSERT(NULL != method);
	RAD_ASSERT(i < method->NumArguments());
	if (i < args.Size())
	{
		return args.Type(i)->ConstType()->IsA(Type<T>()->ConstType());
	}
	else
	{
		const IFunction::ARGUMENT *arg = method->Argument(i);
		RAD_ASSERT(arg != NULL);
		return NULL != arg->DefaultValue();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MatchArgument<TMethod, T, true, false>::Match()
//////////////////////////////////////////////////////////////////////////////////////////
//
// reference
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
inline bool MatchArgument<TMethod, T, true, false>::Match(
	const TMethod *method,
	const ITypeList &args,
	int           i
)
{
	RAD_ASSERT(NULL != method);
	RAD_ASSERT(i < method->NumArguments());
	if (i < args.Size())
	{
		return args.Type(i)->IsA(Type<T>());
	}
	else
	{
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MatchArgument<TMethod, T, true, true>::Match()
//////////////////////////////////////////////////////////////////////////////////////////
//
// const reference
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename T>
inline bool MatchArgument<TMethod, T, true, true>::Match(
	const TMethod *method,
	const ITypeList &args,
	int           i
)
{
	RAD_ASSERT(NULL != method);
	RAD_ASSERT(i < method->NumArguments());
	if (i < args.Size())
	{
		return args.Type(i)->ConstType()->IsA(Type<T>()->ConstType());
	}
	else
	{
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ArgumentMatch<TMethod, TFunction, I>::Match()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TMethod, typename TFunction, int I>
inline bool ArgumentMatch<TMethod, TFunction, I>::Match(
	const TMethod *method,
	const ITypeList &args
)
{
	return MatchType::Match(method, args, I);
}

} // details
} // reflect

