// Types.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if !defined(__PRIVATE_RAD_OPT_INTBASE__)
	#error "Do not include this file directly, please include Base.h instead"
#endif

#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/static_assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <limits.h>
#include <limits>
#include "../PushPack.h"

enum
{
	DefaultAlignment = RAD_STRUCT_ALIGN,
	MinAlignment = 4,
	MaxU8 =  UCHAR_MAX,
	MinS8 =  SCHAR_MIN,
	MaxS8 =  SCHAR_MAX,
	MaxU16 = USHRT_MAX,
	MinS16 = SHRT_MIN,
	MaxS16 = SHRT_MAX,
	MaxU32 = UINT_MAX,
	MinS32 = LONG_MIN,
	MaxS32 = LONG_MAX
};

extern RADRT_API const U64 MaxU64;
extern RADRT_API const S64 MinS64;
extern RADRT_API const S64 MaxS64;

#if RAD_OPT_MACHINE_WORD_SIZE == 4
enum
{
	MaxUReg = MaxU32,
	MinSReg = MinS32,
	MaxSReg = MaxS32,
	MaxAddrSize = MaxU32
};
#elif RAD_OPT_MACHINE_WORD_SIZE == 8
enum
{
	MaxUReg = MaxU32,
	MinSReg = MinS32,
	MaxSReg = MaxS32,
	MaxAddrSize = MaxU64
};
#endif

typedef SAddrSize PtrDiff;

template <typename T>
struct Constants
{
	BOOST_STATIC_ASSERT(sizeof(T) >= 2);

	static T Kilo() { return (T)1024; }
	static T Meg() { return (T)(1024*1024); }
	static T Gig() { return (T)(1024*1024*1024); }
};

enum
{
	Kilo = 1024,
	Meg  = Kilo*Kilo,
	Gig  = Meg*Kilo
};

struct StructAlignmentCheck
{
	U8  a;
	U16 b;
	U8  c;
	U32 d;
	U8  e;
	U64 f;
};

#define RAD_DECLARE_EXCEPTION_CODE(_class, _base, _code)\
struct _class : \
public _base \
{ \
	_class() : _base(_code, 0, 0, #_class, _base::tag()) {} \
	_class(const _class &x) : _base(x) {} \
	explicit _class(int code) : _base(code, 0, 0, #_class, _base::tag()) {} \
	explicit _class(const char* what) : _base(_code, what, 0, #_class, _base::tag()) {} \
	explicit _class(int code, const char* what) : _base(code, what, 0, #_class, _base::tag()) {} \
	explicit _class(int code, const _class &e) : _base(code, e.what(), 0, #_class, _base::tag()) {} \
	virtual ~_class() throw() {} \
	_class &operator=(const _class &e) { _base::operator = (e); return *this; } \
protected:\
	struct tag {};\
	_class(int code, const char *const &s, int noalloc, const char *type, const tag&) : \
		_base(code, s, noalloc, type, _base::tag()) \
		{\
		}\
}

#define RAD_DECLARE_EXCEPTION(_class, _base) RAD_DECLARE_EXCEPTION_CODE(_class, _base, 0)

template <typename T>
struct ValueTraits
{
	typedef T Type;
	typedef T ValueType;

	static ValueType ToValue(const Type &n) { return static_cast<ValueType>(n); }
};

#include "../PopPack.h"
