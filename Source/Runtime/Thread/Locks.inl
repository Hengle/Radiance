// Locks.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Thread.h"
#include "../PushSystemMacros.h"

namespace thread {

inline EventMutex::EventMutex() : m_waiting(0), m_ready(0) {
}

inline EventMutex::Sync::Sync(EventMutex &m) : 
L(m.m_m), m_m(m.m_m), m_c(m.m_c), m_x(m) {
}

inline void EventMutex::inc() {
	++m_waiting;
}

inline bool EventMutex::ready() {
	RAD_ASSERT(m_ready >= 0);
	if (m_ready > 0) {
		--m_ready;
		return true;
	}
	return false;
}

inline void EventMutex::Sync::inc() {
	m_x.inc();
}

inline bool EventMutex::Sync::ready() {
	return m_x.ready();
}

inline void EventMutex::Sync::wait() {
	inc();
	do {
		m_c.wait(L);
	} while (!ready());
}

inline bool EventMutex::Sync::timedWait(const boost::system_time &wait_until) {
	return m_c.timed_wait(L, wait_until);
}

inline bool EventMutex::Sync::timedWait(const boost::xtime &wait_until) {
	return m_c.timed_wait(L, wait_until);
}

template <typename duration_type>
inline bool EventMutex::Sync::timedWait(const duration_type &wait_duration) {
	return m_c.timed_wait(L, wait_duration);
}

///////////////////////////////////////////////////////////////////////////////

template <UReg NumKeys>
inline SegmentedLock<NumKeys>::SegmentedLock()
{
}

template <UReg NumKeys>
inline SegmentedLock<NumKeys>::~SegmentedLock()
{
}

template <UReg NumKeys>
inline void SegmentedLock<NumKeys>::Lock(UReg key)
{
	KeyState &state = m_keyState[key];
	if (++state.lockCount == 1) // first one to lock.
	{
		m_m.lock();
		LockOtherKeys(key);
		state.mal.lock_shared();
		m_m.unlock();
		while (state.unlockCount > 0)
		{
			thread::Sleep();
		}
		state.lockGate.Open();
	}
	else
	{
		state.lockGate.Wait();
	}
}

template <UReg NumKeys>
inline void SegmentedLock<NumKeys>::Unlock(UReg key)
{
	KeyState &state = m_keyState[key];
	RAD_ASSERT(state.lockCount > 0);
	++state.unlockCount;
	if (--state.lockCount == 0)
	{
		state.lockGate.Close();
		state.mal.unlock_shared();
		UnlockOtherKeys(key);
	}
	--state.unlockCount;
}

template <UReg NumKeys>
inline void SegmentedLock<NumKeys>::LockOtherKeys(UReg key)
{
	for (UReg i = 0; i < NumKeys; ++i)
	{
		if (i != key)
		{
			m_keyState[i].mal.lock();
		}
	}
}

template <UReg NumKeys>
inline void SegmentedLock<NumKeys>::UnlockOtherKeys(UReg key)
{
	for (UReg i = 0; i < NumKeys; ++i)
	{
		if (i != key)
		{
			m_keyState[i].mal.unlock();
		}
	}
}

} // thread

#include "../PopSystemMacros.h"
