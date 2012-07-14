// MemoryPool.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Steve Nowalk
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "MemoryPool.h"
#include "../StringBase.h"

//////////////////////////////////////////////////////////////////////////////////////////

MemoryPool::MemoryPool() : 
m_zone(0), 
m_poolList(0), 
m_freeList(0), 
m_numUsedObjects(0), 
m_numAllocatedObjects(0)
{
	string::ncpy(m_name, "uninited", MaxNameLen+1);
	m_inited = false;
}

MemoryPool::MemoryPool(
	Zone &zone,
	const char* name,
	AddrSize chunkSize,
	UReg growSize,
	int alignment,
	UReg maxSize
) : 
m_zone(0), 
m_poolList(0), 
m_freeList(0), 
m_numUsedObjects(0), 
m_numAllocatedObjects(0)
{
	m_inited = false;

	Create(
		zone,
		name,
		chunkSize,
		growSize,
		alignment,
		maxSize
	);
}

MemoryPool::~MemoryPool()
{
	if (m_poolList)
	{
		RAD_ASSERT(m_inited);
		Destroy();
	}
	RAD_VERIFY(!m_poolList);
	RAD_VERIFY(m_numUsedObjects == 0);
	RAD_VERIFY(m_numAllocatedObjects == 0);
}

void MemoryPool::Create(
	Zone &zone, 
	const char* name, 
	AddrSize inDataSize, 
	UReg inGrowSize, 
	int alignment, 
	UReg inMaxSize
)
{
	RAD_ASSERT(!m_inited);
	RAD_ASSERT(inDataSize > 0);
	RAD_ASSERT(alignment >= PoolAlignment);
	BOOST_STATIC_ASSERT(MemoryPool::PoolAlignment >= DefaultAlignment);

	m_inited = true;

	string::ncpy(m_name, name, MaxNameLen+1);

	m_zone                  = &zone;
	m_growSize              = inGrowSize;
	m_maxSize               = inMaxSize;
	m_dataSize              = inDataSize;
	m_numUsedObjects        = 0;
	m_numAllocatedObjects   = 0;
	m_poolList              = NULL;
	m_freeList              = NULL;
	m_constructor           = NULL;
	m_destructor            = NULL;
	m_alignment             = alignment;
#if defined(RAD_OPT_DEBUG)
	m_offsetToNext          = Align<AddrSize>(sizeof(PoolNode) + inDataSize + sizeof(UReg), alignment);
#else
	m_offsetToNext          = Align<AddrSize>(sizeof(PoolNode) + inDataSize, alignment);
#endif
}

void MemoryPool::Destroy(WalkCallback callback)
{
	RAD_ASSERT(m_inited);

	Delete(callback);

	m_inited = false;
}

void MemoryPool::Delete(WalkCallback callback)
{
	RAD_ASSERT(m_inited);
	Pool* pool = m_poolList;

	if (m_numUsedObjects == 0) callback = 0; // no callback!

	while (pool)
	{	
		Pool* next = pool->m_next;

		pool->Destroy(m_growSize,m_destructor,callback);
		zone_free(pool);

		pool = next;
	}

	m_numUsedObjects = 0;
	m_numAllocatedObjects = 0;

	m_poolList = NULL;
	m_freeList = NULL;
}

void MemoryPool::Reset(WalkCallback callback)
{
	RAD_ASSERT(m_inited);
	
	if (m_numUsedObjects == 0) callback = 0; // no callback!

	m_freeList = NULL;

	Pool* pool = m_poolList;

	while (pool)
	{
		pool->Reset(m_growSize,&m_freeList,m_destructor,callback);

		pool = pool->m_next;
	}

	m_numUsedObjects = 0;
}

void MemoryPool::WalkUsed(WalkCallback callback)
{
	RAD_ASSERT(callback);
	RAD_ASSERT(m_inited);

	Pool* pool = m_poolList;

	while (pool)
	{
		pool->WalkUsed(m_growSize, callback);
		pool = pool->m_next;
	}
}

void MemoryPool::Compact()
{
	RAD_ASSERT(m_inited);


	Pool* pool		= m_poolList;
	Pool* previous	= NULL;

	while (pool)
	{
		Pool* next = pool->m_next;

		if (pool->m_numNodesInUse == 0)
		{
			m_numAllocatedObjects -= m_growSize;
			m_numUsedObjects -= pool->m_numNodesInUse;

			pool->Destroy(m_growSize,m_destructor,0);
			zone_free(pool);

			if (previous)
			{
				previous->m_next = next;
			}
			else
			{
				m_poolList = next;
			}
		}
		else
		{
			previous = pool;
		}

		pool = next;
	}

	// relink free list

	m_freeList = 0;
	pool = m_poolList;

	while (pool)
	{
		if (pool->m_numNodesInUse < m_growSize)
		{
			for (UReg i = 0; i < m_growSize; ++i)
			{
				PoolNode *node = (PoolNode*)(((U8*)pool->m_array) + (i*m_offsetToNext));
				if (!node->m_used)
				{
					node->m_next = m_freeList;
					m_freeList = node;
				}
			}
		}

		pool = pool->m_next;
	}
}

void* MemoryPool::SafeGetChunk()
{
	void* n = GetChunk();
	RAD_OUT_OF_MEM(n);
	return n;
}

void* MemoryPool::GetChunk()
{
	RAD_ASSERT(m_inited);


	//
	// Is our list of pool nodes empty?
	//

	if (!m_freeList)
	{
		//
		// Have we hit our ceiling?
		//

		if (m_numAllocatedObjects >= m_maxSize)
		{
			return NULL;
		}

		AddrSize	totalBytes = Pool::CalculateMemorySize(m_growSize, m_dataSize, m_alignment);
		AddrSize	nStart;
		AddrSize	nEnd;

		Pool* newPool = (Pool*)safe_zone_malloc(*m_zone, totalBytes, 0, PoolAlignment);
		newPool->m_memPool = this;

		nStart	= ((AddrSize)newPool) + sizeof(Pool);
		nEnd	= newPool->Create(m_growSize, m_dataSize, (void*)nStart, &m_freeList);

		Pool *oldPool = newPool;
		newPool = (Pool*)safe_zone_realloc(*m_zone, newPool, nEnd - ((AddrSize)newPool), 0, PoolAlignment);

		// adjust free list pointers if address changed
		if (newPool != oldPool)
		{
			newPool->m_array = (void*)(((AddrSize)newPool->m_array - (AddrSize)oldPool) + (AddrSize)newPool);
			m_freeList = (PoolNode*)(((AddrSize)m_freeList - (AddrSize)oldPool) + (AddrSize)newPool);
			PoolNode *node = m_freeList;
			while (node)
			{
				node->m_pool = (Pool*)(((AddrSize)node->m_pool - (AddrSize)oldPool) + (AddrSize)newPool);
				if (node->m_next)
				{
					node->m_next = (PoolNode*)(((AddrSize)node->m_next - (AddrSize)oldPool) + (AddrSize)newPool);
				}
				node = node->m_next;
			}
		}

		newPool->m_next = m_poolList;
		m_poolList = newPool;

		m_numAllocatedObjects += m_growSize;
	}

	//
	// Take the first free node off the list.
	//

	RAD_ASSERT(m_freeList);

	PoolNode* node = m_freeList;

	if (m_constructor)
		m_constructor(UserDataFromPoolNode(node));

	m_freeList = m_freeList->m_next;

	node->m_pool->m_numNodesInUse++;
	m_numUsedObjects++;
	RAD_ASSERT(!node->m_used);
	node->m_used = true;

	//
	// Return the address of the user data
	//

	RAD_DEBUG_ONLY(AssertPoolNodeIsValid(node));
	return UserDataFromPoolNode(node);
}

void MemoryPool::ReturnChunk(void* userData)
{
	if (!m_inited) // this can happen during static destruction from a doexit chain.
		return;

	RAD_ASSERT(userData);

	PoolNode* node = PoolNodeFromUserData(userData);

	if (m_destructor)
		m_destructor(UserDataFromPoolNode(node));

	RAD_ASSERT(node);
	RAD_ASSERT(node->m_pool);
	RAD_DEBUG_ONLY(AssertPoolNodeIsValid(node));

#if defined(RAD_OPT_DEBUG)
	AssertPoolNodeIsValid(node);
	RAD_ASSERT(node->m_used);
#endif

	node->m_used = false;
	node->m_next = m_freeList;
	m_freeList = node;

	node->m_pool->m_numNodesInUse--;
	m_numUsedObjects--;
}

MemoryPool* MemoryPool::PoolFromUserData(void *userData)
{
	RAD_ASSERT(userData);

	PoolNode* node = PoolNodeFromUserData(userData);

	RAD_ASSERT(node);
	RAD_ASSERT(node->m_pool);
	RAD_DEBUG_ONLY(AssertPoolNodeIsValid(node));
	
	return node->m_pool->m_memPool;
}

inline void* MemoryPool::UserDataFromPoolNode(PoolNode *node)
{
	return reinterpret_cast<U8*>(node) + sizeof(PoolNode);
}

inline MemoryPool::PoolNode* MemoryPool::PoolNodeFromUserData(void *data)
{
	return (PoolNode*)(reinterpret_cast<U8*>(data) - sizeof(PoolNode));
}

#if defined(RAD_OPT_DEBUG)
bool MemoryPool::IsPoolNodeValid(PoolNode *node)
{
	bool b;

	UReg id = PoolNode::MagicId;
	const U8 *pid = (const U8*)&id;
	const U8 *dst = (U8*)UserDataFromPoolNode(node) + node->m_pool->m_memPool->m_dataSize;

	b = (dst[0] == pid[0] && dst[1] == pid[1] && dst[2] == pid[2] && dst[3] == pid[3]);

	return (node->m_magicID == PoolNode::MagicId) && b;
}

void MemoryPool::AssertPoolNodeIsValid(PoolNode *node)
{
	RAD_ASSERT_MSG(IsPoolNodeValid(node), "MemoryPool: memory corruption detected!");
}
#endif

inline AddrSize MemoryPool::Pool::CalculateMemorySize(UReg inArraySize, AddrSize inDataSize, int alignment)
{
	return (AddrSize)(sizeof(Pool) + PoolAlignment + (alignment - PoolAlignment) + (
#if defined(RAD_OPT_DEBUG)
		Align<AddrSize>(sizeof(PoolNode) + inDataSize + sizeof(UReg), alignment)
#else
		Align<AddrSize>(sizeof(PoolNode) + inDataSize, alignment)
#endif
		) * inArraySize);
}

AddrSize MemoryPool::Pool::Create(UReg inArraySize, AddrSize inDataSize, void *inBaseAddr, PoolNode **inFreeHead)
{
	RAD_ASSERT(inFreeHead);
	RAD_ASSERT(m_memPool);

#if defined (RAD_OPT_DEBUG)
	m_magicID		= MagicId;
#endif

	m_array	= (PoolNode*)(Align((U8*)inBaseAddr + sizeof(PoolNode), m_memPool->m_alignment) - sizeof(PoolNode));
	
	m_next = NULL;
	m_numNodesInUse	= 0;

	//
	// Fill out the array
	//

	void* baseAddr = (void*)m_array;

	PoolNode* localHead = (*inFreeHead);

	for (UReg i = 0;i < inArraySize;i++)
	{
		PoolNode* n = (PoolNode*)baseAddr;

		n->m_pool		= this;
		n->m_next		= localHead;
		n->m_used       = false;
		RAD_DEBUG_ONLY(n->m_magicID = PoolNode::MagicId);

		localHead = n;

#if defined(RAD_OPT_DEBUG)
		// we do it this way to avoid un-aligned write exceptions on shitty hardware.
		{
			UReg id = PoolNode::MagicId;
			const U8 *pid = (const U8*)&id;
			U8 *dst = (U8*)m_memPool->UserDataFromPoolNode(n) + inDataSize;
			
			dst[0] = pid[0];
			dst[1] = pid[1];
			dst[2] = pid[2];
			dst[3] = pid[3];
		}
#endif

		RAD_DEBUG_ONLY(m_memPool->AssertPoolNodeIsValid(n));

		baseAddr = ((U8*)baseAddr) + m_memPool->m_offsetToNext;
	}

	(*inFreeHead) = localHead;

	return (AddrSize)baseAddr;
}

void MemoryPool::Pool::Destroy(UReg inArraySize, ChunkDestructor destructor, WalkCallback callback)
{
	RAD_ASSERT(m_magicID == MagicId);
	RAD_ASSERT(m_memPool);

	if (destructor || callback)
	{
		void* baseAddr = (void*)m_array;

		for (UReg i = 0;i < inArraySize;i++)
		{
			PoolNode* n = (PoolNode*)baseAddr;

			RAD_DEBUG_ONLY(m_memPool->AssertPoolNodeIsValid(n));

			if (n->m_used)
			{
				if (callback)
				{
					callback(m_memPool->UserDataFromPoolNode(n));
				}
				if (destructor)
				{
					destructor(m_memPool->UserDataFromPoolNode(n));
				}
			}

			baseAddr = ((U8*)baseAddr) + m_memPool->m_offsetToNext;
		}
	}
}

void MemoryPool::Pool::Reset(UReg inArraySize, PoolNode** inFreeHead, ChunkDestructor destructor, WalkCallback callback)
{
	RAD_ASSERT(m_magicID == MagicId);
	RAD_ASSERT(inFreeHead);
	RAD_ASSERT(m_memPool);

	PoolNode* localHead = (*inFreeHead);

	AddrSize baseAddr = (AddrSize)m_array;

	for (UReg i = 0;i < inArraySize;i++)
	{
		PoolNode* n = (PoolNode*)baseAddr;

		RAD_DEBUG_ONLY(m_memPool->AssertPoolNodeIsValid(n));

		if (n->m_used)
		{
			if (callback)
			{
				callback(m_memPool->UserDataFromPoolNode(n));
			}
			if (destructor)
			{
				destructor(m_memPool->UserDataFromPoolNode(n));
			}
		}

		n->m_next = localHead;
		n->m_used = false;

		localHead = n;

		baseAddr = (AddrSize)((U8*)baseAddr) + m_memPool->m_offsetToNext;
	}

	(*inFreeHead) = localHead;

	m_numNodesInUse = 0;
}

void MemoryPool::Pool::WalkUsed(UReg inArraySize, WalkCallback callback)
{
	RAD_ASSERT(m_magicID == MagicId);
	RAD_ASSERT(m_memPool);

	AddrSize baseAddr = (AddrSize)m_array;

	for (UReg i = 0;i < inArraySize;i++)
	{
		PoolNode* n = (PoolNode*)baseAddr;

		RAD_DEBUG_ONLY(m_memPool->AssertPoolNodeIsValid(n));	

		if (n->m_used)
		{
			callback(m_memPool->UserDataFromPoolNode(n));
		}

		baseAddr = (AddrSize)((U8*)baseAddr) + m_memPool->m_offsetToNext;
	}

	m_numNodesInUse = 0;
}
