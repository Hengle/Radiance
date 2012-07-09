// WinThread.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../Thread.h"
#include "../Runtime.h"
#include "../Base/CPUCount.h"
#include <stdio.h>
#include "WinHeaders.h"
#include "../PushSystemMacros.h"

#include <vector>

namespace thread {
namespace details {

class ThreadContext : public thread::IThreadContext
{
public:
	typedef boost::shared_ptr<ThreadContext> Ref;
	ThreadContext(int i, int mask) : m_id(i), m_mask(mask) {}
	RAD_DECLARE_GET(id, UReg) { return m_id; }
	int m_id;
	int m_mask;
};

namespace {

typedef std::vector<ThreadContext::Ref, zone_allocator<ThreadContext::Ref, ZRuntimeT> > ThreadContextVec;
ThreadContextVec s_processors;
void nullfunc(IThreadContext *) {}
boost::thread_specific_ptr<IThreadContext> s_curContext(nullfunc);

bool s_hyperThreading=false;

} // namespace

bool IsHyperThreadingOn()
{
	return s_hyperThreading;
}

void Initialize()
{
	SetProcessAffinityMask(GetCurrentProcess(), 0xFFFFFFFF);
	SetThreadAffinityMask(GetCurrentThread(), 0xFFFFFFFF);
	Sleep(0);

	unsigned int logical;
	unsigned int cores;
	unsigned int physical;

	unsigned char code = CPUCount(&logical, &cores, &physical);
	s_hyperThreading = code >= HYPERTHREADING_ENABLED;

	physical *= cores;
	
	// default all processors affinity.
	s_processors.push_back(ThreadContext::Ref(new (ZRuntime) ThreadContext(-1, 0xFFFFFFFF)));

	if (code == USER_CONFIG_ISSUE)
	{ // couldn't detect
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		logical = 1;
		physical = si.dwNumberOfProcessors;
	}
	else if (!s_hyperThreading)
	{
		logical  = 1;
	}
	
	int logMask = 0;

	for (int i = 0; i < (int)logical; ++i)
	{
		logMask |= 1<<i;
	}

	for (int i = 0; i < (int)physical; ++i)
	{
		int mask = logMask << (i*logical);
		ThreadContext::Ref r(new (ZRuntime) ThreadContext(i, mask));
		s_processors.push_back(r);
	}

	s_curContext.reset(Context(DefaultContext));
}

void Finalize()
{
}

void ThreadInitialize()
{
}

void ThreadFinalize()
{
}

void ProcessTasks()
{
}

} // details

RADRT_API UReg RADRT_CALL NumContexts()
{
	return (UReg)details::s_processors.size()-1;
}

RADRT_API IThreadContext* RADRT_CALL Context(UReg num)
{
	if (num == CurrentContext)
	{
		return details::s_curContext.get();
	}

	return details::s_processors[num+1].get();
}

RADRT_API Id ThreadId()
{
	return (Id)::GetCurrentThreadId();
}

RADRT_API bool RADRT_CALL SetProcessPriority(PriorityClass p)
{
	RAD_ASSERT(p >= PriorityLow && p <= PriorityHigh);
	if (ProcessPriority() != p)
	{
		DWORD map[PriorityHigh+1] =
		{
			BELOW_NORMAL_PRIORITY_CLASS,
			NORMAL_PRIORITY_CLASS,
			ABOVE_NORMAL_PRIORITY_CLASS,
			HIGH_PRIORITY_CLASS
		};

		return SetPriorityClass(GetCurrentProcess(), map[p])==TRUE;
	}

	return true;
}

RADRT_API PriorityClass RADRT_CALL ProcessPriority()
{
	PriorityClass p = PriorityNormal; // some windows priority classes are mapped to this, and they don't all make sense (like IDLE and REALTIME).

	DWORD winp = GetPriorityClass(GetCurrentProcess());
	switch(winp)
	{
	case BELOW_NORMAL_PRIORITY_CLASS: p = PriorityLow; break;
	//case NORMAL_PRIORITY_CLASS: p = PriorityNormal; break; --- ALREADY SET!
	case ABOVE_NORMAL_PRIORITY_CLASS: p = PriorityMedium; break;
	case HIGH_PRIORITY_CLASS: p = PriorityHigh; break;
	}

	return p;
}

RADRT_API bool RADRT_CALL SetThreadPriority(PriorityClass p)
{
	bool s = true;
	if (ThreadPriority() != p)
	{
		RAD_ASSERT(p >= PriorityLow && p <= PriorityHigh);
		int map[PriorityHigh+1] =
		{
			THREAD_PRIORITY_BELOW_NORMAL,
			THREAD_PRIORITY_NORMAL,
			THREAD_PRIORITY_ABOVE_NORMAL,
			THREAD_PRIORITY_HIGHEST
		};

		s = ::SetThreadPriority(GetCurrentThread(), map[p]) == TRUE;
	}

	return s;
}

RADRT_API PriorityClass RADRT_CALL ThreadPriority()
{
	PriorityClass p = PriorityNormal; // some windows priority classes are mapped to this, and they don't all make sense (like IDLE and REALTIME).

	int winp = GetThreadPriority(GetCurrentThread());
	switch(winp)
	{
	case THREAD_PRIORITY_BELOW_NORMAL: p = PriorityLow; break;
	//case THREAD_PRIORITY_NORMAL: p = PRIORITY_NORMAL; break; --- ALREADY SET!
	case THREAD_PRIORITY_ABOVE_NORMAL: p = PriorityMedium; break;
	case THREAD_PRIORITY_HIGHEST: p = PriorityHigh; break;
	}

	return p;
}

namespace details {

#if defined(RAD_OPT_FIBERS)

//////////////////////////////////////////////////////////////////////////////////////////
// thread::details::Fiber
//////////////////////////////////////////////////////////////////////////////////////////

Fiber::Fiber() : m_fiber(0)
{
}

Fiber::Fiber(AddrSize stackSize) : m_fiber(0)
{
	Create(stackSize);
}

Fiber::~Fiber()
{
	Destroy();
}

void Fiber::Create(AddrSize stackSize)
{
	RAD_ASSERT(!m_fiber);
//#pragma message ("JMR: So MSDN says on XP and 2000SP4 that FIBER_FLAG_FLOAT_SWITCH is not supported, so the question is, does the FP state get saved with fibers automatically? This may cause problems later.")
	m_fiber = CreateFiberEx((SIZE_T)stackSize, (SIZE_T)stackSize, 0, FiberProc, this);
#if defined(RAD_OPT_DEBUG)
	if (!m_fiber)
	{
		DWORD error = GetLastError();
		LPVOID textBuffer;
		DWORD dwCount = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|
			FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&textBuffer, 0, NULL);

		RAD_ASSERT_MSG(m_fiber, (const char*)textBuffer);
	}
#endif
}

void Fiber::Destroy()
{
	if (m_fiber)
	{
		DeleteFiber(m_fiber);
		m_fiber = 0;
	}
}

void Fiber::SwitchToFiber(thread::Fiber &fiber)
{
	::SwitchToFiber(fiber.m_imp.m_fiber);
}

void Fiber::SwitchToThread(thread::Thread &thread)
{
	::SwitchToFiber(thread.m_imp.m_fiber);
}

void Fiber::FiberProc(void *parm)
{
	RAD_ASSERT(parm);
	Fiber *self = reinterpret_cast<thread::details::Fiber*>(parm);
	thread::Fiber* fiber = RAD_CLASS_FROM_MEMBER(thread::Fiber, m_imp, self);
	fiber->FiberProc();
	fiber->SwitchToThread();
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////
// thread::details::Thread
//////////////////////////////////////////////////////////////////////////////////////////

Thread::Thread(AddrSize stackSize) :
m_stackSize(stackSize),
m_thread(0),
m_id(0),
m_exited(true),
#if defined(RAD_OPT_FIBERS)
m_fiber(0),
#endif
m_exitEvent(0),
m_priorityClass(PriorityNormal)
{
	::InitializeCriticalSection(&m_cs);
	m_exitEvent = ::CreateEventA(0, TRUE, TRUE, 0);
	RAD_ASSERT(0 != m_exitEvent);
	::SetEvent(m_exitEvent); // thread state is exited right now.
}

Thread::~Thread()
{
	::CloseHandle(m_exitEvent);
	::DeleteCriticalSection(&m_cs);
}

void Thread::Create(AddrSize stackSize)
{
	m_stackSize = stackSize;
}

#if defined(RAD_OPT_FIBERS)
void Thread::SwitchToFiber(thread::details::Fiber &fiber)
{
	if (!m_fiber)
	{
		m_fiber = ConvertThreadToFiber(0);
		RAD_ASSERT(m_fiber);
	}
	::SwitchToFiber(fiber.m_fiber);
}
#endif

bool Thread::Run(thread::IThreadContext *context)
{
	if (!context)
	{
		context = Context(DefaultContext);
	}

	::EnterCriticalSection(&m_cs);
	if (m_thread)
	{
		::LeaveCriticalSection(&m_cs);
		return true;
	}
	else
	{
		m_exited = false;
		::ResetEvent(m_exitEvent);
		m_context = context;
		m_thread = ::CreateThread(0, m_stackSize, Thread::ThreadProc, this, 0, (DWORD*)&m_id);
		if (m_thread) { SetPriority(m_priorityClass); }
	}
	bool s = m_thread != 0;
	::LeaveCriticalSection(&m_cs);

	return s;
}

bool Thread::SetPriority(PriorityClass p)
{
	bool s = true;
	if (Priority() != m_priorityClass)
	{
		RAD_ASSERT(p >= PriorityLow && p <= PriorityHigh);
		DWORD map[PriorityHigh+1] =
		{
			THREAD_PRIORITY_BELOW_NORMAL,
			THREAD_PRIORITY_NORMAL,
			THREAD_PRIORITY_ABOVE_NORMAL,
			THREAD_PRIORITY_HIGHEST
		};

		::EnterCriticalSection(&m_cs);
		m_priorityClass = p;
		if (m_thread)
		{
			s = ::SetThreadPriority(m_thread, map[p]) == TRUE;
		}
		::LeaveCriticalSection(&m_cs);
	}

	return s;
}

PriorityClass Thread::Priority()
{
	PriorityClass p = PriorityNormal; // some windows priority classes are mapped to this, and they don't all make sense (like IDLE and REALTIME).
	::EnterCriticalSection(&m_cs);
	if (m_thread)
	{
		DWORD winp = GetThreadPriority(m_thread);
		switch(winp)
		{
		case THREAD_PRIORITY_BELOW_NORMAL: p = PriorityLow; break;
		//case THREAD_PRIORITY_NORMAL: p = PriorityNormal; break; --- ALREADY SET!
		case THREAD_PRIORITY_ABOVE_NORMAL: p = PriorityMedium; break;
		case THREAD_PRIORITY_HIGHEST: p = PriorityHigh; break;
		}
	}
	::LeaveCriticalSection(&m_cs);
	return p;
}

bool Thread::Join(U32 maxWaitTime)
{
	// i don't think it's necessary to wrap this in a crit.
	return (::WaitForSingleObject(m_exitEvent, maxWaitTime) == WAIT_OBJECT_0) ? true : false;
}

DWORD WINAPI Thread::ThreadProc(void* parm)
{
	RAD_ASSERT(parm);
	rt::ThreadInitialize();

	Thread *self = reinterpret_cast<thread::details::Thread*>(parm);
	s_curContext.reset(self->m_context);
	thread::Thread* thread = RAD_CLASS_FROM_MEMBER(thread::Thread, m_imp, self);
	SetThreadAffinityMask(thread->m_imp.m_thread, static_cast<ThreadContext*>(self->m_context)->m_mask);

	int ret = thread->ThreadProc();

	::EnterCriticalSection(&thread->m_imp.m_cs);
	thread->m_imp.m_retCode = ret;
#if defined(RAD_OPT_FIBERS)
	if (thread->m_imp.m_fiber)
	{
		ConvertFiberToThread();
		thread->m_imp.m_fiber = 0;
	}
#endif
	CloseHandle(thread->m_imp.m_thread);
	thread->m_imp.m_thread = 0;
	::LeaveCriticalSection(&thread->m_imp.m_cs);
	thread->m_imp.m_exited = true;
	::SetEvent(thread->m_imp.m_exitEvent);

	rt::ThreadFinalize();

	return (DWORD)ret;
}

} // details
} // thread

