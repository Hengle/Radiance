// WinFile.cpp
// File System for Windows 95/98/2000/XP.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../File.h"
#include "../Thread.h"
#include "../File/PrivateFile.h"
#include "../StringBase.h"
#include <stdlib.h>
#include <direct.h>

#undef DeleteFile
using namespace string;

#pragma warning(disable:4800) // warning C4800: 'BOOL' : forcing value to bool 'true' or 'false' (performance warning)
#if !defined(RAD_TARGET_GOLDEN)
	#define CHECK_ERRORS
#endif

namespace file {

const char * const NativePathSeparator = "\\";

namespace details {

enum { IO_CHUNK_SIZE = 256*Kilo };

//////////////////////////////////////////////////////////////////////////////////////////
// IOThread
//////////////////////////////////////////////////////////////////////////////////////////

class IO
{
	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

public:

	IO() : m_startHead(0), m_cancelFile(INVALID_HANDLE_VALUE) {}
	virtual ~IO() {}

	void QueueIO(AsyncIO *io)
	{
		io->m_gate.Close();
		Lock lock(m_cs);
		io->m_nextStart = m_startHead;
		m_startHead = io;
	}

	bool Cancel(HANDLE file)
	{
		Lock lock(m_cs2);
		m_cancelFile = file;
		while (m_cancelFile != INVALID_HANDLE_VALUE)
		{
			::SleepEx(0, TRUE);
			::SwitchToThread();
		}

		return m_cancelResult;
	}

	void Process()
	{
		m_cs.lock();
		while (m_startHead)
		{
			AsyncIO *cur = m_startHead;
			StartIO(cur);
			m_startHead = cur->m_nextStart;
			cur->m_nextStart = 0;
		}
		m_cs.unlock();
		if (m_cancelFile != INVALID_HANDLE_VALUE)
		{
			m_cancelResult = ::CancelIo(m_cancelFile);
			m_cancelFile = INVALID_HANDLE_VALUE;
		}
	}

private:

	void StartIO(AsyncIO *io)
	{
		RAD_ASSERT(io);

		if (io->m_read)
		{
			SetLastError(ERROR_SUCCESS);
			if (!ReadFileEx(io->m_file, io->m_buffer, io->m_chunkSize, &io->m_o, AsyncIO::IOCompletion))
			{
				DWORD error = GetLastError();
#if defined(CHECK_ERRORS)
				if (error != ERROR_HANDLE_EOF)
				{
					LPVOID textBuffer;
					DWORD dwCount = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|
						FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&textBuffer, 0, NULL);

					RAD_FAIL("Windows IO Failure");
					::LocalFree(textBuffer);
				}
#endif
				io->TriggerStatusChange(
					error == ERROR_HANDLE_EOF ? ErrorPartial : ErrorGeneric, 
					true
				);
			}
#if defined(CHECK_ERRORS)
			else
			{
				DWORD error = GetLastError();
				if (error != ERROR_SUCCESS)
				{
					LPVOID textBuffer;
					DWORD dwCount = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|
						FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&textBuffer, 0, NULL);

					RAD_FAIL("Windows IO Failure");
					::LocalFree(textBuffer);
				}
			}
#endif
		}
		else
		{
			SetLastError(ERROR_SUCCESS);
			if (!WriteFileEx(io->m_file, io->m_buffer, io->m_chunkSize, &io->m_o, AsyncIO::IOCompletion))
			{
#if defined(CHECK_ERRORS)
				DWORD error = GetLastError();
				LPVOID textBuffer;
				DWORD dwCount = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|
					FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&textBuffer, 0, NULL);

				RAD_FAIL("Windows IO Failure");
				::LocalFree(textBuffer);
#endif
				io->TriggerStatusChange(ErrorGeneric, true);
			}
#if defined(CHECK_ERRORS)
			else
			{
				DWORD error = GetLastError();
				if (error != ERROR_SUCCESS)
				{
					LPVOID textBuffer;
					DWORD dwCount = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|
						FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&textBuffer, 0, NULL);

					RAD_FAIL("Windows IO Failure");
					::LocalFree(textBuffer);
				}
			}
#endif
		}
	}

	AsyncIO * volatile m_startHead;
	volatile bool m_run;
	volatile bool m_cancelResult;
	volatile HANDLE m_cancelFile;
	Mutex m_cs, m_cs2;
};

namespace
{
	IO s_io;
}

//////////////////////////////////////////////////////////////////////////////////////////
// AsyncIO Object
//////////////////////////////////////////////////////////////////////////////////////////

AsyncIO::AsyncIO() :
m_gate(true, false),
m_nextStart(0),
m_bytes(0),
m_req(0),
m_chunkSize(0),
m_file(INVALID_HANDLE_VALUE),
m_read(false),
m_cancel(false),
m_status(Success)
{
	m_o.hEvent = 0;
	m_o.Internal = 0;
	m_o.InternalHigh = 0;
}

AsyncIO::~AsyncIO()
{
	RAD_VERIFY_MSG((m_o.Internal == 0) && (m_o.InternalHigh == 0), "AsyncIO object destructed but still in use!");
}

void CALLBACK AsyncIO::IOCompletion(DWORD errorCode, DWORD numBytes, LPOVERLAPPED ovr)
{
	RAD_ASSERT(ovr);
	AsyncIO* io = (AsyncIO*)ovr;
	RAD_ASSERT(io->m_status == file::Pending);

	io->m_bytes += numBytes;
	io->m_o.Offset += numBytes;

	file::Result r = file::Pending;

	if (errorCode == ERROR_SUCCESS)
	{
//		RAD_VERIFY(numBytes);
#if 0
		std::wcout << "numBytes == " << numBytes << std::endl;
		if (numBytes == 0)
		{
			/*DWORD error = GetLastError();
			LPVOID textBuffer;
			DWORD dwCount = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|
				FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&textBuffer, 0, NULL);

			RAD_FAIL("Windows IO Failure");
			::LocalFree(textBuffer);*/
		}
#endif
		if (io->m_bytes >= io->m_req)
		{
			r = file::Success;
		}
		else if (io->m_cancel)
		{
			r = file::ErrorAborted;
		}
		else
		{
			FPos chunkSize = ((io->m_bytes+io->m_chunkSize) < io->m_req) ? io->m_chunkSize : (io->m_req - io->m_bytes);

			if (io->m_read)
			{
				SetLastError(ERROR_SUCCESS);
				if (!ReadFileEx(io->m_file, io->m_buffer + io->m_bytes, chunkSize, &io->m_o, AsyncIO::IOCompletion))
				{
					DWORD error = GetLastError();
#if defined(CHECK_ERRORS)
					if (error != ERROR_HANDLE_EOF)
					{
						LPVOID textBuffer;
						DWORD dwCount = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|
							FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&textBuffer, 0, NULL);

						RAD_FAIL("Windows IO Failure");
						::LocalFree(textBuffer);
					}
#endif
					r = error == ERROR_HANDLE_EOF ? ErrorPartial : ErrorGeneric;
				}
#if defined(CHECK_ERRORS)
				else
				{
					DWORD error = GetLastError();
					if (error != ERROR_SUCCESS)
					{
						LPVOID textBuffer;
						DWORD dwCount = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|
							FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&textBuffer, 0, NULL);
						RAD_FAIL("Windows IO Failure");
						::LocalFree(textBuffer);
					}
				}
#endif
			}
			else
			{
				SetLastError(ERROR_SUCCESS);
 				if (!WriteFileEx(io->m_file, io->m_buffer + io->m_bytes, chunkSize, &io->m_o, AsyncIO::IOCompletion))
				{
#if defined(CHECK_ERRORS)
					DWORD error = GetLastError();
					LPVOID textBuffer;
					DWORD dwCount = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|
						FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&textBuffer, 0, NULL);

					RAD_FAIL("Windows IO Failure");
					::LocalFree(textBuffer);
#endif
					r = file::ErrorGeneric;
				}
#if defined(CHECK_ERRORS)
				else
				{
					DWORD error = GetLastError();
					if (error != ERROR_SUCCESS)
					{
						LPVOID textBuffer;
						DWORD dwCount = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|
							FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPSTR)&textBuffer, 0, NULL);
						RAD_FAIL("Windows IO Failure");
						::LocalFree(textBuffer);
					}
				}
#endif
			}
		}
	}
	else
	{
		r = 
			(errorCode == ERROR_OPERATION_ABORTED) ? file::ErrorAborted : 
			(errorCode == ERROR_HANDLE_EOF) ? file::ErrorPartial : file::ErrorGeneric;
	}

	//
	// NOTE: TriggerCompletion() will only do something if the status is NOT Pending
	//
	if (r != file::Pending)
	{
		io->TriggerStatusChange(r, true);
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
		m_o.Internal = 0;
		m_o.InternalHigh = 0;
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
	return m_gate.Wait(timeout);
}

//////////////////////////////////////////////////////////////////////////////////////////
// File Object
//////////////////////////////////////////////////////////////////////////////////////////

File::File()
{
	m_file = INVALID_HANDLE_VALUE;
	m_flags = FileOptions(0);
}

File::~File()
{
	if (m_file != INVALID_HANDLE_VALUE)
	{
		Close(0);
	}

	RAD_ASSERT(m_flags == 0);
	RAD_ASSERT(m_file == INVALID_HANDLE_VALUE);
}

// the file must be opened in Async mode to use asyncronous IO.
//
// unless otherwise noted, if the AsyncIO object on any function is NULL, the function is
// blocking.

Result File::Open(const char *filename, CreationType creationType, AccessMode accessMode, ShareMode shareMode, FileOptions fileOptions, AsyncIO *io)
{
	RAD_ASSERT(m_file == INVALID_HANDLE_VALUE);
	RAD_ASSERT(filename && filename[0]);
	RAD_DEBUG_ONLY( AssertCreationType( creationType ) );

	char nativeFilename[MaxFilePathLen+1];
	DWORD access, share, create, attrs;

	// make sure we can expand this.
	if (fileOptions&NativePath)
	{
		cpy(nativeFilename, filename);
	}
	else
	{
		if (!ExpandToNativePath(filename, nativeFilename, MaxFilePathLen+1)) 
			return ErrorExpansionFailed;
	}

	access = 0;
	if (accessMode & AccessRead ) 
		access |= GENERIC_READ;
	if (accessMode & AccessWrite) 
		access |= GENERIC_WRITE;

	share = 0;
	if (shareMode & ShareRead ) 
		share |= FILE_SHARE_READ;
	if (shareMode & ShareWrite) 
		share |= FILE_SHARE_WRITE;
	if (shareMode & ShareTemporary) 
		share |= FILE_SHARE_DELETE;

	create = 0;
	switch(creationType)
	{
	case CreateNew: 
		create = 1; 
		break;
	case CreateAlways: 
		create = 2; 
		break;
	case OpenExisting: 
		create = 3; 
		break;
	case OpenAlways: 
		create = 4; 
		break;
	case TruncateExisting: 
		create = 5; 
		break;
	default: 
		return ErrorInvalidArguments; 
		break;
	}

	attrs = 0;

	if (fileOptions & Async)
	{
		attrs |= FILE_FLAG_OVERLAPPED;
//		if (!(accessMode&AccessWrite)) { attrs |= FILE_FLAG_NO_BUFFERING; }
	}

	m_file = CreateFileA(nativeFilename, access, share, 0, create, attrs|FILE_ATTRIBUTE_NORMAL, 0);

	if (m_file != INVALID_HANDLE_VALUE)
	{
		m_flags = fileOptions;
		m_sectorSize = DeviceSectorSize(nativeFilename, (fileOptions&MiscFlagsAll)|NativePath);
		RAD_ASSERT(m_sectorSize != 0);

		if (io)
		{
			io->TriggerStatusChange(file::Success, true);
		}
		return file::Success;
	}

	if (io)
	{
		io->TriggerStatusChange(file::ErrorGeneric, true);
	}

	return file::ErrorGeneric;
}

Result File::Close(AsyncIO* io)
{
	RAD_ASSERT(m_file  != INVALID_HANDLE_VALUE);
#if defined(RAD_OPT_DEBUG)
	if (io)
	{
		RAD_ASSERT_MSG((m_flags&Async), "Async IO object used on a file that was not opened in Async mode!");
	}
#endif
	CloseHandle(m_file);
	m_file = INVALID_HANDLE_VALUE;
	m_flags = FileOptions(0);
	return Success;
}

bool File::CancelIO()
{
	RAD_ASSERT(m_file  != INVALID_HANDLE_VALUE);
	return s_io.Cancel(m_file);
}

bool File::Flush()
{
	RAD_ASSERT(m_file  != INVALID_HANDLE_VALUE);
	return (bool)FlushFileBuffers(m_file);
}

Result File::Read(void *buffer, FPos bytesToRead, FPos *bytesRead, FPos filePos, AsyncIO *io)
{
	RAD_ASSERT(m_file != INVALID_HANDLE_VALUE);
	RAD_ASSERT(m_sectorSize != 0);

#if defined(RAD_OPT_DEBUG)
	if (m_flags & Async)
	{
		RAD_ASSERT_MSG(((m_sectorSize-1)&filePos) == 0, "File accesses must be sector size aligned (filePos)!");
		RAD_ASSERT_MSG(((m_sectorSize-1)&bytesToRead) == 0, "File accesses must be sector size aligned (bytesToRead)!");
		RAD_ASSERT_MSG((((AddrSize)(m_sectorSize-1))&((AddrSize)buffer)) == 0, "Buffer for read operations must be aligned to a sector-size boundary.");
	}
#endif

	Result result;

	if (m_flags & Async)
	{
		file::AsyncIO _io;
		if (!io)
			io = &_io.m_imp;
		RAD_ASSERT_MSG((io->m_o.Internal == 0) && (io->m_o.InternalHigh == 0), "Async IO object still in use!");
		io->TriggerStatusChange(Pending, false);
		io->m_cancel = false;
		io->m_o.Offset = filePos;
		io->m_o.OffsetHigh = 0;
		io->m_bytes = 0;
		io->m_req = bytesToRead;
		io->m_chunkSize = (bytesToRead > IO_CHUNK_SIZE) ? IO_CHUNK_SIZE : bytesToRead;
		io->m_read = true;
		io->m_buffer = (U8*)buffer;
		io->m_file = m_file;

		s_io.QueueIO(io);

		if (io == &_io.m_imp)
		{
			io->WaitForCompletion();
			result = io->Result();
			RAD_ASSERT(result != Pending);
			if ((result == Success || result == ErrorPartial) && bytesRead)
			{
				*bytesRead = io->ByteCount();
			}
		}
		else
		{
			result = Pending;
		}
	}
	else
	{
		RAD_ASSERT(!io);
		DWORD br;

		if ((SetFilePointer(m_file, (DWORD)filePos, 0, FILE_BEGIN) == (DWORD)filePos) &&
            ReadFile(m_file, buffer, bytesToRead, &br, 0))
		{
			result = (br == bytesToRead) ? Success : ErrorPartial;
			if (bytesRead) *bytesRead = (FPos)br;
		}
		else
		{
			result = ErrorGeneric;
		}
	}

	return result;
}

Result File::Write(const void *buffer, FPos bytesToWrite, FPos *bytesWritten, FPos filePos, AsyncIO *io)
{
	RAD_ASSERT(m_file != INVALID_HANDLE_VALUE);
	RAD_ASSERT(m_sectorSize != 0);

#if defined(RAD_OPT_DEBUG)
	if (m_flags & Async)
	{
		RAD_ASSERT_MSG(((m_sectorSize-1)&filePos) == 0, "File accesses must be sector size aligned (filePos)!");
		RAD_ASSERT_MSG((((AddrSize)(m_sectorSize-1))&((AddrSize)buffer)) == 0, "Buffer for write operations must be aligned to a sector-size boundary.");
	}
#endif

	Result result;

	if (m_flags & Async)
	{
		/*if (Size() < filePos+bytesToWrite)
		{
			SetFilePointer(m_file, filePos+bytesToWrite, 0, FILE_BEGIN);
			SetEndOfFile(m_file);
		}*/

		file::AsyncIO _io;
		if (!io) { io = &_io.m_imp; }
		RAD_ASSERT_MSG((io->m_o.Internal == 0) && (io->m_o.InternalHigh == 0), "Async IO object still in use!");
		io->TriggerStatusChange(Pending, false);
		io->m_cancel = false;
		io->m_o.Offset = filePos;
		io->m_o.OffsetHigh = 0;
		io->m_bytes = 0;
		io->m_req = bytesToWrite;
		io->m_chunkSize = (bytesToWrite > IO_CHUNK_SIZE) ? IO_CHUNK_SIZE : bytesToWrite;
		io->m_read = false;
		io->m_buffer = (U8*)buffer;
		io->m_file = m_file;

		s_io.QueueIO(io);

		if (io == &_io.m_imp)
		{
			io->WaitForCompletion();
			result = io->Result();
			RAD_ASSERT(result != Pending);
			if ((result == Success || result == ErrorPartial) && bytesWritten)
			{
				*bytesWritten = io->ByteCount();
			}
		}
		else
		{
			result = Pending;
		}
	}
	else
	{
		RAD_ASSERT(!io);
		DWORD br;

		if ((SetFilePointer(m_file, (DWORD)filePos, 0, FILE_BEGIN) == (DWORD)filePos) &&
            WriteFile(m_file, buffer, bytesToWrite, &br, 0))
		{
			result = (br == bytesToWrite) ? Success : ErrorPartial;
			if (bytesWritten) *bytesWritten = (FPos)br;
		}
		else
		{
			result = ErrorGeneric;
		}
	}

	return result;
}

FPos File::Size() const
{
	RAD_ASSERT(m_file != INVALID_HANDLE_VALUE);
	return GetFileSize(m_file, 0);
}

Search::Search() :
m_search(INVALID_HANDLE_VALUE),
m_recursed(0),
m_flags(SearchFlags(0))
{
	m_root[0] = 0;
	m_findData.cFileName[0] = 0;
	m_dir[0] = 0;
	m_ext[0] = 0;
}

Search::~Search()
{
	if (m_search != INVALID_HANDLE_VALUE)
	{
		Close();
	}
}

bool Search::Open(const char *directory, const char *ext, SearchFlags flags)
{
	RAD_ASSERT(directory && directory[0]);
	RAD_ASSERT(ext && ext[0]);
	RAD_ASSERT(m_flags == 0);
	RAD_ASSERT(m_recursed == 0);
	RAD_ASSERT(m_root[0] == 0);
	RAD_ASSERT(m_search == INVALID_HANDLE_VALUE);
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

	return PrivateOpen("", directory, ext, flags);
}

bool Search::PrivateOpen(const char *root, const char *directory, const char *ext, SearchFlags flags)
{
	RAD_ASSERT(root);
	RAD_ASSERT(directory && directory[0]);
	RAD_ASSERT(ext && ext[0]);
	RAD_ASSERT(m_flags == 0);
	RAD_ASSERT(m_recursed == 0);
	RAD_ASSERT(m_root[0] == 0);
	RAD_ASSERT(m_search == INVALID_HANDLE_VALUE);
	RAD_ASSERT_MSG(len(directory) <= MaxFilePathLen, "Directory path exceeds MaxFilePathLen!");
	RAD_ASSERT_MSG(len(root) <= MaxFilePathLen, "Root path exceeds MaxFilePathLen!");

#if defined(RAD_OPT_DEBUG)
	if (!(flags&NativePath))
	{
		AssertExtension(ext);
	}
#endif

	cpy(m_ext, ext);

	char buff[MaxFilePathLen+1];
	if (flags & NativePath)
	{
		ncpy(buff, directory, MaxFilePathLen+1);
		// make sure it doesn't end in a '/' or '\\'.
		UReg l = (UReg)len(buff);
		if (buff[l-1] == L'\\' || buff[l-1] == L'/') buff[l-1] = 0;
	}
	else
	{
		if (!ExpandToNativePath(directory, buff, MaxFilePathLen+1))
		{
			return false;
		}
	}

	if (flags & Recursive)
	{
		RAD_ASSERT_MSG(len(buff) < MaxFilePathLen-4, "Native directory path exceeds internal buffer. See MaxFilePathLen!");
		cat(buff, "\\*.*");
	}
	else
	{
		RAD_ASSERT_MSG(len(buff) < MaxFilePathLen-2-len(ext), "Native directory path exceeds internal buffer. See MaxFilePathLen!");
		cat(buff, "\\*");
		cat(buff, ext);
	}

	m_search = FindFirstFileA(buff, &m_findData);

	if (m_search != INVALID_HANDLE_VALUE)
	{
		m_flags = flags;
		ncpy(m_root, root, MaxFilePathLen+1);
		ncpy(m_dir, directory, MaxFilePathLen+1);

		UReg l = (UReg)len(m_root);
		if (l > 0 && (m_root[l-1] == '\\' || m_root[l-1] == '/')) 
			m_root[l-1] = 0;
		l = (UReg)len(m_dir);
		if (l > 0 && (m_dir[l-1] == '\\' || m_dir[l-1] == '/'))
			m_dir[l-1] = 0;

		return true;
	}

	return false;
}

bool Search::NextFile(char *filenameBuffer, UReg filenameBufferSize, FileAttributes *fileFlags, xtime::TimeDate *fileTime)
{
	RAD_ASSERT(filenameBuffer);
	RAD_ASSERT(filenameBufferSize > 0);
	RAD_ASSERT(m_search != INVALID_HANDLE_VALUE);

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
	char ext[MaxExtLen+1];

	for (;;)
	{
		RAD_ASSERT(m_recursed == 0);

		if (m_findData.cFileName[0] == 0)
		{
			if (!FindNextFileA(m_search, &m_findData)) 
				return false;
		}

		if (cmp(m_findData.cFileName, ".") && cmp(m_findData.cFileName, "..")) // skip directory links
		{
			// is this a directory?
			if (m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// what do we do with it?
				if (m_flags & DirNames)
				{
					UReg l;

					if (m_root[0] != 0)
					{
						// search this directory.
						l = filenameBufferSize;
						ncpy(filenameBuffer, m_root, (int)l);
						l -= (UReg)len(filenameBuffer);
						if (m_flags & NativePath)
						{
							ncat(filenameBuffer, "\\", (int)l);
						}
						else
						{
							ncat(filenameBuffer, "/", (int)l);
						}
						if (l > 0)
						{
							--l;
							ncat(filenameBuffer, m_findData.cFileName, (int)l);
						}
					}
					else
					{
						ncpy(filenameBuffer, m_findData.cFileName, (int)filenameBufferSize);
					}

#if defined(RAD_OPT_DEBUG)
					if (!(m_flags&NativePath))
					{
						for (l = 0; filenameBuffer[l] != 0; l++)
						{
							RAD_ASSERT(filenameBuffer[l] != '\\');
						}
					}
#endif

					if (fileFlags)
					{
						*fileFlags = Directory;

						if (m_findData.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)
							(*fileFlags) = FileAttributes((*fileFlags) | Hidden);
						if (m_findData.dwFileAttributes&FILE_ATTRIBUTE_READONLY)
							(*fileFlags) = FileAttributes((*fileFlags) | ReadOnly);
						if (m_findData.dwFileAttributes&FILE_ATTRIBUTE_NORMAL)
							(*fileFlags) = FileAttributes((*fileFlags) | Normal);
					}
					if (fileTime)
					{
						SYSTEMTIME tm;
						FileTimeToSystemTime(&m_findData.ftLastWriteTime, &tm);

						fileTime->dayOfMonth = (U8)tm.wDay;
						fileTime->month =      (U8)tm.wMonth;
						fileTime->year =       (U16)tm.wYear;
						fileTime->hour =       (U8)tm.wHour;
						fileTime->minute =     (U8)tm.wMinute;
						fileTime->second =     (U8)tm.wSecond;
						fileTime->dayOfWeek =  (U8)tm.wDayOfWeek;
						fileTime->millis =     (U16)tm.wMilliseconds;
					}

					if (!(m_flags&Recursive))
					{
						m_findData.cFileName[0] = 0; // clear.
#if defined (RAD_OPT_DEBUG)
						if (!(m_flags&NativePath)) AssertFilePath(filenameBuffer, false);
#endif
						return true;
					}
				}
				if (m_flags & Recursive)
				{
					UReg l;
					char root[MaxFilePathLen+1], dir[MaxFilePathLen+1];

					if (m_root[0] != 0)
					{
						// search this directory.
						l = MaxFilePathLen;
						ncpy(root, m_root, (int)l);
						l -= (UReg)len(root);

						if (m_flags & NativePath)
						{
							ncat(root, "\\", (int)l);
						}
						else
						{
							ncat(root, "/", (int)l);
						}

						if (l > 0)
						{
							--l;
							ncat(root, m_findData.cFileName, (int)l);
						}
					}
					else
					{
						ncpy(root, m_findData.cFileName, MaxFilePathLen+1);
					}

#if defined(RAD_OPT_DEBUG)
					if (!(m_flags&NativePath))
					{
						for (l = 0; root[l] != 0; l++)
						{
							RAD_ASSERT(root[l] != '\\');
						}
					}
#endif

					if (m_dir[0] != 0)
					{
						// search this directory.
						l = MaxFilePathLen;
						ncpy(dir, m_dir, (int)l);
						l -= (UReg)len(dir);

						if (m_flags & NativePath)
						{
							ncat(dir, "\\", (int)l);
						}
						else
						{
							ncat(dir, "/", (int)l);
						}

						if (l > 0)
						{
							--l;
							ncat(dir, m_findData.cFileName, (int)l);
						}
					}
					else
					{
						ncpy(dir, m_findData.cFileName, MaxFilePathLen+1);
					}

#if defined(RAD_OPT_DEBUG)
					if (!(m_flags&NativePath))
					{
						for (l = 0; dir[l] != 0; l++)
						{
							RAD_ASSERT(dir[l] != '\\');
						}
					}
#endif

					m_findData.cFileName[0] = 0; // clear.

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
			else if (m_flags & FileNames)
			{
				if (m_flags & Recursive) // search wildcard is all files so we need to filter manually
				{
					if (m_ext[1] != '*' || m_ext[2] != 0) // all files
					{
						FileExt(m_findData.cFileName, ext, MaxExtLen+1);
						if (icmp(ext, m_ext))
						{
							m_findData.cFileName[0] = 0; // clear.
							continue; // filtered.
						}
					}
				}

				UReg l;

				if (m_root[0] != 0)
				{
					// search this directory.
					l = filenameBufferSize;
					ncpy(filenameBuffer, m_root, (int)l);
					l -= (UReg)len(filenameBuffer);

					if (m_flags & NativePath)
					{
						ncat(filenameBuffer, "\\", (int)l);
					}
					else
					{
						ncat(filenameBuffer, "/", (int)l);
					}

					if (l > 0)
					{
						--l;
						ncat(filenameBuffer, m_findData.cFileName, (int)l);
					}
				}
				else
				{
					ncpy(filenameBuffer, m_findData.cFileName, (int)filenameBufferSize);
				}

#if defined(RAD_OPT_DEBUG)
				for (l = 0; filenameBuffer[l] != 0; l++)
				{
					if (!(m_flags&NativePath))
					{
						RAD_ASSERT(filenameBuffer[l] != '\\');
					}
				}
#endif

				if (fileFlags)
				{
					*fileFlags = FileAttributes(0);

					if (m_findData.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)
						(*fileFlags) = FileAttributes((*fileFlags) | Hidden);
					if (m_findData.dwFileAttributes&FILE_ATTRIBUTE_READONLY)
						(*fileFlags) = FileAttributes((*fileFlags) | ReadOnly);
					if (m_findData.dwFileAttributes&FILE_ATTRIBUTE_NORMAL)
						(*fileFlags) = FileAttributes((*fileFlags) | Normal);
				}

				if (fileTime)
				{
					SYSTEMTIME tm;
					FileTimeToSystemTime(&m_findData.ftLastWriteTime, &tm);

					fileTime->dayOfMonth = (U8)tm.wDay;
					fileTime->month =      (U8)tm.wMonth;
					fileTime->year =       (U16)tm.wYear;
					fileTime->hour =       (U8)tm.wHour;
					fileTime->minute =     (U8)tm.wMinute;
					fileTime->second =     (U8)tm.wSecond;
					fileTime->dayOfWeek =  (U8)tm.wDayOfWeek;
					fileTime->millis =     (U16)tm.wMilliseconds;
				}

				m_findData.cFileName[0] = 0; // clear.
#if defined (RAD_OPT_DEBUG)
				if (!(m_flags&NativePath)) AssertFilePath(filenameBuffer, false);
#endif
				return true;
			}
		}
		else
		{
			m_findData.cFileName[0] = 0;
		}
	}
}

bool Search::IsValid()
{
	return m_search != INVALID_HANDLE_VALUE;
}

void Search::Close()
{
	RAD_ASSERT(m_search != INVALID_HANDLE_VALUE);
	if (m_recursed)
	{
		delete m_recursed;
		m_recursed = 0;
	}

	FindClose(m_search);
	m_search = INVALID_HANDLE_VALUE;

	m_flags = SearchFlags(0);
	m_findData.cFileName[0] = 0;
	m_root[0] = 0;
	m_dir[0] = 0;
	m_ext[0] = 0;

}

namespace {

void SetDefaultAliases()
{
	{
		char buff[MaxFilePathLen+1];

		{
			getcwd(buff, MaxFilePathLen);
			buff[MaxFilePathLen] = 0;
			size_t l = len(buff);
			if (buff[0] != 0 && buff[l-1] != '/' && buff[l-1] != '\\')
			{
				cat(buff, "/"); // fake directory seperator.
			}
		}

		for (UReg i = 0; buff[i] != 0; i++)
		{
			if (buff[i] == '\\') 
				buff[i] = '/';
		}

		// backup and find the last directory separator.
		if (buff[0] != 0)
		{
			bool found = false;
			UReg l = (UReg)len(buff);
			while (l-- > 0)
			{
				if (buff[l] == '/')
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

	bool setHDD, setCDDVD;
	char path[4], i;
	const char firstDrive = 'C';
	const char lastDrive  = 'Z';

	DWORD driveList;

	setHDD = setCDDVD = false;

	driveList = GetLogicalDrives();
	for (i = firstDrive; i <= lastDrive && (!setHDD || !setCDDVD); i++)
	{
		DWORD bit = (DWORD)(1<<(i-'A'));

		if (driveList & bit) // drive exists
		{
			path[0] = i;
			path[1] = ':';
			path[2] = '\\';
			path[3] = 0;

			DWORD type = GetDriveTypeA(path);

			path[0] = 'a' + (i - 'A');
			path[2] = 0;

			if (type == DRIVE_CDROM && !setCDDVD)
			{
				setCDDVD = true;
#if defined(RAD_OPT_DEBUG)
				details::UncheckedSetAlias(AliasCDDVD, path);
#else
				SetAlias(AliasCDDVD, path);
#endif
			}
			else if (type == DRIVE_FIXED && !setHDD)
			{
				setHDD = true;
#if defined(RAD_OPT_DEBUG)
				details::UncheckedSetAlias(AliasHDD, path);
#else
				SetAlias(AliasHDD, path);
#endif
			}
		}
	}
}

} // namespace
} // details

RADRT_API bool RADRT_CALL ExpandToNativePath(const char *portablePath, char *nativePath, UReg nativePathBufferSize)
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

	if (ExpandAliases(portablePath, nativePath, nativePathBufferSize))
	{
		while (nativePath[0] != 0)
		{
			if (nativePath[0] == '/') nativePath[0] = '\\';
			nativePath++;
		}

		return true;
	}
	else
	{
		return false;
	}
}

// returns number of characters that would be written to buffer via ExpandPath if successful (excluding null terminator),
// or 0 if error (or path is a null string).

RADRT_API UReg RADRT_CALL ExpandToNativePathLength(const char *portablePath)
{
	RAD_ASSERT(portablePath);

#if defined(RAD_OPT_DEBUG)
	details::AssertFilePath(portablePath, true);
#endif
	return ExpandAliasesLength(portablePath);
}

// Returns the sector size of the device that the given file path resides on. Note this
// doesn't have to be a filename, it can be a path, or an alias.

RADRT_API FPos RADRT_CALL DeviceSectorSize(const char *path, int flags)
{
	RAD_ASSERT(path);
	DWORD bytesPerSector = 0;

	char buff[MaxFilePathLen+1];

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

	// is a drive now, just need the letter.
	buff[2] = '\\';
	buff[3] = 0;

	{
		DWORD temp;
		if (!GetDiskFreeSpaceA(buff, &temp, &bytesPerSector, &temp, &temp))
		{
			bytesPerSector = 0;
		}
	}

	return (FPos)bytesPerSector;
}

RADRT_API bool RADRT_CALL DeleteFile(const char *path, int flags)
{
	RAD_ASSERT(path);

	if (flags & NativePath)
	{
		return (bool)::DeleteFileA(path);
	}
	else
	{
		#if defined(RAD_OPT_DEBUG)
			details::AssertFilePath(path, true);
		#endif

		char buff[MaxFilePathLen+1];
		if (ExpandToNativePath(path, buff, MaxFilePathLen+1))
		{
			return ::DeleteFileA(buff) != 0;
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
		if (nativePath[ofs] == '\\')
		{
			::string::ncpy(buff, nativePath, ofs+1);
			buff[ofs] = 0;
			if (buff[1] != ':' || ofs > 2) // not drive letter
			{
				if (!::CreateDirectoryA(buff, 0))
				{
					if (::GetLastError() != ERROR_ALREADY_EXISTS) 
						return false;
				}
			}
		}
		++ofs;
	}

	bool b = true;

	if (nativePath[1] != ':' || len(nativePath) > 2)
	{
		b = ::CreateDirectoryA(nativePath, 0);
		if (!b) b = ::GetLastError() == ERROR_ALREADY_EXISTS;
	}
	return b;
}

#undef CreateDirectory
RADRT_API bool RADRT_CALL CreateDirectory(const char *path, int flags)
{
	RAD_ASSERT(path);

	if (flags & NativePath)
	{
		return PrivateCreateDirectory(path);
	}
	else
	{
		char buff[MaxFilePathLen+1];
		if (ExpandToNativePath(path, buff, MaxFilePathLen+1))
		{
			return PrivateCreateDirectory(buff);
		}
		else
		{
			return false;
		}
	}
}

static bool DeleteDirectory_r(const char *nativePath)
{
	RAD_ASSERT(nativePath);

	Search s;

	size_t l = ::string::len(nativePath);

	if (s.Open(nativePath, ".*", SearchFlags(NativePath|FileNames|DirNames)))
	{
		char file[MaxFilePathLen+1], path[MaxFilePathLen+1];
		FileAttributes fa;
		while (s.NextFile(file, MaxFilePathLen+1, &fa, 0))
		{
			::string::cpy(path, nativePath);
			if (nativePath[l] != '\\') 
				::string::cat(path, "\\");
			::string::cat(path, file);

			if (fa & Directory)
			{
				// recurse!
				if (!DeleteDirectory_r(path)) 
					return false;
			}
			else
			{
				if (!::DeleteFileA(path)) 
					return false;
			}
		}

		s.Close();
	}

	return ::RemoveDirectoryA(nativePath);
}

RADRT_API bool RADRT_CALL DeleteDirectory(const char *path, int flags)
{
	RAD_ASSERT(path);

	if (flags & NativePath)
	{
		return DeleteDirectory_r(path);
	}
	else
	{
		char buff[MaxFilePathLen+1];
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


RADRT_API bool RADRT_CALL FileTime(const char *path, xtime::TimeDate* td, int flags)
{
	RAD_ASSERT(path);

	HANDLE h = INVALID_HANDLE_VALUE;
	if (flags & NativePath)
	{
		h = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
			0, 3, FILE_ATTRIBUTE_NORMAL, 0 );
	}
	else
	{
		#if defined(RAD_OPT_DEBUG)
			details::AssertFilePath(path, true);
		#endif

		char buff[MaxFilePathLen+1];
		if (ExpandToNativePath(path, buff, MaxFilePathLen+1))
		{
			h = CreateFileA( buff, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
					0, 3, FILE_ATTRIBUTE_NORMAL, 0 );
		}
	}

	if (h != INVALID_HANDLE_VALUE)
	{
		FILETIME ft;
		SYSTEMTIME st;

		if( GetFileTime( h, 0, 0, &ft ) )
		{
			FileTimeToLocalFileTime( &ft, &ft );
			FileTimeToSystemTime( &ft, &st );

			td->dayOfMonth =	(U8)st.wDay;
			td->month =			(U8)st.wMonth;
			td->year =			(U16)st.wYear;
			td->hour =			(U8)st.wHour;
			td->minute =		(U8)st.wMinute;
			td->second =		(U8)st.wSecond;
			td->dayOfWeek =		(U8)st.wDayOfWeek;
			td->millis =		(U16)st.wMilliseconds;

			CloseHandle( h );
		}
		else
		{
			CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
	}

	return h != INVALID_HANDLE_VALUE;
}

RADRT_API bool RADRT_CALL FileExists(const char *path, int flags)
{
	RAD_ASSERT(path);

	if (flags & NativePath)
	{
		return ::GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
	}
	else
	{
		RAD_DEBUG_ONLY(details::AssertFilePath(path, true));

		char buff[MaxFilePathLen+1];
		if (ExpandToNativePath(path, buff, MaxFilePathLen+1))
		{
			return ::GetFileAttributesA(buff) != INVALID_FILE_ATTRIBUTES;
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
	s_io.Process();
}

} // details
} // file

