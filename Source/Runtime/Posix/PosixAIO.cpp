// PosixAIO.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "PosixAIO.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define AIO_SIG SIGRTMAX

namespace rt {
namespace details {
void SemPut();
} // details
} // rt

namespace posix_aio {
namespace details {

class Sig
{
public:

	Sig() : m_head(0)
	{
	}

	~Sig()
	{
	}

	void Process()
	{
		while (m_head)
		{
			AIO *aio = m_head;
			m_head = m_head->m_next;

			aio->m_next = 0;
			aio->OnComplete(aio->m_vals[0], aio->m_vals[1]);
		}
	}

	void SigHandler(int signal, siginfo_t *info, void *)
	{
		AIO *aio  = reinterpret_cast<AIO*>(info->si_value.sival_ptr);
		aio->m_vals[0] = aio_return(&aio->m_cb);
		aio->m_vals[1] = aio_error(&aio->m_cb);
		aio->m_cb.aio_fildes = 0;

		aio->m_next = m_head;
		m_head = aio;
	}

private:

	AIO *m_head;
};

namespace {

Sig s_sig;
thread::Interlocked<int> s_numActive;

} // namespace

void Initialize()
{
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, AIO_SIG);
	pthread_sigmask(SIG_BLOCK, &sigset, 0); // disable main AIO_SIG signal interruptions by default
	s_numActive = 0;
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
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, AIO_SIG);
	siginfo_t info;
	int sig;

	struct timespec tm;
	tm.tv_sec = 0;
	tm.tv_nsec = 0;

	bool busy = false;

	while ((sig=sigtimedwait(&sigset, &info, &tm)) > 0)
	{
		RAD_ASSERT(sig==AIO_SIG);
		s_sig.SigHandler(sig, &info, 0);
		busy = true;
	}
	s_sig.Process();

	if (s_numActive > 0)
		rt::details::SemPut();
	if(!busy)
	{
		thread::Sleep(20);
	}
}

} // details

AIO::AIO() : m_next(0)
{
	memset(&m_cb, 0, sizeof(m_cb));
}

AIO::~AIO()
{
	RAD_VERIFY_MSG(m_cb.aio_fildes == 0, "AIO object destroyed but is still in use!");
}

int AIO::Read(int fd, int priority, void *dst, AddrSize len, AddrSize ofs, int flags)
{
	RAD_VERIFY_MSG(m_cb.aio_fildes == 0, "AIO object in use!");
	memset(&m_cb, 0, sizeof(m_cb));
	m_cb.aio_fildes = fd;
	m_cb.aio_reqprio = priority;
	m_cb.aio_buf = dst;
	m_cb.aio_nbytes = len;
	m_cb.aio_offset = ofs;
	m_cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	m_cb.aio_sigevent.sigev_signo  = AIO_SIG;
	m_cb.aio_sigevent.sigev_value.sival_ptr = this;
	++s_numActive;
	rt::details::SemPut();
	return aio_read(&m_cb);
}

int AIO::Write(int fd, int priority, const void *src, AddrSize len, AddrSize ofs, int flags)
{
	RAD_VERIFY_MSG(m_cb.aio_fildes == 0, "AIO object in use!");
	memset(&m_cb, 0, sizeof(m_cb));
	m_cb.aio_fildes = fd;
	m_cb.aio_reqprio = priority;
	m_cb.aio_buf = const_cast<void*>(src);
	m_cb.aio_nbytes = len;
	m_cb.aio_offset = ofs;
	m_cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	m_cb.aio_sigevent.sigev_signo  = AIO_SIG;
	m_cb.aio_sigevent.sigev_value.sival_ptr = this;
	++s_numActive;
	rt::details::SemPut();
	return aio_write(&m_cb);
}

int AIO::Cancel()
{
	return aio_cancel(m_cb.aio_fildes, &m_cb);
}

int AIO::Cancel(int fd)
{
	return aio_cancel(fd, 0);
}

int AIO::Flush(int fd)
{
	struct aiocb cb;
	memset(&cb, 0, sizeof(cb));
	cb.aio_fildes = fd;
	cb.aio_sigevent.sigev_notify = SIGEV_NONE;
	return aio_fsync(O_SYNC, &cb);
}

} // posix_aio

