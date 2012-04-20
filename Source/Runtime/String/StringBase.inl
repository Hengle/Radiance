// ANSIStringBase.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel
// See Radiance/LICENSE for licensing terms.

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

template<>
inline bool isspace<char>(char c)
{
	return ::isspace(static_cast<int>(c) & 0xFF) != 0;
}

template<>
inline bool isspace<wchar_t>(wchar_t c)
{
	return ::iswspace(static_cast<wint_t>(c)) != 0;
}

template<>
inline bool isalpha<char>(char c)
{
	return ::isalpha(static_cast<int>(c) & 0xFF) != 0;
}

template<>
inline bool isalpha<wchar_t>(wchar_t c)
{
	return ::iswalpha(static_cast<wint_t>(c)) != 0;
}

template<>
inline bool isdigit<char>(char c)
{
	return ::isdigit(static_cast<int>(c) & 0xFF) != 0;
}

template<>
inline bool isdigit<wchar_t>(wchar_t c)
{
	return ::iswdigit(static_cast<wint_t>(c)) != 0;
}

template<>
inline bool isprint<char>(char c)
{
	return ::isprint(static_cast<int>(c) & 0xFF) != 0;
}

template<>
inline bool isprint<wchar_t>(wchar_t c)
{
	return ::iswprint(static_cast<wint_t>(c)) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// string::cmp()
//////////////////////////////////////////////////////////////////////////////////////////

template<>
inline int cmp<char>(const char *a, const char *b)
{
	return ::strcmp(a, b);
}

template<>
inline int cmp<wchar_t>(const wchar_t *a, const wchar_t *b)
{
	return ::wcscmp(a, b);
}

template<>
inline int icmp<char>(const char *a, const char *b)
{
#if defined(RAD_OPT_WINX)
	return ::stricmp(a, b);
#else
	return ::strcasecmp(a, b);
#endif
}

template<>
inline int icmp<wchar_t>(const wchar_t *a, const wchar_t *b)
{
#if defined(RAD_OPT_WINX)
	return ::wcsicmp(a, b);
#elif defined(RAD_OPT_APPLE)
	wchar_t *_a = (wchar_t*)stack_alloc((::wcslen(a)+1)*sizeof(wchar_t));
	wchar_t *_b = (wchar_t*)stack_alloc((::wcslen(b)+1)*sizeof(wchar_t));
	wchar_t *z  = _a;
	while(*a) { *z = ::towlower(*a); ++z; ++a; }
	*z = 0;
	z = _b;
	while(*b) { *z = ::towlower(*b); ++z; ++b; }
	*z = 0;
	return ::wcscmp(_a, _b);
#else
	return ::wcscasecmp(a, b);
#endif
}

template<>
inline int ncmp<char>(const char *str0, const char *str1, size_t len)
{
	return ::strncmp(str0, str1, len);
}
template<>
inline int ncmp<wchar_t>(const wchar_t *str0, const wchar_t *str1, size_t len)
{
	return ::wcsncmp(str0, str1, len);
}

template<>
inline int nicmp<char>(const char *str0, const char *str1, size_t len)
{
#if defined(RAD_OPT_WINX)
	return ::strnicmp(str0, str1, len);
#else
	return ::strncasecmp(str0, str1, len);
#endif
}

template<>
inline int nicmp<wchar_t>(const wchar_t *a, const wchar_t *b, size_t _len)
{
#if defined(RAD_OPT_WINX)
	return ::wcsnicmp(a, b, _len);
#elif defined(RAD_OPT_APPLE)
	wchar_t *_a = (wchar_t*)a;
	size_t _l = _len;
	size_t _s = 0;
	while (*_a++ && _l--) { ++_s; }
	_a = (wchar_t*)stack_alloc((_s+1)*sizeof(wchar_t));
	wchar_t *_b = (wchar_t*)b;
	_s = 0;
	_l = _len;
	while (*_b++ && _l--) { ++_s; }
	_b = (wchar_t*)stack_alloc((_s+1)*sizeof(wchar_t));
	wchar_t *z  = _a;
	_l = _len;
	while(*a && _l) { *z = ::towlower(*a); ++z; ++a; --_l; }
	*z = 0;
	z = _b;
	_l = _len;
	while(*b && _l) { *z = ::towlower(*b); ++z; ++b; --_l; }
	*z = 0;
	return ::wcsncmp(_a, _b, _len);
#else
	return ::wcsncasecmp(a, b, _len);
#endif
}

template<>
inline int coll<char>(const char *a, const char *b)
{
	return ::strcoll(a, b);
}

template<>
inline int coll<wchar_t>(const wchar_t *a, const wchar_t *b)
{
	return ::wcscoll(a, b);
}

template<>
inline size_t spn<char>(const char *a, const char *b)
{
	return ::strspn(a, b);
}

template<>
inline size_t spn<wchar_t>(const wchar_t *a, const wchar_t *b)
{
	return ::wcsspn(a, b);
}

template<>
inline size_t cspn<char>(const char *a, const char *b)
{
	return ::strcspn(a, b);
}

template<>
inline size_t cspn<wchar_t>(const wchar_t *a, const wchar_t *b)
{
	return ::wcscspn(a, b);
}

template<>
inline const char *strstr(const char *a, const char *b)
{
	return ::strstr(a, b);
}

template<>
inline const wchar_t *strstr(const wchar_t *a, const wchar_t *b)
{
	return ::wcsstr(a, b);
}

template<>
inline int atoi(const char *str)
{
	return ::atoi(str);
}

template<>
inline int atoi(const wchar_t *str)
{
#if defined(RAD_OPT_WINX)
	return ::_wtoi(str);
#else
	wchar_t *x;
	return ::wcstod(str, &x);
#endif
}

template<>
inline float atof(const char *str)
{
	return (float)::atof(str);
}

template<>
inline float atof(const wchar_t *str)
{
#if defined(RAD_OPT_WINX)
	return (float)::_wtof(str);
#else
	return (float)::wcstod(str, 0);
#endif
}

template<>
inline char *itoa<char>(int i, char *dst)
{
	::sprintf(dst, "%d", i);
	return dst;
}

template<>
inline wchar_t *itoa<wchar_t>(int i, wchar_t *dst)
{
	::swprintf(dst, 0xFFFFFFFF, L"%d", i);
	return dst;
}

template<>
inline char *toupper<char *>(char *a)
{
	char *z = a;
	while (*z != 0)
	{
		*z = ::toupper(*z);
		++z;
	}

	return a;
}

template<>
inline wchar_t *toupper<wchar_t *>(wchar_t *a)
{
	wchar_t *z = a;
	while (*z != 0)
	{
		*z = ::towupper(*z);
		++z;
	}

	return a;
}

template<>
inline char toupper<char>(char a)
{
	return static_cast<char>(::toupper(a));
}

template<>
inline wchar_t toupper<wchar_t>(wchar_t a)
{
	return ::towupper(a);
}

template<>
inline char *tolower<char *>(char *a)
{
	char *z = a;
	while (*z != 0)
	{
		*z = ::tolower(*z);
		++z;
	}

	return a;
}

template<>
inline wchar_t *tolower<wchar_t *>(wchar_t *a)
{
	wchar_t *z = a;
	while (*z != 0)
	{
		*z = ::towlower(*z);
		++z;
	}

	return a;
}

template<>
inline char tolower<char>(char a)
{
	return (char)::tolower(a);
}

template<>
inline wchar_t tolower<wchar_t>(wchar_t a)
{
	return ::towlower(a);
}

template <typename T>
inline T *reverse(T *a)
{
	T *f = a;
	T *b = a + len(a) - 1;

	while (f < b)
	{
		std::swap(*f, *b);
		++f;
		++b;
	}

	return a;
}

template<>
inline size_t len(const char *a)
{
	RAD_ASSERT(a);
	return ::strlen(a);
}

template<>
inline size_t len(const wchar_t *a)
{
	RAD_ASSERT(a);
	return ::wcslen(a);
}

template<>
inline char *cpy<char>(char *dst, const char *src)
{
	RAD_ASSERT(dst && src);
	return ::strcpy(dst, src);
}

template<>
inline wchar_t *cpy<wchar_t>(wchar_t *dst, const wchar_t *src)
{
	RAD_ASSERT(dst && src);
	return ::wcscpy((wchar_t *)dst, (const wchar_t *)src);
}

template<>
inline char *ncpy<char>(char *dst, const char *src, size_t len)
{
	RAD_ASSERT(dst && src);
	if (len > 0)
	{
		::strncpy(dst, src, len-1);
		dst[len-1] = 0;
	}

	return dst;
}

template<>
inline wchar_t *ncpy<wchar_t>(wchar_t *dst, const wchar_t *src, size_t len)
{
	RAD_ASSERT(dst && src);
	if (len > 0)
	{
		::wcsncpy(dst, src, len-1);
		dst[len-1] = 0;
	}

	return dst;
}

template<>
inline char *cat<char>(char *dst, const char *src)
{
	RAD_ASSERT(dst && src);
	return ::strcat(dst, src);
}

template<>
inline wchar_t *cat<wchar_t>(wchar_t *dst, const wchar_t *src)
{
	return ::wcscat(dst, src);
}

template<>
inline char *ncat<char>(char *dst, const char *src, size_t len)
{
	RAD_ASSERT(dst && src);
	return ::strncat(dst, src, len - 1);
}

template<>
inline wchar_t *ncat<wchar_t>(wchar_t *dst, const wchar_t *src, size_t len)
{
	return ::wcsncat(dst, src, len);
}

template<>
inline int vsprintf<char>(char *buffer, const char *format, va_list argptr)
{
	return ::vsprintf(buffer, format, argptr );
}

template<>
inline int vsprintf<wchar_t>(wchar_t *buffer, const wchar_t *format, va_list argptr)
{
	return ::vswprintf(buffer, MaxAddrSize, format, argptr);
}

template<>
inline int vsnprintf<char>(char *buffer, size_t count, const char *format, va_list argptr)
{
	return ::vsnprintf(buffer, count, format, argptr);
}

template<>
inline int vsnprintf<wchar_t>(wchar_t *buffer, size_t count, const wchar_t *format, va_list argptr)
{
	return ::vswprintf(buffer, count, format, argptr);
}

template<>
inline int vscprintf<char>(const char *format, va_list argptr)
{
	enum { TempMax = 32*Kilo };
	char temp[TempMax];
	int z = ::vsnprintf(temp, TempMax-1, format, argptr);
	if (z < 0)
	{
		z = TempMax-1;
	}
	temp[TempMax-1] = 0;
	return z;
}

template<>
inline int vscprintf<wchar_t>(const wchar_t *format, va_list argptr)
{
	enum { TempMax = 32*Kilo };
	wchar_t temp[TempMax];
	int z = ::vswprintf(temp, TempMax-1, format, argptr);
	if (z < 0)
	{
		z = TempMax-1;
	}
	temp[TempMax-1] = 0;
	return z;
}

template<>
inline int sprintf<char>(char *dst, const char *fmt, ...)
{
	RAD_ASSERT(dst && fmt);

	va_list args;
	va_start(args, fmt);
	int c = vsprintf(dst, fmt, args);
	va_end(args);

	return c;
}

template<>
inline int sprintf<wchar_t>(wchar_t *dst, const wchar_t *fmt, ...)
{
	RAD_ASSERT(dst && fmt);

	va_list args;
	va_start(args, fmt);
#if defined(RAD_OPT_WIN)
	int c = vswprintf(dst, fmt, args);
#else
	int c = vswprintf(dst, 1024*1024, fmt, args);
#endif
	va_end(args);

	return c;
}

// Unlike the Windows version, this snprintf will always append a NULL character,
// and will therefore possibly copy one less character than _snprintf() would on
// windows.
// note this handles char and wchar_t!

template<>
inline int snprintf<char>(char *dst, size_t len, const char *fmt, ...)
{
	RAD_ASSERT(dst && fmt);

	int c = 0;
	if (len > 0)
	{
		va_list args;
		va_start(args, fmt);
		c = vsnprintf(dst, len, fmt, args);
		va_end(args);
		dst[len-1] = 0;

		if (c < -1)
			return Error;
		if (c == -1)
			c = (int)len - 1; // don't count NULL.
	}

	return c;
}

template<>
inline int snprintf<wchar_t>(wchar_t *dst, size_t len, const wchar_t *fmt, ...)
{
	RAD_ASSERT(dst && fmt);

	int c = 0;
	if (len > 0)
	{
		va_list args;
		va_start(args, fmt);
#if defined(RAD_OPT_WIN)
		c = _vsnwprintf(dst, len, fmt, args);
#else
		c = vswprintf(dst, len, fmt, args);
#endif
		va_end(args);
		dst[len-1] = 0;

		if (c < -1)
			return Error;
		if (c == -1)
			c = (int)len - 1; // don't count NULL.
	}

	return c;
}

template<>
inline int scprintf<char>(const char *fmt, ...)
{
	RAD_ASSERT(fmt);
	va_list args;
	va_start(args, fmt);
#if defined(RAD_OPT_WIN)
	int c = vscprintf(fmt, args);
#else
	int c = vsnprintf((char*)0, 0, fmt, args);
#endif
	va_end(args);
	return c;
}

template<>
inline int scprintf<wchar_t>(const wchar_t *fmt, ...)
{
	RAD_ASSERT(fmt);
	va_list args;
	va_start(args, fmt);
#if defined(RAD_OPT_WIN)
	int c = _vscwprintf(fmt, args);
#else
	int c = vswprintf((wchar_t*)0, 0, fmt, args);
#endif
	va_end(args);
	return c;
}

} // string

