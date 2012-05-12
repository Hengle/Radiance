// StringDetails.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "StringDef.h"
#include "../Base/MemoryPool.h"
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
	static M &Get() {
		static M s_m;
		return s_m;
	}
};

//! Immutable String Buffer
class StringBuf : public boost::noncopyable {
public:

	typedef StringBufRef Ref;
	typedef StringBufWRef WRef;

	static Ref New(
		RefType refType,
		const char *src,
		int numBytes,
		Zone &zone
	);

	static Ref New(
		const wchar_t *src,
		int numChars,
		Zone &zone
	);

	static Ref New(
		const wchar_t *src,
		int numChars,
		Zone &zone
	);

	RAD_DECLARE_READONLY_PROPERTY(StringBuf, buf, const char *);
	RAD_DECLARE_READONLY_PROPERTY(StringBuf, numBytes, int);

private:
	
	StringBuf(
		RefType refType,
		const char *src,
		int numBytes,
		MemoryPool *pool
	) : m_refType(refType),
	    m_buf(src),
	    m_numBytes(numBytes),
	    m_pool(pool) {
	}

	~StringBuf() {
		if (m_buf && RT_Copy == m_refType) {
			if (m_pool) {
				Mutex::ScopedLock L(Mutex::Get());
				m_pool->ReturnChunk(m_buf);
			}
		}
	}

	RAD_DECLARE_GET(buf, const char *) {
		return m_buf;
	}

	RAD_DECLARE_GET(numBytes, int) {
		return m_numBytes;
	}

	const char *m_buf;
	MemoryPool *m_pool;
	int m_numBytes;
	RefType m_refType;

	enum {
		MinPoolSize = 4,
		NumPools = 5,
		MaxPoolSize = MinPoolSize << NumPools
	};

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	static bool s_init;
	static MemoryPool s_pools[NumPools];
	static MemoryPool *PoolForSize(int size, Mutex::ScopedLock &L);
};

} // details
} // string

#include "../PopPack.h"
