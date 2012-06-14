// File.inl
// Platform Agnostic File System (inlines)
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../Utils.h"


namespace file {

//////////////////////////////////////////////////////////////////////////////////////////
// file::Search
//////////////////////////////////////////////////////////////////////////////////////////

inline Search::Search()
{
}

inline Search::~Search()
{
}

inline bool Search::Open(
	const char *directory,
	const char *extWithPeriod,
	SearchFlags flags
)
{
	return m_imp.Open(
		directory,
		extWithPeriod,
		flags
	);
}

inline bool Search::NextFile(
	char *filenameBuffer,
	UReg filenameBufferSize,
	FileAttributes *fileFlags,
	xtime::TimeDate *fileTime
)
{
	return m_imp.NextFile(
		filenameBuffer,
		filenameBufferSize,
		fileFlags,
		fileTime
	);
}

inline bool Search::IsValid()
{
	return m_imp.IsValid();
}

inline void Search::Close()
{
	m_imp.Close();
}

//////////////////////////////////////////////////////////////////////////////////////////
// file::AsyncIO
//////////////////////////////////////////////////////////////////////////////////////////

inline AsyncIO::AsyncIO()
{
}

inline AsyncIO::~AsyncIO()
{
}

inline Result AsyncIO::Result() const
{
	return m_imp.Result();
}

inline void AsyncIO::Cancel()
{
	m_imp.Cancel();
}

inline bool AsyncIO::IsCancelled()
{
	return m_imp.IsCancelled();
}

inline bool AsyncIO::WaitForCompletion(UReg timeout) const
{
	return m_imp.WaitForCompletion(timeout);
}

inline FPos AsyncIO::ByteCount() const
{
	return m_imp.ByteCount();
}

inline void AsyncIO::SetByteCount(FPos count)
{
	m_imp.SetByteCount(count);
}

inline void AsyncIO::TriggerStatusChange(
	file::Result result,
	bool force
)
{
	m_imp.TriggerStatusChange(
		result,
		force
	);
}

inline void AsyncIO::OnComplete(file::Result result)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// file::File
//////////////////////////////////////////////////////////////////////////////////////////

inline File::File()
{
}

inline File::~File()
{
}

inline Result File::Open(
	const char *filename,
	CreationType creationType,
	AccessMode accessMode,
	ShareMode shareMode,
	FileOptions fileOptions,
	AsyncIO *io
)
{
	return m_imp.Open(
		filename,
		creationType,
		accessMode,
		shareMode,
		fileOptions,
		(io) ? (&(io->m_imp)) : 0
	);
}

inline Result File::Close(AsyncIO* io)
{
	return m_imp.Close((io) ? (&(io->m_imp)) : 0);
}

inline Result File::Read(
	void *buffer,
	FPos bytesToRead,
	FPos *bytesRead,
	FPos filePos,
	AsyncIO *io
)
{
	return m_imp.Read(
		buffer,
		bytesToRead,
		bytesRead,
		filePos,
		(io) ? (&(io->m_imp)) : 0
	);
}

inline Result File::Write(
	const void *buffer,
	FPos bytesToWrite,
	FPos* bytesWritten,
	FPos filePos,
	AsyncIO* io
)
{
	return m_imp.Write(
		buffer,
		bytesToWrite,
		bytesWritten,
		filePos,
		(io) ? (&(io->m_imp)) : 0
	);
}

inline FPos File::Size() const
{
	return m_imp.Size();
}

inline FPos File::SectorSize() const
{
	return m_imp.SectorSize();
}

inline bool File::CancelIO()
{
	return m_imp.CancelIO();
}

inline bool File::Flush()
{
	return m_imp.Flush();
}

inline void *File::IOMalloc(AddrSize size, AddrSize *ioSize, Zone &zone)
{
	RAD_ASSERT(size);
	size = Align(size, SectorSize());
	if (ioSize) { *ioSize = size; }
	return zone_malloc(zone, size, 0, SectorSize());
}

inline void *File::SafeIOMalloc(AddrSize size, AddrSize *ioSize, Zone &zone)
{
	void *p = IOMalloc(size, ioSize, zone);
	RAD_VERIFY(p);
	return p;
}

inline void File::IOFree(void *p)
{
	zone_free(p);
}

} // file

