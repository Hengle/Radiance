// PosixAIO.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"
#include <aio.h>

namespace posix_aio {
namespace details {
class Sig;
} // details

class RADRT_CLASS AIO
{
	friend class details::Sig;
public:
	AIO();
	virtual ~AIO();

	int Read(int fd, int priority, void *dst, AddrSize len, AddrSize ofs, int flags);
	int Write(int fd, int priority, const void *src, AddrSize len, AddrSize ofs, int flags);
	int Cancel();

	static int Cancel(int fd);
	static int Flush(int fd);

protected:

	virtual void OnComplete(int bytes, int error) = 0;

private:

	void Complete(int bytes, int error);

	AIO *m_next;
	struct aiocb m_cb;
	int m_vals[2];
};

} // posix_aio
