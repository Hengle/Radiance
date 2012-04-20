// MemoryPool.inl                                                                         
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////

inline ThreadSafeMemoryPool::ThreadSafeMemoryPool()
{
}

inline ThreadSafeMemoryPool::ThreadSafeMemoryPool(
	Zone &zone, 
	const char* name, 
	AddrSize chunkSize, 
	UReg growSize, 
	int alignment, 
	UReg maxSize
)
{
	Create(
		zone,
		name,
		chunkSize,
		growSize,
		alignment,
		maxSize
	);
}

inline ThreadSafeMemoryPool::~ThreadSafeMemoryPool()
{
}

inline void ThreadSafeMemoryPool::Create(
	Zone &zone, 
	const char* name, 
	AddrSize chunkSize, 
	UReg growSize, 
	int alignment, 
	UReg maxSize
)
{
	m_pool.Create(
		zone,
		name, 
		chunkSize, 
		growSize, 
		alignment, 
		maxSize
	);
}

inline void ThreadSafeMemoryPool::Destroy(MemoryPool::WalkCallback usedChunkCallback)
{
	Lock L(m_cs);
	m_pool.Destroy(usedChunkCallback);
}

inline void ThreadSafeMemoryPool::Reset(MemoryPool::WalkCallback usedChunkCallback)
{
	Lock L(m_cs);
	m_pool.Reset(usedChunkCallback);
}

inline void ThreadSafeMemoryPool::Compact()
{
	Lock L(m_cs);
	m_pool.Compact();
}

inline void ThreadSafeMemoryPool::Delete(MemoryPool::WalkCallback usedChunkCallback)
{
	Lock L(m_cs);
	m_pool.Delete(usedChunkCallback);
}

inline void ThreadSafeMemoryPool::WalkUsed(MemoryPool::WalkCallback callback)
{
	Lock L(m_cs);
	m_pool.WalkUsed(callback);
}

inline void* ThreadSafeMemoryPool::GetChunk()
{
	Lock L(m_cs);
	return m_pool.GetChunk();
}

inline void* ThreadSafeMemoryPool::SafeGetChunk()
{
	Lock L(m_cs);
	return m_pool.SafeGetChunk();
}

inline void ThreadSafeMemoryPool::ReturnChunk(void* pT)
{
	Lock L(m_cs);
	m_pool.ReturnChunk( pT );
}

inline UReg ThreadSafeMemoryPool::NumUsedChunks() const
{
	return m_pool.NumUsedChunks();
}

inline UReg ThreadSafeMemoryPool::NumAllocatedChunks() const
{
	return m_pool.NumAllocatedChunks();
}

inline UReg ThreadSafeMemoryPool::GrowSize() const
{
	return m_pool.GrowSize();
}

inline UReg ThreadSafeMemoryPool::MaxSize() const
{
	return m_pool.MaxSize();
}

inline void ThreadSafeMemoryPool::SetChunkConstructor(MemoryPool::ChunkConstructor c)
{
	Lock L(m_cs);
	m_pool.SetChunkConstructor( c );
}

inline void ThreadSafeMemoryPool::SetChunkDestructor (MemoryPool::ChunkDestructor  d)
{
	Lock L(m_cs);
	m_pool.SetChunkDestructor( d );
}

inline ThreadSafeMemoryPool* ThreadSafeMemoryPool::PoolFromUserData(void *p)
{
	ThreadSafeMemoryPool *t = reinterpret_cast<ThreadSafeMemoryPool*>(MemoryPool::PoolFromUserData(p));
	return t;
}

inline UReg MemoryPool::NumUsedChunks() const
{
	RAD_ASSERT(m_inited);
	return m_numUsedObjects;
}

inline UReg MemoryPool::NumAllocatedChunks() const
{
	RAD_ASSERT(m_inited);
	return m_numAllocatedObjects;
}

inline UReg MemoryPool::GrowSize() const
{
	RAD_ASSERT(m_inited);
	return m_growSize;
}

inline UReg MemoryPool::MaxSize() const
{
	RAD_ASSERT(m_inited);
	return m_maxSize;
}

inline void MemoryPool::SetChunkConstructor(ChunkConstructor c)
{
	RAD_ASSERT(m_inited);
	m_constructor = c;
}

inline void MemoryPool::SetChunkDestructor(ChunkDestructor d)
{
	RAD_ASSERT(m_inited);
	m_destructor = d;
}

//////////////////////////////////////////////////////////////////////////////////////////

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::Destroy(MemoryPool::WalkCallback usedChunkCallback)
{
	GetPool().Destory(usedChunkCallback);
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::Reset(MemoryPool::WalkCallback usedChunkCallback)
{
	GetPool().Reset(usedChunkCallback);
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::Compact()
{
	GetPool().Compact();
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::Delete(MemoryPool::WalkCallback usedChunkCallback)
{
	GetPool().Delete(usedChunkCallback);
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::WalkUsed(MemoryPool::WalkCallback callback)
{
	GetPool().WalkUsed(callback);
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline void *SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::GetChunk()
{
	return GetPool().GetChunk();
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline void *SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::SafeGetChunk()
{
	return GetPool().SafeGetChunk();
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::ReturnChunk(void* pT)
{
	GetPool().ReturnChunk(pT);
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline UReg SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::NumUsedChunks()
{
	return GetPool().NumUsedChunks();
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline UReg SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::NumAllocatedChunks()
{
	return GetPool().NumAllocatedChunks();
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline UReg SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::GrowSize()
{
	return GetPool().GrowSize();
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline UReg SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::MaxSize()
{
	return GetPool().MaxSize();
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::SetChunkConstructor(MemoryPool::ChunkConstructor c)
{
	GetPool().SetChunkConstructor(c);
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::SetChunkDestructor(MemoryPool::ChunkDestructor d)
{
	GetPool().SetChunkDestructor(d);
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline  Pool *SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::PoolFromUserData(void *p)
{
	return Pool::PoolFromUserData(p);
}

template <typename Tag, typename _Zone, UReg ChunkSize, UReg _GrowSize, UReg _MaxSize, int Alignment, typename Pool>
inline Pool &SingletonPool<Tag, _Zone, ChunkSize, _GrowSize, _MaxSize, Alignment, Pool>::GetPool()
{
	static Pool s_pool(_Zone::Get(), "singleton_pool", ChunkSize, _GrowSize, Alignment, _MaxSize);
	return s_pool;
}
