// PosixThread.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if defined(RAD_OPT_FIBERS)
#define RAD_OPT_SETCTX

#if defined(RAD_OPT_SETJMP)
	#include <setjmp.h>
#else
	#include <ucontext.h>
#endif
#endif

#include "../Thread/Locks.h"

namespace thread {
namespace details {

#if defined(RAD_OPT_FIBERS)
class RADRT_CLASS Fiber
{
	friend class thread::Fiber;
	friend class Thread;

	Fiber();
	Fiber(AddrSize stackSize);
	~Fiber();

	void Create(AddrSize stackSize);
	void Destroy();
	void SwitchToFiber(thread::Fiber &fiber);
	void SwitchToThread(thread::Thread &thread);

	static void FiberProc(Fiber *fiber);

#if defined(RAD_OPT_SETJMP)
	jmp_buf m_ctx;
	void JmpRun();
#else
	ucontext_t m_ctx;
	void *m_stack;
#endif
	
};

#endif

class RADRT_CLASS Thread
{
	typedef boost::mutex Mutex;
	friend class thread::Thread;
#if defined(RAD_OPT_FIBERS)
	friend class Fiber;
#endif

	Thread(AddrSize stackSize);
	~Thread();

	void Create(AddrSize stackSize) { m_stackSize = stackSize; }
	bool Run(thread::IThreadContext *context);
	Id   GetId() const { return m_thread; }
	bool SetPriority(PriorityClass p);
	PriorityClass Priority() { return m_priorityClass; }

	UReg ReturnCode() { return m_retCode; }
	bool HasExited() { return m_exited; }
	bool Join(UReg maxWaitTime);
	void Destroy();

#if defined(RAD_OPT_FIBERS)
	void SwitchToFiber(Fiber &fiber);
#endif

	pthread_t         m_thread;
	AddrSize          m_stackSize;
	volatile UReg     m_retCode;
	PriorityClass     m_priorityClass;
	volatile bool     m_exited;
#if defined(RAD_OPT_FIBERS)
	Fiber            *m_fiber;
#endif
	Gate              m_exitGate;
	bool              m_valid;
	thread::IThreadContext *m_context;

#if defined(RAD_OPT_FIBERS)
#if defined(RAD_OPT_SETJMP)
	jmp_buf m_ctx;
#else
	ucontext_t m_ctx;
#endif
#endif

	static void *ThreadProc(void *parm);

};

bool IsHyperThreadingOn();

} // details
} // thread


