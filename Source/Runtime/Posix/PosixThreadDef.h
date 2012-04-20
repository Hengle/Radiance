// PosixThreadDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <pthread.h>


namespace thread {
namespace details {

typedef pthread_t Id;
class ThreadContext;
class Thread;
class Fiber;

enum
{
	TimeSlice = 1
};

} // details
} // thread

