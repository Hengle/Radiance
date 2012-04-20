// Assert.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if !defined(__PRIVATE_RAD_OPT_INTBASE__)
	#error "Do not include this file directly, please include Base.h instead"
#endif

#define __RAD_ASSERT_CHAR char
#define __RAD_ASSERT_WIDE(_str) _str

RADRT_API void RADRT_CALL __rad_assert(
	const __RAD_ASSERT_CHAR *message,
	const __RAD_ASSERT_CHAR *file,
	const __RAD_ASSERT_CHAR *function,
	unsigned int line
);

#include <boost/static_assert.hpp>

#define RAD_STATIC_ASSERT(_expr) BOOST_STATIC_ASSERT(_expr)
#define RAD_VERIFY_MSG(_expr, _msg) (void)((!!(_expr))||(__rad_assert(_msg, __RAD_ASSERT_WIDE(__FILE__), __RAD_ASSERT_WIDE(__FUNCTION__), __LINE__),0))
#define RAD_VERIFY(_expr) RAD_VERIFY_MSG(_expr, __RAD_ASSERT_WIDE(#_expr))

#if defined(NDEBUG)
#define RAD_ASSERT(_expr) ((void)0)
#else
#define RAD_ASSERT(_expr) RAD_VERIFY(_expr)
#endif

#define RAD_ASSERT_MSG(_expr, _msg) RAD_ASSERT((_expr) && (_msg))
#define RAD_FAIL(_msg) RAD_VERIFY(!(_msg))
#define RAD_FAIL_EX(_msg) RAD_VERIFY_MSG(0, _msg)
#define RAD_OUT_OF_MEM(_x) RAD_VERIFY_MSG(_x, "Out Of Memory!")

#if defined(RAD_OPT_DEBUG)
RADRT_API void RADRT_CALL DebugString(const char* message, ...);
#else
inline void DebugString(const char* message, ...) {}
#endif

