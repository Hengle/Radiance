// Thread.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Time.h"
#include "../PushSystemMacros.h"


namespace thread {

inline void Sleep(xtime::TimeVal millis)
{
#if defined(RAD_OPT_WINX)
	boost::detail::win32::SleepEx(millis, 1);
#else
	boost::this_thread::sleep(xtime::duration(millis));
#endif
}

inline void Yield()
{
	boost::this_thread::yield();
}

#if defined(RAD_OPT_FIBERS)

inline Fiber::Fiber() : m_thread(0)
{
}

inline Fiber::Fiber(AddrSize stackSize) :
m_thread(0),
m_imp(stackSize != Default ? stackSize : Meg)
{
}

inline void Fiber::Create(AddrSize stackSize)
{
	m_imp.Create(stackSize != Default ? stackSize : Meg);
}

inline void Fiber::Destroy()
{
	m_imp.Destroy();
}

inline Fiber::~Fiber()
{
	RAD_ASSERT(!m_thread);
}

inline void Fiber::SwitchToFiber(Fiber &fiber)
{
	RAD_ASSERT_MSG(!fiber.m_thread, "Fiber is already running on another thread!");
	fiber.m_thread = m_thread;
	m_thread = 0;
	m_imp.SwitchToFiber(fiber);
}

inline void Fiber::SwitchToThread()
{
	RAD_ASSERT(m_thread);
	Thread *thread = m_thread;
	m_thread = 0;
	m_imp.SwitchToThread(*thread);
}

inline Thread *Fiber::RAD_IMPLEMENT_GET(thread)
{
	return m_thread;
}

#endif

inline Thread::Thread(AddrSize stackSize) :
m_imp(stackSize != Default ? stackSize : 4*Meg)
{
}

inline Thread::~Thread()
{
	Join();
}

inline void Thread::Create(AddrSize stackSize)
{
	m_imp.Create(stackSize != Default ? stackSize : 4*Meg);
}

inline bool Thread::Run(IThreadContext *context)
{
	return m_imp.Run(context ? context : Context(CurrentContext));
}

inline Id Thread::RAD_IMPLEMENT_GET(id)
{
	return m_imp.GetId();
}

inline int Thread::RAD_IMPLEMENT_GET(returnCode)
{
	return (int)m_imp.ReturnCode();
}

inline bool Thread::RAD_IMPLEMENT_GET(exited)
{
	return m_imp.HasExited();
}

inline PriorityClass Thread::RAD_IMPLEMENT_GET(priority)
{
	return m_imp.Priority();
}

inline bool Thread::Join(U32 maxWaitTime)
{
	return m_imp.Join(maxWaitTime);
}

#if defined(RAD_OPT_FIBERS)

inline void Thread::SwitchToFiber(Fiber &fiber)
{
	RAD_ASSERT_MSG(!fiber.m_thread, "Fiber is already running on another thread!");
	fiber.m_thread = this;
	m_imp.SwitchToFiber(fiber.m_imp);
}

#endif

} // thread


#include "../PopSystemMacros.h"
