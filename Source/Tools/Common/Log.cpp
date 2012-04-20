// Log.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "Log.h"

namespace
{
	int s_level = LogInfo;
};

void SetLogLevel(int level)
{
	s_level = level;
}

void Log(int level, const char *fmt, ...)
{
	if (level <= s_level)
	{
		char z[1024];
		va_list args;
		va_start(args, fmt);
		vsprintf(z, fmt, args);
		va_end(args);
		printf(z);
	}
}

void Error(const char *fmt, ...)
{
	char z[1024];
	va_list args;
	va_start(args, fmt);
	vsprintf(z, fmt, args);
	va_end(args);
	printf(z);
	exit(-1);
}