// PosixThread.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../Thread.h"
#include "../Utils.h"
#include "../Base/CPUCount.h"
#include <execinfo.h>
#include <cxxabi.h>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <sched.h>

namespace thread {
namespace details {

#if defined(RAD_OPT_PTHREAD_NO_SPINLOCK)
InterlockedBase::Mutex InterlockedBase::s_mt;

InterlockedBase::Mutex::Mutex()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
	RAD_VERIFY(pthread_mutex_init(&m_mutex, &attr) == 0);
	pthread_mutexattr_destroy(&attr);
}

InterlockedBase::Mutex::~Mutex()
{
	pthread_mutex_destroy(&m_mutex);
}

#else

InterlockedBase::Spin InterlockedBase::s_spin;

InterlockedBase::Spin::Spin()
{
	RAD_VERIFY(pthread_spin_init(&m_spin, PTHREAD_PROCESS_PRIVATE) == 0);
}

InterlockedBase::Spin::~Spin()
{
	pthread_spin_destroy(&m_spin);
}
#endif

class ThreadContext : public thread::IThreadContext
{
public:
	typedef boost::shared_ptr<ThreadContext> Ref;
	ThreadContext(int i) : m_id(i) {}
	RAD_DECLARE_GET(id, UReg) { return m_id; }
	int m_id;
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
	int numCPUs = (int)sysconf(_SC_NPROCESSORS_CONF);

#if !defined(RAD_OPT_APPLE)
	{
		cpu_set_t cpus;
		CPU_ZERO(&cpus);

		for (int i = 0; i < numCPUs; ++i)
		{
			CPU_SET(i, &cpus);
		}

		if (sched_setaffinity(0, sizeof(cpus), &cpus) == 0)
		{
			sleep(0); // switch
		}
	}
#endif

	unsigned int logical;
	unsigned int cores;
	unsigned int physical;

	unsigned char code = CPUCount(&logical, &cores, &physical);
	s_hyperThreading = code >= HYPERTHREADING_ENABLED;

	physical *= cores;

	// default all processors affinity.
	s_processors.push_back(ThreadContext::Ref(new (ZRuntime) ThreadContext(-1)));

	if (code != USER_CONFIG_ISSUE && !s_hyperThreading)
	{
		numCPUs = (int)physical;
	}

	for (int i = 0; i < numCPUs; ++i)
	{
		ThreadContext::Ref r(new (ZRuntime) ThreadContext(i));
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
	return pthread_self();
}

RADRT_API bool RADRT_CALL SetProcessPriority(PriorityClass p)
{
	RAD_ASSERT(p >= PriorityLow && p <= PriorityHigh);
	if (ProcessPriority() != p)
	{
		int map[PriorityHigh+1] =
		{
			5,
			0,
			-5,
			-10
		};

		return setpriority(PRIO_PROCESS, 0, map[p]);
	}

	return true;
}

RADRT_API PriorityClass RADRT_CALL ProcessPriority()
{
	int z = getpriority(PRIO_PROCESS, 0);
	if (z >= 5) return PriorityLow;
	if (z >= 0) return PriorityNormal;
	if (z <= -10) return PriorityHigh;
	return PriorityMedium;
}

namespace
{
	int CalcPriority(int policy, PriorityClass p)
	{
		int min = sched_get_priority_min(policy);
		int max = sched_get_priority_max(policy);
		int mid = (max+min)/2;

		switch (p)
		{
		case PriorityLow:
			return min;
		case PriorityNormal:
			return mid;
		case PriorityMedium:
			return (mid+max)/2;
		default:
			break;
		}

		return max;
	}

	bool SetThreadPriority(const pthread_t &tid, PriorityClass p)
	{
		int policy;
		struct sched_param parm;
		pthread_getschedparam(tid, &policy, &parm);
		parm.sched_priority = CalcPriority(policy, p);
		return pthread_setschedparam(tid,  policy, &parm) == 0;
	}

	PriorityClass ThreadPriority(const pthread_t &tid)
	{
		int policy;
		sched_param parm;

		if (pthread_getschedparam(tid, &policy, &parm) != 0)
		{
			RAD_FAIL("pthread_getschedparam() failed!");
			return PriorityNormal; // not sure what to do here.
		}

		int min = sched_get_priority_min(policy);
		int max = sched_get_priority_max(policy);
		int mid = (max+min)/2;

		if (parm.sched_priority <= min) return PriorityLow;
		if (parm.sched_priority <= mid) return PriorityNormal;
		if (parm.sched_priority <  max) return PriorityMedium;

		return PriorityHigh;
	}
}

RADRT_API bool RADRT_CALL SetThreadPriority(PriorityClass p)
{
	return SetThreadPriority(pthread_self(), p);
}

RADRT_API PriorityClass RADRT_CALL ThreadPriority()
{
	return ThreadPriority(pthread_self());
}

namespace details {

#if defined(RAD_OPT_FIBERS)

Fiber::Fiber()
{
#if defined(RAD_OPT_SETCTX)
	m_stack = 0;
	m_ctx.uc_stack.ss_sp = 0;
#endif
}

Fiber::Fiber(AddrSize stackSize)
{
	Create(stackSize);
}

Fiber::~Fiber()
{
	Destroy();
}

void Fiber::Create(AddrSize stackSize)
{
#if defined(RAD_OPT_SETCTX)
	::getcontext(&m_ctx);
	m_ctx.uc_link = 0;
	m_stack = zone_malloc(ZRuntime, stackSize, 0);
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = stackSize;
	m_ctx.uc_stack.ss_flags = 0;
	sigfillset(&m_ctx.uc_sigmask);
	::makecontext(&m_ctx, (void(*)())FiberProc, 1, this);
#endif
}

void Fiber::Destroy()
{
#if defined(RAD_OPT_SETCTX)
	if (m_stack)
	{
		::zone_free(m_stack);
		m_stack = 0;
	}
#endif
}

void Fiber::SwitchToFiber(thread::Fiber &fiber)
{
#if defined(RAD_OPT_SETCTX)
	RAD_ASSERT_MSG(m_ctx.uc_stack.ss_sp, "Fiber not created!");
	RAD_VERIFY(::swapcontext(&m_ctx, &fiber.m_imp.m_ctx)==0);
#else
	if (setjmp(m_ctx)==0)
		longjmp(fiber.m_imp.m_ctx, 1);
#endif
}

void Fiber::SwitchToThread(thread::Thread &thread)
{
#if defined(RAD_OPT_SETCTX)
	RAD_VERIFY(::swapcontext(&m_ctx, &thread.m_imp.m_ctx)==0);
#else
	if (setjmp(m_ctx)==0)
		longjmp(thread.m_imp.m_ctx, 1);
#endif
}

void Fiber::FiberProc(Fiber *self)
{
	thread::Fiber* fiber = RAD_CLASS_FROM_MEMBER(thread::Fiber, m_imp, self);
	fiber->FiberProc();
	fiber->SwitchToThread();
}
	
#if defined(RAD_OPT_SETJMP)
void Fiber::JmpRun()
{
	FiberProc(this);
}
#endif

#endif
	
Thread::Thread(AddrSize stackSize) :
m_stackSize(stackSize),
m_priorityClass(PriorityNormal),
m_exited(true),
#if defined(RAD_OPT_FIBERS)
m_fiber(0),
#endif
m_context(0),
m_valid(false)
{
	m_exitGate.Open();
}

Thread::~Thread()
{
	Destroy();
}

void Thread::Destroy()
{
	if (m_valid)
	{
		Join(Infinite);
		pthread_detach(m_thread);
		m_valid = false;
	}
}

bool Thread::Run(thread::IThreadContext *context)
{
	if (m_exited && m_valid)
	{
		Destroy(); // cleanup old thread.
	}

	pthread_attr_t attr;
	sched_param parm;
	int policy;

	pthread_attr_init(&attr);
	pthread_attr_getschedparam(&attr, &parm);
	pthread_attr_getschedpolicy(&attr, &policy);
	parm.sched_priority = CalcPriority(policy, m_priorityClass);

	RAD_VERIFY(pthread_attr_setschedparam(&attr, &parm) == 0);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setstacksize(&attr, m_stackSize > PTHREAD_STACK_MIN ? m_stackSize : PTHREAD_STACK_MIN);

	m_exitGate.Close();
	m_exited  = false;
	m_context = context;
	m_valid = pthread_create(&m_thread, &attr, ThreadProc, this) == 0;
	pthread_attr_destroy(&attr);
	return m_valid;
}

bool Thread::SetPriority(PriorityClass p)
{
	RAD_ASSERT(m_valid);
	bool r = SetThreadPriority(m_thread, p);
	m_priorityClass = (r) ? p : m_priorityClass;
	return r;
}

bool Thread::Join(U32 maxWaitTime)
{
	if (!m_valid)
		return true;
	
	bool join = true;

	if (maxWaitTime != Infinite)
	{
		join = m_exitGate.Wait(maxWaitTime);
	}

	if (join)
	{
		join = pthread_join(m_thread, 0) == 0;
	}

	return join;
}

#if defined(RAD_OPT_FIBERS)
void Thread::SwitchToFiber(thread::details::Fiber &fiber)
{
	RAD_ASSERT(m_valid);
#if defined(RAD_OPT_SETCTX)
	RAD_VERIFY(::swapcontext(&m_ctx, &fiber.m_ctx)==0);
#else
	if (setjmp(m_ctx)==0)
		fiber.JmpRun();
#endif
}
#endif

void *Thread::ThreadProc(void *arg)
{
	RAD_ASSERT(arg);
	Thread *self = reinterpret_cast<thread::details::Thread*>(arg);
	s_curContext.reset(self->m_context);
	thread::Thread *thread = RAD_CLASS_FROM_MEMBER(thread::Thread, m_imp, self);

	sigset_t sigset;
	sigfillset(&sigset);
	pthread_sigmask(SIG_BLOCK, &sigset, 0); // disable all signal interruption by default

	self->m_retCode = (int)thread->ThreadProc();
	self->m_exited = true;
	self->m_exitGate.Open();

	return (void*)self->m_retCode;
}

} // details
} // thread
