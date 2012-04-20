// Locks.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"
#include "../Thread/Interlocked.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/locks.hpp>

namespace thread {

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Gate : private boost::noncopyable
{
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
	bool Wait(UReg timeout = Infinite) const;

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
	typedef boost::lock_guard<MutexType> Lock;

	mutable boost::condition m_sc; // state condition
	mutable MutexType m_x;

	mutable volatile bool m_pulse;
	mutable volatile bool m_open;
	bool m_single;
	mutable Interlocked<int> m_waiting;
	mutable Interlocked<int> m_waitingBeforePulse;
};

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Semaphore
{
public:
	Semaphore(int count = 0);

	void Put();
	int Get(UReg timeout = Infinite, bool clear=false); // returns count obtained (useful on clear).
	int Reset(); // returns count at time of reset.

private:

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;
	Gate m_gate;
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
class SegmentedLock : private boost::noncopyable
{
public:

	SegmentedLock();
	virtual ~SegmentedLock();

	void Lock(UReg key);
	void Unlock(UReg key);

private:

	typedef boost::mutex Mutex;

	friend struct KeyState;
	struct KeyState
	{
		KeyState() : lockCount(0), unlockCount(0), lockGate(false, false)
		{
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

///////////////////////////////////////////////////////////////////////////////
// boost doesn't supply a lock concept that goes from a read->write lock
// although you can do this with the naked mutex.
// Note: going from shared_lock, to upgrade_lock, to upgrade_to_unique_lock
// will hang because upgrade_to_unique_lock will wait for shared_locks to go
// away.
///////////////////////////////////////////////////////////////////////////////

template <class Mutex>
class upgrade_to_exclusive_lock
{
private:
    boost::shared_lock<Mutex>* source;
    explicit upgrade_to_exclusive_lock(upgrade_to_exclusive_lock&);
    upgrade_to_exclusive_lock& operator=(upgrade_to_exclusive_lock&);
public:
    explicit upgrade_to_exclusive_lock(boost::shared_lock<Mutex>& m_):
        source(&m_)
    {
		source->mutex()->lock_upgrade();
		source->mutex()->unlock_shared();
		source->mutex()->unlock_upgrade_and_lock();
	}
    ~upgrade_to_exclusive_lock()
    {
        if(source)
        {
            source->mutex()->unlock_and_lock_shared();
        }
    }

    upgrade_to_exclusive_lock(boost::detail::thread_move_t<upgrade_to_exclusive_lock<Mutex> > other):
        source(other->source)
    {
        other->source=0;
    }
    
    upgrade_to_exclusive_lock& operator=(boost::detail::thread_move_t<upgrade_to_exclusive_lock<Mutex> > other)
    {
        upgrade_to_exclusive_lock temp(other);
        swap(temp);
        return *this;
    }
    void swap(upgrade_to_exclusive_lock& other)
    {
        std::swap(source,other.source);
    }
    typedef void (upgrade_to_exclusive_lock::*bool_type)(upgrade_to_exclusive_lock&);
    operator bool_type() const
    {
        return source?&upgrade_to_exclusive_lock::swap:0;
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

