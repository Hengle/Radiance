// ReflectPrivateMap.h
// Private macros for creating a reflection map
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <boost/thread/recursive_mutex.hpp>
#include "ReflectPrivateMapDef.h"

//////////////////////////////////////////////////////////////////////////////////////////
// operator new()
//////////////////////////////////////////////////////////////////////////////////////////

void *operator new(size_t size, const reflect::Class *type, void *location);

//////////////////////////////////////////////////////////////////////////////////////////
// operator delete()
//////////////////////////////////////////////////////////////////////////////////////////

void operator delete(void *p, const reflect::Class *type, void *location);

//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace reflect::details
//////////////////////////////////////////////////////////////////////////////////////////


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::TypeLock
//////////////////////////////////////////////////////////////////////////////////////////

class TypeLock
{
public:

	TypeLock() { CS().lock(); }
	~TypeLock() { CS().unlock(); }

private:

	typedef boost::recursive_mutex Lock;
	static Lock &CS() 
	{
		static Lock cs;
		return cs;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::GetInstance<Predicate, T, TTuple, I>
//////////////////////////////////////////////////////////////////////////////////////////

template <bool Predicate, typename T, typename TTuple, int I>
struct GetInstance
{
	static void Execute(const T **dest, const TTuple &src);
};

template <typename T, typename TTuple, int I>
struct GetInstance<false, T, TTuple, I>
{
	static void Execute(const T **dest, const TTuple &src);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::TypeTraitsShared
//////////////////////////////////////////////////////////////////////////////////////////

struct RADRT_CLASS TypeTraitsShared
{
	static CLASSDESCRIPTOR Descriptor();

	static void Construct(const Class *type, void *location);
	static void Destruct(const Class *type, void *location);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::TypeTraitsBase<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct TypeTraitsBase :
public TypeTraitsShared
{
	typedef T TYPE;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::TypeBase<TTraits, IsConst>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, bool IsConst>
class TypeBase;

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::TypeBase<TTraits, false>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, bool IsConst>
class TypeBase :
public Class
{
protected:

	TypeBase(CLASSDESCRIPTOR descriptor);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::TypeBase<TTraits, true>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
class TypeBase<TTraits, true> :
public Class
{
protected:

	TypeBase(CLASSDESCRIPTOR descriptor);
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Type<TTraits, false>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits, bool IsConst>
class ClassType :
public TypeBase<TTraits, IsConst>
{
public:

	typedef TTraits                   TraitsType;
	typedef typename TraitsType::TYPE TYPE;

	~ClassType();

	virtual const Class *ConstType() const;

private:

	friend struct TypeTraits<TYPE>;

	ClassType();	
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Type<TTraits, true>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTraits>
class ClassType<TTraits, true> :
public TypeBase<TTraits, true>
{
public:

	typedef TTraits                   TraitsType;
	typedef typename TraitsType::TYPE TYPE;

	~ClassType();

	virtual const Class *ConstType() const;

private:

	friend struct TypeTraits<TYPE>;

	ClassType();	
};

} // details
} // reflect


#include "ReflectPrivateClass.h"
#include "ReflectPrivateMap.inl"
