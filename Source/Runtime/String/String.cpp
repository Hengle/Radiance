// String.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "String.h"
#include "utf8.h"
#include <limits.h>
#include <iostream>
#include "../PushSystemMacros.h"

//#define DISABLE_POOLS

namespace string {

RAD_ZONE_DEF(RADRT_API, ZString, "Strings", ZRuntime);

int utf8to16len(const char *src, int len) {
	U16 b[2];
	int l = 0;
	const char *end = src + len;
	while (src < end) {
		U16 *a = utf8::unchecked::utf8to16(&src, src+1, b);
		l += (int)(a-b);
	}
	return l;
}

int utf8to32len(const char *src, int len) {
	U32 b[1];
	int l = 0;
	const char *end = src + len;
	while (src < end) {
		utf8::unchecked::utf8to32(&src, src+1, b);
		++l;
	}
	return l;
}

int utf8to16(U16 *dst, const char *src, int len) {
	U16 *end = utf8::unchecked::utf8to16(src, src+len, dst);
	return (int)(end-dst);
}

int utf8to32(U32 *dst, const char *src, int len) {
	U32 *end = utf8::unchecked::utf8to32(src, src+len, dst);
	return (int)(end-dst);
}

int utf16to8len(const U16 *src, int len) {
	char b[4];
	int l = 0;
	const U16 *end = src + len;
	while (src < end) {
		char *a = utf8::unchecked::utf16to8(&src, src+1, b);
		l += (int)(a-b);
	}
	return l;
}

int utf16to32len(const U16 *src, int len) {
	U8 b[4];
	int l = 0;
	const U16 *end = src + len;
	while (src < end) {
		utf8::unchecked::utf16to8(&src, src+1, b);
		++l;
	}
	return l;
}

int utf16to8(char *dst, const U16 *src, int len) {
	char *end = utf8::unchecked::utf16to8(src, src+len, dst);
	return (int)(end-dst);
}

int utf16to32(U32 *dst, const U16 *src, int len) {
	char b[4];
	U32 *x = dst;
	const U16 *end = src + len;
	while (src < end) {
		char *a = utf8::unchecked::utf16to8(&src, src+1, b);
		x = utf8::unchecked::utf8to32(b, a, x);
	}
	return (int)(x-dst);
}

int utf32to8len(const U32 *src, int len) {
	char b[4];
	int l = 0;
	const U32 *end = src + len;
	while (src < end) {
		char *a = utf8::unchecked::utf32to8(&src, src+1, b);
		l += (int)(a-b);
	}
	return l;
}

int utf32to16len(const U32 *src, int len) {
	char x[4];
	U16 b[2];
	int l = 0;
	const U32 *end = src + len;
	while (src < end) {
		char *y = utf8::unchecked::utf32to8(&src, src+1, x);
		U16 *a = utf8::unchecked::utf8to16(x, y, b);
		l += (int)(a-b);
	}
	return l;
}

int utf32to8(char *dst, const U32 *src, int len) {
	char *end = utf8::unchecked::utf32to8(src, src+len, dst);
	return (int)(end-dst);
}

int utf32to16(U16 *dst, const U32 *src, int len) {
	char b[4];
	U16 *x = dst;
	const U32 *end = src + len;
	while (src < end) {
		char *a = utf8::unchecked::utf32to8(&src, src+1, b);
		x = utf8::unchecked::utf8to16(b, a, x);
	}
	return (int)(x-dst);
}

namespace details {

bool DataBlock::s_init = false;
DataBlock::DataBlockPool DataBlock::s_dataBlockPool;
MemoryPool DataBlock::s_pools[DataBlock::kNumPools];

void DataBlock::InitPools() {
	if (s_init)
		return;

	for (int i = 0; i < kNumPools; ++i) {
		s_pools[i].Create(
			ZString.Get(),
			"string_pool",
			kMinPoolSize << i,
			1024 / (kMinPoolSize << i)
		);
	}

	s_dataBlockPool.Create(ZString.Get(), "string_datablock_pool", 32);
	s_init = true;
}

MemoryPool *DataBlock::PoolForSize(int size, int &poolIdx, Mutex::Lock &_Lout) {

#if defined(DISABLE_POOLS)
	return 0;
#else
	if (size > kMaxPoolSize)
		return 0;

	Mutex::Lock L(Mutex::Get());
	L.swap(_Lout);

	InitPools();

	MemoryPool *pool = 0;
	poolIdx = 0;

	for (int i = 0; i < kNumPools; ++i) {
		if (size <= (kMinPoolSize<<i)) {
			pool = &s_pools[i];
			poolIdx = i;
			break;
		}
	}

	RAD_ASSERT(pool);
	return pool;
#endif
}

DataBlock::Ref DataBlock::New(
	RefType refType,
	int len,
	const void *src,
	int srcLen,
	::Zone &zone
) {

	MemoryPool *pool = 0;
	int poolIdx = 0;

	if (len == 0)
		len = srcLen;

	RAD_ASSERT(len);

	if (kRefType_Copy == refType) {
		char *buf = 0;

		if (&zone == &ZString.Get()) {
			Mutex::Lock L;
			pool = PoolForSize(len, poolIdx, L);
			if (pool) {
				buf = reinterpret_cast<char*>(pool->GetChunk());
			}
		}

		if (!buf) {
			buf = reinterpret_cast<char*>(safe_zone_malloc(
				zone,
				len
			));
		}

		if (src)
			memcpy(buf, src, srcLen);
		src = buf;
	}

	RAD_ASSERT(src);

	Mutex::Lock L(Mutex::Get());
	InitPools();
	DataBlock::Ref r(s_dataBlockPool.Construct(refType, const_cast<void*>(src), len, pool, poolIdx), &DataBlock::Destroy);
#if defined(RAD_OPT_MEMPOOL_DEBUG)
	r->Validate();
#endif
	return r;
}

DataBlock::Ref DataBlock::New(
	const U16 *u16,
	int u16Len,
	int numBytes,
	::Zone &zone
) {
	DataBlock::Ref r;

	if (numBytes > 0) {
		r = New(kRefType_Copy, numBytes + 1, 0, 0, zone);
		char *x = (char*)r->m_buf;
		utf16to8(x, u16, u16Len);
		x[numBytes] = 0;
#if defined(RAD_OPT_MEMPOOL_DEBUG)
		r->Validate();
#endif
	}

	return r;
}

DataBlock::Ref DataBlock::New(
	const U32 *u32,
	int u32Len,
	int numBytes,
	::Zone &zone
) {
	DataBlock::Ref r;

	if (numBytes > 0) {
		r = New(kRefType_Copy, numBytes + 1, 0, 0, zone);
		char *x = (char*)r->m_buf;
		utf32to8(x, u32, u32Len);
		x[numBytes] = 0;
#if defined(RAD_OPT_MEMPOOL_DEBUG)
		r->Validate();
#endif
	}

	return r;
}

DataBlock::Ref DataBlock::Resize(const DataBlock::Ref &block, int size, ::Zone &zone) {
	
	if (block && (block->m_refType != kRefType_Ref) && block.unique()) {
		if (!block->m_pool) {
			block->m_buf = (char*)safe_zone_realloc(zone, block->m_buf, size);
			block->m_size = size;
			return block;
		} else if (size <= (kMinPoolSize<<block->m_poolIdx)) {
			block->m_size = size;
			return block;
		}
	}

	DataBlock::Ref r = New(kRefType_Copy, size, 0, 0, zone);

	if (block) {
		memcpy(r->m_buf, block->m_buf, std::min(size, block->m_size));
#if defined(RAD_OPT_MEMPOOL_DEBUG)
		block->Validate();
#endif
	}

#if defined(RAD_OPT_MEMPOOL_DEBUG)
	r->Validate();
#endif

	return r;
}

} // details

///////////////////////////////////////////////////////////////////////////////

String::String(const String &s, const CopyTag_t&, ::Zone &zone) {
	m_zone = &zone;
	m_stackLen = s.m_stackLen;
	if (s.m_data) {
		RAD_ASSERT(m_stackLen == 0);
		if (s.m_data->m_refType != kRefType_Copy) {
			m_data = details::DataBlock::New(kRefType_Copy, 0, s.m_data->data, s.m_data->size, zone);
		} else {
			m_data = s.m_data;
		}
#if defined(RAD_OPT_MEMPOOL_DEBUG)
		m_data->Validate();
#endif
	} else {
		RAD_ASSERT(m_stackLen <= kStackSize);
#if defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
		RAD_VERIFY(m_stackLen == 0);
#endif
		memcpy(m_stackBytes, s.m_stackBytes, m_stackLen);
	}
}

String::String(const U16 *sz, int len, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	int numBytes = utf16to8len(sz, len);
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
	if (numBytes+1 > kStackSize) { // +1 for null
#else
	if (numBytes > 0) {
#endif
		m_stackLen = 0;
		m_data = details::DataBlock::New(sz, len, numBytes, zone);
#if defined(RAD_OPT_MEMPOOL_DEBUG)
		m_data->Validate();
#endif
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
	} else if (numBytes) { // fits in stack bytes
		utf16to8(m_stackBytes, sz, len);
		m_stackBytes[numBytes] = 0;
		m_stackLen = numBytes + 1;
#endif
 	} else {
		m_stackLen = 0;
	}
}

String::String(const U32 *sz, int len, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	int numBytes = utf32to8len(sz, len);
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
	if (numBytes+1 > kStackSize) { // +1 for null
#else 
	if (numBytes > 0) {
#endif
		m_stackLen = 0;
		m_data = details::DataBlock::New(sz, len, numBytes, zone);
#if defined(RAD_OPT_MEMPOOL_DEBUG)
		m_data->Validate();
#endif
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
	} else if (numBytes) { // fits in stack bytes
		utf32to8(m_stackBytes, sz, len);
		m_stackBytes[numBytes] = 0;
		m_stackLen = numBytes + 1;
#endif
 	} else {
		m_stackLen = 0;
	}
}

UTF16Buf String::ToUTF16() const {
	UTF16Buf buf;
	buf.m_zone = m_zone;

	if (!empty) {
		int len = utf8to16len(c_str, length + 1) * (int)sizeof(U16);
		if (len > sizeof(U16)) {
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
			if (len > UTF16Buf::kStackSize) {
#endif
				buf.m_data = details::DataBlock::New(kRefType_Copy, len, 0, 0, *m_zone);
				utf8to16((U16*)buf.m_data->data.get(), c_str, length + 1);
#if defined(RAD_OPT_MEMPOOL_DEBUG)
				buf.m_data->Validate();
#endif
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
			} else {
				buf.m_stackLen = len;
				utf8to16((U16*)buf.m_stackBytes, c_str, length + 1);
			}
#endif
		}
	}

	return buf;
}

UTF32Buf String::ToUTF32() const {
	UTF32Buf buf;
	buf.m_zone = m_zone;

	if (!empty) {
		int len = utf8to32len(c_str, length + 1) * (int)sizeof(U32);
		if (len > sizeof(U32)) {
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
			if (len > UTF32Buf::kStackSize) {
#endif
				buf.m_data = details::DataBlock::New(kRefType_Copy, len, 0, 0, *m_zone);
				utf8to32((U32*)buf.m_data->data.get(), c_str, length + 1);
#if defined(RAD_OPT_MEMPOOL_DEBUG)
				buf.m_data->Validate();
#endif
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
			} else {
				buf.m_stackLen = len;
				utf8to32((U32*)buf.m_stackBytes, c_str, length + 1);
			}
#endif
		}
	}

	return buf;
}

String &String::Upper() {
	if (!empty) {
		// toupper UNICODE only works reliable using towupper
		// this requires transcoding to a wide string and then back.
		// expensive.
		WCharBuf buf = ToWChar();
		toupper(const_cast<wchar_t*>(buf.c_str.get()));
		*this = String(buf, *m_zone);
	}
	return *this;
}

String &String::Lower() {
	if (!empty) {
		// tolower UNICODE only works reliable using towlower
		// this requires transcoding to a wide string and then back.
		// expensive.
		WCharBuf buf = ToWChar();
		tolower(const_cast<wchar_t*>(buf.c_str.get()));
		*this = String(buf, *m_zone);
	}
	return *this;
}

String &String::Reverse() {
	if (!empty) {
		UTF32Buf buf = ToUTF32();
		std::reverse(const_cast<U32*>(buf.c_str.get()), const_cast<U32*>(buf.c_str.get() + buf.size + 1));
		*this = String(buf, *m_zone);
	}
	return *this;
}

String String::SubStr(int first, int count) const {
	RAD_VERIFY(first >= 0);
	RAD_VERIFY(count >= 0);

	if (!empty && count > 0) {
		first = ByteForChar(first);
		if (first >= 0) {
			const char *sz = c_str.get() + first;

			String sub;

			U32 b;

			while (count-- > 0) {
				const char *tail = sz;
				RAD_ASSERT(tail < end);
				utf8::unchecked::utf8to32(&tail, tail+1, &b);
				sub.NAppend(sz, (int)(tail-sz));
				sz = tail;
			}

			return sub;
		}
	}
	return String();
}

StringVec String::Split(const String &sep) const {
	StringVec v;
	String workString(*this);

	const int kSepNumChars = sep.numChars;

	int r;
	while ((r=workString.StrStr(sep)) != -1) {
		if (r > 0) {
			String x = workString.Left(r);
			v.push_back(x);
		}

		const int kWorkStringNumChars = workString.numChars;

		int skip = r + kSepNumChars;

		if (kWorkStringNumChars > skip) {
			// skip leading.
			workString = workString.SubStr(skip);
		} else {
			break; // no more
		}
	}

	if (v.empty() && !this->empty)
		v.push_back(*this);

	return v;
}

int String::CharForByte(int pos) const {
	RAD_VERIFY(pos >= 0);

	if (pos >= length)
		return -1;

	const char *sz = c_str.get();
	const char *cpos = sz + pos;

	int c = 0;
	U32 b;

	while (sz < cpos) {
		utf8::unchecked::utf8to32(&sz, sz+1, &b);
		++c;
	}

	return c;
}

int String::ByteForChar(int idx) const {
	RAD_VERIFY(idx >= 0);

	const char *start = c_str.get();
	const char *end = start + length;
	const char *sz = start;

	U32 b;

	while (idx-- > 0) {
		if (sz >= end)
			return -1; // invalid character index.
		utf8::unchecked::utf8to32(&sz, sz+1, &b);
	}

	return (int)(sz-start);
}

String &String::Erase(int ofs, int count) {
	RAD_VERIFY(ofs >= 0);
	RAD_VERIFY(count >= 0);

	int nc = numChars;
	if (ofs >= nc)
		return *this;
	if (ofs+count > nc)
		count = nc - ofs;
	
	String l;
	if (ofs > 0)
		l = SubStr(0, ofs);

	String r;
	if (ofs+count < nc)
		r = SubStr(ofs+count, nc-(ofs+count));

	*this = l + r;
	return *this;
}

String &String::EraseBytes(int ofs, int count) {
	RAD_VERIFY(ofs >= 0);
	RAD_VERIFY(count >= 0);

	int nc = length;
	if (ofs >= nc)
		return *this;
	if (ofs+count > nc)
		count = nc - ofs;
	
	String l;
	if (ofs > 0)
		l = SubStrBytes(0, ofs);

	String r;
	if (ofs+count < nc)
		r = SubStrBytes(ofs+count, nc-(ofs+count));

	*this = l + r;
	return *this;
}

String &String::NAppendBytes(const String &str, int len) {
	RAD_VERIFY(len >= 0);
	RAD_VERIFY(len <= str.length);

	if (len) {
		int orglen = length;
		int newLen = orglen + len;
		char *sz;

		if (m_data) {
			RAD_ASSERT(m_stackLen == 0);
			m_data = details::DataBlock::Resize(m_data, newLen + 1, *m_zone);
			sz = (char*)m_data->m_buf;
		} else {
			// this string is in stackBytes.
			RAD_ASSERT((orglen == 0) || (m_stackLen == orglen + 1));
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
			if (newLen+1 > kStackSize) { // need to move into m_data
#endif
				m_data = details::DataBlock::New(
					kRefType_Copy, 
					newLen + 1, 
					m_stackBytes, 
					(m_stackLen>0) ? m_stackLen-1 : 0, 
					*m_zone
				);
				m_stackLen = 0;
				sz = (char*)m_data->m_buf;
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
			} else {
				// still fits on stack
				sz = m_stackBytes;
				m_stackLen = newLen + 1;
			}
#endif
		}

		memcpy(sz + orglen, str.c_str.get(), len);
		sz[orglen + len] = 0;
#if defined(RAD_OPT_MEMPOOL_DEBUG)
		if (m_data) {
			RAD_VERIFY(m_stackLen == 0);
			m_data->Validate();
		}
#endif
	}
	return *this;
}

String &String::Replace(const String &src, const String &dst) {
	String s(*this);

	int x;
	while ((x=s.StrStr(src)) >= 0) {
		String l, r;

		if (x > 0)
			l = s.Left(x);

		int c = x+src.length;
		r = s.SubStr(c);

		s = l + dst + r;

		RAD_VERIFY(src != dst);
	}

	*this = s;
	return *this;
}

String &String::Printf_valist(const char *fmt, va_list args) {
	String sfmt(CStr(fmt));
#if defined(RAD_OPT_WIN) // what the hell windows
	sfmt.Replace("%s", "%hs");
#endif
	WCharBuf wfmt = sfmt.ToWChar();
	int len = vscprintf(wfmt.c_str.get(), args);
	if (len > 1) { // > 1 because NULL is counted
		// do as wchars for UTF support.
		bool stack = (len*sizeof(wchar_t)) < (4*kKilo);
		wchar_t *wchars;
		if (stack) {
			wchars = (wchar_t*)stack_alloc(len*sizeof(wchar_t));
		} else {
			wchars = (wchar_t*)safe_zone_malloc(*m_zone, len*sizeof(wchar_t));
		}
		vsprintf(wchars, wfmt.c_str.get(), args);
		*this = String(wchars, len - 1, *m_zone);
		if (!stack)
			zone_free(wchars);
	} else {
		Clear();
	}

	return *this;
}

String &String::PrintfASCII_valist(const char *fmt, va_list args) {

	int len = vscprintf(fmt, args);
	if (len > 1) { // > 1 because NULL is counted
		char *chars;
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
		if (len > kStackSize) {
#endif
			m_data = details::DataBlock::New(kRefType_Copy, len, 0, 0, *m_zone);
			chars = reinterpret_cast<char*>(m_data->m_buf);
#if !defined(RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA)
		} else {
			m_stackLen = len;
			chars = m_stackBytes;
			m_data.reset();
		}
#endif
		vsprintf(chars, fmt, args);
	} else {
		Clear();
	}

	return *this;
}

String &String::Write(int pos, const char *sz, int len) {
	if (m_data)
		m_data = details::DataBlock::Isolate(m_data, *m_zone);
	ncpy(const_cast<char*>(c_str.get()), sz, len);
#if defined(RAD_OPT_MEMPOOL_DEBUG)
	if (m_data)
		m_data->Validate();
#endif
	return *this;
}

} // string
