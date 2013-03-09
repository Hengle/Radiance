// ObjectPool.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Steve Nowalk
// See Radiance/LICENSE for licensing terms.

#include "../PushSystemMacros.h"

//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline ObjectPool<T>::ObjectPool() : m_init(false) {
}

template<typename T>
inline ObjectPool<T>::ObjectPool(
	Zone &zone,
	const char* name, 
	int numObjectsInBlock, 
	int objectAlignment, 
	int maxObjects
) : m_init(false)
{
	Create(
		zone,
		name,
		numObjectsInBlock,
		objectAlignment,
		maxObjects
	);
}

template<typename T>
inline ObjectPool<T>::~ObjectPool() {
	if (m_init)
		Destroy(); // this will destruct any allocated objects.
}

template<typename T>
inline void ObjectPool<T>::Create(
	Zone &zone,
	const char* name, 
	int numObjectsInBlock, 
	int objectAlignment, 
	int maxObjects
)
{
	m_memoryPool.Create(
		zone, 
		name, 
		sizeof(T), 
		numObjectsInBlock,
		objectAlignment,
		maxObjects
	);

	m_init = true;
}

template<typename T>
inline void ObjectPool<T>::Reset() {
	m_memoryPool.Reset(DestructorCallback);
}

template<typename T>
inline void ObjectPool<T>::Compact() {
	m_memoryPool.Compact();
}

template<typename T>
inline void ObjectPool<T>::Delete() {
	m_memoryPool.Delete(DestructorCallback);
}

template<typename T>
inline void ObjectPool<T>::Destroy() {
	m_memoryPool.Destroy(DestructorCallback);
	m_init = false;
}

template<typename T>
inline T* ObjectPool<T>::Construct() {
	void *p = m_memoryPool.GetChunk();
	if (!p)
		return 0;
	return new (p) T;
}

template<typename T>
inline T* ObjectPool<T>::SafeConstruct() {
	return new (m_memoryPool.SafeGetChunk()) T;
}

template<typename T>
inline void ObjectPool<T>::Destroy(T* object) {
	RAD_ASSERT(object);
	Destruct(object);
	m_memoryPool.ReturnChunk((void*)object);
}

template<typename T>
inline void ObjectPool<T>::Destruct(T *p) {
	p->~T();
}

template<typename T>
inline void ObjectPool<T>::DestructorCallback(void *p) {
	RAD_ASSERT(p);
	Destruct((T*)p);
}

//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline ThreadSafeObjectPool<T>::ThreadSafeObjectPool() : m_init(false) {
}

template<typename T>
inline ThreadSafeObjectPool<T>::ThreadSafeObjectPool(
	Zone &zone,
	const char* name, 
	int numObjectsInBlock, 
	int objectAlignment, 
	int maxObjects
) : m_init(false) {
	Create(
		zone,
		name,
		numObjectsInBlock,
		objectAlignment,
		maxObjects
	);
}

template<typename T>
inline ThreadSafeObjectPool<T>::~ThreadSafeObjectPool() {
	if (m_init)
		Destroy(); // this will destruct any allocated objects.
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Create(
	Zone &zone,
	const char* name, 
	int numObjectsInBlock, 
	int objectAlignment, 
	int maxObjects
) {
	m_memoryPool.Create(
		zone,
		name, 
		sizeof(T), 
		numObjectsInBlock,
		objectAlignment,
		maxObjects
	);

	m_init = true;
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Reset() {
	m_memoryPool.Reset(DestructorCallback);
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Compact() {
	m_memoryPool.Compact();
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Delete() {
	m_memoryPool.Delete(DestructorCallback);
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Destroy() {
	m_memoryPool.Destroy(DestructorCallback);
	m_init = false;
}

template<typename T>
inline T* ThreadSafeObjectPool<T>::Construct() {
	void *p = m_memoryPool.GetChunk();
	if (!p)
		return 0;
	return new (p) T;
}

template<typename T>
inline T* ThreadSafeObjectPool<T>::SafeConstruct() {
	return new (m_memoryPool.SafeGetChunk()) T;
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Destroy(T* object) {
	RAD_ASSERT(object);
	Destruct(object);
	m_memoryPool.ReturnChunk((void*)object);
}

template<typename T>
inline void ThreadSafeObjectPool<T>::Destruct(T *p) {
	p->~T();
}

template<typename T>
inline void ThreadSafeObjectPool<T>::DestructorCallback(void *p) {
	RAD_ASSERT(p);
	Destruct((T*)p);
}

#include "../PopSystemMacros.h"
