// MemoryPool.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Steve Nowalk
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "MemoryPool.h"
#include "../StringBase.h"

//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_MEMPOOL_DEBUG)
#define MP_ASSERT(_x) RAD_VERIFY(_x)
#define MP_DEBUG_ONLY(_x) _x
#else
#define MP_ASSERT(_x)
#define MP_DEBUG_ONLY(_x)
#endif

MemoryPool::MemoryPool() : 
m_zone(0), 
m_poolList(0), 
m_freeList(0), 
m_numUsedChunks(0), 
m_numAllocatedChunks(0)
#if defined(RAD_OPT_MEMPOOL_DEBUG)
, m_concurrencyCheck(0)
#endif
{
	string::ncpy(m_name, "uninited", kMaxNameLen+1);
	m_inited = false;
}

MemoryPool::MemoryPool(
	Zone &zone,
	const char* name,
	AddrSize chunkSize,
	int numChunksInBlock,
	int chunkAlignment,
	int maxChunks
) : 
m_zone(0), 
m_poolList(0), 
m_freeList(0), 
m_numUsedChunks(0), 
m_numAllocatedChunks(0)
#if defined(RAD_OPT_MEMPOOL_DEBUG)
, m_concurrencyCheck(0)
#endif
{
	m_inited = false;

	Create(
		zone,
		name,
		chunkSize,
		numChunksInBlock,
		chunkAlignment,
		maxChunks
	);
}

MemoryPool::~MemoryPool() {
	if (m_poolList) {
		RAD_ASSERT(m_inited);
		Destroy();
	}
	RAD_VERIFY(!m_poolList);
	RAD_VERIFY(m_numUsedChunks == 0);
	RAD_VERIFY(m_numAllocatedChunks == 0);
}

inline void* MemoryPool::UserDataFromPoolNode(PoolNode *node) {
	return reinterpret_cast<U8*>(node) + sizeof(PoolNode);
}

inline MemoryPool::PoolNode* MemoryPool::PoolNodeFromUserData(void *data) {
	return (PoolNode*)(reinterpret_cast<U8*>(data) - sizeof(PoolNode));
}

void MemoryPool::Create(
	Zone &zone, 
	const char* name, 
	AddrSize chunkSize, 
	int numChunksInBlock, 
	int chunkAlignment, 
	int maxChunks
) {
	MP_ASSERT(!m_inited);
	MP_ASSERT(chunkSize > 0);
	BOOST_STATIC_ASSERT((int)MemoryPool::kPoolAlignment >= (int)kDefaultAlignment);

	chunkAlignment = std::max<int>(chunkAlignment, kPoolAlignment);

	m_inited = true;

	string::ncpy(m_name, name, kMaxNameLen+1);

	m_zone                  = &zone;
	m_numChunksInBlock      = numChunksInBlock;
	m_maxChunks             = maxChunks;
	m_chunkSize             = chunkSize;
	m_numUsedChunks         = 0;
	m_numAllocatedChunks    = 0;
	m_poolList              = NULL;
	m_freeList              = NULL;
	m_constructor           = NULL;
	m_destructor            = NULL;
	m_alignment             = chunkAlignment;
#if defined(RAD_OPT_MEMPOOL_DEBUG)
	// +sizeof(U32) is for overwrite buffer (underwrite buffer is inside PoolNode)
	m_offsetToNext = Align<AddrSize>(
		sizeof(PoolNode) + chunkSize + sizeof(U32), chunkAlignment
	);
#else
	m_offsetToNext = Align<AddrSize>(sizeof(PoolNode) + chunkSize, chunkAlignment);
#endif
}

void MemoryPool::Destroy(ChunkCallback callback) {
	MP_ASSERT(m_inited);
	Delete(callback);
	m_inited = false;
}

void MemoryPool::Delete(ChunkCallback callback)
{
	MP_ASSERT(m_inited);
	Pool* pool = m_poolList;

	if (m_numUsedChunks == 0) 
		callback = 0; // no callback!

	while (pool) {	
		Pool* next = pool->m_next;

		pool->Destroy((AddrSize)m_numChunksInBlock, m_destructor, callback);
		zone_free(pool);

		pool = next;
	}

	m_numUsedChunks = 0;
	m_numAllocatedChunks = 0;

	m_poolList = NULL;
	m_freeList = NULL;
}

void MemoryPool::Reset(ChunkCallback callback)
{
	MP_ASSERT(m_inited);
	
	if (m_numUsedChunks == 0) 
		callback = 0; // no callback!

	m_freeList = NULL;

	Pool* pool = m_poolList;

	while (pool) {
		pool->Reset((AddrSize)m_numChunksInBlock, &m_freeList, m_destructor, callback);
		pool = pool->m_next;
	}

	m_numUsedChunks = 0;
}

void MemoryPool::WalkUsed(ChunkCallback callback) {
	MP_ASSERT(callback);
	MP_ASSERT(m_inited);

	Pool* pool = m_poolList;

	while (pool) {
		pool->WalkUsed((AddrSize)m_numChunksInBlock, callback);
		pool = pool->m_next;
	}
}

void MemoryPool::Compact() {
	MP_ASSERT(m_inited);

	Pool* pool		= m_poolList;
	Pool* previous	= NULL;

	while (pool) {
		Pool* next = pool->m_next;

		if (pool->m_numNodesInUse == 0) {
			RAD_ASSERT(m_numAllocatedChunks >= m_numChunksInBlock);
			m_numAllocatedChunks -= m_numChunksInBlock;
			RAD_ASSERT((AddrSize)m_numUsedChunks >= pool->m_numNodesInUse);
			m_numUsedChunks -= pool->m_numNodesInUse;

			pool->Destroy(m_numChunksInBlock, m_destructor, 0);
			zone_free(pool);

			if (previous) {
				previous->m_next = next;
			} else {
				m_poolList = next;
			}
		} else {
			previous = pool;
		}

		pool = next;
	}

	// relink free list

	m_freeList = 0;
	pool = m_poolList;

	while (pool) {
		if (pool->m_numNodesInUse < (AddrSize)m_numChunksInBlock) {
			for (AddrSize i = 0; i < (AddrSize)m_numChunksInBlock; ++i) {
				PoolNode *node = (PoolNode*)(((U8*)pool->m_array) + (i*m_offsetToNext));
				if (!node->m_used) {
					node->m_next = m_freeList;
					m_freeList = node;
				}
			}
		}

		pool = pool->m_next;
	}
}

void* MemoryPool::SafeGetChunk() {
	void* n = GetChunk();
	RAD_OUT_OF_MEM(n);
	return n;
}

void* MemoryPool::GetChunk() {
	MP_ASSERT(m_inited);
	MP_ASSERT(++m_concurrencyCheck == 1);

	//
	// Is our list of pool nodes empty?
	//

	if (!m_freeList) {
		//
		// Have we hit our ceiling?
		//

		if (m_numAllocatedChunks >= m_maxChunks) {
			MP_ASSERT(--m_concurrencyCheck == 0);
			return NULL;
		}

		AddrSize totalBytes = Pool::CalculateMemorySize(
			(AddrSize)m_numChunksInBlock, 
			m_offsetToNext, 
			(AddrSize)m_alignment
		);

		AddrSize nStart;
		AddrSize nEnd;

		Pool* newPool = (Pool*)safe_zone_malloc(*m_zone, totalBytes, 0, kPoolAlignment);
		newPool->m_memPool = this;

		nStart	= ((AddrSize)newPool) + sizeof(Pool);
		nEnd	= newPool->Create(
			(AddrSize)m_numChunksInBlock, 
			m_chunkSize, 
			(void*)nStart, 
			&m_freeList
		);

		Pool *oldPool = newPool;
		newPool = (Pool*)safe_zone_realloc(
			*m_zone, 
			newPool, 
			nEnd - ((AddrSize)newPool), 
			0, 
			kPoolAlignment
		);

		// adjust free list pointers if address changed
		if (newPool != oldPool) {
			newPool->m_array = (void*)(((AddrSize)newPool->m_array - (AddrSize)oldPool) + (AddrSize)newPool);
			m_freeList = (PoolNode*)(((AddrSize)m_freeList - (AddrSize)oldPool) + (AddrSize)newPool);
			PoolNode *node = m_freeList;
			while (node) {
				node->m_pool = (Pool*)(((AddrSize)node->m_pool - (AddrSize)oldPool) + (AddrSize)newPool);
				if (node->m_next) {
					node->m_next = (PoolNode*)(((AddrSize)node->m_next - (AddrSize)oldPool) + (AddrSize)newPool);
				}
				MP_DEBUG_ONLY(AssertPoolNodeIsValid(node));
				MP_ASSERT(!node->m_used);
				node = node->m_next;
			}
		}

		newPool->m_next = m_poolList;
		m_poolList = newPool;

		m_numAllocatedChunks += m_numChunksInBlock;
	}

	//
	// Take the first free node off the list.
	//

	MP_ASSERT(m_freeList);

	PoolNode* node = m_freeList;

	if (m_constructor)
		m_constructor(UserDataFromPoolNode(node));

	m_freeList = m_freeList->m_next;

	node->m_pool->m_numNodesInUse++;
	m_numUsedChunks++;
	MP_DEBUG_ONLY(AssertPoolNodeIsValid(node));
	MP_ASSERT(!node->m_used);
	node->m_used = true;

	MP_ASSERT(--m_concurrencyCheck == 0);

	//
	// Return the address of the user data
	//
	return UserDataFromPoolNode(node);
}

void MemoryPool::ReturnChunk(void* userData) {
	if (!m_inited) // this can happen during static destruction from a doexit chain.
		return;

	MP_ASSERT(++m_concurrencyCheck == 1);

	MP_ASSERT(userData);

	PoolNode* node = PoolNodeFromUserData(userData);

	if (m_destructor)
		m_destructor(UserDataFromPoolNode(node));

	MP_ASSERT(node);
	MP_ASSERT(node->m_pool);
	MP_DEBUG_ONLY(AssertPoolNodeIsValid(node));

#if defined(RAD_OPT_MEMPOOL_DEBUG)
	AssertPoolNodeIsValid(node);
	MP_ASSERT(node->m_used);
#endif

	node->m_used = false;
	node->m_next = m_freeList;
	m_freeList = node;

	node->m_pool->m_numNodesInUse--;
	m_numUsedChunks--;

	MP_ASSERT(--m_concurrencyCheck == 0);
}

MemoryPool* MemoryPool::PoolFromChunk(void *chunk) {
	MP_ASSERT(chunk);

	PoolNode* node = PoolNodeFromUserData(chunk);

	RAD_ASSERT(node);
	RAD_ASSERT(node->m_pool);
	RAD_DEBUG_ONLY(AssertPoolNodeIsValid(node));
	
	return node->m_pool->m_memPool;
}

#if defined(RAD_OPT_MEMPOOL_DEBUG)
bool MemoryPool::IsPoolNodeValid(PoolNode *node) {
	bool b;

	UReg id = PoolNode::MagicId;
	const U8 *pid = (const U8*)&id;
	const U8 *dst = (U8*)UserDataFromPoolNode(node) + node->m_pool->m_memPool->m_chunkSize;

	b = (dst[0] == pid[0] && dst[1] == pid[1] && dst[2] == pid[2] && dst[3] == pid[3]);

	return (node->m_magicID == PoolNode::MagicId) && b;
}

void MemoryPool::AssertPoolNodeIsValid(PoolNode *node) {
	RAD_VERIFY_MSG(IsPoolNodeValid(node), "MemoryPool: memory corruption detected!");
}

#if defined(RAD_OPT_MEMPOOL_DEBUG)
void MemoryPool::AssertChunkIsValid(void *pT) {
	PoolNode *node = PoolNodeFromUserData(pT);
	AssertPoolNodeIsValid(node);
}
#endif

#endif

inline AddrSize MemoryPool::Pool::CalculateMemorySize(
	AddrSize inArraySize, 
	AddrSize inDataSize, 
	AddrSize alignment
) {
	return (AddrSize)(sizeof(Pool) + (alignment-1) + (inDataSize * inArraySize));
}

AddrSize MemoryPool::Pool::Create(
	AddrSize inArraySize, 
	AddrSize inDataSize, 
	void *inBaseAddr, 
	PoolNode **inFreeHead
) {
	RAD_ASSERT(inFreeHead);
	RAD_ASSERT(m_memPool);

#if defined (RAD_OPT_MEMPOOL_DEBUG)
	m_magicID = MagicId;
#endif

	m_array	= (PoolNode*)(Align((U8*)inBaseAddr + sizeof(PoolNode), m_memPool->m_alignment) - sizeof(PoolNode));
	
	m_next = NULL;
	m_numNodesInUse	= 0;

	//
	// Fill out the array
	//

	void* baseAddr = (void*)m_array;

	PoolNode* localHead = (*inFreeHead);

	for (AddrSize i = 0;i < inArraySize;i++) {
		PoolNode* n = (PoolNode*)baseAddr;

		n->m_pool		= this;
		n->m_next		= localHead;
		n->m_used       = false;
		MP_DEBUG_ONLY(n->m_magicID = PoolNode::MagicId);

		localHead = n;

#if defined(RAD_OPT_MEMPOOL_DEBUG)
		// we do it this way to avoid un-aligned write exceptions on shitty hardware.
		{
			U32 id = PoolNode::MagicId;
			const U8 *pid = (const U8*)&id;
			U8 *dst = (U8*)m_memPool->UserDataFromPoolNode(n) + inDataSize;
			
			dst[0] = pid[0];
			dst[1] = pid[1];
			dst[2] = pid[2];
			dst[3] = pid[3];
		}
#endif

		MP_DEBUG_ONLY(m_memPool->AssertPoolNodeIsValid(n));

		baseAddr = ((U8*)baseAddr) + m_memPool->m_offsetToNext;
	}

	(*inFreeHead) = localHead;

	return (AddrSize)baseAddr;
}

void MemoryPool::Pool::Destroy(
	AddrSize inArraySize, 
	ChunkCallback destructor, 
	ChunkCallback callback
) {
	RAD_ASSERT(m_magicID == MagicId);
	RAD_ASSERT(m_memPool);

	if (destructor || callback) {
		void* baseAddr = (void*)m_array;

		for (AddrSize i = 0;i < inArraySize;i++) {
			PoolNode* n = (PoolNode*)baseAddr;

			MP_DEBUG_ONLY(m_memPool->AssertPoolNodeIsValid(n));

			if (n->m_used) {
				if (callback) {
					callback(m_memPool->UserDataFromPoolNode(n));
				}
				if (destructor) {
					destructor(m_memPool->UserDataFromPoolNode(n));
				}
			}

			baseAddr = ((U8*)baseAddr) + m_memPool->m_offsetToNext;
		}
	}
}

void MemoryPool::Pool::Reset(
	AddrSize inArraySize, 
	PoolNode** inFreeHead, 
	ChunkCallback destructor, 
	ChunkCallback callback
) {
	MP_ASSERT(m_magicID == MagicId);
	MP_ASSERT(inFreeHead);
	MP_ASSERT(m_memPool);

	PoolNode* localHead = (*inFreeHead);

	AddrSize baseAddr = (AddrSize)m_array;

	for (AddrSize i = 0;i < inArraySize;i++) {
		PoolNode* n = (PoolNode*)baseAddr;

		RAD_DEBUG_ONLY(m_memPool->AssertPoolNodeIsValid(n));

		if (n->m_used) {
			if (callback) {
				callback(m_memPool->UserDataFromPoolNode(n));
			}
			if (destructor) {
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

void MemoryPool::Pool::WalkUsed(AddrSize inArraySize, ChunkCallback callback) {
	MP_ASSERT(m_magicID == MagicId);
	MP_ASSERT(m_memPool);

	AddrSize baseAddr = (AddrSize)m_array;

	for (AddrSize i = 0;i < inArraySize;i++) {
		PoolNode* n = (PoolNode*)baseAddr;

		RAD_DEBUG_ONLY(m_memPool->AssertPoolNodeIsValid(n));	

		if (n->m_used) {
			callback(m_memPool->UserDataFromPoolNode(n));
		}

		baseAddr = (AddrSize)((U8*)baseAddr) + m_memPool->m_offsetToNext;
	}

	m_numNodesInUse = 0;
}
