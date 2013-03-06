// Locks.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"
#include "../ThreadDef.h"
#include "../Thread/Interlocked.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/locks.hpp>

namespace thread {

///////////////////////////////////////////////////////////////////////////////

//! A mutex with event functionality (wait & notify).
/*! The difference between an EventMutex and a Gate is subtle but important.
    A Gate allows the control of access to a resource based on some condition
	(gate is open or closed) however a Gate does not provide a mechanism to
	serialize access to a resource it may be gate'ing.

	The EventMutex class serves as an Event, like a Gate, but also provides
	mutex style synchronization to a resource.

	Any callers with an EventMutex::Sync object in scope will serialize eachother.
	When an EventMutex::Sync::Wait call returns the callers has exclusive access
	to the EventMutex's resource after the object has been signaled. */
class EventMutex : private boost::noncopyable {
public:
	
	EventMutex();

	class Sync {
	public:

		Sync(EventMutex &m);

		void Wait();

		bool TimedWait(const boost::system_time &wait_until);
		bool TimedWait(const boost::xtime &wait_until);

		template <typename duration_type>
		bool TimedWait(const duration_type &wait_duration);
	
	private:

		typedef boost::unique_lock<boost::mutex> Lock;

		void Inc();
		bool Ready();

		Lock L;
		EventMutex &m_x;
		boost::condition_variable &m_c;
		boost::mutex &m_m;
	};

	// Do not call the notify functions while you have a Sync object you will deadlock.

	void NotifyOne();
	void NotifyAll();

private:
	
	typedef boost::unique_lock<boost::mutex> Lock;

	void Inc();
	bool Ready();

	friend class Sync;

	volatile int m_waiting;
	volatile int m_ready;

	boost::mutex m_m;
	boost::condition_variable m_c;
};

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Gate : private boost::noncopyable {
public:

	Gate(bool initiallyOpen = false, bool autoCloseSingleRelease = false)
	: m_pulse(false),
	  m_open(initiallyOpen),
	  m_single(autoCloseSingleRelease),
	  m_waiting(0),
	  m_waitingBeforePulse(0)
	{}

	~Gate() {}

	//
	// Wait for gate to be "opened".
	//
	bool Wait(U32 timeout = Infinite) const;

	//
	// Open the gate. If "autoCloseSingleRelease" is true, then one waiting thread will
	// be released, and the gate will be closed, blocking any other waiting threads.
	//
	void Open();

	//
	// Close the gate.
	//
	void Close();

	//
	// Pulse the gate.
	//
	// If the gate is open this does nothing.
	// Cylces the gate and releases any waiting threads (opens the gate, wakes up the thread
	// and closes the gate). If "autoCloseSingleRelease" is true then only one thread
	// is released.
	//
	void Pulse();

private:

	typedef boost::mutex MutexType;
	typedef boost::unique_lock<MutexType> Lock;

	mutable boost::condition m_sc; // state condition
	mutable MutexType m_x;

	mutable volatile bool m_pulse;
	mutable volatile bool m_open;
	bool m_single;
	mutable Interlocked<int> m_waiting;
	mutable Interlocked<int> m_waitingBeforePulse;
};

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Semaphore {
public:
	Semaphore(int count = 0);

	void Put();
	int Get(U32 timeout = Infinite, bool clear=false); // returns count obtained (useful on clear).
	int Reset(); // returns count at time of reset.

private:

	typedef boost::mutex Mutex;
	typedef boost::unique_lock<Mutex> Lock;
	boost::condition m_gate;
	Mutex m_m;
	int m_count;
};

///////////////////////////////////////////////////////////////////////////////

// This lock performs a similar function to boost::shared_mutex (which is a reader/writer lock).
// This lock contains a shared_mutex per key, and replicates the lock state across the mutexes:
//
// // K1 = Key1, K2 = Key2, RL = Read Lock, WL = Write Lock
// ------------------------------------------------
// -- Action  -- State of Key 1 -- State of Key 2 --
// -- K1 Lock --      RL        --       WL       --
// -- K2 Lock --      WL        --       RL       --
// ------------------------------------------------

template <UReg NumKeys>
class SegmentedLock : private boost::noncopyable {
public:

	SegmentedLock();
	virtual ~SegmentedLock();

	void Lock(UReg key);
	void Unlock(UReg key);

private:

	typedef boost::mutex Mutex;

	friend struct KeyState;
	struct KeyState {
		KeyState() : lockCount(0), unlockCount(0), lockGate(false, false) {
		}

		Interlocked<UReg> lockCount;
		Interlocked<UReg> unlockCount;
		Gate              lockGate;
		boost::shared_mutex mal;
	};

	Mutex m_m;
	boost::array<KeyState, NumKeys> m_keyState;
	void LockOtherKeys(UReg key);
	void UnlockOtherKeys(UReg key);
};

//! A mutex that can be upgraded from a read -> write -> read
/*! Boost has some weird restrictions on how shared_mutex can be upgraded to a write lock
 ** (i.e. only a single thread can upgrade to write, if you ever have 2 or more threads
     trying to upgrade to write it will deadlock).

	 This class is heavier than a boost::shared_mutex but does reliable read->write->read upgrading.
*/
class SharedMutex {
public:

	SharedMutex();
	~SharedMutex();

	void ReadLock();
	void WriteLock();
	void Unlock();

	void UpgradeToWriteLock();
	void DowngradeToReadLock();

	// boost lock concept support:

	void lock() {
		WriteLock();
	}

	void lock_shared() {
		ReadLock();
	}

	void unlock() {
		Unlock();
	}

	void unlock_shared() {
		Unlock();
	}

private:

	typedef boost::recursive_mutex Mutex;
	typedef boost::unique_lock<Mutex> Lock;

	enum LockState {
		kLockState_Unlocked,
		kLockState_Read,
		kLockState_Write
	};

	boost::condition m_readGate;
	boost::condition m_writeGate;
	boost::condition m_upgradeGate;
	Mutex m_m;
	volatile int m_numReaders[2];
	volatile int m_numWriters[2];
	volatile int m_upgrades;
	volatile int m_lockCount;
	volatile LockState m_state;

#if defined(RAD_OPT_DEBUG)
	volatile Id m_threadId;
#endif
};

//! SharedMutex upgrade to write lock helper
class UpgradeToWriteLock {
public:
	UpgradeToWriteLock(SharedMutex &m) : m_m(m) {
		m.UpgradeToWriteLock();
	}
	
	~UpgradeToWriteLock() {
		m_m.DowngradeToReadLock();
	}

private:
	SharedMutex &m_m;
};

/*! Boost doesn't supply a lock concept that goes from a read->write lock that works
 ** without exception. Boost only supports a single thread upgrading to write.
 ** This class avoids this issue by simply unlocking the read and locking for write
 ** and not going through the upgrade path at all.
 **
 ** This is, however, unsafe in the sense that in between these operations a read or write
 ** thread could consume or change data. If you require fully safe access, use the SharedMutex
 ** class above.
*/

template <class Mutex>
class unsafe_upgrade_to_exclusive_lock
{
private:
    boost::shared_lock<Mutex>* source;
    explicit unsafe_upgrade_to_exclusive_lock(unsafe_upgrade_to_exclusive_lock&);
    unsafe_upgrade_to_exclusive_lock& operator=(unsafe_upgrade_to_exclusive_lock&);
public:
    explicit unsafe_upgrade_to_exclusive_lock(boost::shared_lock<Mutex>& m_):
        source(&m_)
    {
		source->mutex()->unlock_shared();
		source->mutex()->lock();
	}
    ~unsafe_upgrade_to_exclusive_lock() {
        if(source) {
            source->mutex()->unlock_and_lock_shared();
        }
    }

    unsafe_upgrade_to_exclusive_lock(boost::detail::thread_move_t<unsafe_upgrade_to_exclusive_lock<Mutex> > other):
        source(other->source)
    {
        other->source=0;
    }
    
    unsafe_upgrade_to_exclusive_lock& operator=(boost::detail::thread_move_t<unsafe_upgrade_to_exclusive_lock<Mutex> > other)
    {
        unsafe_upgrade_to_exclusive_lock temp(other);
        swap(temp);
        return *this;
    }
    void swap(unsafe_upgrade_to_exclusive_lock& other)
    {
        std::swap(source,other.source);
    }
    typedef void (unsafe_upgrade_to_exclusive_lock::*bool_type)(unsafe_upgrade_to_exclusive_lock&);
    operator bool_type() const
    {
        return source?&unsafe_upgrade_to_exclusive_lock::swap:0;
    }
    bool operator!() const
    {
        return !owns_lock();
    }
    bool owns_lock() const
    {
        return source!=0;
    }
};

} // thread

#include "Locks.inl"

