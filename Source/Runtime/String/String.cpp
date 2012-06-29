// String.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "String.h"
#include "utf8.h"
#include <limits.h>
#include <iostream>
#include "../PushSystemMacros.h"

namespace string {

RAD_ZONE_DEF(RADRT_API, ZString, "Strings", ZRuntime);

int utf8to16len(const char *src, int len) {
	U16 b[2];
	int l = 0;
	const char *end = src + len;
	while (src < end) {
		U16 *a = utf8::unchecked::utf8to16(&src, src+1, b);
		l += a-b;
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
	return end-dst;
}

int utf8to32(U32 *dst, const char *src, int len) {
	U32 *end = utf8::unchecked::utf8to32(src, src+len, dst);
	return end-dst;
}

int utf16to8len(const U16 *src, int len) {
	char b[4];
	int l = 0;
	const U16 *end = src + len;
	while (src < end) {
		char *a = utf8::unchecked::utf16to8(&src, src+1, b);
		l += a-b;
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
	return end-dst;
}

int utf16to32(U32 *dst, const U16 *src, int len) {
	char b[4];
	U32 *x = dst;
	const U16 *end = src + len;
	while (src < end) {
		char *a = utf8::unchecked::utf16to8(&src, src+1, b);
		x = utf8::unchecked::utf8to32(b, a, x);
	}
	return x-dst;
}

int utf32to8len(const U32 *src, int len) {
	char b[4];
	int l = 0;
	const U32 *end = src + len;
	while (src < end) {
		char *a = utf8::unchecked::utf32to8(&src, src+1, b);
		l += a-b;
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
		l += a-b;
	}
	return l;
}

int utf32to8(char *dst, const U32 *src, int len) {
	char *end = utf8::unchecked::utf32to8(src, src+len, dst);
	return end-dst;
}

int utf32to16(U16 *dst, const U32 *src, int len) {
	char b[4];
	U16 *x = dst;
	const U32 *end = src + len;
	while (src < end) {
		char *a = utf8::unchecked::utf32to8(&src, src+1, b);
		x = utf8::unchecked::utf8to16(b, a, x);
	}
	return x-dst;
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

MemoryPool *DataBlock::PoolForSize(int size, int &poolIdx, Mutex::ScopedLock &L) {

	if (size > kMaxPoolSize)
		return 0;

	L.lock(Mutex::Get());

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
			Mutex::ScopedLock L;
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
	return r;
}

DataBlock::Ref DataBlock::New(
	const U16 *u16,
	int u16Len,
	::Zone &zone
) {
	DataBlock::Ref r;

	int len = utf16to8len(u16, u16Len);
	if (len > 0) {
		r = New(kRefType_Copy, len + 1, 0, 0, zone);
		char *x = (char*)r->m_buf;
		utf16to8(x, u16, u16Len);
		x[len] = 0;
	}

	return r;
}

DataBlock::Ref DataBlock::New(
	const U32 *u32,
	int u32Len,
	::Zone &zone
) {
	DataBlock::Ref r;

	int len = utf32to8len(u32, u32Len);
	if (len > 0) {
		r = New(kRefType_Copy, len + 1, 0, 0, zone);
		char *x = (char*)r->m_buf;
		utf32to8(x, u32, u32Len);
		x[len] = 0;
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

	if (block)
		memcpy(r->m_buf, block->m_buf, std::min(size, block->m_size));

	return r;
}

} // details

///////////////////////////////////////////////////////////////////////////////

String::String(const String &s, const CopyTag_t&, ::Zone &zone) {
	m_zone = &zone;
	if (s.m_data) {
		if (s.m_data->m_refType != kRefType_Copy) {
			m_data = details::DataBlock::New(kRefType_Copy, 0, s.m_data->data, s.m_data->size, zone);
		} else {
			m_data = s.m_data;
		}
	}
}

UTF16Buf String::ToUTF16() const {
	UTF16Buf buf;

	if (m_data) {
		int len = utf8to16len(c_str, length + 1);
		if (len > 0) {
			buf.m_zone = m_zone;
			buf.m_data = details::DataBlock::New(kRefType_Copy, len*sizeof(U16), 0, 0, *m_zone);
			utf8to16((U16*)buf.m_data->data.get(), c_str, length + 1);
		}
	}

	return buf;
}

UTF32Buf String::ToUTF32() const {
	UTF32Buf buf;

	if (m_data) {
		int len = utf8to32len(c_str, length + 1);
		if (len > 0) {
			buf.m_zone = m_zone;
			buf.m_data = details::DataBlock::New(kRefType_Copy, len*sizeof(U32), 0, 0, *m_zone);
			utf8to32((U32*)buf.m_data->data.get(), c_str, length + 1);
		}
	}

	return buf;
}

String &String::Upper() {
	if (m_data) {
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
	if (m_data) {
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
	if (m_data) {
		UTF32Buf buf = ToUTF32();
		std::reverse(const_cast<U32*>(buf.c_str.get()), const_cast<U32*>(buf.c_str.get() + buf.size + 1));
		*this = String(buf, *m_zone);
	}
	return *this;
}

String String::SubStr(int first, int count) const {
	if (m_data) {
		first = ByteForChar(first);
		if (first >= 0) {
			const char *sz = (const char*)m_data->data.get() + first;

			String sub;

			U32 b;

			while (count-- > 0) {
				const char *tail = sz;
				utf8::unchecked::utf8to32(&tail, tail+1, &b);
				sub.NAppend(sz, tail-sz);
				sz = tail;
			}

			return sub;
		}
	}
	return String();
}

int String::CharForByte(int pos) const {
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
	const char *start = c_str.get();
	const char *end = start + length;
	const char *sz = start;

	U32 b;

	while (idx-- > 0) {
		if (sz >= end)
			return -1; // invalid character index.
		utf8::unchecked::utf8to32(&sz, sz+1, &b);
	}

	return sz-start;
}

String &String::Erase(int ofs, int count) {
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
	if (!str.empty && len) {
		int orglen = length;
		m_data = details::DataBlock::Resize(m_data, orglen + len + 1, *m_zone);
		char *sz = (char*)m_data->m_buf;
		memcpy(sz + orglen, str.m_data->data, len);
		sz[orglen + len] = 0;
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

		int c = s.length - (x+dst.length);
		if (c > 0)
			r = s.RightBytes(c);

		s = l + dst + r;
	}

	*this = s;
	return *this;
}

String &String::Printf(const char *fmt, va_list args) {
	String sfmt(CStr(fmt));
#if defined(RAD_OPT_WIN) // fuck you windows you non-standard piece of shit
	sfmt.Replace("%s", "%hs");
#endif
	WCharBuf wfmt = sfmt.ToWChar();
	int len = vscprintf(wfmt.c_str.get(), args);
	if (len > 1) { // > 1 because NULL is counted
		// do as wchars for UTF support.
		details::DataBlock::Ref wcs = 
			details::DataBlock::New(kRefType_Copy, len*sizeof(wchar_t), 0, 0, *m_zone);
		wchar_t *wchars = reinterpret_cast<wchar_t*>(wcs->m_buf);
		vsprintf(wchars, wfmt.c_str.get(), args);
		*this = String(wchars, len - 1, *m_zone);
	} else {
		Clear();
	}

	return *this;
}

String &String::PrintfASCII(const char *fmt, va_list args) {

	int len = vscprintf(fmt, args);
	if (len > 1) { // > 1 because NULL is counted
		m_data = details::DataBlock::New(kRefType_Copy, len, 0, 0, *m_zone);
		char *chars = reinterpret_cast<char*>(m_data->m_buf);
		vsprintf(chars, fmt, args);
	} else {
		Clear();
	}

	return *this;
}

String &String::Write(int pos, const char *sz, int len) {
	m_data = details::DataBlock::Isolate(m_data, *m_zone);
	ncpy(
		reinterpret_cast<char*>(m_data->data.get()),
		sz,
		len
	);
	return *this;
}

} // string
