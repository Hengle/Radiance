// Locks.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Thread.h"
#include "../PushSystemMacros.h"

namespace thread {

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
