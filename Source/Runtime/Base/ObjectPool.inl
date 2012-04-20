// ObjectPool.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Steve Nowalk
// See Radiance/LICENSE for licensing terms.

#include "../PushSystemMacros.h"

//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline ObjectPool<T>::ObjectPool() : m_init(false)
{
}

template<typename T>
inline ObjectPool<T>::ObjectPool(
	Zone &zone,
	const char* name, 
	UReg growSize, 
	int alignment, 
	UReg maxSize
) : m_init(false)
{
	Create(
		zone,
		name,
		growSize,
		alignment,
		maxSize
	);
}

template<typename T>
inline ObjectPool<T>::~ObjectPool()
{
	if (m_init)
		Destroy(); // this will destruct any allocated objects.
}

template<typename T>
inline void ObjectPool<T>::Create(
	Zone &zone,
	const char* name, 
	UReg growSize, 
	int alignment, 
	UReg maxSize
)
{
	m_memoryPool.Create(
		zone, 
		name, 
		sizeof(T), 
		growSize, 
		alignment, 
		maxSize
	);

	m_init = true;
}

template<typename T>
inline void ObjectPool<T>::Reset()
{
	m_memoryPool.Reset(DestructUsedCallback);
}

template<typename T>
inline void ObjectPool<T>::Compact()
{
	m_memoryPool.Compact();
}

template<typename T>
inline void ObjectPool<T>::Delete()
{
	m_memoryPool.Delete(DestructUsedCallback);
}

template<typename T>
inline UReg ObjectPool<T>::NumAllocatedObjects() const
{
	return m_memoryPool.NumAllocatedChunks();
}

template<typename T>
inline UReg ObjectPool<T>::GrowSize() const
{
	return m_memoryPool.GrowSize();
}

template<typename T>
inline UReg ObjectPool<T>::MaxSize() const
{
	return m_memoryPool.MaxSize();
}

template<typename T>
inline UReg ObjectPool<T>::NumUsedObjects() const
{
	return m_memoryPool.NumUsedChunks();
}

template<typename T>
inline void ObjectPool<T>::Destroy()
{
	m_memoryPool.Destroy(DestructUsedCallback);
	m_init = false;
}

template<typename T>
inline T* ObjectPool<T>::Construct()
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
		return 0;
	return new (p) T;
}

template<typename T>
inline T* ObjectPool<T>::SafeConstruct()
{
	return new (m_memoryPool.SafeGetChunk()) T;
}


template<typename T>
inline void ObjectPool<T>::Destroy(T* object)
{
	RAD_ASSERT(object);
	Destruct(object);
	m_memoryPool.ReturnChunk((void*)object);
}

template<typename T>
inline void ObjectPool<T>::Destruct(T *p)
{
	p->~T();
}

template<typename T>
inline void ObjectPool<T>::DestructUsedCallback(void *p)
{
	RAD_ASSERT(p);
	Destruct((T*)p);
}

//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline ThreadSafeObjectPool<T>::ThreadSafeObjectPool() : m_init(false)
{
}

template<typename T>
inline ThreadSafeObjectPool<T>::ThreadSafeObjectPool(
	Zone &zone,
	const char* name, 
	UReg growSize, 
	int alignment, 
	UReg maxSize
) : m_init(false)
{
	Create(
		zone,
		name,
		growSize,
		alignment,
		maxSize
	);
}

template<typename T>
inline ThreadSafeObjectPool<T>::~ThreadSafeObjectPool()
{
	if (m_init)
		Destroy(); // this will destruct any allocated objects.
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Create(
	Zone &zone,
	const char* name, 
	UReg growSize, 
	int alignment, 
	UReg maxSize
)
{
	m_memoryPool.Create(
		zone,
		name, 
		sizeof(T), 
		growSize, 
		alignment, 
		maxSize
	);

	m_init = true;
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Reset()
{
	m_memoryPool.Reset(DestructUsedCallback);
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Compact()
{
	m_memoryPool.Compact();
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Delete()
{
	m_memoryPool.Delete(DestructUsedCallback);
}

template<typename T>
inline UReg ThreadSafeObjectPool<T>::NumAllocatedObjects() const
{
	return m_memoryPool.NumAllocatedChunks();
}

template<typename T>
inline UReg ThreadSafeObjectPool<T>::GrowSize() const
{
	return m_memoryPool.GrowSize();
}

template<typename T>
inline UReg ThreadSafeObjectPool<T>::MaxSize() const
{
	return m_memoryPool.MaxSize();
}

template<typename T>
inline UReg ThreadSafeObjectPool<T>::NumUsedObjects() const
{
	return m_memoryPool.NumUsedChunks();
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Destroy()
{
	m_memoryPool.Destroy(DestructUsedCallback);
	m_init = false;
}

template<typename T>
inline T* ThreadSafeObjectPool<T>::Construct()
{
	void *p = m_memoryPool.GetChunk();
	if (!p)
		return 0;
	return new (p) T;
}

template<typename T>
inline T* ThreadSafeObjectPool<T>::SafeConstruct()
{
	return new (m_memoryPool.SafeGetChunk()) T;
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Destroy(T* object)
{
	RAD_ASSERT(object);
	Destruct(object);
	m_memoryPool.ReturnChunk((void*)object);
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Destruct(T *p)
{
	p->~T();
}

template<typename T>
inline void ThreadSafeObjectPool<T>::DestructUsedCallback(void *p)
{
	RAD_ASSERT(p);
	Destruct((T*)p);
}

#include "../PopSystemMacros.h"
