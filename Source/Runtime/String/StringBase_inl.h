/*! \file StringBase_inl.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Runtime
*/

#include <string.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#if defined(RAD_OPT_GCC) && !defined(_GNU_SOURCE)
	#define _GNU_SOURCE
#endif
#include <wchar.h>
#include <wctype.h>
#include <algorithm>

#if defined(RAD_OPT_APPLE)
	#include "../Utils.h"
#endif


namespace string {

inline int wcstombslen(const wchar_t *src, int len) {
#if defined(RAD_OPT_4BYTE_WCHAR)
	return utf32to8len((const U32*)src, len);
#else
	return utf16to8len((const U16*)src, len);
#endif
}

inline int wcstombs(char *dst, const wchar_t *src, int len) {
#if defined(RAD_OPT_4BYTE_WCHAR)
	return utf32to8(dst, (const U32*)src, len);
#else
	return utf16to8(dst, (const U16*)src, len);
#endif
}

inline int mbstowcslen(const char *src, int len) {
#if defined(RAD_OPT_4BYTE_WCHAR)
	return utf8to32len(src, len);
#else
	return utf8to16len(src, len);
#endif
}

inline int mbstowcs(wchar_t *dst, const char *src, int len) {
#if defined(RAD_OPT_4BYTE_WCHAR)
	return utf8to32((U32*)dst, src, len);
#else
	return utf8to16((U16*)dst, src, len);
#endif
}

template<>
inline bool isspace<char>(char c) {
	return ::isspace(static_cast<int>(c) & 0xFF) != 0;
}

template<>
inline bool isspace<wchar_t>(wchar_t c) {
	return ::iswspace(static_cast<wint_t>(c)) != 0;
}

template<>
inline bool isalpha<char>(char c) {
	return ::isalpha(static_cast<int>(c) & 0xFF) != 0;
}

template<>
inline bool isalpha<wchar_t>(wchar_t c) {
	return ::iswalpha(static_cast<wint_t>(c)) != 0;
}

template<>
inline bool isdigit<char>(char c) {
	return ::isdigit(static_cast<int>(c) & 0xFF) != 0;
}

template<>
inline bool isdigit<wchar_t>(wchar_t c) {
	return ::iswdigit(static_cast<wint_t>(c)) != 0;
}

template<>
inline bool isprint<char>(char c) {
	return ::isprint(static_cast<int>(c) & 0xFF) != 0;
}

template<>
inline bool isprint<wchar_t>(wchar_t c) {
	return ::iswprint(static_cast<wint_t>(c)) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// string::cmp()
//////////////////////////////////////////////////////////////////////////////////////////

template<>
inline int cmp<char>(const char *a, const char *b) {
	return ::strcmp(a, b);
}

template<>
inline int cmp<wchar_t>(const wchar_t *a, const wchar_t *b) {
	return ::wcscmp(a, b);
}

template<>
inline int icmp<char>(const char *a, const char *b) {
#if defined(RAD_OPT_WINX)
	return ::stricmp(a, b);
#else
	return ::strcasecmp(a, b);
#endif
}

template<>
inline int icmp<wchar_t>(const wchar_t *a, const wchar_t *b) {
#if defined(RAD_OPT_WINX)
	return ::wcsicmp(a, b);
#elif defined(RAD_OPT_APPLE)
	wchar_t *_a = (wchar_t*)stack_alloc((::wcslen(a)+1)*sizeof(wchar_t));
	wchar_t *_b = (wchar_t*)stack_alloc((::wcslen(b)+1)*sizeof(wchar_t));
	wchar_t *z  = _a;
	while(*a) { 
		*z = ::towlower(*a); 
		++z; 
		++a; 
	}
	*z = 0;
	z = _b;
	while(*b) { 
		*z = ::towlower(*b); 
		++z; 
		++b; 
	}
	*z = 0;
	return ::wcscmp(_a, _b);
#else
	return ::wcscasecmp(a, b);
#endif
}

template<>
inline int ncmp<char>(const char *str0, const char *str1, int len) {
	return ::strncmp(str0, str1, (size_t)len);
}
template<>
inline int ncmp<wchar_t>(const wchar_t *str0, const wchar_t *str1, int len) {
	return ::wcsncmp(str0, str1, (size_t)len);
}

template<>
inline int nicmp<char>(const char *str0, const char *str1, int len) {
#if defined(RAD_OPT_WINX)
	return ::strnicmp(str0, str1, (size_t)len);
#else
	return ::strncasecmp(str0, str1, (size_t)len);
#endif
}

template<>
inline int nicmp<wchar_t>(const wchar_t *a, const wchar_t *b, int _len) {
#if defined(RAD_OPT_WINX)
	return ::wcsnicmp(a, b, (size_t)_len);
#elif defined(RAD_OPT_APPLE)
	wchar_t *_a = (wchar_t*)a;
	int _l = _len;
	int _s = 0;
	while (*_a++ && _l--) { 
		++_s; 
	}
	_a = (wchar_t*)stack_alloc((_s+1)*sizeof(wchar_t));
	wchar_t *_b = (wchar_t*)b;
	_s = 0;
	_l = _len;
	while (*_b++ && _l--) { 
		++_s; 
	}
	_b = (wchar_t*)stack_alloc((_s+1)*sizeof(wchar_t));
	wchar_t *z  = _a;
	_l = _len;
	while(*a && _l) { 
		*z = ::towlower(*a); 
		++z; 
		++a; 
		--_l; 
	}
	*z = 0;
	z = _b;
	_l = _len;
	while(*b && _l) { 
		*z = ::towlower(*b); 
		++z; 
		++b; 
		--_l; 
	}
	*z = 0;
	return ::wcsncmp(_a, _b, (size_t)_len);
#else
	return ::wcsncasecmp(a, b, (size_t)_len);
#endif
}

template<>
inline int coll<char>(const char *a, const char *b) {
	return ::strcoll(a, b);
}

template<>
inline int coll<wchar_t>(const wchar_t *a, const wchar_t *b) {
	return ::wcscoll(a, b);
}

template<>
inline int spn<char>(const char *a, const char *b) {
	return (int)::strspn(a, b);
}

template<>
inline int spn<wchar_t>(const wchar_t *a, const wchar_t *b) {
	return (int)::wcsspn(a, b);
}

template<>
inline int cspn<char>(const char *a, const char *b) {
	return (int)::strcspn(a, b);
}

template<>
inline int cspn<wchar_t>(const wchar_t *a, const wchar_t *b) {
	return (int)::wcscspn(a, b);
}

template<>
inline const char *strstr(const char *a, const char *b) {
	return ::strstr(a, b);
}

template<>
inline const wchar_t *strstr(const wchar_t *a, const wchar_t *b) {
	return ::wcsstr(a, b);
}

template<>
inline int atoi(const char *str) {
	return ::atoi(str);
}

template<>
inline int atoi(const wchar_t *str) {
#if defined(RAD_OPT_WINX)
	return (int)::_wtoi(str);
#else
	wchar_t *x;
	return (int)::wcstod(str, &x);
#endif
}

template<>
inline float atof(const char *str) {
	return (float)::atof(str);
}

template<>
inline float atof(const wchar_t *str) {
#if defined(RAD_OPT_WINX)
	return (float)::_wtof(str);
#else
	return (float)::wcstod(str, 0);
#endif
}

template<>
inline char *itoa<char>(int i, char *dst) {
	::sprintf(dst, "%d", i);
	return dst;
}

template<>
inline wchar_t *itoa<wchar_t>(int i, wchar_t *dst) {
	::swprintf(dst, 0xFFFFFFFF, L"%d", i);
	return dst;
}

template<>
inline char *toupper<char *>(char *a) {
	char *z = a;
	while (*z != 0) {
		*z = ::toupper(*z);
		++z;
	}

	return a;
}

template<>
inline wchar_t *toupper<wchar_t *>(wchar_t *a) {
	wchar_t *z = a;
	while (*z != 0) {
		*z = ::towupper(*z);
		++z;
	}

	return a;
}

template<>
inline char toupper<char>(char a) {
	return static_cast<char>(::toupper(a));
}

template<>
inline wchar_t toupper<wchar_t>(wchar_t a) {
	return ::towupper(a);
}

template<>
inline char *tolower<char *>(char *a) {
	char *z = a;
	while (*z != 0) {
		*z = ::tolower(*z);
		++z;
	}

	return a;
}

template<>
inline wchar_t *tolower<wchar_t *>(wchar_t *a) {
	wchar_t *z = a;
	while (*z != 0) {
		*z = ::towlower(*z);
		++z;
	}

	return a;
}

template<>
inline char tolower<char>(char a) {
	return (char)::tolower(a);
}

template<>
inline wchar_t tolower<wchar_t>(wchar_t a) {
	return ::towlower(a);
}

template <typename T>
inline T *reverse(T *a) {
	T *f = a;
	T *b = a + len(a) - 1;

	while (f < b) {
		std::swap(*f, *b);
		++f;
		++b;
	}

	return a;
}

template<>
inline int len(const char *a) {
	RAD_ASSERT(a);
	return (int)::strlen(a);
}

template<>
inline int len(const wchar_t *a) {
	RAD_ASSERT(a);
	return (int)::wcslen(a);
}

template <typename T>
inline int len(const T *a) {
	const T *b = a;
	while (*b) {
		++b;
	}
	return (int)(b-a);
}

template<>
inline char *cpy<char>(char *dst, const char *src) {
	RAD_ASSERT(dst && src);
	return ::strcpy(dst, src);
}

template<>
inline wchar_t *cpy<wchar_t>(wchar_t *dst, const wchar_t *src) {
	RAD_ASSERT(dst && src);
	return ::wcscpy((wchar_t *)dst, (const wchar_t *)src);
}

template<>
inline char *ncpy<char>(char *dst, const char *src, int len) {
	RAD_ASSERT(dst && src);
	if (len > 0) {
		::strncpy(dst, src, len-1);
		dst[len-1] = 0;
	}

	return dst;
}

template<>
inline wchar_t *ncpy<wchar_t>(wchar_t *dst, const wchar_t *src, int len) {
	RAD_ASSERT(dst && src);
	if (len > 0) {
		::wcsncpy(dst, src, len-1);
		dst[len-1] = 0;
	}

	return dst;
}

template<>
inline char *cat<char>(char *dst, const char *src) {
	RAD_ASSERT(dst && src);
	return ::strcat(dst, src);
}

template<>
inline wchar_t *cat<wchar_t>(wchar_t *dst, const wchar_t *src) {
	return ::wcscat(dst, src);
}

template<>
inline char *ncat<char>(char *dst, const char *src, int len) {
	RAD_ASSERT(dst && src);
	return ::strncat(dst, src, len - 1);
}

template<>
inline wchar_t *ncat<wchar_t>(wchar_t *dst, const wchar_t *src, int len) {
	RAD_ASSERT(dst && src);
	return ::wcsncat(dst, src, len);
}

template<>
inline int vsprintf<char>(char *dst, const char *format, va_list argptr) {
	RAD_ASSERT(dst && format);
#if defined(RAD_OPT_GCC)
	va_list _valist;
	va_copy(_valist, argptr);
	int r = ::vsprintf(dst, format, _valist ) + 1;
	va_end(_valist);
	return r;
#else
	return ::vsprintf(dst, format, argptr ) + 1;
#endif
}

template<>
inline int vsprintf<wchar_t>(wchar_t *dst, const wchar_t *format, va_list argptr) {
	RAD_ASSERT(dst && format);
#if defined(RAD_OPT_GCC)
	va_list _valist;
	va_copy(_valist, argptr);
	int r = ::vswprintf(dst, 0xffffffff, format, _valist) + 1;
	va_end(_valist);
	return r;
#else
	return ::vswprintf(dst, 0xffffffff, format, argptr) + 1;
#endif
}

template<>
inline int vsnprintf<char>(char *dst, int count, const char *format, va_list argptr) {
	RAD_ASSERT(dst && format);
#if defined(RAD_OPT_GCC)
	va_list _valist;
	va_copy(_valist, argptr);
	int r = ::vsnprintf(dst, count, format, _valist);
	va_end(_valist);
	return r;
#else
	int r = ::vsnprintf(dst, (size_t)count, format, argptr);
#endif
#if defined(RAD_OPT_WINX)
	if (r < 0) // conform the return value from vsnprintf on windows (microsoft thinks they are cool).
		r = count;
#endif
	if (r < count)
		++r; // count null terminator in return.
	dst[count-1] = 0; // always null terminate.
	return r;
}

template<>
inline int vsnprintf<wchar_t>(wchar_t *dst, int count, const wchar_t *format, va_list argptr) {
	RAD_ASSERT(dst && format);
#if defined(RAD_OPT_GCC)
	va_list _valist;
	va_copy(_valist, argptr);
	int r = ::vswprintf(dst, count, format, _valist);
	va_end(_valist);
	return r;
#else
	int r = ::vswprintf(dst, (size_t)count, format, argptr);
#endif
	if (r < count)
		++r; // count null terminator in return.
	dst[count-1] = 0; // always null terminate.
	return r;
}

template<>
inline int vscprintf<char>(const char *format, va_list argptr) {
#if defined(RAD_OPT_WINX)
	return ::_vscprintf(format, argptr) + 1;
#else
	va_list _valist;
	va_copy(_valist, argptr);
	int r = ::vsnprintf(0, 0, format, _valist) + 1;
	va_end(_valist);
	return r;
#endif
}

template<>
inline int vscprintf<wchar_t>(const wchar_t *format, va_list argptr) {
#if defined(RAD_OPT_WINX)
	return ::_vscwprintf(format, argptr) + 1;
#else

	va_list _valist;
	va_copy(_valist, argptr);
	
	// Posix systems lack a vsnwprintf... sigh
	enum { MaxLen = kKilo*4 };
	
	int r = ::vswprintf((wchar_t*)stack_alloc(MaxLen), MaxLen, format, _valist) + 1;
	RAD_ASSERT(r != -1); // make sure this buffer is large enough.
	
	va_end(_valist);
	return r;
#endif
}

template<>
inline int sprintf<char>(char *dst, const char *fmt, ...) {
	RAD_ASSERT(dst && fmt);

	va_list args;
	va_start(args, fmt);
	int c = vsprintf(dst, fmt, args);
	va_end(args);

	return c;
}

template<>
inline int sprintf<wchar_t>(wchar_t *dst, const wchar_t *fmt, ...) {
	RAD_ASSERT(dst && fmt);

	va_list args;
	va_start(args, fmt);
#if defined(RAD_OPT_WIN)
	int c = vswprintf(dst, fmt, args);
#else
	int c = vswprintf(dst, std::numeric_limits<size_t>::max(), fmt, args);
#endif
	va_end(args);

	return c;
}

// Unlike the Windows version, this snprintf will always append a NULL character,
// and will therefore possibly copy one less character than _snprintf() would on
// windows.
// note this handles char and wchar_t!

template <typename T>
inline int snprintf(T *dst, int len, const T *fmt, ...) {
	RAD_ASSERT(dst && fmt);

	int c = 0;
	if (len > 0) {
		va_list args;
		va_start(args, fmt);
		c = vsnprintf<T>(dst, len, fmt, args);
		va_end(args);
		dst[len-1] = 0;
	}

	return c;
}

template <typename T>
inline int scprintf(const T *fmt, ...)
{
	RAD_ASSERT(fmt);
	va_list args;
	va_start(args, fmt);
	int c = vscprintf<T>(fmt, args);
	va_end(args);
	return c;
}

} // string

