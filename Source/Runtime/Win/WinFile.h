// WinFile.h
// File System for Windows 95/98/2000/XP.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "WinHeaders.h"
#include "../TimeDef.h"
#include "../Thread/Locks.h"
#include "../PushPack.h"


namespace file {

enum
{
	MaxAliasLen   = 255,
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

	bool Open(const char* directory,
		const char* extWithPeriod,
		SearchFlags flags
	);

	bool NextFile(
		char* filenameBuffer,
		UReg filenameBufferSize,
		FileAttributes* fileFlags,
		xtime::TimeDate* fileTime
	);

	void Close();

	bool PrivateOpen(
		const char* root,
		const char* directory,
		const char* extWithPeriod,
		SearchFlags flags
	);

	bool IsValid();

	char m_root[MaxFilePathLen+1];
	char m_dir[MaxFilePathLen+1];
	char m_ext[MaxExtLen+1];
	HANDLE m_search;
	WIN32_FIND_DATAA m_findData;
	Search* m_recursed;
	SearchFlags m_flags;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Asynchronous IO management
//////////////////////////////////////////////////////////////////////////////////////////

class File;
class IO;
class RADRT_CLASS AsyncIO
{
	friend class file::AsyncIO;

	AsyncIO();
	~AsyncIO();

	Result Result() const { return m_status; }
	void Cancel() { m_cancel = true; }
	bool WaitForCompletion(U32 timeout = thread::Infinite) const;
	FPos ByteCount() const { return m_bytes; }
	void TriggerStatusChange(file::Result result, bool force);
	bool IsCancelled() { return m_cancel; }
	void SetByteCount(FPos count) { m_bytes = count; }

	mutable OVERLAPPED          m_o;
	volatile FPos               m_bytes;
	FPos                        m_req;
	FPos                        m_chunkSize;
	HANDLE                      m_file;
	volatile file::Result  m_status;
	bool                        m_read;
	volatile bool               m_cancel;
	U8*                         m_buffer;
	AsyncIO*                    m_nextStart;
	mutable thread::Gate   m_gate;

	static void CALLBACK IOCompletion(DWORD errorCode, DWORD numBytes, LPOVERLAPPED ovr);

	friend class RADRT_CLASS File;
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
		const char* filename,
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

	HANDLE m_file;
	FileOptions m_flags;
	FPos   m_sectorSize;
};

} // details
} // file


#include"../PopPack.h"
