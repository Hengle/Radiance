// ThreadSafeFIFO.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Thread/Interlocked.h"
#include "../Thread/Locks.h"
#include "../Base/MemoryPool.h"
#include <list>

#include "../PushPack.h"

namespace container {

template<
	typename T,
	UReg NumBuckets = 16,
	typename Allocator = pool_allocator<T, ZRuntimeT, 8, MaxUReg, DefaultAlignment, ThreadSafeMemoryPool>
>
class DynamicThreadSafeFIFO
{
private:
	class _Item;
	typedef typename Allocator::template rebind<_Item*>::other OtherA;
	struct _ItemList : public std::list<_Item*, OtherA>
	{
		boost::mutex m;
	};

public:

	class Item
	{
	public:
		Item() : i(0) {}
		bool IsValid() { return i != 0; }
	private:
		friend class DynamicThreadSafeFIFO<T, NumBuckets, Allocator>;
		_Item *i;
		_ItemList *l;
		typename _ItemList::iterator it;
	};

	DynamicThreadSafeFIFO();
	~DynamicThreadSafeFIFO();

	// Read/WriteErase/Length/Empty are synchronized. Item level
	// synchronization is up to the caller (don't Write/Erase the same item
	// at the same time).

	bool Read(T &item);
	void Write(const T &item, Item *pos = 0);
	bool Erase(Item &pos, T *item = 0);
	UReg Length() const;
	bool Empty() const;

	// Clear is not synchronized so do not operate on the container while
	// calling it.

	void Clear();

private:

	struct _Item
	{
		T t;
		Item *p;
	};

	typedef typename Allocator::template rebind<_Item>::other _ItemAlloc;

	enum
	{
		ReadKey = 0,
		EraseKey,
		NumKeys
	};

	// we use this segmented lock to synchronize the type
	// of access on the Item objects which aren't managed
	// by us.
	thread::SegmentedLock<NumKeys> m_itemLock;
	_ItemList m_list[NumBuckets];
	_ItemAlloc m_alloc;
	thread::Interlocked<UReg> m_nextRead;
	thread::Interlocked<UReg> m_nextWrite;
	thread::Interlocked<UReg> m_queueLength;
	boost::mutex m_readCS;
};

} // container

#include "../PopPack.h"
#include "ThreadSafeFIFO.inl"
