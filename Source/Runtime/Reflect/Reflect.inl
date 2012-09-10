// Reflect.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel

#pragma once

#include "../Utils.h"
#include <boost/thread/shared_mutex.hpp>


namespace reflect {
namespace details {

RADRT_API boost::shared_mutex &RADRT_CALL ClassListLock();

} // details

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::TypeEnumerator::TypeEnumerator()
//////////////////////////////////////////////////////////////////////////////////////////

inline TypeEnumerator::TypeEnumerator() : m_pos(0), m_locked(false)
{
}

inline TypeEnumerator::TypeEnumerator(const Class *pos) : m_pos(pos), m_locked(false)
{
	Begin(pos);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::TypeEnumerator::~TypeEnumerator()
//////////////////////////////////////////////////////////////////////////////////////////

inline TypeEnumerator::~TypeEnumerator()
{
	if (m_locked) Release();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::TypeEnumerator::Begin()
//////////////////////////////////////////////////////////////////////////////////////////

inline void TypeEnumerator::Begin(const Class *pos)
{
	RAD_ASSERT(pos);
	Begin();
	m_pos = pos;
}

inline const Class *TypeEnumerator::Begin()
{
	if (!m_locked)
	{
		details::ClassListLock().lock_shared();
		m_locked = true;
	}
	m_pos = details::ClassList().First();
	return m_pos;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::TypeEnumerator::Next()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class *TypeEnumerator::Next()
{
	RAD_ASSERT(m_pos);
	m_pos = details::ClassList().Next(m_pos);
	return m_pos;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::TypeEnumerator::Prev()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class *TypeEnumerator::Prev()
{
	if (m_pos)
	{
		m_pos = details::ClassList().Prev(m_pos);
		return m_pos;
	}
	else
	{
		return End();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::TypeEnumerator::End()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class *TypeEnumerator::End()
{
	if (!m_locked)
	{
		details::ClassListLock().lock_shared();
		m_locked = true;
	}
	m_pos = details::ClassList().Last();
	return m_pos;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::TypeEnumerator::Release()
//////////////////////////////////////////////////////////////////////////////////////////

inline void TypeEnumerator::Release()
{
	RAD_ASSERT(m_locked);
	details::ClassListLock().unlock_shared();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Attribute::Name<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const T* Attribute::Name() const
{
	return m_name.As<T>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Attribute::Attribute()
//////////////////////////////////////////////////////////////////////////////////////////

inline Attribute::Attribute(const NAME &name) :
m_name(name)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Anonymous::NumAttributes()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Anonymous::NumAttributes() const
{
	return m_attributes.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Anonymous::Attribute()
//////////////////////////////////////////////////////////////////////////////////////////

inline const ATTRIBUTE *Anonymous::Attribute(int i) const
{
	return m_attributes[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Anonymous::Anonymous()
//////////////////////////////////////////////////////////////////////////////////////////

inline Anonymous::Anonymous(
	const ATTRIBUTEARRAY &attributes
) :
m_attributes(attributes)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Anonymous::FindAttribute()
//////////////////////////////////////////////////////////////////////////////////////////

inline const ATTRIBUTE *Anonymous::FindAttribute(const Class *type) const
{
	return details::FindAttribute(this, type);
}

template <typename TChar>
inline const ATTRIBUTE *Anonymous::FindAttribute(const TChar *name) const
{
	return details::FindAttribute(this, name);
}

template<typename TChar>
inline const ATTRIBUTE *Anonymous::FindAttribute(const Class *type, const TChar *name) const
{
	return details::FindAttribute(this, type, name);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Anonymous::AttributeValue()
//////////////////////////////////////////////////////////////////////////////////////////

inline const ATTRIBUTE *Anonymous::AttributeValue(const Class *type, ConstReflected &value) const
{
	return details::AttributeValue(this, type, value);
}

template<typename TChar>
inline const ATTRIBUTE *Anonymous::AttributeValue(const TChar *name, ConstReflected &value) const
{
	return details::AttributeValue(this, name, value);
}

template<typename TChar>
inline const ATTRIBUTE *Anonymous::AttributeValue(const Class *type, const TChar *name, ConstReflected &value) const
{
	return details::AttributeValue(this, type, name, value);
}

template<typename X>
inline const ATTRIBUTE *Anonymous::AttributeValue(X &value) const
{
	return details::AttributeValue(this, value);
}

template<typename X, typename TChar>
inline const ATTRIBUTE *Anonymous::AttributeValue(const TChar *name, X &value) const
{
	return details::AttributeValue(this, name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Named::NumAttributes()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Named::NumAttributes() const
{
	return m_attributes.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Named::Attribute()
//////////////////////////////////////////////////////////////////////////////////////////

inline const ATTRIBUTE *Named::Attribute(int i) const
{
	return m_attributes[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Named::Name<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline const T* Named::Name() const
{
	return m_name.As<T>();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Named::Named()
//////////////////////////////////////////////////////////////////////////////////////////

inline Named::Named(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name
) :
m_attributes(attributes),
m_name(name)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Named::FindAttribute()
//////////////////////////////////////////////////////////////////////////////////////////

inline const ATTRIBUTE *Named::FindAttribute(const Class *type) const
{
	return details::FindAttribute(this, type);
}

template <typename TChar>
inline const ATTRIBUTE *Named::FindAttribute(const TChar *name) const
{
	return details::FindAttribute(this, name);
}

template<typename TChar>
inline const ATTRIBUTE *Named::FindAttribute(const Class *type, const TChar *name) const
{
	return details::FindAttribute(this, type, name);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Named::AttributeValue()
//////////////////////////////////////////////////////////////////////////////////////////

inline const ATTRIBUTE *Named::AttributeValue(const Class *type, ConstReflected &value) const
{
	return details::AttributeValue(this, type, value);
}

template<typename TChar>
inline const ATTRIBUTE *Named::AttributeValue(const TChar *name, ConstReflected &value) const
{
	return details::AttributeValue(this, name, value);
}

template<typename TChar>
inline const ATTRIBUTE *Named::AttributeValue(const Class *type, const TChar *name, ConstReflected &value) const
{
	return details::AttributeValue(this, type, name, value);
}

template<typename X>
inline const ATTRIBUTE *Named::AttributeValue(X &value) const
{
	return details::AttributeValue(this, value);
}

template<typename X, typename TChar>
inline const ATTRIBUTE *Named::AttributeValue(const TChar *name, X &value) const
{
	return details::AttributeValue(this, name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Instanced::Offset()
//////////////////////////////////////////////////////////////////////////////////////////

inline AddrSize Instanced::Offset() const
{
	return m_offset;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Instanced::Instanced()
//////////////////////////////////////////////////////////////////////////////////////////

inline Instanced::Instanced(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	AddrSize             offset
) :
Named(attributes, name),
m_offset(offset)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Static::Address()
//////////////////////////////////////////////////////////////////////////////////////////

inline void *Static::Address() const
{
	return m_address;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Static::Static()
//////////////////////////////////////////////////////////////////////////////////////////

inline Static::Static(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	void                 *address
) :
Named(attributes, name),
m_address(address)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::StaticConst::Address()
//////////////////////////////////////////////////////////////////////////////////////////

inline const void *StaticConst::Address() const
{
	return m_address;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::StaticConst::StaticConst()
//////////////////////////////////////////////////////////////////////////////////////////

inline StaticConst::StaticConst(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	const void           *address
) :
Named(attributes, name),
m_address(address)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::CallException<FunctionType>::CallException()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::CallException<FunctionType>::CallException(const FunctionType *function) :
SuperType(function)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::CallException<FunctionType>::~CallException()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::CallException<FunctionType>::~CallException() throw()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::CallException<FunctionType>::Function()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline const FunctionType *IFunction::CallException<FunctionType>::Function() const
{
	return (const FunctionType *)this->m_function;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::CallException<FunctionType>::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::CallException<FunctionType> &IFunction::CallException<FunctionType>::operator=(const SelfType &e)
{
	SuperType::operator=(e);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::CallException<IFunction>::CallException()
//////////////////////////////////////////////////////////////////////////////////////////

inline IFunction::CallException<IFunction>::CallException(const IFunction *function) :
m_function(function)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::CallException<IFunction>::~CallException()
//////////////////////////////////////////////////////////////////////////////////////////

inline IFunction::CallException<IFunction>::~CallException() throw()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::CallException<IFunction>::Function()
//////////////////////////////////////////////////////////////////////////////////////////

inline const IFunction *IFunction::CallException<IFunction>::Function() const
{
	return this->m_function;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::CallException<IFunction>::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

inline IFunction::CallException<IFunction> &IFunction::CallException<IFunction>::operator=(const SelfType &e)
{
	this->m_function = e.m_function;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::ArgumentException<FunctionType>::ArgumentException()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::ArgumentException<FunctionType>::ArgumentException(const FunctionType *function, int argument) :
SuperType(function),
m_argument(argument)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::ArgumentException<FunctionType>::~ArgumentException()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::ArgumentException<FunctionType>::~ArgumentException() throw()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::ArgumentException<FunctionType>::ArgumentIndex()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline int IFunction::ArgumentException<FunctionType>::ArgumentIndex() const
{
	return m_argument;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::ArgumentException<FunctionType>::ArgumentIndex()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline const IFunction::ARGUMENT *IFunction::ArgumentException<FunctionType>::Argument() const
{
	RAD_ASSERT(NULL != this->m_function);
	return this->m_function->Argument(m_argument);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::ArgumentException<FunctionType>::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::ArgumentException<FunctionType> &IFunction::ArgumentException<FunctionType>::operator=(const SelfType &e)
{
	SuperType::operator=(e);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::InvalidArgumentException<FunctionType>::InvalidArgumentException()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::InvalidArgumentException<FunctionType>::InvalidArgumentException(const FunctionType *function, int argument) :
SuperType(function, argument)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::InvalidArgumentException<FunctionType>::~InvalidArgumentException()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::InvalidArgumentException<FunctionType>::~InvalidArgumentException() throw()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::InvalidArgumentException<FunctionType>::what()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
const char *IFunction::InvalidArgumentException<FunctionType>::what() const throw()
{
	RAD_ASSERT(NULL != this->m_function);
	return this->m_function->InvalidArgumentDescription();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::InvalidArgumentException<FunctionType>::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::InvalidArgumentException<FunctionType> &IFunction::InvalidArgumentException<FunctionType>::operator=(const SelfType &e)
{
	SuperType::operator=(e);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::MissingArgumentException<FunctionType>::MissingArgumentException()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::MissingArgumentException<FunctionType>::MissingArgumentException(const FunctionType *function, int argument) :
SuperType(function, argument)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::MissingArgumentException<FunctionType>::~MissingArgumentException()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::MissingArgumentException<FunctionType>::~MissingArgumentException() throw()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::MissingArgumentException<FunctionType>::what()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
const char *IFunction::MissingArgumentException<FunctionType>::what() const throw()
{
	RAD_ASSERT(NULL != this->m_function);
	return this->m_function->MissingArgumentDescription();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::IFunction::MissingArgumentException<FunctionType>::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline IFunction::MissingArgumentException<FunctionType> &IFunction::MissingArgumentException<FunctionType>::operator=(const SelfType &e)
{
	SuperType::operator=(e);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Method::Argument::Argument()
//////////////////////////////////////////////////////////////////////////////////////////

inline Class::Method::Argument::Argument(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name
) :
Named(attributes, name)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::NamedFunction::NamedFunction()
//////////////////////////////////////////////////////////////////////////////////////////

inline NamedFunction::NamedFunction(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	const ARGUMENTARRAY  &arguments
) :
Named(attributes, name),
m_arguments(arguments)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::NamedFunction::NumArguments()
//////////////////////////////////////////////////////////////////////////////////////////

inline int NamedFunction::NumArguments() const
{
	return m_arguments.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::NamedFunction::Argument()
//////////////////////////////////////////////////////////////////////////////////////////

inline const NamedFunction::ARGUMENT *NamedFunction::Argument(int i) const
{
	return m_arguments[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::AnonymousFunction::AnonymousFunction()
//////////////////////////////////////////////////////////////////////////////////////////

inline AnonymousFunction::AnonymousFunction(
	const ATTRIBUTEARRAY &attributes,
	const ARGUMENTARRAY  &arguments
) :
Anonymous(attributes),
m_arguments(arguments)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::AnonymousFunction::NumArguments()
//////////////////////////////////////////////////////////////////////////////////////////

inline int AnonymousFunction::NumArguments() const
{
	return m_arguments.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::AnonymousFunction::Argument()
//////////////////////////////////////////////////////////////////////////////////////////

inline const AnonymousFunction::ARGUMENT *AnonymousFunction::Argument(int i) const
{
	return m_arguments[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Size()
//////////////////////////////////////////////////////////////////////////////////////////

inline AddrSize Class::Size() const
{
	return m_size;
}

inline AddrSize Class::Alignment() const
{
	return m_alignment;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Find()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
inline const Class *Class::Find(const TChar *name)
{
	return ::reflect::details::ClassList::Find(name);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Destroy()
//////////////////////////////////////////////////////////////////////////////////////////

inline void Class::Destroy(void *location) const
{
	RAD_ASSERT(NULL != m_dtor);
	m_dtor(this, location);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Super::Super()
//////////////////////////////////////////////////////////////////////////////////////////

inline Class::Super::Super(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	AddrSize             offset
) :
Instanced(attributes, name, offset)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::NumSupers()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Class::NumSupers() const
{
	return m_supers.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Super()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class::SUPER *Class::Super(int i) const
{
	return m_supers[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindSuper<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::SUPER *Class::FindSuper(const TChar *name, bool recurse) const
{
	if (recurse)
	{
		const Class::SUPER *super = FindSuper(name, false);
		if (NULL != super) { return super; }

		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			super = Super(i)->Type()->FindSuper(name, true);
			if (super) { return super; }
		}
	}
	else
	{
		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; ++i)
		{
			const SUPER *super = Super(i);
			if (0 == string::cmp(name, super->Type()->Name<TChar>())) { return super; }
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Member::Member()
//////////////////////////////////////////////////////////////////////////////////////////

inline Class::Member::Member(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	AddrSize             offset
) :
Instanced(attributes, name, offset)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::NumMembers()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Class::NumMembers() const
{
	return m_members.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Member()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class::MEMBER *Class::Member(int i) const
{
	return m_members[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindMember<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::MEMBER *Class::FindMember(const TChar *name, bool findInSupers) const
{
	if (findInSupers)
	{
		const Class::MEMBER *member = FindMember(name, false);
		if (NULL != member) { return member; }

		const int NUM_SUPERS = NumMembers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			member = Super(i)->Type()->FindMember(name, true);
			if (member) { return member; }
		}
	}
	else
	{
		const int NUM_MEMBERS = NumMembers();
		for (int i = 0; i < NUM_MEMBERS; ++i)
		{
			const MEMBER *member = Member(i);
			if (0 == string::cmp(name, member->Name<TChar>())) { return member; }
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::StaticMember::StaticMember()
//////////////////////////////////////////////////////////////////////////////////////////

inline Class::StaticMember::StaticMember(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	void                 *address
) :
Static(attributes, name, address)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::NumStaticMembers()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Class::NumStaticMembers() const
{
	return m_staticMembers.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::StaticMember()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class::STATICMEMBER *Class::StaticMember(int i) const
{
	return m_staticMembers[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindStaticMember<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::STATICMEMBER *Class::FindStaticMember(const TChar *name, bool findInSupers) const
{
	if (findInSupers)
	{
		const Class::STATICMEMBER *member = FindStaticMember(name, false);
		if (NULL != member)
			return member;

		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			member = Super(i)->Type()->FindStaticMember(name, true);
			if (NULL != member)
				return member;
		}
	}
	else
	{
		const int NUM_MEMBERS = NumStaticMembers();
		for (int i = 0; i < NUM_MEMBERS; ++i)
		{
			const STATICMEMBER *member = StaticMember(i);
			if (0 == string::cmp(name, member->Name<TChar>())) { return member; }
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::StaticConstant::StaticConstant()
//////////////////////////////////////////////////////////////////////////////////////////

inline Class::StaticConstant::StaticConstant(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	const void           *address
) :
StaticConst(attributes, name, address)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::NumStaticConstants()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Class::NumStaticConstants() const
{
	return m_staticConstants.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::StaticConstant()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class::STATICCONSTANT *Class::StaticConstant(int i) const
{
	return m_staticConstants[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindStaticConstant<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::STATICCONSTANT *Class::FindStaticConstant(const TChar *name, bool findInSupers) const
{
	if (findInSupers)
	{
		const Class::STATICCONSTANT *member = FindStaticConstant(name, false);
		if (NULL != member)
			return member;

		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			member = Super(i)->Type()->FindStaticConstant(name, true);
			if (NULL != member)
				return member;
		}
	}
	else
	{
		const int NUM_MEMBERS = NumStaticConstants();
		for (int i = 0; i < NUM_MEMBERS; ++i)
		{
			const STATICCONSTANT *member = StaticConstant(i);
			if (0 == string::cmp(name, member->Name<TChar>())) { return member; }
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Constructor::Constructor()
//////////////////////////////////////////////////////////////////////////////////////////

inline Class::Constructor::Constructor(
	const ATTRIBUTEARRAY &attributes,
	const ARGUMENTARRAY  &arguments
) :
SuperType(attributes, arguments)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::NumConstructors()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Class::NumConstructors() const
{
	return m_constructors.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Constructor()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class::CONSTRUCTOR *Class::Constructor(int i) const
{
	return m_constructors[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindConstructor<FunctionType>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
const Class::CONSTRUCTOR *Class::FindConstructor() const
{
	const int NUM_METHODS = NumConstructors();
	for (int i = 0; i < NUM_METHODS; ++i)
	{
		const CONSTRUCTOR *method = Constructor(i);
		if (reflect::details::MethodArgumentsMatch<Class::CONSTRUCTOR, FunctionType>::Matches(method))
		{
			return method;
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Construct<FunctionType>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline const Class::CONSTRUCTOR *Class::Construct(void *place, const IArgumentList &args) const
{
	const CONSTRUCTOR *method = FindConstructor<FunctionType>();
	if (NULL != method)
	{
		method->Call(place, args);
	}
	else
	{
		throw reflect::MethodNotFoundException();
	}
	return method;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Construct()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class::CONSTRUCTOR *Class::Construct(void *place, const IArgumentList &args) const
{
	const CONSTRUCTOR *method = FindConstructor(args);
	if (NULL != method)
	{
		method->Call(place, args);
	}
	else
	{
		throw MethodNotFoundException();
	}
	return method;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Method::Method()
//////////////////////////////////////////////////////////////////////////////////////////

inline Class::Method::Method(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	const ARGUMENTARRAY  &arguments
) :
SuperType(attributes, name, arguments)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::ConstMethod::ConstMethod()
//////////////////////////////////////////////////////////////////////////////////////////

inline Class::ConstMethod::ConstMethod(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	const ARGUMENTARRAY  &arguments
) :
SuperType(attributes, name, arguments)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::NumConstMethods()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Class::NumConstMethods() const
{
	return m_constMethods.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::ConstMethod()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class::CONSTMETHOD *Class::ConstMethod(int i) const
{
	return m_constMethods[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindConstMethod<FunctionType, TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType, typename TChar>
const Class::CONSTMETHOD *Class::FindConstMethod(const TChar *name, bool findInSupers) const
{
	if (findInSupers)
	{
		const Class::CONSTMETHOD *method = FindConstMethod<FunctionType, TChar>(name, false);
		if (NULL != method) { return method; }

		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			const Class::CONSTMETHOD *method = Super(i)->Type()->FindConstMethod<FunctionType, TChar>(name, true);
			if (NULL != method) { return method; }
		}
	}
	else
	{
		const int NUM_METHODS = NumConstMethods();
		for (int i = 0; i < NUM_METHODS; ++i)
		{
			const CONSTMETHOD *method = ConstMethod(i);
			if (
				0 == string::cmp(name, method->Name<TChar>()) &&
				reflect::details::MethodArgumentsMatch<Class::CONSTMETHOD, FunctionType>::Matches(method)
			)
			{
				return method;
			}
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindConstMethod<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::CONSTMETHOD *Class::FindConstMethod(const TChar *name, const ITypeList &args, bool findInSupers) const
{
	if (findInSupers)
	{
		const Class::CONSTMETHOD *method = FindConstMethod<TChar>(name, args, false);
		if (NULL != method) { return method; }

		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			const Class::CONSTMETHOD *method = Super(i)->Type()->FindConstMethod<TChar>(name, args, true);
			if (NULL != method) { return method; }
		}
	}
	else
	{
		const int NUM_METHODS = NumConstMethods();
		for (int i = 0; i < NUM_METHODS; ++i)
		{
			const CONSTMETHOD *method = ConstMethod(i);
			if (
				0 == string::cmp(name, method->Name<TChar>()) &&
				method->Match(args)
			)
			{
				return method;
			}
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::MutableMethod::MutableMethod()
//////////////////////////////////////////////////////////////////////////////////////////

inline Class::MutableMethod::MutableMethod(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	const ARGUMENTARRAY  &arguments
) :
SuperType(attributes, name, arguments)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::NumMethods()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Class::NumMethods() const
{
	return m_methods.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::Method()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class::MUTABLEMETHOD *Class::Method(int i) const
{
	return m_methods[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindMethod<FunctionType, TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType, typename TChar>
const Class::MUTABLEMETHOD *Class::FindMethod(const TChar *name, bool findInSupers) const
{
	if (findInSupers)
	{
		const Class::MUTABLEMETHOD *method = FindMethod<FunctionType, TChar>(name, false);
		if (NULL != method) { return method; }

		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			const Class::MUTABLEMETHOD *method = Super(i)->Type()->FindMethod<FunctionType, TChar>(name, true);
			if (NULL != method) { return method; }
		}
	}
	else
	{
		const int NUM_METHODS = NumMethods();
		for (int i = 0; i < NUM_METHODS; ++i)
		{
			const MUTABLEMETHOD *method = Method(i);
			if (
				0 == string::cmp(name, method->Name<TChar>()) &&
				reflect::details::MethodArgumentsMatch<Class::MUTABLEMETHOD, FunctionType>::Matches(method)
			)
			{
				return method;
			}
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindMethod<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::MUTABLEMETHOD *Class::FindMethod(const TChar *name, const ITypeList &args, bool findInSupers) const
{
	if (findInSupers)
	{
		const Class::MUTABLEMETHOD *method = FindMethod<TChar>(name, args, false);
		if (NULL != method) { return method; }

		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			const Class::MUTABLEMETHOD *method = Super(i)->Type()->FindMethod<TChar>(name, args, true);
			if (NULL != method) { return method; }
		}
	}
	else
	{
		const int NUM_METHODS = NumMethods();
		for (int i = 0; i < NUM_METHODS; ++i)
		{
			const MUTABLEMETHOD *method = Method(i);
			if (
				0 == string::cmp(name, method->Name<TChar>()) &&
				method->Match(args)
			)
			{
				return method;
			}
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::StaticMethod::StaticMethod()
//////////////////////////////////////////////////////////////////////////////////////////

inline Class::StaticMethod::StaticMethod(
	const ATTRIBUTEARRAY &attributes,
	const NAME           &name,
	const ARGUMENTARRAY  &arguments
) :
SuperType(attributes, name, arguments)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::NumStaticMethods()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Class::NumStaticMethods() const
{
	return m_staticMethods.Length();
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::StaticMethod()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class::STATICMETHOD *Class::StaticMethod(int i) const
{
	return m_staticMethods[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindStaticMethod<FunctionType, TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType, typename TChar>
const Class::STATICMETHOD *Class::FindStaticMethod(const TChar *name, bool findInSupers) const
{
	if (findInSupers)
	{
		const STATICMETHOD *method = FindStaticMethod<FunctionType, TChar>(name, false);
		if (NULL != method) { return method; }

		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			const SUPER *super = Super(i);
			RAD_ASSERT(NULL != super);
			const Class *superType = super->Type();
			RAD_ASSERT(NULL != superType);
			const STATICMETHOD *method = superType->FindStaticMethod<FunctionType, TChar>(name, true);
			if (NULL != method) { return method; }
		}
	}
	else
	{
		const int NUM_METHODS = NumStaticMethods();
		for (int i = 0; i < NUM_METHODS; ++i)
		{
			const STATICMETHOD *method = StaticMethod(i);
			if (
				0 == string::cmp(name, method->Name<TChar>()) &&
				reflect::details::MethodArgumentsMatch<Class::STATICMETHOD, FunctionType>::Matches(method)
			)
			{
				return method;
			}
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::FindStaticMethod<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::STATICMETHOD *Class::FindStaticMethod(const TChar *name, const ITypeList &args, bool findInSupers) const
{
	if (findInSupers)
	{
		{
			const Class::STATICMETHOD *method = FindStaticMethod<TChar>(name, args, false);
			if (NULL != method)
				return method;
		}

		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			const SUPER *super = Super(i);
			RAD_ASSERT(NULL != super);
			const Class *superType = super->Type();
			RAD_ASSERT(NULL != superType);
			const STATICMETHOD *method = superType->FindStaticMethod<TChar>(name, args, true);
			if (NULL != method)
				return method;
		}
	}
	else
	{
		const int NUM_METHODS = NumStaticMethods();
		for (int i = 0; i < NUM_METHODS; ++i)
		{
			const STATICMETHOD *method = StaticMethod(i);
			if (
				0 == string::cmp(name, method->Name<TChar>()) &&
				method->Match(args)
			)
			{
				return method;
			}
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::CallStaticMethod<FunctionType, TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType, typename TChar>
inline const Class::STATICMETHOD *Class::FindAndCallStaticMethod(
		const Reflected     &result,
		const TChar   *name,
		const IArgumentList &args,
		bool findInSupers
) const
{
	const STATICMETHOD *method = FindStaticMethod<FunctionType>(name, findInSupers);
	if (NULL != method)
	{
		method->Call(result, args);
	}
	else
	{
		throw reflect::MethodNotFoundException();
	}
	return method;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Class::CallStaticMethod<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
inline const Class::STATICMETHOD *Class::FindAndCallStaticMethod(
		const Reflected     &result,
		const TChar   *name,
		const IArgumentList &args,
		bool findInSupers
) const
{
	const STATICMETHOD *method = FindStaticMethod(name, args, findInSupers);
	if (NULL != method)
	{
		method->Call(result, args);
	}
	else
	{
		throw reflect::MethodNotFoundException();
	}
	return method;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflect<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline typename ReflectResult<T>::Type Reflect(T &obj)
{
	return Reflect(
		static_cast<typename ReflectResult<T>::VoidType *>(&obj),
		reflect::Type<T>()
	);
}

template<>
inline ReflectResult<const ATTRIBUTE*>::Type Reflect<const ATTRIBUTE*>(const ATTRIBUTE *&attribute)
{
	if (attribute)
	{
		return Reflect(attribute->Value(), attribute->Type());
	}

	return Reflect((const void *)NULL, NULL);
}

template<>
inline ReflectResult<const Class::STATICMEMBER*>::Type Reflect<const Class::STATICMEMBER*>(const Class::STATICMEMBER *&member)
{
	if (member)
	{
		return Reflect(member->Address(), member->Type());
	}

	return Reflect((void *)NULL, NULL);
}

template<>
inline ReflectResult<const Class::STATICCONSTANT*>::Type Reflect<const Class::STATICCONSTANT*>(const Class::STATICCONSTANT *&constant)
{
	if (constant)
	{
		return Reflect(constant->Address(), constant->Type());
	}

	return Reflect((const void *)NULL, NULL);
}

template<>
inline ReflectResult<const IFunction::ARGUMENT*>::Type Reflect<const IFunction::ARGUMENT*>(const IFunction::ARGUMENT *&argument)
{
	if (argument)
	{
		return Reflect(argument->DefaultValue(), argument->Type());
	}

	return Reflect((const void *)NULL, NULL);
}

inline Reflected Reflect(void *data, const Class *type)
{
	return Reflected(data, type);
}

inline ConstReflected Reflect(const void *data, const Class *type)
{
	return ConstReflected(data, type);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::ConstReflected()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected::ConstReflected() :
m_data(NULL),
m_type(NULL)
{
}

inline ConstReflected::ConstReflected(const ConstReflected &reflected) :
m_data(reflected.m_data),
m_type(reflected.m_type)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::~ConstReflected()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected::~ConstReflected()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::operator const T *<T>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
ConstReflected::operator const T* () const
{
	RAD_ASSERT(IsValid());
	const Class *t = reflect::Type<T>();
	RAD_ASSERT(NULL != t);
	if (m_type->ConstType() == t->ConstType()) { return reinterpret_cast<const T *>(m_data); }
	const int NUM_SUPERS = NumSupers();
	for (int i = 0; i < NUM_SUPERS; i++)
	{
		try
		{
			const T *result = (const T *)Super(i);
			return result;
		}
		catch (InvalidCastException &)
		{
		}
	}
	throw reflect::InvalidCastException();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::Data()
//////////////////////////////////////////////////////////////////////////////////////////

inline const void *ConstReflected::Data() const
{
	return m_data;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::Type()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class *ConstReflected::Type() const
{
	return m_type;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::Close()
//////////////////////////////////////////////////////////////////////////////////////////

inline void ConstReflected::Close()
{
	m_data = NULL;
	m_type = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::IsValid()
//////////////////////////////////////////////////////////////////////////////////////////

inline bool ConstReflected::IsValid() const
{
	return (m_data && m_type);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::operator bool()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected::operator bool () const
{
	return IsValid();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::NumAttributes()
//////////////////////////////////////////////////////////////////////////////////////////

inline int ConstReflected::NumAttributes() const
{
	RAD_ASSERT(NULL != m_type);
	return m_type->NumAttributes();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::Attribute()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected ConstReflected::Attribute(int i) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(i < m_type->NumAttributes());
	const ATTRIBUTE *attribute = m_type->Attribute(i);
	RAD_ASSERT(NULL != attribute);
	RAD_ASSERT(NULL != attribute->Type());
	return ConstReflected(attribute->Value(), attribute->Type());
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::Cast<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
ConstReflected ConstReflected::Cast(const TChar *typeName) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(NULL != typeName);
	if (0 == string::cmp(typeName, m_type->Name<TChar>())) { return *this; }
	ConstReflected r = CastByName(typeName);
	if (!r) { throw reflect::InvalidCastException(); }
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::Cast()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected ConstReflected::Cast(const Class *type) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(NULL != type);
	if (type->ConstType() == m_type->ConstType()) { return *this; }
	ConstReflected r = CastByType(type);
	if (!r)
	{
		throw InvalidCastException();
	}
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::UnsafeCast<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
ConstReflected ConstReflected::UnsafeCast(const TChar *typeName) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(NULL != typeName);
	if (0 == string::cmp(typeName, m_type->Name<TChar>())) { return *this; }
	Class *type = reflect::Class::Find<TChar>(typeName);
	if (!type) { throw reflect::InvalidCastException(); }
	return UnsafeCast(type);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::UnsafeCast<Class>()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected ConstReflected::UnsafeCast(const Class *type) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(NULL != type);
	if (type->ConstType() == m_type->ConstType()) { return *this; }
	return ConstReflected(m_data, type);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::NumSupers()
//////////////////////////////////////////////////////////////////////////////////////////

inline int ConstReflected::NumSupers() const
{
	RAD_ASSERT(IsValid());
	return m_type->NumSupers();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::Super()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected ConstReflected::Super(int i) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(i < m_type->NumSupers());
	const Class::SUPER *superclass = m_type->Super(i);
	RAD_ASSERT(NULL != superclass);
	RAD_ASSERT(NULL != superclass->Type());
	return ConstReflected(m_data + superclass->Offset(), superclass->Type());
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::NumMembers()
//////////////////////////////////////////////////////////////////////////////////////////

inline int ConstReflected::NumMembers() const
{
	RAD_ASSERT(IsValid());
	return m_type->NumMembers();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::Member()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected ConstReflected::Member(int i) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(i < m_type->NumMembers());
	const Class::MEMBER *member = m_type->Member(i);
	RAD_ASSERT(NULL != member);
	RAD_ASSERT(NULL != member->Type());
	return ConstReflected(m_data + member->Offset(), member->Type());
}

inline ConstReflected ConstReflected::Member(const Class::MEMBER *member, bool cast) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(member);
	RAD_ASSERT(member->OwnerType());

	if (!cast || member->OwnerType() == m_type)
	{
		return ConstReflected(m_data + member->Offset(), member->Type());
	}

	ConstReflected r = Cast(member->OwnerType());
	if (!r) { return r.Member(member); }

	return ConstReflected();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::UncheckedCastByType()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline ConstReflected ConstReflected::UncheckedCastByType(const T *t) const
{
	return ConstReflected(m_data + t->Offset(), t->Type());
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::FindSuper<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
ConstReflected ConstReflected::FindSuper(const TChar *name, bool recurse) const
{
	RAD_ASSERT(IsValid());

	const Class::SUPER *super = m_type->FindSuper(name, false);
	if (NULL != super) { return UncheckedCastByType(super); }

	if (recurse)
	{
		const int NUM_SUPERS = NumSupers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			ConstReflected r = Super(i).FindSuper(name, true);
			if (r) { return r; }
		}
	}

	return ConstReflected();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::FindMember<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
ConstReflected ConstReflected::FindMember(const TChar *name, bool findInSupers) const
{
	RAD_ASSERT(IsValid());

	const Class::MEMBER *member = m_type->FindMember(name);
	if (NULL != member) { return UncheckedCastByType(member); }

	if (findInSupers)
	{
		const int NUM_SUPERS = NumMembers();
		for (int i = 0; i < NUM_SUPERS; i++)
		{
			ConstReflected r = Super(i).FindMember(name);
			if (r) { return r; }
		}
	}

	return ConstReflected();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::FindConstMethod<FunctionType, TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType, typename TChar>
const Class::CONSTMETHOD *ConstReflected::FindConstMethod(const TChar *name, bool findInSupers) const
{
	RAD_ASSERT(IsValid());
	return m_type->FindConstMethod<FunctionType, TChar>(name, findInSupers);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::FindConstMethod<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::CONSTMETHOD *ConstReflected::FindConstMethod(
	const TChar *name,
	const ITypeList &args,
	bool findInSupers
) const
{
	RAD_ASSERT(IsValid());
	return m_type->FindConstMethod<TChar>(name, args, findInSupers);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::CallConstMethod<FunctionType, TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType, typename TChar>
const Class::CONSTMETHOD *ConstReflected::FindAndCallConstMethod(
		const Reflected     &result,
	const TChar   *name,
	const IArgumentList &args,
	bool findInSupers
) const
{
	RAD_ASSERT(IsValid());
	const Class::CONSTMETHOD *method = FindConstMethod<FunctionType, TChar>(name, args, findInSupers);
	if (method)
	{
		CallConstMethod(result, method, args);
	}
	else
	{
		throw reflect::MethodNotFoundException();
	}
	return method;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::CallConstMethod<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::CONSTMETHOD *ConstReflected::FindAndCallConstMethod(
	const Reflected     &result,
	const TChar   *name,
	const IArgumentList &args,
	bool findInSupers
) const
{
	RAD_ASSERT(IsValid());
	const Class::CONSTMETHOD *method = FindConstMethod<TChar>(name, args, findInSupers);
	if (method)
	{
		CallConstMethod(result, method, args);
	}
	else
	{
		throw reflect::MethodNotFoundException();
	}
	return method;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::CallConstMethod<FunctionType, TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

inline void ConstReflected::CallConstMethod(
	const Reflected                &result,
	const Class::CONSTMETHOD *method,
	const IArgumentList            &args
) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(NULL != method);
	if (method->OwnerType()->ConstType() != m_type->ConstType())
	{
		Cast(method->OwnerType()).CallConstMethod(result, method, args);
	}
	else
	{
		method->Call(result, m_data, args);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected &ConstReflected::operator=(const ConstReflected &reflected)
{
	m_data = reflected.m_data;
	m_type = reflected.m_type;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::ConstReflected()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected::ConstReflected(const void *data, const Class *type) :
m_data(reinterpret_cast<const U8*>(data)),
m_type(type)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ConstReflected::CastByName<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
ConstReflected ConstReflected::CastByName(const TChar *typeName) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(NULL != typeName);

	const int NUM_SUPERS = NumSupers();
	for (int i = 0; i < NUM_SUPERS; ++i)
	{
		ConstReflected super = Super(i);
		const Class *gccIsABuggyTurd = super.Type();
		if (0 == string::cmp(typeName, gccIsABuggyTurd->Name<TChar>())) { return super; }
		ConstReflected r = super.CastByName(typeName);
		if (r) { return r; }
	}

	return ConstReflected();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::Reflected()
//////////////////////////////////////////////////////////////////////////////////////////

inline Reflected::Reflected()
{
}

inline Reflected::Reflected(const Reflected &reflected) :
ConstReflected(reflected)
{
}

inline Reflected::Reflected(const ConstReflected &reflected) :
ConstReflected(reflected)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::~Reflected()
//////////////////////////////////////////////////////////////////////////////////////////

inline Reflected::~Reflected()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::operator T *<T *>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Reflected::operator T *() const
{
	return const_cast<T *>((const T *)*static_cast<const ConstReflected *>(this));
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::Data()
//////////////////////////////////////////////////////////////////////////////////////////

inline void *Reflected::Data() const
{
	return const_cast<U8*>(m_data);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::Cast<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
inline Reflected Reflected::Cast(const TChar *typeName) const
{
	return Reflected(ConstReflected::Cast(typeName));
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::Cast()
//////////////////////////////////////////////////////////////////////////////////////////

inline Reflected Reflected::Cast(const Class *type) const
{
	return Reflected(ConstReflected::Cast(type));
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::UnsafeCast<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
inline Reflected Reflected::UnsafeCast(const TChar *typeName) const
{
	return Reflected(ConstReflected::UnsafeCast(typeName));
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::UnsafeCast()
//////////////////////////////////////////////////////////////////////////////////////////

inline Reflected Reflected::UnsafeCast(const Class *type) const
{
	return Reflected(ConstReflected::UnsafeCast(type));
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::NumSupers()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Reflected::NumSupers() const
{
	return ConstReflected::NumSupers();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::Super()
//////////////////////////////////////////////////////////////////////////////////////////

inline Reflected Reflected::Super(int i) const
{
	return Reflected(ConstReflected::Super(i));
}

inline Reflected Reflected::Super(const Class::SUPER *super) const
{
	return Reflected(ConstReflected::Super(super));
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::NumMembers()
//////////////////////////////////////////////////////////////////////////////////////////

inline int Reflected::NumMembers() const
{
	return ConstReflected::NumMembers();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::Member()
//////////////////////////////////////////////////////////////////////////////////////////

inline Reflected Reflected::Member(int i) const
{
	return Reflected(ConstReflected::Member(i));
}

inline Reflected Reflected::Member(const Class::MEMBER *member, bool cast) const
{
	return Reflected(ConstReflected::Member(member, cast));
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::FindSuper<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
inline Reflected Reflected::FindSuper(const TChar *name) const
{
	return Reflected(ConstReflected::FindSuper(name));
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::FindMember()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
inline Reflected Reflected::FindMember(const TChar *name) const
{
	return Reflected(ConstReflected::FindMember(name));
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::FindMethod<FunctionType, TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType, typename TChar>
const Class::MUTABLEMETHOD *Reflected::FindMethod(const TChar *name, bool findInSupers) const
{
	RAD_ASSERT(IsValid());
	return m_type->FindMethod<FunctionType, TChar>(name, findInSupers);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::FindMethod<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::MUTABLEMETHOD *Reflected::FindMethod(
	const TChar *name,
	const ITypeList &args,
	bool findInSupers
) const
{
	RAD_ASSERT(IsValid());
	return m_type->FindMethod<TChar>(name, args, findInSupers);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::CallMethod<FunctionType, TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType, typename TChar>
const Class::MUTABLEMETHOD *Reflected::FindAndCallMethod(
	const Reflected     &result,
	const TChar   *name,
	const IArgumentList &args,
	bool findInSupers
) const
{
	RAD_ASSERT(IsValid());
	const Class::MUTABLEMETHOD *method = FindMethod<FunctionType, TChar>(name, args, findInSupers);
	if (method)
	{
		CallMethod(result, method, args);
	}
	else
	{
		throw reflect::MethodNotFoundException();
	}
	return method;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::CallMethod<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
const Class::MUTABLEMETHOD *Reflected::FindAndCallMethod(
	const Reflected     &result,
	const TChar   *name,
	const IArgumentList &args,
	bool findInSupers
) const
{
	RAD_ASSERT(IsValid());
	const Class::MUTABLEMETHOD *method = FindMethod<TChar>(name, args, findInSupers);
	if (method)
	{
		CallMethod(result, method, args);
	}
	else
	{
		throw reflect::MethodNotFoundException();
	}
	return method;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::CallMethod<FunctionType, TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

inline void Reflected::CallMethod(
	const Reflected                  &result,
	const Class::MUTABLEMETHOD *method,
	const IArgumentList              &args
) const
{
	RAD_ASSERT(IsValid());
	RAD_ASSERT(NULL != method);
	if (method->OwnerType()->ConstType() != m_type->ConstType())
	{
		Cast(method->OwnerType()).CallMethod(result, method, args);
	}
	else
	{
		method->Call(result, const_cast<U8 *>(m_data), args);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::operator=()
//////////////////////////////////////////////////////////////////////////////////////////

inline Reflected &Reflected::operator=(const Reflected &reflected)
{
	ConstReflected::operator=(static_cast<const ConstReflected&>(reflected));
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::New<FunctionType>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FunctionType>
inline Reflected Reflected::New(
		Zone                   &zone,
		const Class            *type,
		const IArgumentList    &args,
		void                   *data,
		UReg                    alignment
)
{
	RAD_ASSERT(type);
	if (!data)
	{
		data = Allocate(zone, type, alignment);
	}
	RAD_VERIFY(IsAligned(data, type->Alignment()));
	type->Construct<FunctionType>(data, args);
	return Reflect(data, type);
}

inline Reflected Reflected::New(
		Zone                   &zone,
		const Class            *type,
		const IArgumentList    &args,
		void                   *data,
		UReg                    alignment
)
{
	RAD_ASSERT(type);
	if (!data)
	{
		data = Allocate(zone, type, alignment);
	}
	RAD_VERIFY(IsAligned(data, type->Alignment()));
	type->Construct(data, args);
	return Reflect(data, type);
}

inline Reflected Reflected::New(
	Zone                     &zone,
	const Class              *type,
	const Class::CONSTRUCTOR *constructor,
	const IArgumentList      &args,
	void                     *data,
	UReg                      alignment
)
{
	RAD_ASSERT(type);
	RAD_ASSERT(constructor);
	if (!data)
	{
		data = Allocate(zone, type, alignment);
	}
	RAD_VERIFY(IsAligned(data, type->Alignment()));
	constructor->Call(data, args);
	return Reflect(data, type);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::Delete()
//////////////////////////////////////////////////////////////////////////////////////////

inline void Reflected::Delete()
{
	RAD_ASSERT(IsValid());
	void *data = const_cast<void*>(static_cast<const void*>(m_data));
	m_type->Destroy(data);
	reflect::Free(data);
	Close();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::Destroy()
//////////////////////////////////////////////////////////////////////////////////////////

inline void Reflected::Destroy()
{
	RAD_ASSERT(IsValid());
	void *data = const_cast<void*>(static_cast<const void*>(m_data));
	m_type->Destroy(data);
	Close();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::Reflected::Reflected()
//////////////////////////////////////////////////////////////////////////////////////////

inline Reflected::Reflected(void *data, const Class *type) :
ConstReflected(reinterpret_cast<const U8*>(data), type)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ArgumentList::ArgumentList()
//////////////////////////////////////////////////////////////////////////////////////////

inline ArgumentList::ArgumentList() :
m_list()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ArgumentList::~ArgumentList()
//////////////////////////////////////////////////////////////////////////////////////////

inline ArgumentList::~ArgumentList()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ArgumentList::PushBack()
//////////////////////////////////////////////////////////////////////////////////////////

inline ArgumentList &ArgumentList::PushBack(const ConstReflected &r)
{
	m_list.push_back(ARG(true, r));
	return *this;
}

inline ArgumentList &ArgumentList::PushBack(const Reflected &r)
{
	m_list.push_back(ARG(false, r));
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ArgumentList::Size()
//////////////////////////////////////////////////////////////////////////////////////////

inline int ArgumentList::Size() const
{
	return int(m_list.size());
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ArgumentList::Type()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class *ArgumentList::Type(int i) const
{
	RAD_ASSERT(i < int(m_list.size()));
	return m_list[i].Type();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ArgumentList::IsConst()
//////////////////////////////////////////////////////////////////////////////////////////

inline bool ArgumentList::IsConst(int i) const
{
	RAD_ASSERT(i < int(m_list.size()));
	return m_list[i].m_isConst;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ArgumentList::ConstArgument()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected ArgumentList::ConstArgument(int i) const
{
	RAD_ASSERT(i < int(m_list.size()));
	return m_list[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::ArgumentList::Argument()
//////////////////////////////////////////////////////////////////////////////////////////

inline Reflected ArgumentList::Argument(int i) const
{
	RAD_ASSERT(!m_list[i].m_isConst);
	RAD_ASSERT(i < int(m_list.size()));
	return m_list[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedArgumentList::FixedArgumentList()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline FixedArgumentList<NUMARGS>::FixedArgumentList() : m_ofs(0)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedArgumentList::~FixedArgumentList()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline FixedArgumentList<NUMARGS>::~FixedArgumentList()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedArgumentList::PushBack()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline FixedArgumentList<NUMARGS> &FixedArgumentList<NUMARGS>::PushBack(const ConstReflected &r)
{
	m_list[m_ofs++] = ARG(true, r);
	return *this;
}

template <AddrSize NUMARGS>
inline FixedArgumentList<NUMARGS> &FixedArgumentList<NUMARGS>::PushBack(const Reflected &r)
{
	m_list[m_ofs++] = ARG(false, r);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedArgumentList::Size()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline int FixedArgumentList<NUMARGS>::Size() const
{
	return int(m_ofs);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedArgumentList::Type()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline const Class *FixedArgumentList<NUMARGS>::Type(int i) const
{
	RAD_ASSERT(i < Size());
	return m_list[i].Type();
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedArgumentList::IsConst()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline bool FixedArgumentList<NUMARGS>::IsConst(int i) const
{
	RAD_ASSERT(i < Size());
	return m_list[i].m_isConst;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedArgumentList::ConstArgument()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline ConstReflected FixedArgumentList<NUMARGS>::ConstArgument(int i) const
{
	RAD_ASSERT(i < Size());
	return m_list[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedArgumentList::Argument()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline Reflected FixedArgumentList<NUMARGS>::Argument(int i) const
{
	RAD_ASSERT(i < Size());
	RAD_ASSERT(!m_list[i].m_isConst);
	return m_list[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::NullArgs::Size()
//////////////////////////////////////////////////////////////////////////////////////////

inline int NullArgs::Size() const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::NullArgs::Type()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class *NullArgs::Type(int i) const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::NullArgs::IsConst()
//////////////////////////////////////////////////////////////////////////////////////////

inline bool NullArgs::IsConst(int i) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::NullArgs::ConstArgument()
//////////////////////////////////////////////////////////////////////////////////////////

inline ConstReflected NullArgs::ConstArgument(int i) const
{
	return Reflect((const void*)0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::NullArgs::Argument()
//////////////////////////////////////////////////////////////////////////////////////////

inline Reflected NullArgs::Argument(int i) const
{
	return Reflect((void*)0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::TypeList::TypeList()
//////////////////////////////////////////////////////////////////////////////////////////

inline TypeList::TypeList()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::TypeList::~TypeList()
//////////////////////////////////////////////////////////////////////////////////////////

inline TypeList::~TypeList()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::TypeList::PushBack()
//////////////////////////////////////////////////////////////////////////////////////////

inline TypeList &TypeList::PushBack(const Class *type)
{
	m_list.push_back(type);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::TypeList::Size()
//////////////////////////////////////////////////////////////////////////////////////////

inline int TypeList::Size() const
{
	return int(m_list.size());
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::TypeList::Type()
//////////////////////////////////////////////////////////////////////////////////////////

inline const Class *TypeList::Type(int i) const
{
	RAD_ASSERT(i < Size());
	return m_list[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedTypeList::FixedTypeList()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline FixedTypeList<NUMARGS>::FixedTypeList() : m_ofs(0)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedTypeList::~FixedTypeList()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline FixedTypeList<NUMARGS>::~FixedTypeList()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedTypeList::PushBack()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline FixedTypeList<NUMARGS> &FixedTypeList<NUMARGS>::PushBack(const Class *type)
{
	m_list[m_ofs++] = type;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedTypeList::Size()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline int FixedTypeList<NUMARGS>::Size() const
{
	return int(m_ofs);
}

//////////////////////////////////////////////////////////////////////////////////////////
// ::reflect::FixedTypeList::Type()
//////////////////////////////////////////////////////////////////////////////////////////

template <AddrSize NUMARGS>
inline const Class *FixedTypeList<NUMARGS>::Type(int i) const
{
	RAD_ASSERT(i < Size());
	return m_list[i];
}

//////////////////////////////////////////////////////////////////////////////////////////
// Allocate/Free
//////////////////////////////////////////////////////////////////////////////////////////

inline void *Allocate(Zone &zone, const Class *type, AddrSize alignment)
{
	RAD_ASSERT(type);
	if (!alignment) { alignment = type->Alignment(); }
	RAD_VERIFY(alignment); // alignment can be zero if we are trying to allocate an abstract class.
	return zone_malloc(zone, type->Size(), 0, alignment);
}

inline void *SafeAllocate(Zone &zone, const Class *type, AddrSize alignment)
{
	void *p = Allocate(zone, type, alignment);
	RAD_OUT_OF_MEM(p);
	return p;
}

inline void Free(void *data)
{
	if (data)
	{
		zone_free(data);
	}
}

} // reflect

