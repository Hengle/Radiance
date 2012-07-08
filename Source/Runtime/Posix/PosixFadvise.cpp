// PosixFadvise.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Standard C File IO backend, will use posix_fadvise (async read ahead) if supported.
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "PosixFadvise.h"
#include "../Base/ObjectPool.h"
#include "../Thread/Thread.h"
#include "../Thread/Locks.h"
#include "../Container/ZoneVector.h"
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

#if defined(RAD_OPT_POSIXFADVISE)
#if !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE < 600
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif
#endif

#include <fcntl.h>

namespace rt {
namespace details {
void SemPut();
} // details
} // rt

namespace _posix_fadvise {
	namespace details {
		
		class IO// : public thread::Thread
		{
		public:
			enum Type { Read, Write };
			
		private:
			struct Req
			{
				Type type;
				int fd;
				int r;
				U8 *buf;
				AddrSize len;
				AddrSize ofs;
				AddrSize c;
				AIO *notify;
				Req *next;
			};
			
		public:
			
			IO() : m_head(0), m_notify(0), m_exit(false)
			{
				m_reqPool.Create(ZRuntime, "posix-fadvise-io-req", 8);
				//Run();
			}
			
			virtual ~IO()
			{
				//Exit();
			}
			
			void Queue(
					   Type type,
					   int fd,
					   void *buf,
					   AddrSize len,
					   AddrSize ofs,
					   AIO *notify
					   )
			{
				RAD_ASSERT(fd >= 0);
				Req *r = NewReq();
				r->r      = 0;
				r->type   = type;
				r->fd     = fd;
				r->buf    = (U8*)buf;
				r->len = r->c = len;
				r->ofs    = ofs;
				r->notify = notify;
				r->next   = 0;
#if defined(RAD_OPT_POSIXFADVISE)
				if (type == Read)
				{
					posix_fadvise(fd, ofs, len, POSIX_FADV_WILLNEED);
				}
#endif
				QueueP(r);
			}
			
			void Cancel(int fd)
			{
				Lock l(m_m);
				m_fdCancel.push_back(fd);
				//m_sema.Put();
				rt::details::SemPut();
			}
			
			void Notify()
			{
				for (Req *r = DequeN(); r; r = r->next)
				{
					Finish(r);
				}
			}
			
			void Process()
			{
				Req *r;
				
				{
					Lock l(m_m);
					r = DequeP();
					
					for (FdVec::const_iterator it = m_fdCancel.begin(); it != m_fdCancel.end(); ++it)
					{
						for (Req *z = r; z; z = z->next)
						{
							if (z->fd == *it)
							{
								z->r = ECANCELED;
							}
						}
					}
					
					m_fdCancel.clear();
					m_sema.Reset();
				}
				
				for (Req *z = r; z;)
				{
					Req *n = z->next;
					Process(z); // modifies z->next
					z = n;
				}
				
				Notify();
			}

			
		protected:
			
			virtual int ThreadProc()
			{
				do
				{
					m_sema.Get();
					Process();
				} while(!m_exit);
				
				return 0;
			}
			
		private:
			
			void QueueP(Req *r)
			{
				Lock l(m_m);
				r->next = m_head;
				m_head = r;
				//m_sema.Put();
				rt::details::SemPut();
			}
			
			void QueueN(Req *r)
			{
				Lock l(m_m);
				r->next = m_notify;
				m_notify = r;
//				rt::details::SemPut();
			}
			
			Req *DequeP()
			{
				Req *r = m_head;
				m_head = 0;
				return r;
			}
			
			Req *DequeN()
			{
				Lock l(m_m);
				Req *r = m_notify;
				m_notify = 0;
				return r;
			}
			
			enum { MaxSize = 1024*96 };
			
			void Process(Req *r)
			{
				if (r->r == 0)
				{
					AddrSize size = std::min<AddrSize>(MaxSize, r->len);
					size = std::min<AddrSize>(size, SSIZE_MAX);
					
					if (r->type == Read)
					{
						r->r = (int)pread(r->fd, r->buf, size, r->ofs);
					}
					else
					{
						r->r = (int)pwrite(r->fd, r->buf, size, r->ofs);
					}
					
					if (r->r >= 0)
					{
						r->buf += r->r;
						r->ofs += (AddrSize)r->r;
						r->len -= (AddrSize)r->r;
						r->r = 0;
					}
					else
					{
						RAD_ASSERT(r->r == -1);
						r->r = errno;
					}
				}
				
				if (r->r == 0 && r->len > 0)
				{
					QueueP(r);
				}
				else
				{
//					QueueN(r);
					Finish(r);
				}
			}
			
			void Finish(Req *r)
			{
				r->notify->Complete((int)(r->c - r->len), r->r);
				DelReq(r);
			}
			
			Req *NewReq()
			{
				Lock l(m_m);
				return m_reqPool.Construct();
			}
			
			void DelReq(Req *r)
			{
				Lock l(m_m);
				m_reqPool.Destroy(r);
			}
			
			void Exit()
			{
				//m_exit = true;
				//m_sema.Put();
				//Join();
			}
			
			typedef boost::mutex Mutex;
			typedef boost::lock_guard<Mutex> Lock;
			
			typedef ObjectPool<Req> ReqPool;
			typedef zone_vector<int, ZRuntimeT>::type FdVec;
			
			FdVec  m_fdCancel;
			ReqPool m_reqPool;
			Mutex m_m;
			thread::Semaphore m_sema;
			Req * volatile m_head;
			Req * volatile m_notify;
			volatile bool m_exit;
		};
		
		IO s_io;
		
		void Initialize()
		{
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
			//s_io.Notify();
			s_io.Process();
		}
		
	} // details
	
	AIO::AIO() : m_fd(-1)
	{
	}
	
	AIO::~AIO()
	{
		RAD_ASSERT_MSG(m_fd == -1, "IO object still in use!");
	}
	
	int AIO::Read(int fd, int priority, void *dst, AddrSize len, AddrSize ofs, int flags)
	{
		RAD_ASSERT(m_fd == -1);
		m_fd = fd;
		details::s_io.Queue(details::IO::Read, fd, dst, len, ofs, this);
		return 0;
	}
	
	int AIO::Write(int fd, int priority, const void *src, AddrSize len, AddrSize ofs, int flags)
	{
		RAD_ASSERT(m_fd == -1);
		m_fd = fd;
		details::s_io.Queue(details::IO::Write, fd, const_cast<void*>(src), len, ofs, this);
		return 0;
	}
	
	void AIO::Complete(int bytes, int error)
	{
		RAD_ASSERT(m_fd >= 0);
		m_fd = -1;
		OnComplete(bytes, error);
	}
	
	int AIO::Cancel()
	{
		if (m_fd >= 0)
		{
			return Cancel(m_fd);
		}
		return EBADF;
	}
	
	int AIO::Cancel(int fd)
	{
		details::s_io.Cancel(fd);
		return 0;
	}
	
	int AIO::Flush(int fd)
	{
		return 0;
	}
	
} // _posix_fadvise
