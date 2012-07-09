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
bool Gate::Wait(U32 timeout) const
{
	Lock l(m_x);

	bool r = true;
	++m_waiting;

	while (!m_open)
	{
		if (timeout != Infinite)
		{
			if (!m_sc.timed_wait(m_x, duration(timeout)) && !m_pulse)
			{
				r = false;
				break;
			}
		}
		else
		{
			m_sc.wait(m_x);
		}
		
		if (m_pulse)
			break;
	}

	if (m_pulse)
	{
		if (--m_waitingBeforePulse == 0)
		{
			m_pulse = false;
		}
	}
	else
	{
		--m_waiting;
	}

	return r;
}

//
// Open the gate. If "autoCloseSingleRelease" is true, then one waiting thread will
// be released, and the gate will be closed, blocking any other waiting threads.
//
void Gate::Open()
{
	Lock l(m_x);
	if(m_open) 
		return;
	if (m_single)
	{
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
void Gate::Close()
{
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
void Gate::Pulse()
{
	Lock l(m_x);

	// previous pulse not processed yet?
	if (m_pulse)
	{
		return; // don't do anything.
	}
	else if (m_open)
	{
		m_open = false;
		return;
	}

	if (m_waiting > 0) // threads are waiting.
	{
		m_pulse = true;

		if (m_single)
		{
			--m_waiting;
			m_waitingBeforePulse = 1;
			m_sc.notify_one();
		}
		else
		{
			m_waitingBeforePulse = m_waiting;
			m_waiting = 0;
			m_sc.notify_all();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

Semaphore::Semaphore(int count) :
m_gate(count > 0),
m_count(count)
{
}

void Semaphore::Put()
{
	Lock l(m_m);
	++m_count;
	m_gate.Open();
}

int Semaphore::Get(U32 timeout, bool clear)
{
	xtime::TimeVal start = xtime::ReadMilliseconds();
	int r = 0;

	while (timeout == Infinite || (xtime::ReadMilliseconds() - start) < timeout)
	{
		m_gate.Wait(timeout); // force thread round robin
		Lock l(m_m);
		if (m_count > 0)
		{
			if (clear)
			{
				r = m_count;
				m_count = 0;
			}
			else
			{
				r = 1;
				--m_count;
			}
			
			if (m_count < 1)
				m_gate.Close();
			break;
		}
	}

	return r;
}

int Semaphore::Reset()
{
	Lock l(m_m);
	m_gate.Close();
	
	int r = m_count;
	m_count = 0;
	return r;
}

} // thread

