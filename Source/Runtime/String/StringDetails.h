// StringDetails.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "StringDef.h"
#include "../Base/MemoryPool.h"
#include "../Base/ObjectPool.h"
#include "../Thread/Locks.h"

#include "../PushPack.h"

namespace string {
namespace details {

//! Global String Mutex
class Mutex {
public:
	typedef boost::mutex M;
	typedef boost::lock_guard<M> Lock;
	typedef thread::scoped_lock_guard<M> ScopedLock;
	static M &get() {
		static M s_m;
		return s_m;
	}
};

class DataBlock {
public:
	typedef boost::shared_ptr<DataBlock> Ref;
	
	~DataBlock() {
		if (m_refType == kRefType_Copy) {
			if (m_pool) {
				m_pool->ReturnChunk(m_buf);
			} else {
				zone_free(m_buf);
			}
		}
	}

private:

	friend class String;
	template <typename> friend class CharBuf;
	friend class ObjectPool<DataBlock>;

	typedef void (*unspecified_bool_type) ();

	RAD_DECLARE_READONLY_PROPERTY(DataBlock, data, void*);
	RAD_DECLARE_READONLY_PROPERTY(DataBlock, size, int);

	// len here is the total buffer length.
	static DataBlock::Ref create(
		RefType type,
		int len,
		const void *src,
		int srcLen,
		Zone &zone
	);

	static Ref create(
		const U16 *src,
		int len,
		Zone &zone
	);

	static Ref create(
		const U32 *src,
		int len,
		Zone &zone
	);

#if defined(RAD_NATIVE_WCHAR_T_DEFINED)
	static Ref create(
		const wchar_t *src,
		int len,
		Zone &zone
	) {
#if defined(RAD_OPT_4BYTE_WCHAR)
		return DataBlock::create((const U32*)src, len, zone);
#else
		return DataBlock::create((const U16*)src, len, zone);
#endif
	}
#endif

	static DataBlock::Ref resize(const DataBlock::Ref &block, int size, Zone &zone);

	static DataBlock::Ref isolate(const DataBlock::Ref &block, Zone &zone) {
		RAD_ASSERT(block);
		if (!block.unique() || (block->m_refType != kRefType_Copy))
			return create(kRefType_Copy, block->m_size, block->m_buf, block->m_size, zone);
		return block;
	}

	DataBlock(
		RefType type,
		void *src,
		int size,
		MemoryPool *pool,
		int poolIdx
	) : 
	m_buf(src),
	m_size(size),
	m_pool(pool),
	m_poolIdx(poolIdx),
	m_refType(type) {
	}

	void *m_buf;
	int m_size;

	MemoryPool *m_pool;
	int m_poolIdx;
	RefType m_refType;

	RAD_DECLARE_GET(data, void*) {
		return m_buf;
	}

	RAD_DECLARE_GET(size, int) {
		return m_size;
	}

	static void destroy(DataBlock *d) {
		s_dataBlockPool.Destroy(d);
	}

	enum {
		kMinPoolSize = 4,
		kNumPools = 5,
		kMaxPoolSize = kMinPoolSize << kNumPools
	};

	typedef ObjectPool<DataBlock> DataBlockPool;

	static bool s_init;
	static DataBlockPool s_dataBlockPool;
	static MemoryPool s_pools[kNumPools];
	static MemoryPool *poolForSize(
		int size, 
		int &poolIdx, 
		Mutex::ScopedLock &L
	);
};

} // details
} // string

#include "../PopPack.h"
