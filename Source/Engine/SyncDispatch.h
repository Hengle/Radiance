// SyncDispatch.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"
#include "TaskManager/TaskManager.h"
#include "SyncDispatchFunctor.inl"
#include <Runtime/Time.h>

namespace sync_dispatch {

RADENG_API void RADENG_CALL QueueCallback(IFunctorBase *m, const task::HGate &gate);
RADENG_API void RADENG_CALL ProcessCallbacks(const xtime::TimeSlice &time);

} // sync_dispatch

#include "SyncDispatchMethods.inl"
