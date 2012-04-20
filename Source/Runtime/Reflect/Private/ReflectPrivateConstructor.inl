// ReflectPrivateConstructor.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Constructor<TConstructorTraits>::OwnerType()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits>
const Class *Constructor<TConstructorTraits>::OwnerType() const
{
	return reflect::Type<ObjectType>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Constructor<TConstructorTraits>::Match()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits>
bool Constructor<TConstructorTraits>::Match(const ITypeList &args) const
{
	if (NumArguments() < args.Size()) { return false; }
	return MatchMethod<Class::CONSTRUCTOR, typename TraitsType::FunctionType>::Match(
		this,
		args
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Constructor<TConstructorTraits>::Call()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits>
void Constructor<TConstructorTraits>::Call(
	void          *object,
	const IArgumentList &args
) const
{
	RAD_ASSERT(NULL != object);
	if (NumArguments() < args.Size()) { ThrowInvalidArgumentException(NumArguments()); }
	CallConstructor<Class::CONSTRUCTOR, ObjectType, typename TraitsType::FunctionType>::Call(
		this,
		object,
		args
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Constructor<TConstructorTraits>::Constructor()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits>
inline Constructor<TConstructorTraits>::Constructor(
	const ATTRIBUTEARRAY           &attributes,
	const IFunction::ARGUMENTARRAY &args
) :
Class::CONSTRUCTOR(attributes, args)
{
}

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
inline typename BindRequiredArgumentTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>::ResultType BindRequiredArgumentTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>::BindArgument(
	const InitType &constructorInitializer,
	const TBind    &argumentInitializer
)
{
	return constructorInitializer.BindRequiredArgument(argumentInitializer);
}

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
inline typename BindDefaultArgumentTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>::ResultType BindDefaultArgumentTraits<
	ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>,
	TBind
>::BindArgument(
	const InitType &constructorInitializer,
	const TBind    &argumentInitializer
)
{
	return constructorInitializer.BindDefaultArgument(argumentInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::ConstructorInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits, typename TAttributeTuple, typename TArgumentTuple>
inline ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::ConstructorInitializer(
	const AttributeTupleType &attributes,
	const ArgTupleType       &arguments
) :
m_attributes(attributes),
m_arguments(arguments)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorInitializer<TConstructorTraits, TArgumentTuple>::Instance<I>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits, typename TAttributeTuple, typename TArgumentTuple>
template <int I>
inline const typename Class::CONSTRUCTOR *ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::Instance() const
{
	static ConstructorType constructor(
		InitializeAttributes<TraitsType, TAttributeTuple, -1, I>(m_attributes),
		TraitsType::template InitializeArguments<I, TArgumentTuple>(m_arguments)
	);
	return &constructor;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorInitializer<TConstructorTraits, TArgumentTuple>::BindArgument<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits, typename TAttributeTuple, typename TArgumentTuple>
template <typename T>
inline typename BindArgumentTraits<
		ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>, T
>::ResultType ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::BindArgument(
	const T &argumentInitializer
) const
{
	return BindArgumentTraits<SelfType, T>::BindArgument(*this, argumentInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorInitializer<TConstructorTraits, TArgumentTuple>::BindRequiredArgument<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits, typename TAttributeTuple, typename TArgumentTuple>
template <typename T>
inline typename BindRequiredArgumentTraits<
		ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>, T
>::ResultType ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::BindRequiredArgument(
	const T &argumentInitializer
) const
{
	return typename BindRequiredArgumentTraits<SelfType, T>::ResultType(
		m_attributes,
		m_arguments + argumentInitializer
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::BindDefaultArgument<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits, typename TAttributeTuple, typename TArgumentTuple>
template <typename T>
inline typename BindDefaultArgumentTraits<
		ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>, T
>::ResultType ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::BindDefaultArgument(
	const T &argumentInitializer
) const
{
	return BindDefaultArgumentTraits<SelfType, T>::ResultType(
		m_attributes,
		m_arguments + argumentInitializer
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::BindAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits, typename TAttributeTuple, typename TArgumentTuple>
template <typename T>
inline typename BindAttributeTraits<
		ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>, T
>::ResultType ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::BindAttribute(
	const T &attributeInitializer
) const
{
	return typename BindAttributeTraits<SelfType, T>::ResultType(
		m_attributes + attributeInitializer,
		m_arguments
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::BindArgumentAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTraits, typename TAttributeTuple, typename TArgumentTuple>
template <typename T>
inline typename BindArgumentAttributeTraits<
		ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>, T
>::ResultType ConstructorInitializer<TConstructorTraits, TAttributeTuple, TArgumentTuple>::BindArgumentAttribute(
	const T &attributeInitializer
) const
{
	return typename BindArgumentAttributeTraits<SelfType, T>::ResultType(
		m_attributes,
		m_arguments.Tail() + m_arguments.Head().BindAttribute(attributeInitializer)
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorTraits<TFunction, TOwner>::CreateInitializer()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner>
inline ConstructorInitializer<
	ConstructorTraits<TFunction, TOwner>
> ConstructorTraits<TFunction, TOwner>::CreateInitializer()
{
	return ConstructorInitializer<SelfType, TupleTraits<>::Type>(
		MakeTuple(),
		MakeTuple()
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstructorTraits<TFunction, TOwner>::InitializeArguments<I, TArgumentTuple>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TFunction, typename TOwner>
template <int I, typename TArgumentTuple>
inline const Class::CONSTRUCTOR::ARGUMENT **ConstructorTraits<TFunction, TOwner>::InitializeArguments(const TArgumentTuple &t)
{
	static const Class::CONSTRUCTOR::ARGUMENT *arguments[TArgumentTuple::SIZE + 1];
	arguments[TArgumentTuple::SIZE] = NULL;
	RecurseArguments<SelfType, I, TArgumentTuple, TArgumentTuple::SIZE - 1>::Initialize(arguments, t);
	return arguments;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseConstructors<TConstructorTuple, J, I>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTuple, int J, int I>
inline void RecurseConstructors<TConstructorTuple, J, I>::Initialize(const Class::CONSTRUCTOR **constructors, const TConstructorTuple &t)
{
	GetInstance<true, Class::CONSTRUCTOR, TConstructorTuple, J>::Execute(constructors, t);
	RecurseConstructors<typename TConstructorTuple::TailType, NEXT_J, I - 1>::Initialize(constructors, t.Tail());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RecurseConstructors<TConstructorTuple, J, -1>::Initialize()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TConstructorTuple, int J>
inline void RecurseConstructors<TConstructorTuple, J, -1>::Initialize(const Class::CONSTRUCTOR **constructors, const TConstructorTuple &t)
{
}

} // details
} // reflect

