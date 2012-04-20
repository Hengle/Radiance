// Assert.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Base.h"
#undef NDEBUG
#include <assert.h>
#include "../String.h"

using namespace string;

RADRT_API void RADRT_CALL __rad_assert(
	const __RAD_ASSERT_CHAR *message,
	const __RAD_ASSERT_CHAR *file,
	const __RAD_ASSERT_CHAR *function,
	unsigned int line)
{
#if defined(RAD_OPT_WIN)
	RAD_NOT_USED(function);
	_wassert(Widen(message).c_str(), Widen(file).c_str(), line);
#elif defined(RAD_OPT_APPLE)
	__assert_rtn(function, file, line, message);
#else
	__assert_fail(message, file, line, function);
#endif
}

#if defined(RAD_OPT_DEBUG)

#if defined(RAD_OPT_WINX)
#include "../Win/WinHeaders.h"
#else
#include <stdlib.h>
#endif

RADRT_API void RADRT_CALL DebugString(const char* message, ...)
{
	enum { MaxSize = 32*Kilo };
	char dbgString[MaxSize];
	va_list arglist;
	int count;

	va_start(arglist, message);
	count = ::string::vscprintf(message, arglist);
	if (count > MaxSize)
		count = MaxSize;

	::string::vsnprintf(dbgString, count, message, arglist);
	dbgString[count] = 0;
	va_end(arglist);

#if defined(RAD_OPT_WINX)
	OutputDebugStringA(dbgString);
#else
	fprintf(stderr, "%s", dbgString);
	fflush(stderr);
#endif
}
#endif
