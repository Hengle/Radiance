// MemoryPool.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Steve Nowalk
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <limits>
#include "../PushPack.h"

//////////////////////////////////////////////////////////////////////////////////////////

class MemoryPool : public boost::noncopyable
{
public:

	typedef void (*ChunkConstructor)(void* chunk);
	typedef void (*ChunkDestructor) (void* chunk);
	typedef void (*WalkCallback) (void* chunk);

	MemoryPool();
	MemoryPool(
		Zone &zone, 
		const char* name, 
		AddrSize chunkSize, 
		UReg growSize, 
		int alignment = DefaultAlignment, 
		UReg maxSize = std::numeric_limits<UReg>::max()
	);

	~MemoryPool();

	//
	// "name" is the name.
	// "chunkSize" is the size of each chunk.
	// "growSize" is the number of chunks to grow when space is exhausted.
	// "maxSize" is the maximum number of objects.
	//

	void Create(
		Zone &zone, 
		const char* name, 
		AddrSize chunkSize, 
		UReg growSize, 
		int alignment = DefaultAlignment, 
		UReg maxSize = std::numeric_limits<UReg>::max()
	);

	void Destroy(WalkCallback usedChunkCallback = 0);
	void Reset(WalkCallback usedChunkCallback = 0);
	void Compact();
	void Delete(WalkCallback usedChunkCallback = 0);
	void WalkUsed(WalkCallback usedChunkCallback);

	void* GetChunk();
	void* SafeGetChunk();

	void ReturnChunk(void* pT);

	UReg NumUsedChunks() const;
	UReg NumAllocatedChunks() const;

	UReg GrowSize() const;
	UReg MaxSize() const;

	void SetChunkConstructor(ChunkConstructor c);
	void SetChunkDestructor (ChunkDestructor  d);

	static MemoryPool *PoolFromUserData(void *p);

private:

	struct Pool;
	struct PoolNode;

	enum { PoolAlignment = DefaultAlignment };

	struct PoolNode
	{
	
#if defined (RAD_OPT_DEBUG)
		enum { MagicId = RAD_FOURCC_LE('M','E','P','N') };
		UReg        m_magicID;
#endif

		Pool*       m_pool;
		PoolNode*   m_next;
		bool        m_used;
	};

	struct RADRT_CLASS Pool
	{
		static AddrSize	CalculateMemorySize(UReg inArraySize, AddrSize inDataSize, int alignment);

		AddrSize Create(UReg inArraySize,AddrSize chunkSize,void* pBaseAddr,PoolNode** inFreeHead);
		void Destroy(UReg inArraySize,ChunkDestructor destructor, WalkCallback callback);

		void Reset(UReg inArraySize,PoolNode** inFreeHead, ChunkDestructor destructor, WalkCallback callback);
		void WalkUsed(UReg inArraySize,WalkCallback callback);

#if defined (RAD_OPT_DEBUG)
		enum { MagicId = RAD_FOURCC_LE('M','E','M','P') };
		UReg        m_magicID;
#endif

		UReg         m_numNodesInUse;
		void*        m_array;
		Pool*        m_next;
		MemoryPool * m_memPool;
	};

	friend struct Pool;

	static void *UserDataFromPoolNode(PoolNode *node);
	static PoolNode *PoolNodeFromUserData(void *data);

#if defined(RAD_OPT_DEBUG)
	static bool IsPoolNodeValid(PoolNode *node);
	static void AssertPoolNodeIsValid(PoolNode *node);
#endif

	enum { MaxNameLen = 31 };

	bool                        m_inited;
	char                        m_name[MaxNameLen+1];
	UReg                        m_growSize;
	UReg                        m_maxSize;
	AddrSize                    m_dataSize;
	UReg                        m_numUsedObjects;
	UReg                        m_numAllocatedObjects;
	Pool*                       m_poolList;
	PoolNode*                   m_freeList;
	ChunkConstructor            m_constructor;
	ChunkDestructor             m_destructor;
	int                         m_alignment;
	AddrSize                    m_offsetToNext;
	Zone*                       m_zone;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS ThreadSafeMemoryPool : public boost::noncopyable
{
public:

	ThreadSafeMemoryPool();
	ThreadSafeMemoryPool(
		Zone &zone, 
		const char* name, 
		AddrSize chunkSize, 
		UReg growSize, 
		int alignment = DefaultAlignment, 
		UReg maxSize = std::numeric_limits<UReg>::max()
	);
	~ThreadSafeMemoryPool();

	//
	// "name" is the name.
	// "chunkSize" is the size of each chunk.
	// "growSize" is the number of chunks to grow when space is exhausted.
	// "maxSize" is the maximum number of objects.
	//

	void Create(
		Zone &zone,
		const char* name, 
		AddrSize chunkSize, 
		UReg growSize, 
		int alignment = DefaultAlignment, 
		UReg maxSize = std::numeric_limits<UReg>::max()
	);

	void Destroy(MemoryPool::WalkCallback usedChunkCallback = 0);

	void Reset(MemoryPool::WalkCallback usedChunkCallback = 0);
	void Compact();
	void Delete(MemoryPool::WalkCallback usedChunkCallback = 0);
	void WalkUsed(MemoryPool::WalkCallback callback);

	void *GetChunk();
	void *SafeGetChunk();

	void ReturnChunk(void* pT);

	UReg NumUsedChunks() const;
	UReg NumAllocatedChunks() const;

	UReg GrowSize() const;
	UReg MaxSize() const;

	void SetChunkConstructor(MemoryPool::ChunkConstructor c);
	void SetChunkDestructor (MemoryPool::ChunkDestructor  d);

	static ThreadSafeMemoryPool *PoolFromUserData(void *p);

private:

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	MemoryPool m_pool;
	Mutex m_cs;
};

//////////////////////////////////////////////////////////////////////////////////////////

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
class SingletonPool : boost::noncopyable
{
public:

	static void Destroy(MemoryPool::WalkCallback usedChunkCallback = 0);

	static void Reset(MemoryPool::WalkCallback usedChunkCallback = 0);
	static void Compact();
	static void Delete(MemoryPool::WalkCallback usedChunkCallback = 0);
	static void WalkUsed(MemoryPool::WalkCallback callback);

	static void *GetChunk();
	static void *SafeGetChunk();

	static void ReturnChunk(void* pT);

	static UReg NumUsedChunks();
	static UReg NumAllocatedChunks();

	static UReg GrowSize();
	static UReg MaxSize();

	static void SetChunkConstructor(MemoryPool::ChunkConstructor c);
	static void SetChunkDestructor (MemoryPool::ChunkDestructor  d);

	static Pool *PoolFromUserData(void *p);

private:

	static Pool &GetPool();
};

//////////////////////////////////////////////////////////////////////////////////////////

// NOTE: pool_allocator only supports allocating 1 element at a time
// suitable for list/set/map, not useable with vectors

struct pool_allocator_tag {};

template <typename T, typename _Zone, UReg GrowSize, UReg MaxSize, int Alignment, typename Pool>
class pool_allocator
{
public:
	typedef pool_allocator<T, _Zone, GrowSize, MaxSize, Alignment, Pool> self_type;
	typedef _Zone zone_type;
	typedef T value_type;
	typedef SingletonPool<pool_allocator_tag, _Zone, sizeof(T), GrowSize, MaxSize, Alignment, Pool> pool_type;
	typedef value_type *pointer;
	typedef const value_type *const_pointer;
	typedef value_type &reference;
	typedef const value_type &const_reference;
	typedef AddrSize size_type;
	typedef SAddrSize difference_type;

	template <typename U>
	struct rebind { typedef pool_allocator<U, _Zone, GrowSize, MaxSize, Alignment, Pool> other; };

	pool_allocator() {}
	// The following is not explicit, mimicking std::allocator [20.4.1]
	template <typename U>
	pool_allocator(const pool_allocator<U, _Zone, GrowSize, MaxSize, Alignment, Pool> &) {}

	static pointer address(reference r) { return &r; }
	static const_pointer address(const_reference s) { return &s; }
	static size_type max_size()	{ return (std::numeric_limits<size_type>::max)(); }
	static void construct(const pointer ptr, const value_type & t) { new (ptr) T(t); }
	static void destroy(const pointer ptr)
	{
		ptr->~T();
		(void) ptr;
	}

	bool operator==(const pool_allocator<T, _Zone, GrowSize, MaxSize, Alignment, Pool> &) const { return true; }
	bool operator!=(const pool_allocator<T, _Zone, GrowSize, MaxSize, Alignment, Pool> &) const { return false; }

	static pointer allocate(const size_type n)
	{
		RAD_ASSERT(n==1);
		return (pointer)pool_type::SafeGetChunk();
	}

	static pointer allocate(const size_type n, const void * const) { return allocate(n); }

	static void deallocate(const pointer ptr, const size_type n)
	{
#ifdef BOOST_NO_PROPER_STL_DEALLOCATE
		if (ptr == 0 || n == 0)
			return;
#endif
		RAD_ASSERT(n==1);
		pool_type::ReturnChunk(ptr);
	}
};

#include "../PopPack.h"
#include "MemoryPool.inl"
