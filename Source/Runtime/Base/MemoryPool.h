// MemoryPool.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Steve Nowalk & Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <limits>

#if !defined(RAD_OPT_SHIP) || defined(RAD_OPT_DEBUG)
#define RAD_OPT_MEMPOOL_DEBUG
#endif

#if defined(RAD_OPT_MEMPOOL_DEBUG)
#include "../Thread/Interlocked.h"
#endif

#include "../PushPack.h"

//! Memory pool efficiently batches small memory allocations into large ones
//! effectively reducing memory fragmentation.
class MemoryPool : public boost::noncopyable {
public:

	typedef void (*ChunkCallback)(void* chunk);
	
	//! Default constructs the pool, Create() must be called
	//! before the pool can be used.
	MemoryPool();
	
	//! Initializes the pool (no memory is allocated initially). \sa Create()
	MemoryPool(
		Zone &zone, 
		const char* name, 
		AddrSize chunkSize, 
		int numChunksInBlock, 
		int chunkAlignment = kDefaultAlignment, 
		int maxChunks = std::numeric_limits<int>::max()
	);

	//! Calls Destroy() if the memory pool was initialized.
	~MemoryPool();

	//! Initializes the pool (no memory is allocated initially).
	/*! \param zone The memory zone used for allocations.
	    \param name The name of the pool (for debugging).
		\param chunkSize The size of the individual chunks that will be returned
		                 by the pool when requested.
		\param numChunksInBlock How many chunks should be allocated in a single block.
		                This block will be allocated from zone memory.
		\param chunkAlignment The minimum alignment of the memory chunks returned by the
		                 pool.
		\param maxChunks The maximum number of allocated chunks (not blocks).
	 */
	void Create(
		Zone &zone, 
		const char* name, 
		AddrSize chunkSize, 
		int numChunksInBlock, 
		int chunkAlignment = kDefaultAlignment, 
		int maxChunks = std::numeric_limits<int>::max()
	);

	//! Destroys the pool, freeing all memory used.
	/*! All memory is released, Create() must be called to reinitialize. 
		\param usedChunkCallback optional function is executed on each used chunk before
		       memory is released.
	 */
	void Destroy(ChunkCallback usedChunkCallback = 0);

	//! All used chunks are reset and marked as free.
	/*! \param usedChunkCallback optional function is executed on each used chunk before
	           it is marked as free.
	 */
	void Reset(ChunkCallback usedChunkCallback = 0);

	//! Frees all used memory.
	/*! \param usedChunkCallback optional function is executed on each used chunk before
		       memory is released.
	 */
	void Delete(ChunkCallback usedChunkCallback = 0);

	//! Calls the specified callback routine on each used chunk.
	void WalkUsed(ChunkCallback usedChunkCallback);

	//! Frees any blocks that have no used chunks.
	void Compact();

	//! Returns a chunk of memory, or NULL if the maximum number of allocated chunks
	//! have been used.
	void* GetChunk();

	//! Returns a chunk of memory. If the maximum number of allocated chunks has been
	//! reached the application terminates with an error.
	void* SafeGetChunk();

	//! Returns a chunk of memory to the pool, marking it as free. It will be
	//! reused on a future call to GetChunk() or SafeGetChunk().
	void ReturnChunk(void* pT);

#if defined (RAD_OPT_MEMPOOL_DEBUG)
	//! Validates a chunk of memory from a memory pool.
	static void AssertChunkIsValid(void *pT);
#endif

	RAD_DECLARE_READONLY_PROPERTY(MemoryPool, numUsedChunks, int);
	RAD_DECLARE_READONLY_PROPERTY(MemoryPool, numAllocatedChunks, int);
	RAD_DECLARE_READONLY_PROPERTY(MemoryPool, numChunksInBlock, int);
	RAD_DECLARE_READONLY_PROPERTY(MemoryPool, maxChunks, int);

	void SetChunkConstructor(ChunkCallback c);
	void SetChunkDestructor (ChunkCallback  d);

	static MemoryPool *PoolFromChunk(void *chunk);

private:

	struct Pool;
	struct PoolNode;

	enum { 
		kPoolAlignment = kDefaultAlignment 
	};

	struct PoolNode {
	
#if defined (RAD_OPT_MEMPOOL_DEBUG)
		enum { MagicId = RAD_FOURCC_LE('M','E','P','N') };
		U32 m_magicID;
#endif

		Pool*       m_pool;
		PoolNode*   m_next;
		U8          m_used;
	};

	struct RADRT_CLASS Pool {
		
		static AddrSize	CalculateMemorySize(
			AddrSize inArraySize, 
			AddrSize inDataSize, 
			AddrSize alignment
		);

		AddrSize Create(
			AddrSize inArraySize, 
			AddrSize chunkSize, 
			void* pBaseAddr, 
			PoolNode** inFreeHead
		);
		
		void Destroy(
			AddrSize inArraySize, 
			ChunkCallback destructor, 
			ChunkCallback callback
		);

		void Reset(
			AddrSize inArraySize, 
			PoolNode** inFreeHead, 
			ChunkCallback destructor, 
			ChunkCallback callback
		);

		void WalkUsed(
			AddrSize inArraySize, 
			ChunkCallback callback
		);

#if defined (RAD_OPT_MEMPOOL_DEBUG)
		enum { MagicId = RAD_FOURCC_LE('M','E','M','P') };
		U32 m_magicID;
#endif

		AddrSize     m_numNodesInUse;
		void*        m_array;
		Pool*        m_next;
		MemoryPool * m_memPool;
	};

	friend struct Pool;

	static void *UserDataFromPoolNode(PoolNode *node);
	static PoolNode *PoolNodeFromUserData(void *data);

#if defined(RAD_OPT_MEMPOOL_DEBUG)
	static bool IsPoolNodeValid(PoolNode *node);
	static void AssertPoolNodeIsValid(PoolNode *node);
#endif

	enum { 
		kMaxNameLen = 31 
	};

	RAD_DECLARE_GET(numUsedChunks, int) {
		return m_numUsedChunks;
	}

	RAD_DECLARE_GET(numAllocatedChunks, int) {
		return m_numAllocatedChunks;
	}
	
	RAD_DECLARE_GET(numChunksInBlock, int) {
		return m_numChunksInBlock;
	}

	RAD_DECLARE_GET(maxChunks, int) {
		return m_maxChunks;
	}
	
	AddrSize      m_chunkSize;
	AddrSize      m_offsetToNext;
	Zone*         m_zone;
	Pool*         m_poolList;
	PoolNode*     m_freeList;
	ChunkCallback m_constructor;
	ChunkCallback m_destructor;
	
#if defined(RAD_OPT_MEMPOOL_DEBUG)
	thread::Interlocked<int> m_concurrencyCheck;
#endif

	int m_numChunksInBlock;
	int m_maxChunks;
	int m_numUsedChunks;
	int m_numAllocatedChunks;
	int m_alignment;
		 
	bool m_inited;
	char m_name[kMaxNameLen+1];
};

//////////////////////////////////////////////////////////////////////////////////////////

//! Memory pool efficiently batches small memory allocations into large ones
//! effectively reducing memory fragmentation.
/*! This class is identical to MemoryPool except that it is thread-safe. */
class RADRT_CLASS ThreadSafeMemoryPool : public boost::noncopyable {
public:

	//! Default constructs the pool, Create() must be called
	//! before the pool can be used.
	ThreadSafeMemoryPool();

	//! Initializes the pool (no memory is allocated initially). \sa Create()
	ThreadSafeMemoryPool(
		Zone &zone, 
		const char* name, 
		AddrSize chunkSize, 
		int numChunksInBlock, 
		int chunkAlignment = kDefaultAlignment, 
		int maxChunks = std::numeric_limits<int>::max()
	);

	//! Calls Destroy() if the pool was initialized.
	~ThreadSafeMemoryPool();

	//! Initializes the pool (no memory is allocated initially).
	/*! \param zone The memory zone used for allocations.
	    \param name The name of the pool (for debugging).
		\param chunkSize The size of the individual chunks that will be returned
		                 by the pool when requested.
		\param numChunksInBlock How many chunks should be allocated in a single block.
		                This block will be allocated from zone memory.
		\param chunkAlignment The minimum alignment of the memory chunks returned by the
		                 pool.
		\param maxChunks The maximum number of allocated chunks (not blocks).
	 */
	void Create(
		Zone &zone,
		const char* name, 
		AddrSize chunkSize, 
		int numChunksInBlock, 
		int chunkAlignment = kDefaultAlignment, 
		int maxChunks = std::numeric_limits<int>::max()
	);

	//! Destroys the pool, freeing all memory used.
	/*! All memory is released, Create() must be called to reinitialize. 
		\param usedChunkCallback optional function is executed on each used chunk before
		       memory is released.
	 */
	void Destroy(MemoryPool::ChunkCallback usedChunkCallback = 0);

	//! All used chunks are reset and marked as free.
	/*! \param usedChunkCallback optional function is executed on each used chunk before
	           it is marked as free.
	 */
	void Reset(MemoryPool::ChunkCallback usedChunkCallback = 0);

	//! Frees all used memory.
	/*! \param usedChunkCallback optional function is executed on each used chunk before
		       memory is released.
	 */
	void Delete(MemoryPool::ChunkCallback usedChunkCallback = 0);

	//! Calls the specified callback routine on each used chunk.
	void WalkUsed(MemoryPool::ChunkCallback callback);

	//! Frees any blocks that have no used chunks.
	void Compact();

	//! Returns a chunk of memory. If the maximum number of allocated chunks have been
	//! reached the application terminates with an error.
	void *GetChunk();

	//! Returns a chunk of memory. If the maximum number of allocated chunks has been
	//! reached the application terminates with an error.
	void *SafeGetChunk();

	//! Returns a chunk of memory to the pool, marking it as free. It will be
	//! reused on a future call to GetChunk() or SafeGetChunk().
	void ReturnChunk(void* pT);

	RAD_DECLARE_READONLY_PROPERTY(ThreadSafeMemoryPool, numUsedChunks, int);
	RAD_DECLARE_READONLY_PROPERTY(ThreadSafeMemoryPool, numAllocatedChunks, int);
	RAD_DECLARE_READONLY_PROPERTY(ThreadSafeMemoryPool, numChunksInBlock, int);
	RAD_DECLARE_READONLY_PROPERTY(ThreadSafeMemoryPool, maxChunks, int);

	void SetChunkConstructor(MemoryPool::ChunkCallback c);
	void SetChunkDestructor (MemoryPool::ChunkCallback  d);

	static ThreadSafeMemoryPool *PoolFromChunk(void *chunk);

private:

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	RAD_DECLARE_GET(numUsedChunks, int) {
		Lock L(m_m);
		return m_pool.numUsedChunks;
	}

	RAD_DECLARE_GET(numAllocatedChunks, int) {
		Lock L(m_m);
		return m_pool.numAllocatedChunks;
	}
	
	RAD_DECLARE_GET(numChunksInBlock, int) {
		Lock L(m_m);
		return m_pool.numChunksInBlock;
	}

	RAD_DECLARE_GET(maxChunks, int) {
		Lock L(m_m);
		return m_pool.maxChunks;
	}

	MemoryPool m_pool;
	mutable Mutex m_m;
};

//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename Tag, 
	typename _Zone, 
	AddrSize ChunkSize, 
	int _NumChunksInBlock, 
	int _MaxChunks, 
	int Alignment, 
	typename Pool
>
class SingletonPool : boost::noncopyable
{
public:

	static void Destroy(MemoryPool::ChunkCallback usedChunkCallback = 0);
	static void Reset(MemoryPool::ChunkCallback usedChunkCallback = 0);
	static void Delete(MemoryPool::ChunkCallback usedChunkCallback = 0);
	static void WalkUsed(MemoryPool::ChunkCallback callback);

	static void Compact();

	static void *GetChunk();
	static void *SafeGetChunk();

	static void ReturnChunk(void* pT);

	static int NumUsedChunks();
	static int NumAllocatedChunks();

	static int NumChunksInBlock();
	static int MaxChunks();

	static void SetChunkConstructor(MemoryPool::ChunkCallback c);
	static void SetChunkDestructor (MemoryPool::ChunkCallback  d);

	static Pool *PoolFromChunk(void *p);

private:

	static Pool &GetPool();
};

//////////////////////////////////////////////////////////////////////////////////////////

// NOTE: pool_allocator only supports allocating 1 element at a time
// suitable for list/set/map, not useable with vectors

struct pool_allocator_tag {};

template <typename T, typename _Zone, int NumChunksInBlock, int MaxChunks, typename Pool>
class pool_allocator {
public:
	typedef pool_allocator<T, _Zone, NumChunksInBlock, MaxChunks, Pool> self_type;
	typedef _Zone zone_type;
	typedef T value_type;
	typedef SingletonPool<pool_allocator_tag, _Zone, sizeof(T), NumChunksInBlock, MaxChunks, RAD_ALIGNOF(T), Pool> pool_type;
	typedef value_type *pointer;
	typedef const value_type *const_pointer;
	typedef value_type &reference;
	typedef const value_type &const_reference;
	typedef AddrSize size_type;
	typedef SAddrSize difference_type;

	template <typename U>
	struct rebind { typedef pool_allocator<U, _Zone, NumChunksInBlock, MaxChunks, Pool> other; };

	pool_allocator() {}
	// The following is not explicit, mimicking std::allocator [20.4.1]
	template <typename U>
	pool_allocator(const pool_allocator<U, _Zone, NumChunksInBlock, MaxChunks, Pool> &) {}

	static pointer address(reference r) { return &r; }
	static const_pointer address(const_reference s) { return &s; }
	static size_type max_size()	{ return (std::numeric_limits<size_type>::max)(); }
	static void construct(const pointer ptr, const value_type & t) { new (ptr) T(t); }
	static void destroy(const pointer ptr) {
		ptr->~T();
		(void) ptr;
	}

	bool operator==(const self_type &) const { return true; }
	bool operator!=(const self_type &) const { return false; }

	static pointer allocate(const size_type n) {
		RAD_ASSERT(n==1);
		return (pointer)pool_type::SafeGetChunk();
	}

	static pointer allocate(const size_type n, const void * const) { return allocate(n); }

	static void deallocate(const pointer ptr, const size_type n) {
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
