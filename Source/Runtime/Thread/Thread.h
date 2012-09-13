// Thread.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"
#include "../TimeDef.h"
#include "../PushPack.h"


namespace thread {
namespace details {

class ThreadContext;

} // details

class RADRT_CLASS IThreadContext
{
public:
	RAD_DECLARE_READONLY_PROPERTY(IThreadContext, id, UReg);
private:
	virtual RAD_DECLARE_GET(id, UReg) = 0;
	IThreadContext() {}
	virtual ~IThreadContext() {}
	IThreadContext & operator = (const IThreadContext&) { return *this; }
	friend class details::ThreadContext;
};

} // thread


#include "../PopPack.h"
#include "ThreadDef.h"
#include "../PushPack.h"

namespace thread {

RADRT_API bool RADRT_CALL SetProcessPriority(PriorityClass p);
RADRT_API PriorityClass RADRT_CALL ProcessPriority();
RADRT_API bool RADRT_CALL SetThreadPriority(PriorityClass p);
RADRT_API PriorityClass RADRT_CALL ThreadPriority();
RADRT_API UReg RADRT_CALL NumContexts();
RADRT_API IThreadContext * RADRT_CALL Context(UReg num = CurrentContext);
RADRT_API Id ThreadId();
void Sleep(xtime::TimeVal millis = TimeSlice);
void Yield();

} // thread

#include "../PopPack.h"
#include "Backend.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>
#include "../PushPack.h"


namespace thread {

#if defined(RAD_OPT_FIBERS)

class RADRT_CLASS Fiber : private boost::noncopyable
{
public:

	Fiber();
	Fiber(AddrSize stackSize);
	virtual ~Fiber();

	void Create(AddrSize stackSize = Default);

	void Destroy();

	void SwitchToFiber(Fiber &fiber);
	void SwitchToThread();

protected:

	virtual void FiberProc() = 0;

	RAD_DECLARE_READONLY_PROPERTY(Fiber, thread, Thread*);

private:

	RAD_DECLARE_GET(thread, Thread*);

	friend class Thread;
	friend class details::Thread;
	friend class details::Fiber;
	mutable details::Fiber m_imp;
	Thread * volatile m_thread;
};

#endif

class RADRT_CLASS Thread : private boost::noncopyable
{
public:

	explicit Thread(AddrSize stackSize = Default);
	virtual ~Thread();

	void Create(AddrSize stackSize = Default);
	bool Run(IThreadContext *context = 0); // null is default
	bool Join(U32 maxWaitTime = Infinite) const;

	RAD_DECLARE_READONLY_PROPERTY(Thread, id, Id);
	RAD_DECLARE_READONLY_PROPERTY(Thread, returnCode, int);
	RAD_DECLARE_READONLY_PROPERTY(Thread, exited, bool);
	RAD_DECLARE_PROPERTY(Thread, priority, PriorityClass, PriorityClass);

#if defined(RAD_OPT_FIBERS)
	void SwitchToFiber(Fiber &fiber);
#endif

protected:

	virtual int ThreadProc() = 0;

private:

	RAD_DECLARE_GET(id, Id);
	RAD_DECLARE_GET(returnCode, int);
	RAD_DECLARE_GET(exited, bool);
	RAD_DECLARE_GETSET(priority, PriorityClass, PriorityClass);

	friend class details::Thread;
#if defined(RAD_OPT_FIBERS)
	friend class details::Fiber;
#endif
	mutable details::Thread m_imp;
};

} // thread


#include "../PopPack.h"
#include "Thread.inl"
