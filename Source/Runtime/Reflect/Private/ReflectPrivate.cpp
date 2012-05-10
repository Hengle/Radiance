// Reflect.cpp
// Reflection private implementation.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

#include "../Reflect.h"
#include "ReflectPrivateMap.h"
#include "../../Predicate.h"
#include "../../Stream.h"


namespace reflect {
namespace details {

typedef boost::shared_mutex Lock;

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::TypeTraitsShared::Construct()
//////////////////////////////////////////////////////////////////////////////////////////

void TypeTraitsShared::Construct(const Class *type, void *location)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::TypeTraitsShared::Destruct()
//////////////////////////////////////////////////////////////////////////////////////////

void TypeTraitsShared::Destruct(const Class *type, void *location)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::MultiAccessLock()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API Lock& RADRT_CALL ClassListLock()
{
	static Lock lock;
	return lock;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassTree()
//////////////////////////////////////////////////////////////////////////////////////////

static container::LWRedBlackNodeTree &ClassTree()
{
	static container::LWRedBlackNodeTree tree;
	return tree;
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassAdaptor
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
class ClassAdaptor
{
public:

	typedef const TChar* RetType;
	typedef container::LWRedBlackNodeTree::Node* ArgType;

	const TChar *operator()(container::LWRedBlackNodeTree::Node *node)
	{
		RAD_ASSERT(NULL != node);
		return static_cast<reflect::Class *>(node)->Name<TChar>();
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Comparator<T, TChar>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename TChar>
class Comparator
{
public:

	typedef T _ARG0_ADAPTER;
	typedef ClassAdaptor<TChar> _ARG1_ADAPTER;

	typedef typename _ARG0_ADAPTER::RetType Adapter0RetType;
	typedef typename _ARG0_ADAPTER::ArgType Adapter0ArgType;

	typedef typename _ARG1_ADAPTER::RetType Adapter1RetType;
	typedef typename _ARG1_ADAPTER::ArgType Adapter1ArgType;

	_ARG0_ADAPTER arg0Adapter;
	_ARG1_ADAPTER arg1Adapter;

	SReg operator()(const TChar *a, const TChar *b)
	{
		return SReg(string::cmp(a, b));
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassListTraits<TChar>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
struct ClassListTraits
{
	typedef predicate::ImplicitCastAdapter<const TChar *, const TChar *> StringAdaptor;
	typedef Comparator<ClassAdaptor<TChar>, TChar>                            InsertComparator;
	typedef Comparator<StringAdaptor, TChar>                                  FindComparator;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassList::Insert()
//////////////////////////////////////////////////////////////////////////////////////////

void ClassList::Insert(Class &type)
{
	boost::lock_guard<Lock> lock(ClassListLock());
	ClassListTraits<char>::InsertComparator x;
	ClassTree().Insert(&type, x);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassList::Remove()
//////////////////////////////////////////////////////////////////////////////////////////

void ClassList::Remove(Class &type)
{
	boost::lock_guard<Lock> lock(ClassListLock());
	ClassTree().Remove(&type);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassList::FindWorker<TChar>()
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TChar>
inline const Class *FindWorker(const TChar *name)
{
	boost::shared_lock<Lock> lock(ClassListLock());
	typename ClassListTraits<TChar>::FindComparator x;
	return static_cast<const Class *>(ClassTree().Find(name, x));
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassList::Find()
//////////////////////////////////////////////////////////////////////////////////////////

const Class *ClassList::Find(const char *name)
{
	return FindWorker(name);
}

const Class *ClassList::Find(const wchar_t *name)
{
	return FindWorker(name);
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassList::First()
//////////////////////////////////////////////////////////////////////////////////////////

const Class *ClassList::First()
{
	return static_cast<const Class*>(ClassTree().First());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassList::Last()
//////////////////////////////////////////////////////////////////////////////////////////

const Class *ClassList::Last()
{
	return static_cast<const Class*>(ClassTree().Last());
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassList::Next()
//////////////////////////////////////////////////////////////////////////////////////////

const Class *ClassList::Next(const Class *pos)
{
	RAD_ASSERT(pos);
	return static_cast<const Class*>(ClassTree().Next(pos));
}

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ClassList::Prev()
//////////////////////////////////////////////////////////////////////////////////////////

const Class *ClassList::Prev(const Class *pos)
{
	RAD_ASSERT(pos);
	return static_cast<const Class*>(ClassTree().Previous(pos));
}

} // details
} // reflect
