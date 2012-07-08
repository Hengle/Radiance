// PosixFile.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../TimeDef.h"
#include "../Thread/Locks.h"
#include "../PushPack.h"

#if defined(RAD_OPT_POSIXAIO)
	#include "PosixAIO.h"
#else
	#include "PosixFadvise.h"
#endif

struct dirent;

namespace file {

enum
{
	MaxAliasLen    = 255,
	MaxFilePathLen = 255,
	MaxExtLen      = 31
};

namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// File searching
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Search
{
private:
	friend class file::Search;

	Search();
	~Search();

	bool Open(const wchar_t* directory,
		const wchar_t* extWithPeriod,
		SearchFlags flags
	);

	bool NextFile(
		wchar_t* filenameBuffer,
		UReg filenameBufferSize,
		FileAttributes* fileFlags,
		xtime::TimeDate* fileTime
	);

	void Close();

	bool PrivateOpen(
		const wchar_t* root,
		const wchar_t* directory,
		const wchar_t* extWithPeriod,
		SearchFlags flags
	);

	bool IsValid();

	size_t  m_trimLen;
	wchar_t m_root[MaxFilePathLen+1];
	wchar_t m_dir[MaxFilePathLen+1];
	wchar_t m_ext[MaxExtLen+1];
	void *m_sdir;
	struct dirent *m_cur;
	Search* m_recursed;
	SearchFlags m_flags;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Asyncronous IO management
//////////////////////////////////////////////////////////////////////////////////////////
class File;
class IO;
class RADRT_CLASS AsyncIO : public
#if defined(RAD_OPT_POSIXAIO)
posix_aio::AIO
#else
_posix_fadvise::AIO
#endif
{
	friend class file::AsyncIO;

	AsyncIO();
	virtual ~AsyncIO();

	file::Result Result() const { return m_status; }
	void Cancel() { AIO::Cancel(); m_cancel = true; }
	bool WaitForCompletion(U32 timeout=thread::Infinite) const;
	FPos ByteCount() const { return m_bytes; }
	void TriggerStatusChange(file::Result result, bool force);
	bool IsCancelled() { return m_cancel; }
	void SetByteCount(FPos count) { m_bytes = count; }
	void Go(int bytes);
	virtual void OnComplete(int bytes, int error);

	volatile FPos               m_bytes;
	FPos                        m_req;
	FPos                        m_chunkSize;
	FPos                        m_ofs;
	FPos                        m_fsize;
	int                         m_fd;
	volatile file::Result  m_status;
	bool                        m_read;
	volatile bool               m_cancel;
	U8*                         m_buffer;
	mutable thread::Gate   m_gate;

	friend class File;
	friend class file::details::IO;
};

//////////////////////////////////////////////////////////////////////////////////////////
// file::File
//////////////////////////////////////////////////////////////////////////////////////////
//
// This file object operates in two modes: asyncronous IO mode, and
// syncronous IO mode.
//
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS File
{
	friend class file::File;

	File();
	~File();

	Result Open(
		const wchar_t* filename,
		CreationType creationType,
		AccessMode accessMode,
		ShareMode shareMode,
		FileOptions fileOptions,
		AsyncIO* io
	);

	Result Close(AsyncIO* io);

	Result Read (
		void* buffer,
		FPos bytesToRead,
		FPos* bytesRead,
		FPos filePos,
		AsyncIO* io
	);

	Result Write(
		const void* buffer,
		FPos bytesToWrite,
		FPos* bytesWritten,
		FPos filePos,
		AsyncIO* io
	);

	FPos Size() const;
	FPos SectorSize() const { return m_sectorSize; }
	bool CancelIO();
	bool Flush();

	int m_fd;
	FPos m_sectorSize;
	FileOptions m_flags;
};

} // details
} // file


#include"../PopPack.h"
