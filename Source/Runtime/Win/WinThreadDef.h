// WinThreadDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once


namespace thread {
namespace details {

typedef U32 Id;
class ThreadContext;
class Thread;
class Fiber;

enum
{
	TimeSlice = 1 // MSDN docs (Sleep(1) will yield to lower priority threads).
};

} // details
} // thread

