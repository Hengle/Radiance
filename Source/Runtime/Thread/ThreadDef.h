// ThreadDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"


namespace thread {

class Thread;
class Fiber;
class IThreadContext;
class Gate;

} // thread


#include "BackendDef.h"


namespace thread {

typedef details::Id Id;

enum PriorityClass
{
	PriorityLow,
	PriorityNormal,
	PriorityMedium,
	PriorityHigh
};

enum
{
	Infinite = MaxUReg,
	DefaultContext = MaxUReg,
	CurrentContext = MaxUReg-1,
	TimeSlice = details::TimeSlice,
	Default = 0
};

} // thread

