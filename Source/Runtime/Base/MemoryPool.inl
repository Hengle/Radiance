// MemoryPool.inl                                                                         
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////

inline void MemoryPool::SetChunkConstructor(ChunkCallback c)
{
	RAD_ASSERT(m_inited);
	m_constructor = c;
}

inline void MemoryPool::SetChunkDestructor(ChunkCallback d)
{
	RAD_ASSERT(m_inited);
	m_destructor = d;
}

//////////////////////////////////////////////////////////////////////////////////////////

inline ThreadSafeMemoryPool::ThreadSafeMemoryPool()
{
}

inline ThreadSafeMemoryPool::ThreadSafeMemoryPool(
	Zone &zone, 
	const char* name, 
	AddrSize chunkSize, 
	int numChunksInBlock, 
	int chunkAlignment, 
	int maxBlocks
) {
	Create(
		zone,
		name,
		chunkSize,
		numChunksInBlock,
		chunkAlignment,
		maxBlocks
	);
}

inline ThreadSafeMemoryPool::~ThreadSafeMemoryPool() {
}

inline void ThreadSafeMemoryPool::Create(
	Zone &zone, 
	const char* name, 
	AddrSize chunkSize, 
	int numChunksInBlock, 
	int chunkAlignment, 
	int maxBlocks
) {
	m_pool.Create(
		zone,
		name, 
		chunkSize,
		numChunksInBlock,
		chunkAlignment,
		maxBlocks
	);
}

inline void ThreadSafeMemoryPool::Destroy(MemoryPool::ChunkCallback usedChunkCallback) {
	Lock L(m_m);
	m_pool.Destroy(usedChunkCallback);
}

inline void ThreadSafeMemoryPool::Reset(MemoryPool::ChunkCallback usedChunkCallback) {
	Lock L(m_m);
	m_pool.Reset(usedChunkCallback);
}

inline void ThreadSafeMemoryPool::Compact() {
	Lock L(m_m);
	m_pool.Compact();
}

inline void ThreadSafeMemoryPool::Delete(MemoryPool::ChunkCallback usedChunkCallback) {
	Lock L(m_m);
	m_pool.Delete(usedChunkCallback);
}

inline void ThreadSafeMemoryPool::WalkUsed(MemoryPool::ChunkCallback callback) {
	Lock L(m_m);
	m_pool.WalkUsed(callback);
}

inline void* ThreadSafeMemoryPool::GetChunk() {
	Lock L(m_m);
	return m_pool.GetChunk();
}

inline void* ThreadSafeMemoryPool::SafeGetChunk() {
	Lock L(m_m);
	return m_pool.SafeGetChunk();
}

inline void ThreadSafeMemoryPool::ReturnChunk(void* pT) {
	Lock L(m_m);
	m_pool.ReturnChunk( pT );
}

inline void ThreadSafeMemoryPool::SetChunkConstructor(MemoryPool::ChunkCallback c) {
	Lock L(m_m);
	m_pool.SetChunkConstructor( c );
}

inline void ThreadSafeMemoryPool::SetChunkDestructor (MemoryPool::ChunkCallback  d) {
	Lock L(m_m);
	m_pool.SetChunkDestructor( d );
}

inline ThreadSafeMemoryPool* ThreadSafeMemoryPool::PoolFromChunk(void *chunk) {
	ThreadSafeMemoryPool *t = reinterpret_cast<ThreadSafeMemoryPool*>(MemoryPool::PoolFromChunk(chunk));
	return t;
}

//////////////////////////////////////////////////////////////////////////////////////////

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::Destroy(MemoryPool::ChunkCallback usedChunkCallback) {
	GetPool().Destory(usedChunkCallback);
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::Reset(MemoryPool::ChunkCallback usedChunkCallback) {
	GetPool().Reset(usedChunkCallback);
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::Compact() {
	GetPool().Compact();
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::Delete(MemoryPool::ChunkCallback usedChunkCallback) {
	GetPool().Delete(usedChunkCallback);
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::WalkUsed(MemoryPool::ChunkCallback callback) {
	GetPool().WalkUsed(callback);
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline void *SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::GetChunk() {
	return GetPool().GetChunk();
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline void *SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::SafeGetChunk() {
	return GetPool().SafeGetChunk();
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::ReturnChunk(void* pT) {
	GetPool().ReturnChunk(pT);
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline int SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::NumUsedChunks() {
	return GetPool().numUsedChunks;
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline int SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::NumAllocatedChunks() {
	return GetPool().numAllocatedChunks;
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline int SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::NumChunksInBlock() {
	return GetPool().numChunksInBlock;
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline int SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::MaxChunks() {
	return GetPool().maxChunks;
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::SetChunkConstructor(MemoryPool::ChunkCallback c) {
	GetPool().SetChunkConstructor(c);
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline void SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::SetChunkDestructor(MemoryPool::ChunkCallback d) {
	GetPool().SetChunkDestructor(d);
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline  Pool *SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::PoolFromChunk(void *p) {
	return Pool::PoolFromChunk(p);
}

template <typename Tag, typename _Zone, AddrSize ChunkSize, int _NumChunksInBlock, int _MaxBlocks, int Alignment, typename Pool>
inline Pool &SingletonPool<Tag, _Zone, ChunkSize, _NumChunksInBlock, _MaxBlocks, Alignment, Pool>::GetPool() {
	static Pool s_pool(_Zone::Get(), "singleton_memory_pool", ChunkSize, _NumChunksInBlock, Alignment, _MaxBlocks);
	return s_pool;
}
