// ReflectPrivateClass.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#include <boost/type_traits/is_abstract.hpp>
#include <boost/type_traits/alignment_of.hpp>


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::METHODS::METHODS()
//////////////////////////////////////////////////////////////////////////////////////////

inline METHODS::METHODS() :
methods(),
constMethods(),
staticMethods()
{
}

inline METHODS::METHODS(
	const Class::MUTABLEMETHODARRAY &m,
	const Class::CONSTMETHODARRAY   &c,
	const Class::STATICMETHODARRAY  &s
) :
methods(m),
constMethods(c),
staticMethods(s)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MEMBERS::MEMBERS()
//////////////////////////////////////////////////////////////////////////////////////////

inline MEMBERS::MEMBERS() :
members(),
staticMembers(),
staticConstants()
{
}

inline MEMBERS::MEMBERS(
	const Class::MEMBERARRAY         &m,
	const Class::STATICMEMBERARRAY   &s,
	const Class::STATICCONSTANTARRAY &c
) :
members(m),
staticMembers(s),
staticConstants(c)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::CLASSDESCRIPTOR::CLASSDESCRIPTOR()
//////////////////////////////////////////////////////////////////////////////////////////

inline CLASSDESCRIPTOR::CLASSDESCRIPTOR() :
attributes(),
name(),
size(0),
alignment(0),
supers(),
members(),
constructors(),
methods(),
dtor(NULL)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindConstructorArgumentTraits<TTraits, TBind>::BindArgument()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
inline typename BindConstructorArgumentTraits<TTraits, TBind>::ResultType BindConstructorArgumentTraits<TTraits, TBind>::BindArgument(
	const ClassDescriptor<TTraits> &descriptor,
	const TBind                    &argumentInitializer
)
{
	return descriptor.BindConstructorArgument(argumentInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindMethodArgumentTraits<TTraits, TBind>::BindArgument()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
inline typename BindMethodArgumentTraits<TTraits, TBind>::ResultType BindMethodArgumentTraits<TTraits, TBind>::BindArgument(
	const ClassDescriptor<TTraits> &descriptor,
	const TBind                    &argumentInitializer
)
{
	return descriptor.BindMethodArgument(argumentInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindSuperAttributeTraits<TTraits, TBind>::BindAttribute()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
inline typename BindSuperAttributeTraits<TTraits, TBind>::ResultType BindSuperAttributeTraits<TTraits, TBind>::BindAttribute(
	const ClassDescriptor<TTraits> &descriptor,
	const TBind                    &attributeInitializer
)
{
	return descriptor.template BindSuperAttribute<TBind>(attributeInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindMemberAttributeTraits<TTraits, TBind>::BindAttribute()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
inline typename BindMemberAttributeTraits<TTraits, TBind>::ResultType BindMemberAttributeTraits<TTraits, TBind>::BindAttribute(
	const ClassDescriptor<TTraits> &descriptor,
	const TBind                    &attributeInitializer
)
{
	return descriptor.template BindMemberAttribute<TBind>(attributeInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindConstructorAttributeTraits<TTraits, TBind>::BindAttribute()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
inline typename BindConstructorAttributeTraits<TTraits, TBind>::ResultType BindConstructorAttributeTraits<TTraits, TBind>::BindAttribute(
	const ClassDescriptor<TTraits> &descriptor,
	const TBind                    &attributeInitializer
)
{
	return descriptor.template BindConstructorAttribute<TBind>(attributeInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindConstructorArgumentAttributeTraits<TTraits, TBind>::BindAttribute()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
inline typename BindConstructorArgumentAttributeTraits<TTraits, TBind>::ResultType BindConstructorArgumentAttributeTraits<TTraits, TBind>::BindAttribute(
	const ClassDescriptor<TTraits> &descriptor,
	const TBind                    &attributeInitializer
)
{
	return descriptor.template BindConstructorArgumentAttribute<TBind>(attributeInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindMethodAttributeTraits<TTraits, TBind>::BindAttribute()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
inline typename BindMethodAttributeTraits<TTraits, TBind>::ResultType BindMethodAttributeTraits<TTraits, TBind>::BindAttribute(
	const ClassDescriptor<TTraits> &descriptor,
	const TBind                    &attributeInitializer
)
{
	return descriptor.template BindMethodAttribute<TBind>(attributeInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::BindMethodArgumentAttributeTraits<TTraits, TBind>::BindAttribute()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, typename TBind>
inline typename BindMethodArgumentAttributeTraits<TTraits, TBind>::ResultType BindMethodArgumentAttributeTraits<TTraits, TBind>::BindAttribute(
	const ClassDescriptor<TTraits> &descriptor,
	const TBind                    &attributeInitializer
)
{
	return descriptor.template BindMethodArgumentAttribute<TBind>(attributeInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::ClassDescriptor()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
inline ClassDescriptor<TTraits>::ClassDescriptor(const NAME &name) :
m_name(name)
{
}

template <typename TTraits>
inline ClassDescriptor<TTraits>::ClassDescriptor(
	const AttributeTupleType   &attributes,
	const NAME                 &name,
	const SuperTupleType       &supers,
	const MemberTupleType      &members,
	const ConstructorTupleType &constructors,
	const MethodTupleType      &methods
) :
m_attributes(attributes),
m_name(name),
m_supers(supers),
m_members(members),
m_constructors(constructors),
m_methods(methods)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::Descriptor()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TClass, bool isabs>
struct AlignOf
{
	static const int value = boost::alignment_of<TClass>::value;
};

template <typename TClass>
struct AlignOf<TClass, true>
{
	static const int value = 0;
};

template <typename TTraits>
inline CLASSDESCRIPTOR ClassDescriptor<TTraits>::Descriptor() const
{
	CLASSDESCRIPTOR descriptor;
	descriptor.attributes = InitializeAttributes<TTraits, AttributeTupleType, -1, -1>(m_attributes);
	descriptor.name = m_name;
	descriptor.size = sizeof(typename TTraits::ClassType);
	descriptor.alignment = AlignOf
		<typename TTraits::ClassType, 
		boost::is_abstract<typename TTraits::ClassType>::value >::value;
	descriptor.supers = InitializeSupers();
	descriptor.members = InitializeMembers();
	descriptor.constructors = InitializeConstructors();
	descriptor.methods = InitializeMethods();
	descriptor.dtor = NULL;
	return descriptor;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindSuper<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindSuperTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindSuper(const T &superInitializer) const
{
	RAD_STATIC_ASSERT(
		STATE != CLASSDESCRIPTOR_STATE::CONSTRUCTOR_BEGIN &&
		STATE != CLASSDESCRIPTOR_STATE::CONSTRUCTOR_ARGUMENT &&
		STATE != CLASSDESCRIPTOR_STATE::METHOD_BEGIN &&
		STATE != CLASSDESCRIPTOR_STATE::METHOD_ARGUMENT
	);

	return typename BindSuperTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers + superInitializer,
		m_members,
		m_constructors,
		m_methods
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindMember<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindMemberTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindMember(const T &memberInitializer) const
{
	RAD_STATIC_ASSERT(
		STATE != CLASSDESCRIPTOR_STATE::CONSTRUCTOR_BEGIN &&
		STATE != CLASSDESCRIPTOR_STATE::CONSTRUCTOR_ARGUMENT &&
		STATE != CLASSDESCRIPTOR_STATE::METHOD_BEGIN &&
		STATE != CLASSDESCRIPTOR_STATE::METHOD_ARGUMENT
	);

	return typename BindMemberTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members + memberInitializer,
		m_constructors,
		m_methods
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BeginConstructor<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BeginConstructorTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BeginConstructor(const T &constructorInitializer) const
{
	RAD_STATIC_ASSERT(
		STATE != CLASSDESCRIPTOR_STATE::CONSTRUCTOR_BEGIN &&
		STATE != CLASSDESCRIPTOR_STATE::CONSTRUCTOR_ARGUMENT &&
		STATE != CLASSDESCRIPTOR_STATE::METHOD_BEGIN &&
		STATE != CLASSDESCRIPTOR_STATE::METHOD_ARGUMENT
	);

	return typename BeginConstructorTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members,
		m_constructors + constructorInitializer,
		m_methods
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::EndConstructor<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
inline typename EndConstructorTraits<TTraits>::ResultType ClassDescriptor<TTraits>::EndConstructor() const
{
	RAD_STATIC_ASSERT(
		STATE == CLASSDESCRIPTOR_STATE::CONSTRUCTOR_BEGIN ||
		STATE == CLASSDESCRIPTOR_STATE::CONSTRUCTOR_ARGUMENT
	);

	return typename EndConstructorTraits<TTraits>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members,
		m_constructors,
		m_methods
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BeginMethod<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BeginMethodTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BeginMethod(const T &methodInitializer) const
{
	RAD_STATIC_ASSERT(
		STATE != CLASSDESCRIPTOR_STATE::CONSTRUCTOR_BEGIN &&
		STATE != CLASSDESCRIPTOR_STATE::CONSTRUCTOR_ARGUMENT &&
		STATE != CLASSDESCRIPTOR_STATE::METHOD_BEGIN &&
		STATE != CLASSDESCRIPTOR_STATE::METHOD_ARGUMENT
	);

	return typename BeginMethodTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members,
		m_constructors,
		m_methods + methodInitializer
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::EndMethod<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
inline typename EndMethodTraits<TTraits>::ResultType ClassDescriptor<TTraits>::EndMethod(
	const NAME                                       &name,
	typename EndMethodTraits<TTraits>::MethodPtrType method
) const
{
	RAD_STATIC_ASSERT(
		STATE == CLASSDESCRIPTOR_STATE::METHOD_BEGIN ||
		STATE == CLASSDESCRIPTOR_STATE::METHOD_ARGUMENT
	);

	return typename EndMethodTraits<TTraits>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members,
		m_constructors,
		m_methods.Tail() + m_methods.Head().BindMethod(name, method)
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindArgument<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindArgumentTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindArgument(
	const T &argumentInitializer
) const
{
	return BindArgumentTraits<TTraits, T>::BindArgument(*this, argumentInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindConstructorArgument<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindConstructorArgumentTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindConstructorArgument(
	const T &argumentInitializer
) const
{
	return typename BindConstructorArgumentTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members,
		m_constructors.Tail() + m_constructors.Head().BindArgument(argumentInitializer),
		m_methods
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindMethodArgument<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindMethodArgumentTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindMethodArgument(
	const T &argumentInitializer
) const
{
	return typename BindMethodArgumentTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members,
		m_constructors,
		m_methods.Tail() + m_methods.Head().BindArgument(argumentInitializer)
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindAttributeTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindAttribute(
	const T &attributeInitializer
) const
{
	return BindAttributeTraits<TTraits, T>::BindAttribute(*this, attributeInitializer);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindClassAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindClassAttributeTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindClassAttribute(
	const T &attributeInitializer
) const
{
	return typename BindClassAttributeTraits<TTraits, T>::ResultType(
		m_attributes + attributeInitializer,
		m_name,
		m_supers,
		m_members,
		m_constructors,
		m_methods
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindSuperAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindSuperAttributeTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindSuperAttribute(
	const T &attributeInitializer
) const
{
	return typename BindSuperAttributeTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers.Tail() + m_supers.Head().BindAttribute(attributeInitializer),
		m_members,
		m_constructors,
		m_methods
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindMemberAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindMemberAttributeTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindMemberAttribute(
	const T &attributeInitializer
) const
{
	return typename BindMemberAttributeTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members.Tail() + m_members.Head().BindAttribute(attributeInitializer),
		m_constructors,
		m_methods
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindConstructorAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindConstructorAttributeTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindConstructorAttribute(
	const T &attributeInitializer
) const
{
	return typename BindConstructorAttributeTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members,
		m_constructors.Tail() + m_constructors.Head().BindAttribute(attributeInitializer),
		m_methods
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindConstructorArgumentAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindConstructorArgumentAttributeTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindConstructorArgumentAttribute(
	const T &attributeInitializer
) const
{
	return typename BindConstructorArgumentAttributeTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members,
		m_constructors.Tail() + m_constructors.Head().BindArgumentAttribute(attributeInitializer),
		m_methods
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindMethodAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindMethodAttributeTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindMethodAttribute(
	const T &attributeInitializer
) const
{
	return typename BindMethodAttributeTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members,
		m_constructors,
		m_methods.Tail() + m_methods.Head().BindAttribute(attributeInitializer)
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::BindMethodArgumentAttribute<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
template <typename T>
inline typename BindMethodArgumentAttributeTraits<TTraits, T>::ResultType ClassDescriptor<TTraits>::BindMethodArgumentAttribute(
	const T &attributeInitializer
) const
{
	return typename BindMethodArgumentAttributeTraits<TTraits, T>::ResultType(
		m_attributes,
		m_name,
		m_supers,
		m_members,
		m_constructors,
		m_methods.Tail() + m_methods.Head().BindArgumentAttribute(attributeInitializer)
	);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::InitializeSupers()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
inline const Class::SUPER **ClassDescriptor<TTraits>::InitializeSupers() const
{
	static const Class::SUPER *supers[SuperTupleType::SIZE + 1];
	supers[SuperTupleType::SIZE] = NULL;
	RecurseSupers<SuperTupleType, SuperTupleType::SIZE - 1>::Initialize(supers, m_supers);
	return supers;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::InitializeMembers()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
inline const MEMBERS ClassDescriptor<TTraits>::InitializeMembers() const
{
	const int NUM_MEMBERS = CountMembers<MemberTupleType>::NUM_MEMBERS;
	static const Class::MEMBER *members[NUM_MEMBERS + 1];
	members[NUM_MEMBERS] = NULL;
	RecurseMembers<MemberTupleType, NUM_MEMBERS, MemberTupleType::SIZE - 1>::Initialize(members, m_members);

	const int NUM_STATICMEMBERS = CountMembers<MemberTupleType>::NUM_STATICMEMBERS;
	static const Class::STATICMEMBER *staticMembers[NUM_STATICMEMBERS + 1];
	staticMembers[NUM_STATICMEMBERS] = NULL;
	RecurseStaticMembers<MemberTupleType, NUM_STATICMEMBERS, MemberTupleType::SIZE - 1>::Initialize(staticMembers, m_members);

	const int NUM_STATICCONSTANTS = CountMembers<MemberTupleType>::NUM_STATICCONSTANTS;
	static const Class::STATICCONSTANT *staticConstants[NUM_STATICCONSTANTS + 1];
	staticConstants[NUM_STATICCONSTANTS] = NULL;
	RecurseStaticConstants<MemberTupleType, NUM_STATICCONSTANTS, MemberTupleType::SIZE - 1>::Initialize(staticConstants, m_members);

	return MEMBERS(members, staticMembers, staticConstants);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::InitializeConstructors()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
inline const Class::CONSTRUCTOR **ClassDescriptor<TTraits>::InitializeConstructors() const
{
	const int NUM_CONSTRUCTORS = CountConstructors<ConstructorTupleType>::NUM_CONSTRUCTORS;
	static const Class::CONSTRUCTOR *constructors[NUM_CONSTRUCTORS + 1];
	constructors[NUM_CONSTRUCTORS] = NULL;
	RecurseConstructors<ConstructorTupleType, NUM_CONSTRUCTORS, ConstructorTupleType::SIZE - 1>::Initialize(constructors, m_constructors);

	return constructors;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassDescriptor<TTraits>::InitializeMethods()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
inline const METHODS ClassDescriptor<TTraits>::InitializeMethods() const
{
	const int NUM_METHODS = CountMethods<MethodTupleType>::NUM_METHODS;
	static const Class::MUTABLEMETHOD *methods[NUM_METHODS + 1];
	methods[NUM_METHODS] = NULL;
	RecurseMethods<MethodTupleType, NUM_METHODS, MethodTupleType::SIZE - 1>::Initialize(methods, m_methods);

	const int NUM_CONSTMETHODS = CountMethods<MethodTupleType>::NUM_CONSTMETHODS;
	static const Class::CONSTMETHOD *constMethods[NUM_CONSTMETHODS + 1];
	constMethods[NUM_CONSTMETHODS] = NULL;
	RecurseConstMethods<MethodTupleType, NUM_CONSTMETHODS, MethodTupleType::SIZE - 1>::Initialize(constMethods, m_methods);

	const int NUM_STATICMETHODS = CountMethods<MethodTupleType>::NUM_STATICMETHODS;
	static const Class::STATICMETHOD *staticMethods[NUM_STATICMETHODS + 1];
	staticMethods[NUM_STATICMETHODS] = NULL;
	RecurseStaticMethods<MethodTupleType, NUM_STATICMETHODS, MethodTupleType::SIZE - 1>::Initialize(staticMethods, m_methods);

	return METHODS(methods, constMethods, staticMethods);
}

} // details
} // reflect

