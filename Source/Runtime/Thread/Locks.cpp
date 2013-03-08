// Locks.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Locks.h"
#include "../Time.h"

using xtime::duration;


namespace thread {

///////////////////////////////////////////////////////////////////////////////

void EventMutex::NotifyOne() {
	Lock L(m_m);
	if (m_waiting < 1)
		return;
	--m_waiting;
	++m_ready;
	m_c.notify_one();
}

void EventMutex::NotifyAll() {
	Lock L(m_m);
	if (m_waiting < 1 )
		return;
	m_ready += m_waiting;
	m_waiting = 0;
	m_c.notify_all();
}

///////////////////////////////////////////////////////////////////////////////

//
// Wait for gate to be "opened".
//
bool Gate::Wait(U32 timeout) const {
	Lock l(m_x);

	bool r = true;
	++m_waiting;

	while (!m_open) {
		if (timeout != Infinite) {
			if (!m_sc.timed_wait(m_x, duration(timeout)) && !m_pulse) {
				r = false;
				break;
			}
		} else {
			m_sc.wait(m_x);
		}
		
		if (m_pulse)
			break;
	}

	if (m_pulse) {
		if (--m_waitingBeforePulse == 0) {
			m_pulse = false;
		}
	} else {
		--m_waiting;
	}

	return r;
}

//
// Open the gate. If "autoCloseSingleRelease" is true, then one waiting thread will
// be released, and the gate will be closed, blocking any other waiting threads.
//
void Gate::Open() {
	Lock l(m_x);
	if(m_open) 
		return;
	if (m_single) {
		Pulse();
		return;
	}
	m_pulse = false;
	m_waitingBeforePulse = 0;
	m_open = true;
	m_sc.notify_all();
}

//
// Close the gate.
//
void Gate::Close() {
	Lock l(m_x);
	if (!m_open) 
		return;
	m_pulse = false;
	m_open  = false;
}

//
// Pulse the gate.
//
// Open the gate, let threads run (unless autoCloseSingleRelease is set, in which case
// only one thread wakes up).
// Closes the gate.
//
void Gate::Pulse() {
	Lock L(m_x);

	// previous pulse not processed yet?
	if (m_pulse){
		return; // don't do anything.
	} else if (m_open) {
		m_open = false;
		return;
	}

	if (m_waiting > 0) { // threads are waiting.
		m_pulse = true;

		if (m_single) {
			--m_waiting;
			m_waitingBeforePulse = 1;
			m_sc.notify_one();
		} else {
			m_waitingBeforePulse = m_waiting;
			m_waiting = 0;
			m_sc.notify_all();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

SharedMutex::SharedMutex() : 
m_upgrades(0),
m_state(kLockState_Unlocked),
m_lockCount(0) {
	m_numReaders[0] = m_numReaders[1] = 0;
	m_numWriters[0] = m_numWriters[1] = 0;
	RAD_DEBUG_ONLY(m_threadId = 0);
}

SharedMutex::~SharedMutex() {
}

void SharedMutex::ReadLock() { // -- doesn't hold mutex.
	Lock L(m_m);

	// if we got in here there are no active writers
	++m_numReaders[0];

	// pending writers?
	if (m_numWriters[0] > 0) {
		m_readGate.wait(L);
	}

	--m_numReaders[0];
	++m_numReaders[1];

	RAD_ASSERT(m_state != kLockState_Write);
	++m_lockCount;

	m_state = kLockState_Read;
}

void SharedMutex::WriteLock() {
	Lock L(m_m);

	while (m_numReaders[0] > 0) { // pending readers
		m_writeGate.wait(L);
	}

	++m_numWriters[0];

	while (m_numReaders[1] > 0) { // active readers
		m_writeGate.wait(L);
	}

	--m_numWriters[0];
	++m_numWriters[1];

	RAD_ASSERT(m_state == kLockState_Unlocked);
	RAD_ASSERT(m_threadId == 0);
	++m_lockCount;
	RAD_ASSERT(m_lockCount == 1);

	m_state = kLockState_Write;
	RAD_DEBUG_ONLY(m_threadId = thread::ThreadId());
	L.release(); // don't unlock mutex in Writer case
}

void SharedMutex::Unlock() {
	Lock L(m_m);
	// if you assert here you are unlocking on a thread that did not initiate a lock
	// or you are unlocking too many times or doing something bad.
#if defined(RAD_OPT_DEBUG)
	if (m_state == kLockState_Write) {
		// consistency check
		RAD_ASSERT(m_threadId == thread::ThreadId());
		RAD_DEBUG_ONLY(m_threadId = 0);
		RAD_ASSERT(m_lockCount == 1);
	} else {
		RAD_ASSERT(m_lockCount > 0);
	}
#endif
	if ((--m_lockCount) == 0) {
		if (m_state == kLockState_Write) {
			RAD_ASSERT(m_numWriters[1] == 1);
			--m_numWriters[1];
			m_state = kLockState_Unlocked;
			m_m.unlock();
			m_readGate.notify_all();
		}
	}

	if (m_state == kLockState_Read) {
		RAD_ASSERT(m_numReaders > 0);
		--m_numReaders[1];
		if (m_numReaders[1] == 0) {
			if (m_upgrades > 0) {
				m_upgradeGate.notify_all();
			} else {
				m_writeGate.notify_all();
			}
			m_state = kLockState_Unlocked;
		}
	}
}

void SharedMutex::UpgradeToWriteLock() {
	Lock L(m_m);
	
	RAD_ASSERT(m_state == kLockState_Read);
	RAD_ASSERT(m_numReaders[1] > 0);
	--m_numReaders[1];
	
	if (m_numReaders[1] > 0) {
		++m_upgrades;
		while (m_numReaders[1] > 0) {
			// we can't write lock this until all readers are done
			m_upgradeGate.wait(L);
		}
		--m_upgrades;
		RAD_ASSERT(m_state == kLockState_Unlocked);
	}

	++m_numWriters[1];
	m_state = kLockState_Write;
	RAD_DEBUG_ONLY(m_threadId = thread::ThreadId());
	L.release(); // don't release writer lock.
}

void SharedMutex::DowngradeToReadLock() {
	Lock L(m_m);

	RAD_ASSERT(m_state == kLockState_Write);
	RAD_ASSERT(m_numWriters[1] == 1);
	--m_numWriters[1];
	++m_numReaders[1];
	m_state = kLockState_Read;
	RAD_DEBUG_ONLY(m_threadId = 0);
	m_m.unlock(); // unlock outer write lock.
}

///////////////////////////////////////////////////////////////////////////////

Semaphore::Semaphore(int count) :
m_count(count) {
}

void Semaphore::Put() {
	Lock L(m_m);
	++m_count;
	m_gate.notify_all();
}

int Semaphore::Get(U32 timeout, bool clear) {
	xtime::TimeVal start = xtime::ReadMilliseconds();
	int r = 0;

	Lock L(m_m);

	do {
		if (m_count > 0) {
			if (clear) {
				r = m_count;
				m_count = 0;
			} else {
				r = 1;
				--m_count;
			}
			break;
		}

		if (timeout != Infinite) {
			m_gate.timed_wait(m_m, duration(timeout));
		} else {
			m_gate.wait(m_m);
		}

	} while (timeout == Infinite || (xtime::ReadMilliseconds() - start) < timeout);

	return r;
}

int Semaphore::Reset() {
	Lock L(m_m);
	int r = m_count;
	m_count = 0;
	return r;
}

} // thread

