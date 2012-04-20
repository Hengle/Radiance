// ObjectPool.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Steve Nowalk
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "MemoryPool.h"
#include "../PushPack.h"

template<typename T>
class ObjectPool : public boost::noncopyable
{
public:

	ObjectPool();
	ObjectPool(
		Zone &zone,
		const char* name, 
		UReg growSize, 
		int alignment = DefaultAlignment, 
		UReg maxSize = MaxUReg
	);
	~ObjectPool();

	void Create(
		Zone &zone,
		const char* name, 
		UReg growSize, 
		int alignment = DefaultAlignment, 
		UReg maxSize = MaxUReg
	);

	void	Destroy();

	void    Reset();
	void    Compact();
	void	Delete();

	T *Construct();
	T *SafeConstruct();

#include "ObjectPoolConstruct.inl"	

	void	Destroy(T* object);

	UReg    NumUsedObjects() const;
	UReg    NumAllocatedObjects() const;

	UReg    GrowSize() const;
	UReg    MaxSize() const;

private:

	bool m_init;

	static void Destruct(T *p);
	static void DestructUsedCallback(void *p);

	MemoryPool m_memoryPool;
};

//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
class ThreadSafeObjectPool : public boost::noncopyable
{
public:

	ThreadSafeObjectPool();
	ThreadSafeObjectPool(
		Zone &zone,
		const char* name, 
		UReg growSize, 
		int alignment = DefaultAlignment, 
		UReg maxSize = MaxUReg
	);

	~ThreadSafeObjectPool();

	void Create(
		Zone &zone,
		const char* name, 
		UReg growSize, 
		int alignment = DefaultAlignment, 
		UReg maxSize = MaxUReg
	);

	void	Destroy();

	void    Reset();
	void    Compact();
	void	Delete();

	T *Construct();
	T *SafeConstruct();

#include "ThreadSafeObjectPoolConstruct.inl"	
	
	void	Destroy(T* object);

	UReg    NumUsedObjects() const;
	UReg    NumAllocatedObjects() const;

	UReg    GrowSize() const;
	UReg    MaxSize() const;

private:

	bool m_init;

	static void Destruct(T *p);
	static void DestructUsedCallback(void *p);

	ThreadSafeMemoryPool m_memoryPool;
};

#include "../PopPack.h"
#include "ObjectPool.inl"