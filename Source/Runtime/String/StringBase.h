// StringBase.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntStringBase.h"
#include <string.h>
#include <cstdarg>
#include "../PushPack.h"


namespace string {

enum { Error = -1 };

template<typename T>
bool isspace(T c);

template<typename T>
bool isalpha(T c);

template<typename T>
bool isdigit(T c);

template<typename T>
bool isprint(T c);

template<typename T>
int cmp(const T* str0, const T* str1);

template<typename T>
int icmp(const T* str0, const T* str1);

template<typename T>
int ncmp(const T* str0, const T* str1, size_t len);

template<typename T>
int nicmp(const T* str0, const T* str1, size_t len);

template<typename T>
int coll(const T *a, const T *b);

template<typename T>
size_t spn(const T *a, const T *b);

template<typename T>
size_t cspn(const T *a, const T *b);

template<typename T>
const T *strstr(const T *a, const T*b);

template<typename T>
int atoi(const T *str);

template<typename T>
float atof(const T *str);

template<typename T>
T *itoa(int i, T *dst);

template<typename T>
T toupper(T a);

template<typename T>
T tolower(T a);

template<typename T> 
T *reverse(T *a);

template<typename T>
size_t len(const T* str0);

template<typename T>
T *cpy(T* dst, const T* src);

// Unlike ANSI strncpy(), ncpy() will always make sure a NULL is appended, in effect meaning
// that sometimes one less character of "src" is copied.
//
// "len" is the size of the "dst" buffer.

template<typename T>
T *ncpy(T* dst, const T* src, size_t len);

template<typename T>
T *cat(T* dst, const T* src);

template<typename T>
T *ncat(T* dst, const T* src, size_t len);

template<typename T> 
int vsprintf(T *dst, const T *format, va_list argptr);

template<typename T> 
int vsnprintf(T *dst, size_t count, const T *format, va_list argptr);

template<typename T> 
int vscprintf(const T *format, va_list argptr);

template<typename T>
int sprintf(T* dst, const T* fmt, ...);

// Unlike the Windows version, this snprintf will always append a NULL character,
// and will therefore possibly copy one less character than _snprintf() would on
// windows.
//
// "count" is the size of the "dst" buffer.

template<typename T>
int snprintf(T* dst, size_t count, const T* fmt, ...);

template<typename T>
int scprintf(const T* fmt, ...);

} // string


#include "../PopPack.h"
#include "StringBase_inl.h"
