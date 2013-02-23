// Runtime.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Runtime.h"
#include "Thread.h"
#include "Base/SIMD.h"

#define DECLARE(_name) namespace _name { namespace details { void Initialize(); void Finalize(); void ThreadInitialize(); void ThreadFinalize(); void ProcessTasks(); } }

//#define TASK_THREAD

DECLARE(thread)

namespace net {
void SocketStart();
} // net

namespace rt {

namespace details
{
#if defined(TASK_THREAD)
	class Tasks : public thread::Thread
	{
	public:

		Tasks() : m_exit(false) 
		{
		}

		~Tasks() {
			if (!m_exit)
				Exit();
		}

		void Exit() 
		{ 
			m_exit = true; 
			SemPut(); 
			Join(); 
		}

		void SemPut()
		{
			m_sem.Put();
		}

	protected:

		virtual int ThreadProc()
		{
			while (!m_exit)
			{
#if defined(RAD_OPT_WIN)
				// This thread is used as windows IO notify thread.
				rt::ProcessTasks();
				thread::Sleep(thread::TimeSlice);
#else
				m_sem.Get(thread::Infinite, true);
				rt::ProcessTasks();
#endif
			}

			return 0;
		}

	private:

		volatile bool m_exit;
		thread::Semaphore m_sem;
	};
#endif
	namespace
	{
		RuntimeFlags s_flags;
#if defined(TASK_THREAD)
		Tasks        s_tasks;
#endif
	};

	void SemPut()
	{
#if defined(TASK_THREAD)
#if !defined(RAD_OPT_WIN)
		s_tasks.SemPut();
#endif
#endif
	}

} // details


RADRT_API void RADRT_CALL Initialize(RuntimeFlags flags, thread::IThreadContext *context)
{
	details::s_flags = flags;

	net::SocketStart();

	SIMDDriver::Select();

	thread::details::Initialize();

#if defined(TASK_THREAD)
	if (flags != RFNoDefaultThreads)
	{
		details::s_tasks.Run(context);
	}
#endif
}

RADRT_API void RADRT_CALL Finalize()
{
#if defined(TASK_THREAD)
	if (details::s_flags != RFNoDefaultThreads)
	{
		details::s_tasks.Exit();
	}
#endif

	thread::details::Finalize();
}

RADRT_API void RADRT_CALL ThreadInitialize()
{
	thread::details::ThreadInitialize();
}

RADRT_API void RADRT_CALL ThreadFinalize()
{
	thread::details::ThreadFinalize();
}

RADRT_API boost::mutex &RADRT_CALL GlobalMutex()
{
	static boost::mutex M;
	return M;
}

} // rt

