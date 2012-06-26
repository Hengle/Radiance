// String.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <algorithm>
#include "../PushSystemMacros.h"

namespace string {

///////////////////////////////////////////////////////////////////////////////

template <typename Traits>
inline CharBuf<Traits>::CharBuf() : m_zone(&ZString.Get()) {
}

template <typename Traits>
inline CharBuf<Traits>::CharBuf(const SelfType &buf) : m_data(buf.m_data), m_zone(buf.m_zone) {
}

template <typename Traits>
inline CharBuf<Traits>::CharBuf(const details::DataBlock::Ref &data, ::Zone &zone) : m_data(data), m_zone(&zone) {
}

template <typename Traits>
inline CharBuf<Traits>::operator unspecified_bool_type () const {
	return IsValid() ? &bool_true : 0;
}

template <typename Traits>
inline typename CharBuf<Traits>::SelfType &CharBuf<Traits>::operator = (const SelfType &buf) {
	m_data = buf.m_data;
	m_zone = buf.m_zone;
	return *this;
}

template <typename Traits>
inline bool CharBuf<Traits>::operator == (const SelfType &buf) const {
	if (!m_data || !buf.m_data)
		return false;
	if (m_data->data != buf.m_data->data)
		return false;
	if (m_data->size != buf.m_data->size)
		return false;
	return true;
}

template <typename Traits>
inline bool CharBuf<Traits>::operator != (const SelfType &buf) const {
	return !(*this == buf);
}

template <typename Traits>
inline const typename CharBuf<Traits>::T *CharBuf<Traits>::RAD_IMPLEMENT_GET(c_str) {
	static U32 s_null(0);
	return (m_data) ? (const T*)m_data->data.get() : (const T*)&s_null;
}

template <typename Traits>
inline const typename CharBuf<Traits>::T *CharBuf<Traits>::RAD_IMPLEMENT_GET(begin) {
	return c_str.get();
}

template <typename Traits>
inline const typename CharBuf<Traits>::T *CharBuf<Traits>::RAD_IMPLEMENT_GET(end) {
	return c_str.get() + size.get();
}

template <typename Traits>
inline int CharBuf<Traits>::RAD_IMPLEMENT_GET(size) {
	return (m_data) ? (m_data->size - sizeof(T)) : 0;
}

template <typename Traits>
inline bool CharBuf<Traits>::RAD_IMPLEMENT_GET(empty) {
	return (m_data) ? false : true;
}

template<typename Traits>
inline int CharBuf<Traits>::RAD_IMPLEMENT_GET(numChars) {
	return size.get() / sizeof(T);
}

template <typename Traits>
inline void CharBuf<Traits>::free() {
	m_data.reset();
}

template <typename Traits>
inline typename CharBuf<Traits>::SelfType CharBuf<Traits>::create(const T *data, int size, const CopyTag_t&, ::Zone &zone) {
	RAD_ASSERT(data);
	return CharBuf(
		details::DataBlock::New(kRefType_Copy, data, size, zone),
		zone
	);
}

template <typename Traits>
inline typename CharBuf<Traits>::SelfType CharBuf<Traits>::create(const T *data, int size, const RefTag_t&, ::Zone &zone) {
	RAD_ASSERT(data);
	return CharBuf(
		details::DataBlock::New(kRefType_Ref, data, size, zone),
		zone
	);
}

///////////////////////////////////////////////////////////////////////////////

inline String::String(::Zone &zone) : m_zone(&zone) {
}

inline String::String(const String &s) : m_data(s.m_data), m_zone(s.m_zone) {
}

inline String::String(const UTF8Buf &buf) : m_data(buf.m_data), m_zone(buf.m_zone) {
}

inline String::String(const UTF16Buf &buf, ::Zone &zone) : m_zone(&zone) {
	m_data = details::DataBlock::create(buf.c_str.get(), buf.numChars, zone);
}

inline String::String(const UTF32Buf &buf, ::Zone &zone) : m_zone(&zone) {
	m_data = details::DataBlock::create(buf.c_str.get(), buf.numChars, zone);
}

inline String::String(const WCharBuf &buf, ::Zone &zone) : m_zone(&zone) {
	m_data = details::DataBlock::create(buf.c_str.get(), buf.numChars, zone);
}

inline String::String(const char *sz, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	if (sz[0])
		m_data = details::DataBlock::create(kRefType_Copy, 0, sz, len(sz) + 1, zone);
}

inline String::String(const char *sz, const RefTag_t&, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	if (sz[0])
		m_data = details::DataBlock::create(kRefType_Ref, 0, sz, len(sz) + 1, zone);
}

inline String::String(const char *sz, int len, const CopyTag_t&, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	if ((len>0) && sz[0]) {
		m_data = details::DataBlock::create(kRefType_Copy, len + 1, sz, len, zone);
		reinterpret_cast<char*>(m_data->m_buf)[len] = 0;
	}
}

inline String::String(const char *sz, int len, const RefTag_t&, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	if ((len>0) && sz[0]) {
		m_data = details::DataBlock::create(kRefType_Copy, len + 1, sz, len, zone);
		reinterpret_cast<char*>(m_data->m_buf)[len] = 0;
	}
}

#if defined(RAD_NATIVE_WCHAR_T_DEFINED)

inline String::String(const wchar_t *sz, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	if (sz[0])
		m_data = details::DataBlock::create((const WCharTraits::TT*)sz, len(sz), zone);
}

inline String::String(const wchar_t *sz, int len, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	if ((len>0) && sz[0])
		m_data = details::DataBlock::create((const WCharTraits::TT*)sz, len, zone);
}

#endif

inline String::String(const U16 *sz, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	if (sz[0])
		m_data = details::DataBlock::create(sz, len(sz), zone);
}

inline String::String(const U16 *sz, int len, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	if ((len>0) && sz[0])
		m_data = details::DataBlock::create(sz, len, zone);
}

inline String::String(const U32 *sz, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	if (sz[0])
		m_data = details::DataBlock::create(sz, len(sz), zone);
}

inline String::String(const U32 *sz, int len, ::Zone &zone) : m_zone(&zone) {
	RAD_ASSERT(sz);
	if ((len>0) && sz[0])
		m_data = details::DataBlock::create(sz, len, zone);
}

inline String::String(char c, ::Zone &zone) : m_zone(&zone) {
	*this = String(&c, 1, CopyTag, zone);
}

#if defined(RAD_NATIVE_WCHAR_T_DEFINED)
inline String::String(wchar_t c, ::Zone &zone) : m_zone(&zone) {
	*this = String(&c, 1, zone);
}
#endif

inline String::String(U16 c, ::Zone &zone) : m_zone(&zone) {
	*this = String(&c, 1, zone);
}

inline String::String(U32 c, ::Zone &zone) : m_zone(&zone) {
	*this = String(&c, 1, zone);
}

inline String::String(const std::string &str, ::Zone &zone) : m_zone(&zone) {
	*this = str.c_str();
}

inline String::String(const std::wstring &str, ::Zone &zone) : m_zone(&zone) {
	*this = str.c_str();
}

inline String::String(int len, ::Zone &zone) : m_zone(&zone) {
	if (len>0) {
		m_data = details::DataBlock::create(kRefType_Copy, len, 0, 0, zone);
		reinterpret_cast<char*>(m_data->m_buf)[len-1] = 0;
	}
}

inline UTF8Buf String::toUTF8() const {
	UTF8Buf buf;
	buf.m_data = m_data;
	buf.m_zone = m_zone;
	return buf;
}

inline WCharBuf String::toWChar() const {
#if defined(RAD_OPT_4BYTE_WCHAR)
	UTF32Buf x = toUTF32();
#else
	UTF16Buf x = toUTF16();
#endif
	return WCharBuf(x.m_data, *m_zone);
}

inline std::string String::toStdString() const {
	return std::string(c_str.get());
}

inline std::wstring String::toStdWString() const {
	WCharBuf x = toWChar();
	return std::wstring(x.c_str.get());
}

inline int String::compare(const String &str) const {
	return compare(str.c_str.get());
}

inline int String::compare(const char *sz) const {
	RAD_ASSERT(sz);
	return cmp(c_str.get(), sz);
}

inline int String::compare(const wchar_t *sz) const {
	RAD_ASSERT(sz);
	return compare(String(sz, *m_zone));
}

inline int String::comparei(const String &str) const {
	return comparei(str.c_str.get());
}

inline int String::comparei(const char *sz) const {
	RAD_ASSERT(sz);
	return icmp(c_str.get(), sz);
}

inline int String::comparei(const wchar_t *sz) const {
	RAD_ASSERT(sz);
	return compare(String(sz, *m_zone));
}

inline int String::nCompare(const String &str, int len) const {
	return nCompare(str.c_str.get(), len);
}

inline int String::nCompare(const char *sz, int len) const {
	RAD_ASSERT(sz);
	return ncmp(c_str.get(), sz, len);
}

inline int String::nCompare(const wchar_t *sz, int len) const {
	RAD_ASSERT(sz);
	int mblen = wcstombslen(sz, len);
	return nCompare(String(sz, *m_zone), mblen);
}

inline int String::nComparei(const String &str, int len) const {
	return nComparei(str.c_str.get(), len);
}

inline int String::nComparei(const char *sz, int len) const {
	RAD_ASSERT(sz);
	return nicmp(c_str.get(), sz, len);
}

inline int String::nComparei(const wchar_t *sz, int len) const {
	RAD_ASSERT(sz);
	int mblen = wcstombslen(sz, len);
	return nComparei(String(sz, *m_zone), mblen);
}

inline int String::strstr(const String &str) const {
	return strstr(str.c_str.get());
}

inline int String::strstr(const char *sz) const {
	RAD_ASSERT(sz);
	const char *root = c_str;
	const char *pos = string::strstr(root, sz);
	return pos ? (pos-root) : -1;
}

inline String String::join(const String &str) const {
	String x(*this);
	x.append(str);
	return x;
}

inline String String::join(const char *sz) const {
	String x(*this);
	x.append(sz);
	return x;
}

inline String String::join(const wchar_t *sz) const {
	String x(*this);
	x.append(sz);
	return x;
}

inline String String::join(const char c) const {
	String x(*this);
	x.append(c);
	return x;
}

inline String String::join(const wchar_t c) const {
	String x(*this);
	x.append(c);
	return x;
}

inline String String::nJoin(const String &str, int len) const {
	String x(*this);
	x.nAppend(str, len);
	return x;
}

inline String String::nJoin(const char *sz, int len) const {
	String x(*this);
	x.nAppend(sz, len);
	return x;
}

inline String String::nJoin(const wchar_t *sz, int len) const {
	String x(*this);
	x.nAppend(sz, len);
	return x;
}

inline String String::substrBytes(int first, int count) const {
	RAD_ASSERT(first < length);
	RAD_ASSERT((first+count) < length);

	return String(
		c_str.get() + first,
		count,
		CopyTag,
		*m_zone
	);
}

inline String String::substr(int ofs) const {
	int x = numChars - ofs;
	if (x < 0)
		return String();
	return right(x);
}

inline String String::substrBytes(int ofs) const {
	int x = length - ofs;
	if (x < 0)
		return String();
	return rightBytes(x);
}

inline String String::left(int count) const {
	return substr(0, count);
}

inline String String::right(int count) const {
	int ofs = numChars - count;
	return substr(ofs, count);
}

inline String String::leftBytes(int count) const {
	return substrBytes(0, count);
}

inline String String::rightBytes(int count) const {
	int ofs = length - count;
	return substrBytes(ofs, count);
}

inline String::operator unspecified_bool_type () const {
	return !empty.get() ? &String::bool_true : 0;
}

inline bool String::operator == (const String &str) const {
	return compare(str) == 0;
}

inline bool String::operator == (const char *sz) const {
	return compare(sz) == 0;
}

inline bool String::operator == (const wchar_t *sz) const {
	return compare(sz) == 0;
}

inline bool String::operator != (const String &str) const {
	return compare(str) != 0;
}

inline bool String::operator != (const char *sz) const {
	return compare(sz) != 0;
}

inline bool String::operator != (const wchar_t *sz) const {
	return compare(sz) != 0;
}

inline bool String::operator > (const String &str) const {
	return compare(str) > 0;
}

inline bool String::operator > (const char *sz) const {
	return compare(sz) > 0;
}

inline bool String::operator > (const wchar_t *sz) const {
	return compare(sz) > 0;
}

inline bool String::operator >= (const String &str) const {
	return compare(str) >= 0;
}

inline bool String::operator >= (const char *sz) const {
	return compare(sz) >= 0;
}

inline bool String::operator >= (const wchar_t *sz) const {
	return compare(sz) >= 0;
}

inline bool String::operator < (const String &str) const {
	return compare(str) < 0;
}

inline bool String::operator < (const char *sz) const {
	return compare(sz) < 0;
}

inline bool String::operator < (const wchar_t *sz) const {
	return compare(sz) < 0;
}

inline bool String::operator <= (const String &str) const {
	return compare(str) <= 0;
}

inline bool String::operator <= (const char *sz) const {
	return compare(sz) <= 0;
}

inline bool String::operator <= (const wchar_t *sz) const {
	return compare(sz) <= 0;
}

inline char String::operator [] (int ofs) const {
	return reinterpret_cast<const char*>(m_data->data.get())[ofs];
}

inline bool String::equalsInstance(const String &str) const {
	return m_data && str.m_data && m_data->data == str.m_data->data;
}

inline bool String::equalsInstance(const UTF8Buf &buf) const {
	return m_data && buf.m_data && m_data->data == buf.m_data->data;
}

inline String &String::upperASCII() {
	if (m_data) {
		m_data = details::DataBlock::isolate(m_data, *m_zone);
		toupper(reinterpret_cast<char*>(m_data->data.get()));
	}
	return *this;
}

inline String &String::lowerASCII() {
	if (m_data) {
		m_data = details::DataBlock::isolate(m_data, *m_zone);
		tolower((char*)m_data->data.get());
	}
	return *this;
}

inline String &String::reverseASCII() {
	if (m_data) {
		m_data = details::DataBlock::isolate(m_data, *m_zone);
		std::reverse((char*)m_data->data.get(), ((char*)m_data->data.get()) + m_data->size);
	}
	return *this;
}

inline String &String::trimSubstr(int ofs, int count) {
	*this = substr(ofs, count);
	return *this;
}

inline String &String::trimSubstrBytes(int ofs, int count) {
	*this = substrBytes(ofs, count);
	return *this;
}

inline String &String::trimLeft(int count) {
	*this = left(count);
	return *this;
}

inline String &String::trimRight(int count) {
	*this = right(count);
	return *this;
}

inline String &String::trimLeftBytes(int count) {
	*this = leftBytes(count);
	return *this;
}

inline String &String::trimRightBytes(int count) {
	*this = rightBytes(count);
	return *this;
}

inline String &String::append(const String &str) {
	return nAppend(str, str.length);
}

inline String &String::append(const char *sz) {
	return append(String(sz, RefTag));
}

inline String &String::append(const wchar_t *sz) {
	return append(String(sz, *m_zone));
}

inline String &String::append(const char c) {
	char x[2] = {c, 0};
	return append(x);
}

inline String &String::append(const wchar_t c) {
	wchar_t x[2] = {c, 0};
	return append(x);
}

inline String &String::nAppend(const String &str, int len) {
	return nAppendBytes(str, str.byteForChar(len));
}

inline String &String::nAppend(const char *sz, int len) {
	return nAppend(String(sz, RefTag), len);
}

inline String &String::nAppend(const wchar_t *sz, int len) {
	return nAppend(String(sz, *m_zone), len);
}

inline String &String::nAppendBytes(const char *sz, int len) {
	return nAppendBytes(String(sz, RefTag), len);
}

inline String &String::nAppendBytes(const wchar_t *sz, int len) {
	return nAppendBytes(String(sz, *m_zone), len);
}

inline String &String::replace(char src, char dst) {
	char x[] = {src, 0};
	char y[] = {dst, 0};
	return replace(x, y);
}

inline String &String::replace(char src, const char *dst) {
	char x[] = {src, 0};
	return replace(x, dst);
}

inline String &String::replace(char src, wchar_t dst) {
	char x[] = {src, 0};
	wchar_t y[] = {dst, 0};
	return replace(x, y);
}

inline String &String::replace(char src, const wchar_t *dst) {
	char x[] = {src, 0};
	return replace(x, dst);
}

inline String &String::replace(const char *src, char dst) {
	char y[] = {dst, 0};
	return replace(src, y);
}

inline String &String::replace(const char *src, const char *dst) {
	return replace(String(src, RefTag), String(dst, RefTag));
}

inline String &String::replace(const char *src, wchar_t dst) {
	wchar_t y[] = {dst, 0};
	return replace(src, y);
}

inline String &String::replace(const char *src, const wchar_t *dst) {
	return replace(String(src, RefTag), String(dst));
}

inline String &String::replace(wchar_t src, char dst) {
	wchar_t x[] = {src, 0};
	char y[] = {dst, 0};
	return replace(x, y);
}

inline String &String::replace(wchar_t src, const char *dst) {
	wchar_t x[] = {src, 0};
	return replace(x, dst);
}

inline String &String::replace(wchar_t src, wchar_t dst) {
	wchar_t x[] = {src, 0};
	wchar_t y[] = {dst, 0};
	return replace(x, y);
}

inline String &String::replace(wchar_t src, const wchar_t *dst) {
	wchar_t x[] = {src, 0};
	return replace(x, dst);
}

inline String &String::replace(const wchar_t *src, char dst) {
	char y[] = {dst, 0};
	return replace(src, y);
}

inline String &String::replace(const wchar_t *src, const char *dst) {
	return replace(String(src), String(dst, RefTag));
}

inline String &String::replace(const wchar_t *src, wchar_t dst) {
	wchar_t y[] = {dst, 0};
	return replace(src, y);
}

inline String &String::replace(const wchar_t *src, const wchar_t *dst) {
	return replace(String(src), String(dst));
}

inline String &String::printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf(fmt, args);
	va_end(args);
	return *this;
}

inline String &String::printfASCII(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printfASCII(fmt, args);
	va_end(args);
	return *this;
}

inline String &String::operator = (const String &string) {
	m_data = string.m_data;
	m_zone = string.m_zone;
	return *this;
}

inline String &String::operator = (const char *sz) {
	return (*this = String(sz));
}

inline String &String::operator = (const wchar_t *sz) {
	return (*this = String(sz));
}

inline String &String::operator = (char c) {
	return (*this = String(c));
}

inline String &String::operator = (wchar_t c) {
	return (*this = String(c));
}

inline String &String::operator += (const String &string) {
	return append(string);
}

inline String &String::operator += (const char *sz) {
	return append(sz);
}

inline String &String::operator += (const wchar_t *sz) {
	return append(sz);
}

inline String &String::operator += (char c) {
	return append(c);
}

inline String &String::operator += (wchar_t c) {
	return append(c);
}

inline String &String::write(int pos, char sz) {
	return write(pos, &sz, 1);
}

inline String &String::write(int pos, const char *sz) {
	return write(pos, sz, len(sz)+1);
}

inline String &String::write(int pos, const String &str) {
	return write(pos, str.c_str, str.length);
}

inline String &String::write(int pos, const String &str, int len) {
	return write(pos, str.c_str, len);
}

inline String &String::clear() {
	m_data.reset();
	return *this;
}

inline String operator + (const String &a, const String &b) {
	String x(a);
	x.append(b);
	return x;
}

inline String operator + (const String &a, const char *sz) {
	String x(a);
	x.append(sz);
	return x;
}

inline String operator + (const String &a, const wchar_t *sz) {
	String x(a);
	x.append(sz);
	return x;
}

inline String operator + (const char *sz, const String &b) {
	String x(sz, RefTag);
	x.append(b);
	return x;
}

inline String operator + (const wchar_t *sz, const String &b) {
	String x(sz);
	x.append(b);
	return x;
}

inline String operator + (char s, const String &b) {
	String x(s);
	x.append(b);
	return x;
}

inline String operator + (wchar_t s, const String &b) {
	String x(s);
	x.append(b);
	return x;
}

inline String operator + (const String &b, char s) {
	String x(b);
	x.append(s);
	return x;
}

inline String operator + (const String &b, wchar_t s) {
	String x(b);
	x.append(s);
	return x;
}

inline int String::RAD_IMPLEMENT_GET(length) {
	return m_data ? (m_data->size - 1) : 0;
}

inline const char *String::RAD_IMPLEMENT_GET(begin) {
	return c_str.get();
}

inline const char *String::RAD_IMPLEMENT_GET(end) {
	return c_str.get() + length.get();
}

inline const char *String::RAD_IMPLEMENT_GET(c_str) {
	return m_data ? (const char*)m_data->data.get() : "";
}

inline int String::RAD_IMPLEMENT_GET(numChars) {
	return m_data ? utf8to32len((const char*)m_data->data.get(), m_data->size - 1) : 0;
}

inline bool String::RAD_IMPLEMENT_GET(empty) {
	return !m_data;
}

} // string

inline string::String CStr(const char *sz) {
	return string::String(sz, string::RefTag);
}


template<class CharType, class Traits>
std::basic_istream<CharType, Traits>& operator >> (std::basic_istream<CharType, Traits> &stream, string::String &string) {
	std::string x;
	stream >> x;
	string = x.c_str();
	return stream;
}


template<class CharType, class Traits>
std::basic_ostream<CharType, Traits>& operator << (std::basic_ostream<CharType, Traits> &stream, const string::String &string) {
	stream << string.c_str.get();
	return stream;
}

#include "../PopSystemMacros.h"
