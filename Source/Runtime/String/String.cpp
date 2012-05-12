// String.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "String.h"
#include "StringBase.h"
#include "utf8.h"
#include <limits.h>

namespace string {
namespace details {

bool StringBuf::s_init = false;

MemoryPool *StringBuf::PoolForSize(int size, Mutex::ScopedLock &L) {

	if (size > MaxPoolSize)
		return 0;

	L.lock(Mutex::Get());

	if (!s_init) {
		for (int i = 0; i < NumPools; ++i) {
			s_pools[i].Create(
				ZString.Get(),
				"string_pool",
				MinPoolSize << i,
				1024 / (MinPoolSize << i)
			);
		}
		s_init = true;
	}

	MemoryPool *pool = 0;

	for (int i = 0; i < NumPools; ++i) {
		if (size <= (NumPools<<i)) {
			pool = &s_pools[i];
			break;
		}
	}

	RAD_ASSERT(pool);
	return 0;
}

StringBuf::Ref StringBuf::New(
	RefType refType,
	const char *src,
	int numBytes,
	Zone &zone
) {

	MemoryPool *pool = 0;

	if (RT_Copy == refType) {
		char *buf = 0;

		if (&zone == &ZString.Get()) {
			Mutex::ScopedLock L;
			pool = PoolForSize(numBytes, L);
			if (pool) {
				buf = reinterpret_cast<char*>(pool->GetChunk());
			}
		}

		if (!buf) {
			buf = reinterpret_cast<char*>(safe_zone_malloc(
				zone,
				numBytes
			));
		}

		cpy(buf, src);
		src = buf;
	}

	StringBuf::Ref r(new (zone) StringBuf(refType, src, numBytes, pool));
	return r;
}

//! Returns number of bytes needed to store the specified widestring in an mbstring.
inline int my_wcstombslen(const wchar_t *src, int numChars) {
	char bytes[4];
	int len = 0;
	while (--numChars >= 0) {
#if defined(RAD_OPT_4BYTE_WCHAR)
		BOOST_STATIC_ASSERT(sizeof(wchar_t) == 4);
		char *end = utf8::unchecked::utf32to8(src, src+1, bytes);
#else
		BOOST_STATIC_ASSERT(sizeof(wchar_t) == 2);
		char *end = utf8::unchecked::utf16to8(src, src+1, bytes);
#endif
		len += (end-bytes);
	}

	return len;
}

//! Converts wchar_t string to UTF8 string.
inline void my_wcstombs(const wchar_t *src, int numChars, char *dst) {
	while (--numChars >= 0) {
#if defined(RAD_OPT_4BYTE_WCHAR)
		BOOST_STATIC_ASSERT(sizeof(wchar_t) == 4);
		dst = utf8::unchecked::utf32to8(src, src+1, dst);
#else
		BOOST_STATIC_ASSERT(sizeof(wchar_t) == 2);
		dst = utf8::unchecked::utf16to8(src, src+1, dst);
#endif
	}
}

StringBuf::Ref StringBuf::New(
	const wchar_t *wsrc,
	int numChars,
	Zone &zone
) {
	char *src = 0;

	int numBytes = my_wcstombslen(wsrc, numChars);

	MemoryPool *pool = 0;
	if (numBytes > 0) {
		
		if (&zone == &ZString.Get()) {
			Mutex::ScopedLock L;
			pool = PoolForSize(numBytes, L);
			if (pool) {
				src = reinterpret_cast<char*>(pool->GetChunk());
			}
		}

		if (!src) {
			src = reinterpret_cast<char*>(safe_zone_malloc(
				zone,
				numBytes
			));
		}

		my_wcstombs(wsrc, numChars, src);
	}

	StringBuf::Ref r(new (zone) StringBuf(RT_Copy, src, numBytes, pool));
	return r;
}

} // details
} // string
