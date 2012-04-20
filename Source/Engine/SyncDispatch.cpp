// SyncDispatch.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "SyncDispatch.h"
#include <Runtime/Container/ZoneDeque.h>
#include <Runtime/Thread/Locks.h>

namespace {

typedef boost::mutex Mutex;
typedef zone_deque<std::pair<sync_dispatch::IFunctorBase*, task::HGate>, ZEngineT>::type CallbackQueue;
CallbackQueue s_cbQueue;
static Mutex s_callbackM;
typedef boost::lock_guard<Mutex> Lock;

} // namespace


namespace sync_dispatch {

RADENG_API void RADENG_CALL QueueCallback(IFunctorBase *m, const task::HGate &gate);

RADENG_API void RADENG_CALL ProcessCallbacks(const xtime::TimeSlice &time)
{
	while (time.remaining)
	{
		std::pair<IFunctorBase*, task::HGate> p;
		{
			Lock L(s_callbackM);
			if (s_cbQueue.empty())
				break;
			p = s_cbQueue.front();
			s_cbQueue.pop_front();
		}
		if (p.first)
		{
			p.first->Call();
			p.first->Release();
			p.second->Open();
		}
	}
}

void QueueCallback(IFunctorBase *m, const task::HGate &gate)
{
	Lock L(s_callbackM);
	s_cbQueue.push_back(std::pair<IFunctorBase*, task::HGate>(m, gate));
}

} // sync_dispatch
