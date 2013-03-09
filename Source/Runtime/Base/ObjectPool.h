// ObjectPool.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Steve Nowalk
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "MemoryPool.h"
#include <limits>
#include "../PushPack.h"

//! A pool of objects. Supports non-POD objects (classes, etc).
//! \sa MemoryPool
template<typename T>
class ObjectPool : public boost::noncopyable {
public:

	//! Default constructs the pool, Create() must be called
	//! before the pool can be used.
	ObjectPool();

	//! Initializes the pool (no memory is allocated initially). \sa Create()
	ObjectPool(
		Zone &zone,
		const char* name, 
		int numObjectsInBlock, 
		int objectAlignment = RAD_ALIGNOF(T), 
		int maxObjects = std::numeric_limits<int>::max()
	);

	//! Calls Destroy() if the object pool was initialized.
	~ObjectPool();

	//! Initializes the pool (no memory is allocated initially).
	/*! \param zone The memory zone used for allocations.
	    \param name The name of the pool (for debugging).
		\param object How many objects should be allocated in a single block.
		              This block will be allocated from zone memory.
		\param objectAlignment The minimum alignment of the objects returned by the
		                       pool.
		\param maxObjects The maximum number of allocated objects (not blocks).
	 */
	void Create(
		Zone &zone,
		const char* name, 
		int numObjectsInBlock, 
		int objectAlignment = RAD_ALIGNOF(T), 
		int maxObjects = std::numeric_limits<int>::max()
	);

	//! Destroys the pool, freeing all memory used.
	/*! All objects are destruct, memory is released, Create() must be called to reinitialize. */
	void Destroy();

	//! All used objects are destructed and marked as free.
	void Reset();

	//! Destructs all used objects and frees all used memory.
	void Delete();

	//! Frees any blocks that have no used objects.
	void Compact();

	//! Constructs an object and returns it, or NULL if the maximum number of 
	//! allocated objects have been used.
	T *Construct();

	//! Constructs an object and returns it .If the maximum number of allocated 
	//! objects has been reached the application terminates with an error.
	T *SafeConstruct();

#include "ObjectPoolConstruct.inl"	

	//! Destructs an object and returns it to the free list.
	void Destroy(T* object);
		
	RAD_DECLARE_READONLY_PROPERTY(ObjectPool, numUsedObjects, int);
	RAD_DECLARE_READONLY_PROPERTY(ObjectPool, numAllocatedObjects, int);
	RAD_DECLARE_READONLY_PROPERTY(ObjectPool, numObjectsInBlock, int);
	RAD_DECLARE_READONLY_PROPERTY(ObjectPool, maxObjects, int);

private:

	static void Destruct(T *p);
	static void DestructorCallback(void *p);

	RAD_DECLARE_GET(numUsedObjects, int) {
		return m_memoryPool.numUsedChunks;
	}

	RAD_DECLARE_GET(numAllocatedObjects, int) {
		return m_memoryPool.numAllocatedChunks;
	}
	
	RAD_DECLARE_GET(numObjectsInBlock, int) {
		return m_memoryPool.numChunksInBlock;
	}

	RAD_DECLARE_GET(maxObjects, int) {
		return m_memoryPool.maxChunks;
	}

	MemoryPool m_memoryPool;
	bool m_init;
};

//////////////////////////////////////////////////////////////////////////////////////////

//! A pool of objects. Supports non-POD objects (classes, etc).
/*! This class is identical to ObjectPool except that it is thread-safe. 
    \sa ThreadSafeMemoryPool
*/
template<typename T>
class ThreadSafeObjectPool : public boost::noncopyable {
public:

	//! Default constructs the pool, Create() must be called
	//! before the pool can be used.
	ThreadSafeObjectPool();

	//! Initializes the pool (no memory is allocated initially). \sa Create()
	ThreadSafeObjectPool(
		Zone &zone,
		const char* name, 
		int numObjectsInBlock, 
		int objectAlignment = RAD_ALIGNOF(T), 
		int maxObjects = std::numeric_limits<int>::max()
	);

	//! Calls Destroy() if the object pool was initialized.
	~ThreadSafeObjectPool();

	//! Initializes the pool (no memory is allocated initially).
	/*! \param zone The memory zone used for allocations.
	    \param name The name of the pool (for debugging).
		\param object How many objects should be allocated in a single block.
		              This block will be allocated from zone memory.
		\param objectAlignment The minimum alignment of the objects returned by the
		                       pool.
		\param maxObjects The maximum number of allocated objects (not blocks).
	 */
	void Create(
		Zone &zone,
		const char* name, 
		int numObjectsInBlock, 
		int objectAlignment = RAD_ALIGNOF(T), 
		int maxObjects = std::numeric_limits<int>::max()
	);

	//! Destroys the pool, freeing all memory used.
	/*! All objects are destruct, memory is released, Create() must be called to reinitialize. */
	void	Destroy();

	//! All used objects are destructed and marked as free.
	void Reset();

	//! Destructs all used objects and frees all used memory.
	void Delete();

	//! Frees any blocks that have no used objects.
	void Compact();

	//! Constructs an object and returns it, or NULL if the maximum number of 
	//! allocated objects have been used.
	T *Construct();

	//! Constructs an object and returns it .If the maximum number of allocated 
	//! objects has been reached the application terminates with an error.
	T *SafeConstruct();

#include "ThreadSafeObjectPoolConstruct.inl"	
	
	//! Destructs an object and returns it to the free list.
	void Destroy(T* object);
		
	RAD_DECLARE_READONLY_PROPERTY(ThreadSafeObjectPool, numUsedObjects, int);
	RAD_DECLARE_READONLY_PROPERTY(ThreadSafeObjectPool, numAllocatedObjects, int);
	RAD_DECLARE_READONLY_PROPERTY(ThreadSafeObjectPool, numObjectsInBlock, int);
	RAD_DECLARE_READONLY_PROPERTY(ThreadSafeObjectPool, maxObjects, int);

private:

	static void Destruct(T *p);
	static void DestructorCallback(void *p);

	RAD_DECLARE_GET(numUsedObjects, int) {
		return m_memoryPool.numUsedChunks;
	}

	RAD_DECLARE_GET(numAllocatedObjects, int) {
		return m_memoryPool.numAllocatedChunks;
	}
	
	RAD_DECLARE_GET(numObjectsInBlock, int) {
		return m_memoryPool.numChunksInBlock;
	}

	RAD_DECLARE_GET(maxObjects, int) {
		return m_memoryPool.maxChunks;
	}

	ThreadSafeMemoryPool m_memoryPool;
	bool m_init;

};

#include "../PopPack.h"
#include "ObjectPool.inl"