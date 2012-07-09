// WinThread.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "WinHeaders.h"


namespace thread {
namespace details {

template <typename T>
class AlignedHelper
{
	AlignedHelper();
	AlignedHelper(const T v);
};

template <>
class AlignedHelper<S32>
{
protected:
	AlignedHelper<S32>() {}
	AlignedHelper<S32>(const S32 &v) : m_val(v) {}
	AlignedHelper<S32>(const AlignedHelper<S32> &s) : m_val(s.m_val) {}
	__declspec(align(4)) volatile S32 m_val;
};

template <>
class AlignedHelper<U32>
{
protected:
	AlignedHelper<U32>() {}
	AlignedHelper<U32>(const U32 &v) : m_val(v) {}
	AlignedHelper<U32>(const AlignedHelper<U32> &s) : m_val(s.m_val) {}
	__declspec(align(4)) volatile U32 m_val;
};

#if RAD_OPT_MACHINE_WORD_SIZE == 8

template <>
class AlignedHelper<S64>
{
protected:
	AlignedHelper<S64>() {}
	AlignedHelper<S64>(const S64 &v) : m_val(v) {}
	AlignedHelper<S64>(const AlignedHelper<S64> &s) : m_val(s.m_val) {}
	__declspec(align(8)) volatile S64 m_val;
};

template <>
class AlignedHelper<U64>
{
protected:
	AlignedHelper<U64>() {}
	AlignedHelper<U64>(const U64 &v) : m_val(v) {}
	AlignedHelper<U64>(const AlignedHelper<U64> &s) : m_val(s.m_val) {}
	__declspec(align(8)) volatile U64 m_val;
};

#endif

template<typename T>
class Interlocked :
public AlignedHelper<T>
{
public:
	Interlocked();
	Interlocked(const Interlocked<T> &s);
	Interlocked(const T &val);
	~Interlocked();

	Interlocked<T> &operator = (const T &val);

	operator T () const;
	T operator ++ (); // prefix (we cannot return a thread safe reference!)
	T operator ++ (int); // postfix
	T operator -- (); // prefix (we cannot return a thread safe reference!)
	T operator -- (int); // postfix
	T operator += (const T& add);
	T operator -= (const T& sub);
	T operator |= (const T& or);
	T operator &= (const T& and);
	T operator ^= (const T& xor);
};

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

	static void CALLBACK FiberProc(void *parm);

	LPVOID m_fiber;
};
#endif

class RADRT_CLASS Thread
{
	friend class thread::Thread;
#if defined(RAD_OPT_FIBERS)
	friend class Fiber;
#endif

	Thread(AddrSize stackSize);
	~Thread();

	void Create(AddrSize stackSize);
	bool Run(thread::IThreadContext *context);
	Id   GetId() const { return m_id; }
	bool SetPriority(PriorityClass p);
	PriorityClass Priority();

	UReg ReturnCode() { return m_retCode; }
	bool HasExited() { return m_exited; }
	bool Join(U32 maxWaitTime);
#if defined(RAD_OPT_FIBERS)
	void SwitchToFiber(Fiber &fiber);
#endif

	AddrSize          m_stackSize;
	HANDLE            m_thread;
	volatile UReg     m_retCode;
	PriorityClass     m_priorityClass;
	volatile bool     m_exited;
#if defined(RAD_OPT_FIBERS)
	LPVOID            m_fiber;
#endif
	DWORD             m_id;
	CRITICAL_SECTION  m_cs;
	HANDLE            m_exitEvent;
	thread::IThreadContext *m_context;

	static DWORD WINAPI ThreadProc(void* parm);

	Thread(const Thread&);
	Thread& operator = (const Thread&);
};

bool IsHyperThreadingOn();

} // details
} // thread


#include "WinThread.inl"
