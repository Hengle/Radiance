// PosixInterlocked.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <pthread.h>


namespace thread {
namespace details {

#if defined(RAD_OPT_PTHREAD_NO_SPINLOCK)
class InterlockedBase
{
public:
	
	void Lock() const { pthread_mutex_lock(s_mt.Get()); }
	void Unlock() const { pthread_mutex_unlock(s_mt.Get()); }
	
private:
	
	class Mutex
	{
	public:
		Mutex();
		~Mutex();
		
		pthread_mutex_t *Get() { return &m_mutex; }
		
	private:
		
		pthread_mutex_t m_mutex;
	};
	
	static Mutex s_mt;
	
};
#else
class InterlockedBase
{
public:

	void Lock() const { pthread_spin_lock(s_spin.Get()); }
	void Unlock() const { pthread_spin_unlock(s_spin.Get()); }

private:

	class Spin
	{
	public:
		Spin();
		~Spin();

		pthread_spinlock_t *Get() { return &m_spin; }

	private:

		pthread_spinlock_t m_spin;
	};

	static Spin s_spin;

};
#endif

template<typename T>
class Interlocked : private InterlockedBase
{
public:
	Interlocked() {}
	Interlocked(const Interlocked<T> &s) : m_v((T)s) {}
	Interlocked(const T &val) : m_v(val) {}
	~Interlocked() {}

	inline Interlocked<T> &operator = (const T &val)
	{
		ScopedLock x(*this);
		m_v = val;
		return *this;
	}

	inline operator T () const
	{
		ScopedLock x(*this);
		return m_v;
	}

	inline T operator ++ () // prefix
	{
		ScopedLock x(*this);
		return (T)(++m_v);
	}

	inline T operator ++ (int) // postfix
	{
		ScopedLock x(*this);
		return (T)(m_v++);
	}

	inline T operator -- () // prefix (we cannot return a thread safe reference!)
	{
		ScopedLock x(*this);
		return (T)(--m_v);
	}

	inline T operator -- (int) // postfix
	{
		ScopedLock x(*this);
		return (T)(m_v--);
	}

	inline T operator += (const T& add)
	{
		ScopedLock x(*this);
		return (T)(m_v += add);
	}

	inline T operator -= (const T& sub)
	{
		ScopedLock x(*this);
		return (T)(m_v -= sub);
	}

	inline T operator |= (const T& _or)
	{
		ScopedLock x(*this);
		return (T)(m_v |= _or);
	}

	inline T operator &= (const T& _and)
	{
		ScopedLock x(*this);
		return (T)(m_v &= _and);
	}

	inline T operator ^= (const T& _xor)
	{
		ScopedLock x(*this);
		return (T)(m_v ^= _xor);
	}

private:

	class ScopedLock
	{
	public:
		ScopedLock(const InterlockedBase &b) : z(b) { z.Lock(); }
		~ScopedLock() { z.Unlock(); }
		const InterlockedBase &z;
	};

	volatile T m_v;
};

} // details
} // thread


