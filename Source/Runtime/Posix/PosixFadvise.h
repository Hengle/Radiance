// PosixFadvise.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"

namespace _posix_fadvise {
namespace details {
class IO;
}
class RADRT_CLASS AIO
{
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

	int m_fd;
	friend struct details::IO;
};

} // _posix_fadvise
