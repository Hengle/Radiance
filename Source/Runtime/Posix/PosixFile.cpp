// PosixFile.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../File.h"
#include "../Thread.h"
#include "../File/PrivateFile.h"
#include "../StringBase.h"
#include "../Utils.h"
#include "../Time.h"
#include <unistd.h>
#include <stdlib.h>
#if defined(RAD_OPT_APPLE)
	#include <fcntl.h>
	#include <sys/statvfs.h>
#else
	#include <sys/vfs.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#if defined(RAD_OPT_APPLE)
	#define statfs statvfs
#endif

#define OVERRIDE_SECTOR_SIZE 512

using namespace string;
using namespace xtime;

namespace {

inline char *WCToAnsi(char *dst, const wchar_t *src) // dst is assumed to be big enough
{
	::wcstombs(dst, src, file::MaxFilePathLen);
	return dst;
}

inline wchar_t *AnsiToWC(wchar_t *dst, const char *src)
{
	::mbstowcs(dst, src, file::MaxFilePathLen);
	return dst;
}

#define CSTR(_src) WCToAnsi((char*)stack_alloc(::wcstombs(0, _src, MaxFilePathLen+1)+1), _src)
#define WSTR(_src) AnsiToWC((wchar_t*)stack_alloc((::mbstowcs(0, _src, MaxFilePathLen+1)+1)*sizeof(wchar_t)), _src)

} // namespace

#if defined(RAD_OPT_IOS)
void __IOS_BundlePath(char*);
#elif defined(RAD_OPT_OSX)
void __OSX_BundlePath(char*);
#endif

namespace file {

const wchar_t * const NativePathSeparator = L"/";

namespace details {
	
namespace {

enum { IO_CHUNK_SIZE = 256*Kilo };

void SetDefaultAliases()
{
	{
		wchar_t buff[MaxFilePathLen+1];

		{
			char z[MaxFilePathLen+1];
#if defined(RAD_OPT_IOS)
			__IOS_BundlePath(z);
#elif defined(RAD_OPT_OSX) && !defined(RAD_OPT_TOOLS)
			__OSX_BundlePath(z);
#else
			getcwd(z, MaxFilePathLen);
#endif
			z[MaxFilePathLen] = 0;
			RAD_VERIFY(::mbstowcs(0, z, MaxFilePathLen) <= MaxFilePathLen);
			::mbstowcs(buff, z, MaxFilePathLen);
			buff[MaxFilePathLen] = 0;
		}

		{
			size_t l = len(buff);
			if (buff[0] != 0 && buff[l-1] != L'/' && buff[l-1] != L'\\')
			{
				cat(buff, L"/"); // fake directory seperator.
			}
		}

		for (UReg i = 0; buff[i] != 0; i++)
		{
			if (buff[i] == L'\\') buff[i] = L'/';
		}

		// backup and find the last directory separator.
		if (buff[0] != 0)
		{
			bool found = false;
			UReg l = (UReg)len(buff);
			while (l-- > 0)
			{
				if (buff[l] == L'/')
				{
					buff[l] = 0; // terminate.
					found = true;
					break;
				}
			}

			// there was no real command line passed in (just the exe name), so terminate here and use the cwd.
			if (!found)
			{
				buff[0] = L'.';
				buff[1] = 0;
			}
		}

#if defined(RAD_OPT_DEBUG)
		details::UncheckedSetAlias(AliasRoot, buff);
#else
		SetAlias(AliasRoot, buff);
#endif
	}


#if defined(RAD_OPT_DEBUG)
	details::UncheckedSetAlias(AliasCDDVD, L"/media/cdrom");
#else
	SetAlias(AliasCDDVD, L"/media/cdrom");
#endif

#if defined(RAD_OPT_DEBUG)
	details::UncheckedSetAlias(AliasHDD, L""); // this is root '/', user paths start with '/' already
#else
	SetAlias(AliasHDD, L"");
#endif

}
} // namespace

Search::Search() :
m_sdir(0),
m_cur(0),
m_recursed(0),
m_flags(SearchFlags(0))
{
	m_dir[0] = 0;
	m_ext[0] = 0;
	m_root[0] = 0;
}

Search::~Search()
{
	if (0 != m_sdir)
	{
		Close();
	}
}

bool Search::Open(const wchar_t* directory,
	const wchar_t* ext,
	SearchFlags flags
)
{
	RAD_ASSERT(directory && directory[0]);
	RAD_ASSERT(ext && ext[0]);
	RAD_ASSERT(m_flags == 0);
	RAD_ASSERT(m_recursed == 0);
	RAD_ASSERT(m_root[0] == 0);
	RAD_ASSERT(m_sdir == 0 && m_cur == 0);
	RAD_ASSERT_MSG(len(directory) <= MaxFilePathLen, "Directory path exceeds MaxFilePathLen!");

#if defined(RAD_OPT_DEBUG)
	if (!(flags&NativePath))
	{
		AssertExtension(ext);
		{
			UReg p, ofs;
			RAD_ASSERT_MSG(ExtractAlias(directory, &p, &ofs), "Directory path must have an alias!");
			if (directory[ofs] != 0)
			{
				AssertFilePath(directory, true);
			}
		}
	}
#endif

	return PrivateOpen(L"", directory, ext, flags);
}

bool Search::PrivateOpen(
	const wchar_t* root,
	const wchar_t* directory,
	const wchar_t* ext,
	SearchFlags flags
)
{
	RAD_ASSERT(root);
	RAD_ASSERT(directory && directory[0]);
	RAD_ASSERT(ext && ext[0]);
	RAD_ASSERT(m_flags == 0);
	RAD_ASSERT(m_recursed == 0);
	RAD_ASSERT(m_root[0] == 0);
	RAD_ASSERT(m_sdir == 0 && m_cur == 0);
	RAD_ASSERT_MSG(len(directory) <= MaxFilePathLen, "Directory path exceeds MaxFilePathLen!");
	RAD_ASSERT_MSG(len(root) <= MaxFilePathLen, "Root path exceeds MaxFilePathLen!");

#if defined(RAD_OPT_DEBUG)
	if (!(flags&NativePath))
	{
		AssertExtension(ext);
	}
#endif

	cpy(m_ext, ext);

	wchar_t buff[MaxFilePathLen+1];
	if (flags & NativePath)
	{
		ncpy(buff, directory, MaxFilePathLen+1);
		// make sure it doesn't end in a '/' or '\\'.
		UReg l = (UReg)len(buff);
		if (l > 1 && (buff[l-1] == '\\' || buff[l-1] == '/')) buff[l-1] = 0;
		//if (l == 1 && buff[0] == '~') { cat(buff, L"/"); }
	}
	else
	{
		if (!ExpandToNativePath(directory, buff, MaxFilePathLen+1))
		{
			return false;
		}
	}

	m_trimLen = len(buff);
	m_sdir = ::opendir(CSTR(buff));

	if (m_sdir != 0)
	{
		m_cur = new dirent;
		m_flags = flags;
		ncpy(m_root, root, MaxFilePathLen+1);
		ncpy(m_dir, directory, MaxFilePathLen+1);

		UReg l = (UReg)len(m_root);
		if (l > 1 && (m_root[l-1] == L'\\' || m_root[l-1] == L'/')) m_root[l-1] = 0;
		l = (UReg)len(m_dir);
		if (l > 1 && (m_dir[l-1] == L'\\' || m_dir[l-1] == L'/')) m_dir[l-1] = 0;

		return true;
	}

	return false;
}

bool Search::NextFile(
	wchar_t* filenameBuffer,
	UReg filenameBufferSize,
	FileAttributes* fileFlags,
	xtime::TimeDate* fileTime
)
{
	RAD_ASSERT(filenameBuffer);
	RAD_ASSERT(filenameBufferSize > 0);
	RAD_ASSERT(m_sdir != 0);

	if (m_recursed)
	{
		if (m_recursed->NextFile(filenameBuffer, filenameBufferSize, fileFlags, fileTime))
		{
			return true; // all good.
		}

		delete m_recursed;
		m_recursed = 0;
	}

	RAD_ASSERT(m_recursed == 0);
	wchar_t ext[MaxExtLen+1];

	for (;;)
	{
		RAD_ASSERT(m_recursed == 0);

		struct dirent *next;
		if (readdir_r((DIR*)m_sdir, m_cur, &next) != 0 || 0 == next)
		{
			return false;
		}

		char *filename = &next->d_name[0];

		if (cmp(filename, ".") && cmp(filename, "..")) // skip directory links
		{
			// is this a directory?
			if (next->d_type == DT_DIR)
			{
				// what do we do with it?
				if (m_flags & DirNames)
				{
					UReg l;

					if (m_root[0] != 0)
					{
						// search this directory.
						l = filenameBufferSize;
						ncpy(filenameBuffer, m_root, l);
						l -= (UReg)len(filenameBuffer);
						ncat(filenameBuffer, L"/", l);

						if (l > 0)
						{
							--l;
							ncat(filenameBuffer, WSTR(filename), l);
						}
					}
					else
					{
						ncpy(filenameBuffer, WSTR(filename), filenameBufferSize);
					}

					if (fileFlags)
					{
						*fileFlags = FileAttributes(Directory|Normal);
					}

					if (fileTime)
					{
						memset(fileTime, 0, sizeof(TimeDate));
						struct stat s;

						if (stat(next->d_name, &s) == 0)
						{
							struct tm t;
							localtime_r(&s.st_mtime, &t);

							fileTime->dayOfMonth = (U8)t.tm_mday;
							fileTime->month      = (U8)t.tm_mon;
							fileTime->year       = (U16)t.tm_year;
							fileTime->hour       = (U8)t.tm_hour;
							fileTime->minute     = (U8)t.tm_min;
							fileTime->second     = (U8)t.tm_sec;
							fileTime->dayOfWeek  = (U8)t.tm_wday;
							fileTime->millis     = 0;
						}
					}

					if (!(m_flags&Recursive))
					{
#if defined (RAD_OPT_DEBUG)
						if (!(m_flags&NativePath)) AssertFilePath(filenameBuffer, false);
#endif
						return true;
					}
				}
				if (m_flags & Recursive)
				{
					UReg l;
					wchar_t root[MaxFilePathLen+1], dir[MaxFilePathLen+1];

					if (m_root[0] != 0)
					{
						// search this directory.
						ncpy(root, m_root, MaxFilePathLen);
						l = len(root);
						// not terminated with a '/'?
						if (root[l-1] != '/') { ncat(root, L"/", MaxFilePathLen - l); }
						l = MaxFilePathLen - l;

						if (l > 0)
						{
							--l;
							ncat(root, WSTR(filename), l);
						}
					}
					else
					{
						ncpy(root, WSTR(filename), MaxFilePathLen+1);
					}

					if (m_dir[0] != 0)
					{
						// search this directory.
						ncpy(dir, m_dir, MaxFilePathLen);
						l = len(dir);
						// not terminated with a '/'?
						if (dir[l-1] != '/') { ncat(dir, L"/", MaxFilePathLen - l); }
						l = MaxFilePathLen - l;

						if (l > 0)
						{
							--l;
							ncat(dir, WSTR(filename), l);
						}
					}
					else
					{
						ncpy(dir, WSTR(filename), MaxFilePathLen+1);
					}

					m_recursed = new Search();

					if (m_recursed->PrivateOpen(root, dir, m_ext, m_flags))
					{
						if (m_flags & DirNames) // return the directory name. next time we're here we'll recurse.
						{
							return true;
						}

						if (m_recursed->NextFile(filenameBuffer, filenameBufferSize, fileFlags, fileTime))
						{
							return true;
						}
					}

					delete m_recursed;
					m_recursed = 0;
				}
			}
			else if (next->d_type == DT_REG && (m_flags & FileNames))
			{
				if (m_ext[1] != L'*' || m_ext[2] != 0) // all files
				{
					FileExt(WSTR(filename), ext, MaxExtLen+1);
					if (icmp(ext, m_ext))
					{
						continue; // filtered.
					}
				}

				UReg l;

				if (m_root[0] != 0)
				{
					// search this directory.
					l = filenameBufferSize;
					ncpy(filenameBuffer, m_root, l);
					l -= (UReg)len(filenameBuffer);

					ncat(filenameBuffer, L"/", l);

					if (l > 0)
					{
						--l;
						ncat(filenameBuffer, WSTR(filename), l);
					}
				}
				else
				{
					ncpy(filenameBuffer, WSTR(filename), filenameBufferSize);
				}

				if (fileFlags)
				{
					*fileFlags = Normal;
				}

				if (fileTime)
				{
					memset(fileTime, 0, sizeof(TimeDate));
					struct stat s;

					if (stat(next->d_name, &s) == 0)
					{
						struct tm t;
						localtime_r(&s.st_mtime, &t);

						fileTime->dayOfMonth = (U8)t.tm_mday;
						fileTime->month      = (U8)t.tm_mon;
						fileTime->year       = (U16)t.tm_year;
						fileTime->hour       = (U8)t.tm_hour;
						fileTime->minute     = (U8)t.tm_min;
						fileTime->second     = (U8)t.tm_sec;
						fileTime->dayOfWeek  = (U8)t.tm_wday;
						fileTime->millis     = 0;
					}
				}

#if defined (RAD_OPT_DEBUG)
				if (!(m_flags&NativePath)) AssertFilePath(filenameBuffer, false);
#endif
				return true;
			}
		}
	}
}

void Search::Close()
{
	RAD_ASSERT(m_sdir != 0);

	if (m_recursed)
	{
		delete m_recursed;
	}

	if (m_cur)
	{
		delete m_cur;
		m_cur = 0;
	}

	if (m_sdir)
	{
		::closedir((DIR*)m_sdir);
		m_sdir = 0;
	}
}

bool Search::IsValid()
{
	return m_sdir != 0;
}

AsyncIO::AsyncIO() :
m_bytes(0),
m_req(0),
m_chunkSize(0),
m_ofs(0),
m_fd(0),
m_status(Success),
m_read(true),
m_cancel(false),
m_buffer(0),
m_gate(true, false)
{
}

AsyncIO::~AsyncIO()
{
}

void AsyncIO::Go(int bytes)
{
	m_bytes += bytes;
	m_ofs += bytes;

	file::Result r = Pending;
	
	if (m_bytes >= m_req)
	{
		r = Success;
	}
	else
	{
		FPos chunkSize = ((m_bytes+m_chunkSize) < m_req) ? m_chunkSize : (m_req-m_bytes);
		if (m_read && m_ofs+chunkSize > m_fsize)
		{
			RAD_ASSERT(m_fsize >= m_ofs);
			chunkSize = m_fsize - m_ofs;
		}

		if (chunkSize > 0)
		{
			if (m_read)
			{
				if (Read(m_fd, 0, m_buffer+m_bytes, chunkSize, m_ofs, 0) != 0)
				{
					r = ErrorGeneric;
				}
			}
			else
			{
				if (Write(m_fd, 0, m_buffer+m_bytes, chunkSize, m_ofs, 0) != 0)
				{
					r = ErrorGeneric;
				}
			}
		}
		else
		{
			// clients will request reads off the end of the file because
			// the generic API requires read sizes that are aligned to sector
			// boundaries.
			r = ErrorPartial;
		}
	}

	if (r != Pending)
	{
		TriggerStatusChange(r, true);
	}
}

void AsyncIO::OnComplete(int bytes, int error)
{
	switch (error)
	{
	case 0:
		// no error.
		RAD_ASSERT(bytes>=0);
		Go(bytes);
		break;
	case ECANCELED:
		TriggerStatusChange(ErrorAborted, false);
		break;
	case EISDIR:
		TriggerStatusChange(ErrorInvalidArguments, false);
		break;
	case EIO:
		TriggerStatusChange(ErrorDriveNotReady, false);
		break;
	default:
		TriggerStatusChange(ErrorGeneric, false);
		break;
	}
}

void AsyncIO::TriggerStatusChange(file::Result result, bool force)
{
	bool wasPending = m_status == file::Pending;
	bool isPending  = result == file::Pending;

	if (isPending)
	{
		m_cancel = false;
		m_gate.Close();
	}

	if ((wasPending && !isPending) || force)
	{
		file::AsyncIO* super = RAD_CLASS_FROM_MEMBER(file::AsyncIO, m_imp, this);
		super->OnComplete(result);
		m_status = result;
		m_gate.Open(); // trigger anyone waiting for completion in WaitForCompletion().
	}
	else
	{
		m_status = result;
	}
}
	
bool AsyncIO::WaitForCompletion(U32 timeout) const
{
	bool r = m_gate.Wait(timeout);
	RAD_ASSERT(m_status != Pending);
	return r;
}

File::File() :
m_fd(-1),
m_flags(FileOptions(0))
{
}

File::~File()
{
	if (m_fd != -1)
	{
		Close(0);
	}
}

Result File::Open(
	const wchar_t* filename,
	CreationType creationType,
	AccessMode accessMode,
	ShareMode shareMode,
	FileOptions fileOptions,
	AsyncIO* io
)
{
	RAD_ASSERT(m_fd == -1);
	RAD_ASSERT(filename && filename[0]);
	RAD_DEBUG_ONLY(AssertCreationType(creationType));

	int flags =
#if defined(O_BINARY)
		O_BINARY;
#else
		0;
#endif

	wchar_t nativeFilename[MaxFilePathLen+1];

	if (fileOptions&NativePath)
	{
		cpy(nativeFilename, filename);
	}
	else
	{
		if (!ExpandToNativePath(filename, nativeFilename, MaxFilePathLen+1)) 
			return ErrorExpansionFailed;
	}

	if ((accessMode&(AccessRead|AccessWrite)) == (AccessRead|AccessWrite))
	{
		flags |= O_RDWR;
	}
	else if (accessMode&AccessRead)
	{
		flags |= O_RDONLY;
	}
	else if (accessMode&AccessWrite)
	{
		flags |= O_WRONLY;
	}

	switch (creationType)
	{
	case CreateNew:
		flags |= O_CREAT|O_EXCL;
		break;
	case CreateAlways:
		flags |= O_CREAT|O_TRUNC;
		break;
	case TruncateExisting:
		flags |= O_CREAT|O_EXCL|O_TRUNC;
		break;
	case OpenAlways:
		flags |= O_CREAT;
		break;
	default:
		if (creationType != OpenExisting) 
			return ErrorInvalidArguments;
		break;
	}

	m_fd = open(CSTR(nativeFilename), flags, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	if (m_fd != -1)
	{
		m_flags = fileOptions;
		m_sectorSize = DeviceSectorSize(nativeFilename, (fileOptions&MiscFlagsAll)|NativePath);
		RAD_ASSERT(m_sectorSize != 0);

		if (io)
		{
			io->TriggerStatusChange(Success, true);
		}
		return Success;
	}

	if (io)
	{
		io->TriggerStatusChange(ErrorGeneric, true);
	}

	return ErrorGeneric;
}

Result File::Close(AsyncIO* io)
{
	RAD_ASSERT(m_fd != -1);
#if defined(RAD_OPT_DEBUG)
	if (io)
	{
		RAD_ASSERT_MSG((m_flags&Async), "Async IO object used on a file that was not opened in Async mode!");
	}
#endif
	::close(m_fd);
	m_fd = -1;
	m_flags = FileOptions(0);
	return Success;
}

Result File::Read (
	void* buffer,
	FPos bytesToRead,
	FPos* bytesRead,
	FPos filePos,
	AsyncIO* io
)
{
	RAD_ASSERT(m_fd != -1);
	RAD_ASSERT(m_sectorSize != 0);

#if defined(RAD_OPT_DEBUG)
	if (m_flags & Async) // this is done for consistency on other platforms.
	{
		RAD_ASSERT_MSG(((m_sectorSize-1)&filePos) == 0, "File accesses must be sector size aligned (filePos)!");
		RAD_ASSERT_MSG(((m_sectorSize-1)&bytesToRead) == 0, "File accesses must be sector size aligned (bytesToRead)!");
		RAD_ASSERT_MSG((((AddrSize)(m_sectorSize-1))&((AddrSize)buffer)) == 0, "Buffer for read operations must be aligned to a sector-size boundary.");
	}
#endif

	Result r;

	if (m_flags & Async)
	{
		file::AsyncIO _io;
		if (!io)
			io = &_io.m_imp;
		io->TriggerStatusChange(Pending, false);
		io->m_cancel = false;
		io->m_bytes = 0;
		io->m_ofs = filePos;
		io->m_req = bytesToRead;
		io->m_chunkSize = (bytesToRead>IO_CHUNK_SIZE)?IO_CHUNK_SIZE:bytesToRead;
		io->m_read = true;
		io->m_buffer = (U8*)buffer;
		io->m_fd = m_fd;
		io->m_fsize = Size();
		io->Go(0);
		if (io == &_io.m_imp)
		{
			io->WaitForCompletion();
			r = io->Result();
			RAD_ASSERT(r != Pending);
			if ((r == Success || r == ErrorPartial) && bytesRead)
			{
				*bytesRead = io->ByteCount();
			}
		}
		else
		{
			r = Pending;
		}
	}
	else
	{
		RAD_ASSERT(!io);
		ssize_t rd = 0;

		if ((::lseek(m_fd, (off_t)filePos, SEEK_SET) == (off_t)filePos) &&
			 ((rd=::read(m_fd, buffer, bytesToRead)) != -1))
		{
			r = (rd == bytesToRead) ? Success : ErrorPartial;
			if (bytesRead) *bytesRead = (FPos)rd;
		}
		else
		{
			r = ErrorGeneric;
		}
	}

	return r;
}

Result File::Write(
	const void* buffer,
	FPos bytesToWrite,
	FPos* bytesWritten,
	FPos filePos,
	AsyncIO* io
)
{
	RAD_ASSERT(m_fd != -1);
	RAD_ASSERT(m_sectorSize != 0);

#if defined(RAD_OPT_DEBUG)
	if (m_flags & Async) // this is done for consistency on other platforms.
	{
		RAD_ASSERT_MSG(((m_sectorSize-1)&filePos) == 0, "File accesses must be sector size aligned (filePos)!");
		RAD_ASSERT_MSG((((AddrSize)(m_sectorSize-1))&((AddrSize)buffer)) == 0, "Buffer for read operations must be aligned to a sector-size boundary.");
	}
#endif

	Result r;

	if (m_flags & Async)
	{
		file::AsyncIO _io;
		if (!io)
			io = &_io.m_imp;
		io->TriggerStatusChange(Pending, false);
		io->m_cancel = false;
		io->m_bytes = 0;
		io->m_ofs = filePos;
		io->m_req = bytesToWrite;
		io->m_chunkSize = (bytesToWrite>IO_CHUNK_SIZE)?IO_CHUNK_SIZE:bytesToWrite;
		io->m_read = false;
		io->m_buffer = (U8*)buffer;
		io->m_fd = m_fd;
		io->m_fsize = Size();
		io->Go(0);
		if (io == &_io.m_imp)
		{
			io->WaitForCompletion();
			r = io->Result();
			RAD_ASSERT(r != Pending);
			if ((r == Success || r == ErrorPartial) && bytesWritten)
			{
				*bytesWritten = io->ByteCount();
			}
		}
		else
		{
			r = Pending;
		}
	}
	else
	{
		RAD_ASSERT(!io);
		ssize_t rd = 0;

		if ((::lseek(m_fd, (off_t)filePos, SEEK_SET) == (off_t)filePos) &&
			 ((rd=::write(m_fd, buffer, bytesToWrite)) != -1))
		{
			r = (rd == bytesToWrite) ? Success : ErrorPartial;
			if (bytesWritten) *bytesWritten = (FPos)rd;
		}
		else
		{
			r = ErrorGeneric;
		}
	}

	return r;
}

FPos File::Size() const
{
	RAD_ASSERT(m_fd != -1);
	struct stat s;
	if (fstat(m_fd, &s) == 0)
	{
		return (FPos)s.st_size;
	}
	return 0;
}

bool File::CancelIO()
{
	RAD_ASSERT(m_fd != -1);
#if defined(RAD_OPT_POSIXAIO)
	return posix_aio::AIO::Cancel(m_fd) == 0;
#else
	return _posix_fadvise::AIO::Cancel(m_fd) == 0;
#endif
}

bool File::Flush()
{
	RAD_ASSERT(m_fd != -1);
#if defined(RAD_OPT_POSIXAIO)
	return posix_aio::AIO::Flush(m_fd) == 0;
#else
	return _posix_fadvise::AIO::Flush(m_fd) == 0;
#endif
}

} // details

RADRT_API bool RADRT_CALL ExpandToNativePath(const wchar_t *portablePath, wchar_t *nativePath, UReg nativePathBufferSize)
{
	RAD_ASSERT(portablePath);
	RAD_ASSERT(nativePath);
	RAD_ASSERT(nativePathBufferSize > 0);

#if defined(RAD_OPT_DEBUG)
	{
		UReg p, ofs;
		RAD_ASSERT_MSG(details::ExtractAlias(portablePath, &p, &ofs), "Path must have an alias!");
		if (portablePath[ofs] != 0) // not just an alias?
		{
			details::AssertFilePath(portablePath, true);
		}
	}
#endif

	return ExpandAliases(portablePath, nativePath, nativePathBufferSize);
}

// returns number of characters that would be written to buffer via ExpandPath if successful (excluding null terminator),
// or 0 if error (or path is a null string).

RADRT_API UReg RADRT_CALL ExpandToNativePathLength(const wchar_t *portablePath)
{
	RAD_ASSERT(portablePath);

#if defined(RAD_OPT_DEBUG)
	details::AssertFilePath(portablePath, true);
#endif
	return ExpandAliasesLength(portablePath);
}

// Returns the sector size of the device that the given file path resides on. Note this
// doesn't have to be a filename, it can be a path, or an alias.

RADRT_API FPos RADRT_CALL DeviceSectorSize(const wchar_t *path, int flags)
{
	RAD_ASSERT(path);

	wchar_t buff[MaxFilePathLen+1];

	if (flags & NativePath)
	{
		::string::cpy(buff, path);
	}
	else
	{
		if (!ExpandAliases(path, buff, MaxFilePathLen+1))
		{
			return 0;
		}
	}

	struct statfs sfs;
	if (statfs(CSTR(path), &sfs) != 0)
	{
		sfs.f_bsize = 0;
	}

#if defined(OVERRIDE_SECTOR_SIZE)
	return OVERRIDE_SECTOR_SIZE;
#else
	return (FPos)sfs.f_bsize;
#endif
}

RADRT_API bool RADRT_CALL DeleteFile(const wchar_t *path, int flags)
{
	RAD_ASSERT(path);

	if (flags & NativePath)
	{
		return ::unlink(CSTR(path)) == 0;
	}
	else
	{
#if defined(RAD_OPT_DEBUG)
		details::AssertFilePath(path, true);
#endif

		wchar_t buff[MaxFilePathLen+1];
		if (ExpandToNativePath(path, buff, MaxFilePathLen+1))
		{
			return ::unlink(CSTR(buff)) == 0;
		}
		else
		{
			return false;
		}
	}
}

static bool PrivateCreateDirectory(const char *nativePath)
{
	RAD_ASSERT(nativePath);
	char buff[MaxFilePathLen+1];

	int ofs = 0;
	while (nativePath[ofs])
	{
		if (nativePath[ofs] == '/')
		{
			::string::ncpy(buff, nativePath, ofs+1);
			buff[ofs] = 0;
			if ((buff[0] && buff[0] != L'/') || ofs > 1) // not root
			{
				if (mkdir(buff, 0777) == -1)
				{
					if (errno != EEXIST)
					{
						return false;
					}
				}
			}
		}
		++ofs;
	}

	bool b = true;

	if (nativePath[0] != L'/' || nativePath[1] != 0 > 1)
	{
		b = mkdir(nativePath, 0777) == 0;
		if (!b)
		{
			b = errno == EEXIST;
		}
	}
	return b;
}

#undef CreateDirectory
RADRT_API bool RADRT_CALL CreateDirectory(const wchar_t *path, int flags)
{
	RAD_ASSERT(path);

	if (flags & NativePath)
	{
		return PrivateCreateDirectory(CSTR(path));
	}
	else
	{
		wchar_t buff[MaxFilePathLen+1];
		if (ExpandToNativePath(path, buff, MaxFilePathLen+1))
		{
			return PrivateCreateDirectory(CSTR(buff));
		}
		else
		{
			return false;
		}
	}
}

static bool DeleteDirectory_r(const wchar_t *nativePath)
{
	RAD_ASSERT(nativePath);

	Search s;

	size_t l = ::string::len(nativePath);

	if (s.Open(nativePath, L".*", SearchFlags(NativePath|FileNames|DirNames)))
	{
		wchar_t file[MaxFilePathLen+1], path[MaxFilePathLen+1];
		FileAttributes fa;
		while (s.NextFile(file, MaxFilePathLen+1, &fa, 0))
		{
			::string::cpy(path, nativePath);
			if (nativePath[l] != L'/') ::string::cat(path, L"/");
			::string::cat(path, file);

			if (fa & Directory)
			{
				// recurse!
				if (!DeleteDirectory_r(path)) return false;
			}
			else
			{
				if (::unlink(CSTR(path)) != 0) return false;
			}
		}

		s.Close();
	}

	return ::rmdir(CSTR(nativePath)) == 0;
}

RADRT_API bool RADRT_CALL DeleteDirectory(const wchar_t *path, int flags)
{
	RAD_ASSERT(path);

	if (flags & NativePath)
	{
		return DeleteDirectory_r(path);
	}
	else
	{
		wchar_t buff[MaxFilePathLen+1];
		if (ExpandToNativePath(path, buff, MaxFilePathLen+1))
		{
			return DeleteDirectory_r(buff);
		}
		else
		{
			return false;
		}
	}
}


RADRT_API bool RADRT_CALL FileTime(const wchar_t *path, xtime::TimeDate* td, int flags)
{
	RAD_ASSERT(path && td);

	struct stat s;

	if (flags & NativePath)
	{
		if (::stat(CSTR(path), &s) != 0)
		{
			return false;
		}
	}
	else
	{
		#if defined(RAD_OPT_DEBUG)
			details::AssertFilePath(path, true);
		#endif

		wchar_t buff[MaxFilePathLen+1];
		if (ExpandToNativePath(path, buff, MaxFilePathLen+1))
		{
			if (::stat(CSTR(buff), &s) != 0)
			{
				return false;
			}
		}
	}


	struct tm t;
	localtime_r(&s.st_mtime, &t);

	td->dayOfMonth = (U8)t.tm_mday;
	td->month      = (U8)t.tm_mon;
	td->year       = (U16)t.tm_year;
	td->hour       = (U8)t.tm_hour;
	td->minute     = (U8)t.tm_min;
	td->second     = (U8)t.tm_sec;
	td->dayOfWeek  = (U8)t.tm_wday;
	td->millis     = 0;

	return true;
}

RADRT_API bool RADRT_CALL FileExists(const wchar_t *path, int flags)
{
	RAD_ASSERT(path);

	struct stat s;

	if (flags & NativePath)
	{
		return ::stat(CSTR(path), &s) == 0;
	}
	else
	{
		RAD_DEBUG_ONLY(details::AssertFilePath(path, true));

		wchar_t buff[MaxFilePathLen+1];
		if (ExpandToNativePath(path, buff, MaxFilePathLen+1))
		{
			return ::stat(CSTR(buff), &s) == 0;
		}
        else
		{
			return false;
		}
	}
}

namespace details {

void InitializeAliasTable();

void Initialize()
{
	InitializeAliasTable();
	SetDefaultAliases();
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
} // file
