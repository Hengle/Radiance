// Log.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

enum
{
	LogError,
	LogNormal,
	LogWarning,
	LogInfo
};

void SetLogLevel(int level);

void Log(int level, const char *fmt, ...);
void Error(const char *fmt, ...); // terminates.
