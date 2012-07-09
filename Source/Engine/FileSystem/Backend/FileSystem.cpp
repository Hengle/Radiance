// FileSystem.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "FileSystem.h"
#include "../../Zones.h"
#include <algorithm>
#include <stdlib.h>
#undef min
#undef max
#include <Runtime/PushSystemMacros.h>

using namespace string;

namespace file {
namespace details {

RAD_ZONE_DEF(RADENG_API, ZFS, "FileSystem", ZEngine);

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(FileData, file.details.FileData);

FileData::FileData() : AtomicRefCount(1)
{
}

FPos FileData::RAD_IMPLEMENT_GET(size)
{
	return m_size;
}

const void* FileData::RAD_IMPLEMENT_GET(ptr)
{
	return (const void *)m_data;
}

bool FileData::MyOnZeroReferences()
{
	if (m_realData != 0)
		zone_free((void*)m_realData);

	HFileSystem temp(m_fs);
	InterfaceComponent<FileSystem>(temp)->m_fileDataPool.Destroy(this);
	return true; // handled.
}

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(StreamInputBuffer, file.details.StreamInputBuffer);

StreamInputBuffer::StreamInputBuffer() :
m_pos(0),
m_spos(0),
m_ofs(0),
m_valid(false),
AtomicRefCount(1)
{
}

void StreamInputBuffer::Bind(const HFile &file)
{
	m_file = file;
	m_valid = false;
	m_pos = 0;
	m_ofs = 0;
	m_spos = 0;
	m_io->Cancel(); // cancel any pending IO.
	m_io->WaitForCompletion();
}

::stream::SPos StreamInputBuffer::Read(void* buff, ::stream::SPos numBytes, UReg* errorCode)
{
	::stream::SPos bytesLeft = numBytes;
	U8 *dst = static_cast<U8*>(buff);

	::stream::SetErrorCode(errorCode, ::stream::Success);

	while (bytesLeft)
	{
		if (m_valid) // valid data exists in buffer.
		{
			HFileData fd = m_io->data;

			FPos avail = fd->size - m_ofs;
			if (avail)
			{
				avail = std::min<FPos>(avail, (FPos)bytesLeft);
				const U8 *src = static_cast<const U8*>((const void*)fd->ptr) + m_ofs;
				memcpy(dst, src, avail);
				bytesLeft -= (::stream::SPos)avail;
				dst += avail;
				m_ofs += avail;
				m_spos += avail;
			}
			else
			{
				m_valid = false;
			}
		}

		if (bytesLeft)
		{
			// we have exhausted our buffered data, read more.
			FPos sizeToRead = m_file->size - m_pos;
			sizeToRead = std::min<FPos>(sizeToRead, m_io->bufferSize);
			if (sizeToRead < 1) 
				break;
			Result r = m_file->Read(m_io, m_pos, sizeToRead, HIONotify());
			if (r != Success && r != Pending)
			{
				break;
			}
			
			m_io->WaitForCompletion();
			r = m_io->result;
						
			if (r != Success &&
				r != ErrorPartial)
			{
				break;
			}
			m_ofs = 0;
			if (m_io->data->size < 1) 
				break; // we read no data abort.
			m_pos += m_io->data->size;
			m_valid = true;
		}
	}

	if (bytesLeft > 0) 
		::stream::SetErrorCode(errorCode, ::stream::ErrorUnderflow);
	return numBytes - bytesLeft;
}

bool StreamInputBuffer::SeekIn(::stream::Seek seekType, ::stream::SPos ofs, UReg* errorCode)
{
	::stream::SPos newPos;
	::stream::SetErrorCode(errorCode, ::stream::Success);
	if (!::stream::CalcSeekPos(seekType, ofs, InPos(), Size(), &newPos))
	{
		::stream::SetErrorCode(errorCode, ::stream::ErrorBadSeekPos);
		return false;
	}
	m_valid = m_valid && (m_pos == (FPos)newPos);
	m_pos = (FPos)newPos;
	m_spos = m_pos;
	return true;
}

::stream::SPos StreamInputBuffer::InPos() const
{
	return (::stream::SPos)m_spos;
}

::stream::SPos StreamInputBuffer::Size()  const
{
	return m_file->size;
}

UReg StreamInputBuffer::InCaps() const
{
	return ::stream::CapSeekInput | ::stream::CapSizeInput;
}

UReg StreamInputBuffer::InStatus() const
{
	return ::stream::StatusInputOpen;
}

::stream::IInputBuffer& StreamInputBuffer::RAD_IMPLEMENT_GET(buffer)
{
	return *const_cast<StreamInputBuffer*>(this);
}

bool StreamInputBuffer::MyOnZeroReferences()
{
	HFileSystem temp(m_fs);
	InterfaceComponent<FileSystem>(temp)->m_streamInputBufferPool.Destroy(this);
	return true; // handled.
}

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(BufferedDiskIO, file.details.BufferedDiskIO);

BufferedDiskIO::BufferedDiskIO() :
	AtomicRefCount(1),
	m_data(0),
	m_size(0),
	m_reqSize(0),
	m_ofs(0),
	m_dataSize(0),
	m_reqDataSize(0),
	m_alignment(0),
	m_dataAlignment(0),
	m_pending(false),
	m_internalIO(0)
{
}

void BufferedDiskIO::Cancel()
{
	file::AsyncIO::Cancel();

	Lock l(m_m);
	if (m_internalIO)
	{
		m_internalIO->CancelInternalIO();
		m_internalIO = 0;
	}
}

void BufferedDiskIO::WaitForCompletion()
{
	file::AsyncIO::WaitForCompletion();
}

FPos BufferedDiskIO::RAD_IMPLEMENT_GET(byteCount)
{
	return file::AsyncIO::ByteCount();
}

Result BufferedDiskIO::RAD_IMPLEMENT_GET(result)
{
	file::Result r = file::AsyncIO::Result();
	if (r == ErrorPartial)
	{
		// ErrorPartial may occur if the read size was aligned to a sector boundry
		// (i.e. we asked for more data than the file had).
		// check to make sure we read the required size.
		if (file::AsyncIO::ByteCount() >= m_reqSize)
		{
			// if this occurs, then we read at least as many bytes as were in the file,
			// so tell the user it was Success (i.e. the user should not have
			// to check if ErrorPartial was only because of our sector aligned read).
			r = Success;
		}
	}

	return r;
}

HFileData BufferedDiskIO::SetupFileData()
{
	FileData* fd = 0;

	if (m_fd)
	{
		fd = InterfaceComponent<FileData>(m_fd);
	}
	else
	{
		fd = InterfaceComponent<FileSystem>(m_fs)->m_fileDataPool.Construct();
		if (!fd)
		{
			RAD_OUT_OF_MEM(fd);
			return HFileData();
		}
		fd->m_fs = m_fs;
		m_fd = fd; // keep a reference.
		fd->m_realData = m_data;
		m_data = 0; // clear this so we don't free it later.
	}

	RAD_ASSERT(fd);
	fd->m_data = static_cast<U8*>((void*)fd->m_realData) + m_ofs;
	fd->m_size = m_size;

	return m_fd;
}

HFileData BufferedDiskIO::RAD_IMPLEMENT_GET(data)
{
	file::Result status = file::AsyncIO::Result();
	if (status == Success ||
		status == ErrorPartial)
	{
		return m_fd;
	}

	return 0;
}

FPos BufferedDiskIO::RAD_IMPLEMENT_GET(ioSize)
{
	return m_size;
}

FPos BufferedDiskIO::RAD_IMPLEMENT_GET(bufferSize)
{
	return m_reqDataSize;
}

FPos BufferedDiskIO::RAD_IMPLEMENT_GET(alignment)
{
	return m_dataAlignment;
}

void BufferedDiskIO::OnComplete(file::Result result)
{
	if (result == Success ||
		result == ErrorPartial)
	{
		FileData *fd = InterfaceComponent<FileData>(m_fd);
		
		if (result == ErrorPartial)
		{
			if (file::AsyncIO::ByteCount() < m_size)
			{
				m_size = file::AsyncIO::ByteCount(); // we read less than the actual file size.
				fd->m_size = m_size;
			}
		}
		
		if (m_ofs & (m_alignment-1))
		{
			// The user loaded data with a specified alignment setting. The first priority was to load
			// the data off disk using the enforced alignment of an accelerated read, which means that
			// the data the user was interested in was not actually the first bytes of the data loaded
			// (i.e. we had to backup the requested file offset in order to align to the file sector).
			memmove((void*)fd->m_realData, (void*)fd->m_data, fd->m_size);
			fd->m_data = fd->m_realData;
			RAD_ASSERT(m_size == fd->m_size);
		}
		
		RAD_ASSERT(IsAligned(fd->m_data, m_alignment));
	}

	m_pending = false;

	HIONotify notify(m_notify);

	//
	// NOTE: this may cause this object to be deleted.
	//
	m_file.Close(); // close file (don't need it anymore).

	if (notify)
	{
		notify->Notify(result);
		notify.Close();
	}
}

bool BufferedDiskIO::MyOnZeroReferences()
{
	if (m_pending)
	{
		Cancel();
		WaitForCompletion();
		RAD_ASSERT_MSG(!m_pending, "IO object is being released while there is still pending file IO!");
	}

	if (m_data != 0)
	{
		zone_free((void*)m_data);
	}

	HFileSystem temp(m_fs);
	InterfaceComponent<FileSystem>(temp)->m_bufferedDiskIOPool.Destroy(this);
	return true; // handled.
}

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(DiskFile, file.details.DiskFile);

DiskFile::DiskFile() :
	AtomicRefCount(1),
	m_sectorSize(0),
	m_size(0),
	m_numSectors(0),
	m_media(0)
{
}

Result DiskFile::Read(const HBufferedAsyncIO &hio, FPos fileOffset, FPos sizeToRead, const HIONotify &ioComplete)
{
	RAD_ASSERT(hio);
	BufferedDiskIO *io = InterfaceComponent<BufferedDiskIO>(hio);
	RAD_ASSERT_MSG(io->m_dataAlignment >= m_sectorSize, "Specified IO objects alignment is insufficient for this file!");
	RAD_ASSERT_MSG(io->m_pending == false, "Specified IO object is in use!");
	RAD_ASSERT_MSG(io->m_reqDataSize >= sizeToRead, "Read size is greater than specified IO object buffer size!");

	Reference();
	io->m_file = this;
	io->m_reqSize = io->m_size = sizeToRead;
	io->m_notify = ioComplete;
	io->m_pending = true;

	// move file position backwards to align it (very important!).
	FPos endPos = fileOffset + sizeToRead;
	FPos startPos = fileOffset - (fileOffset & (m_sectorSize-1));

	io->m_reqSize = endPos - startPos;
	io->m_ofs = fileOffset - startPos;

	sizeToRead = (FPos)Align(io->m_reqSize, m_sectorSize);

	RAD_ASSERT_MSG(sizeToRead <= io->m_dataSize, "Requested read size exceeds aligned buffer capacity!");
	
	HFileData hfd = io->SetupFileData();
	FileData *fd = InterfaceComponent<FileData>(hfd);
	
	RAD_ASSERT(fd->m_realData);

	return m_file.Read((void*)fd->m_realData, sizeToRead, 0, startPos, io);
}

Result DiskFile::Load(
	file::HBufferedAsyncIO &asyncIO, 
	const file::HIONotify &ioComplete, 
	file::FPos alignment, 
	Zone &zone)
{
	asyncIO = m_fs->CreateBufferedIO(m_size, alignment, zone);
	if (!asyncIO) 
		return ErrorOutOfMemory;

	BufferedDiskIO *io = InterfaceComponent<BufferedDiskIO>(asyncIO);
	Reference();
	io->m_file = this;
	return Read(asyncIO, 0, m_size, ioComplete);
}

FPos DiskFile::RAD_IMPLEMENT_GET(size)
{
	return m_size;
}

bool DiskFile::MyOnZeroReferences()
{
	HFileSystem temp(m_fs);
	InterfaceComponent<FileSystem>(temp)->m_diskFilePool.Destroy(this);
	return true; // handled.
}

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(PakSearch, file.details.PakSearch);

PakSearch::PakSearch() :
	AtomicRefCount(1)
{
}

void PakSearch::Initialize(PakList *pakList, const HFileSystem &fs, int media)
{
	m_paks = pakList;
	m_fs = fs;
	m_media = media;
}

bool PakSearch::OpenSearch(const char *path, const char *extIncludingPeriod)
{
	if (m_paks->size() > 0)
	{
		RAD_STL_FOREACH(PakList::iterator, it, *m_paks)
		{
			m_search = (*it)->OpenSearch(path, extIncludingPeriod);
			if (m_search)
			{
				m_path = path;
				m_ext  = extIncludingPeriod;
				m_last = (it+1);
				return true;
			}
		}
	}
	return false;
}

bool PakSearch::NextFile(String &outFilename)
{
	RAD_ASSERT(m_search);
	for (;;)
	{
next_file:
		if (!m_search->NextFile(outFilename))
		{
			for (PakList::iterator it = m_last; it != m_paks->end(); ++it)
			{
				m_search = (*it)->OpenSearch(m_path.c_str, m_ext.c_str);
				if (m_search)
				{
					m_last = (it+1);
					goto next_file;
				}
			}

			return false;
		}
		else
		{
			if (m_media & (Mod|HDD|CDDVD))
			{
				// do we need to check any other media (MOD / HDD / CDDVD will override
				// a pak).
				if (!m_fs->FileExists(outFilename.c_str, int(m_media & ~Paks)))
				{
					break;
				}
			}
		}
	}

	return true;
}

bool PakSearch::MyOnZeroReferences()
{
	HFileSystem temp(m_fs);
	InterfaceComponent<FileSystem>(temp)->m_pakSearchPool.Destroy(this);
	return true; // handled.
}

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(DiskSearch, file.details.DiskSearch);

DiskSearch::DiskSearch() : AtomicRefCount(1)
{
}

bool DiskSearch::OpenSearch(const char* path, const char* extIncludingPeriod)
{
	return m_search.Open(path, extIncludingPeriod, SearchFlags(FileNames|Recursive));
}

bool DiskSearch::NextFile(String &outFilename)
{
	char buffer[MaxFilePathLen+1];
	if (m_search.NextFile(buffer, MaxFilePathLen+1))
	{
		outFilename = buffer;
		return true;
	}
	return false;
}

bool DiskSearch::MyOnZeroReferences()
{
	HFileSystem temp(m_fs);
	InterfaceComponent<FileSystem>(temp)->m_diskSearchPool.Destroy(this);
	return true; // handled.
}

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(CSearch, file.details.CSearch);

CSearch::CSearch() :
	AtomicRefCount(1),
	m_curMedia(int(0))
{
}

void CSearch::Initialize(PakList *pakList, const HFileSystem &fs, int media)
{
	m_fs = fs;
	m_paks = pakList;
	m_media = media;
}

bool CSearch::OpenSearch(const char *path, const char *extIncludingPeriod)
{
	RAD_ASSERT(path);
	RAD_ASSERT(extIncludingPeriod);

	if ((m_media & AllMedia) != 0)
	{
		m_path = path;
		m_ext = extIncludingPeriod;

		// select initial media.

		if (m_media & Mod)
			m_curMedia = Mod;
		else if (m_media & HDD)
			m_curMedia = HDD;
		else if (m_media & CDDVD)
			m_curMedia = CDDVD;
		else
			m_curMedia = Paks;

		while(!MediaSearch())
		{
			if (!SelectNextMedia()) return false;
		}

		return true;
	}

	return false;
}

bool CSearch::NextFile(String &outFilename)
{
	for (;;)
	{
		if (!m_search->NextFile(outFilename))
		{
			bool selected = false;
			while (SelectNextMedia())
			{
				if (MediaSearch())
				{
					selected = true;
					break;
				}
			}
			if (!selected) return false;
		}
		else
		{
			// MOD shadows the HDD & CDDVD
			if ((m_curMedia == HDD || m_curMedia == CDDVD) && (m_media & Mod))
			{
				if (!m_fs->FileExists(outFilename.c_str, Mod)) break;
			}
			// HDD shadows CDDVD
			else if (m_curMedia == CDDVD && (m_media & HDD))
			{
				if (!m_fs->FileExists(outFilename.c_str, HDD)) break;
			}
			else
			{
				break;
			}
		}
	}
	return true;
}

bool CSearch::SelectNextMedia()
{
	if ((m_curMedia == Mod) && (m_media & HDD))
	{
		m_curMedia = HDD;
	}
	else if (((m_curMedia == Mod) || (m_curMedia == HDD)) && (m_media & CDDVD))
	{
		m_curMedia = CDDVD;
	}
	else if (((m_curMedia == Mod) || (m_curMedia == CDDVD) || (m_curMedia == HDD)) &&
		(m_media & Paks))
	{
		m_curMedia = Paks;
	}
	else
	{
		m_curMedia = 0;
	}

	return m_curMedia != 0;
}

bool CSearch::MediaSearch()
{
	FileSystem *fs = InterfaceComponent<FileSystem>(m_fs);
	if (m_curMedia == Mod)
	{
		String path;
		if (!m_path.empty)
			path = fs->m_modPathSlash + m_path;
		else
			path = fs->m_modPath;

		DiskSearch* ds = fs->m_diskSearchPool.Construct();
		if (!ds)
		{
			RAD_OUT_OF_MEM(ds);
			return false;
		}
		ds->m_fs = m_fs;
		if (ds->OpenSearch(path.c_str, m_ext.c_str))
		{
			m_search = ds;
			return true;
		}
		ds->Release();
	}
	else if (m_curMedia == HDD)
	{
		String path;
		if (!m_path.empty)
			path = fs->m_hddPathSlash + m_path;
		else
			path = fs->m_hddPath;

		DiskSearch* ds = fs->m_diskSearchPool.Construct();
		RAD_OUT_OF_MEM(ds);
		ds->m_fs = m_fs;
		if (ds->OpenSearch(path.c_str, m_ext.c_str))
		{
			m_search = ds;
			return true;
		}
		ds->Release();
	}
	else if(m_curMedia == CDDVD)
	{
		String path;
		if (!m_path.empty)
			path = fs->m_cddvdPathSlash + m_path;
		else
			path = fs->m_cddvdPath;

		DiskSearch* ds = fs->m_diskSearchPool.Construct();
		RAD_OUT_OF_MEM(ds);
		ds->m_fs = m_fs;
		if (ds->OpenSearch(path.c_str, m_ext.c_str))
		{
			m_search = ds;
			return true;
		}
		ds->Release();
	}
	else if (m_curMedia == Paks)
	{
		PakSearch* ps = fs->m_pakSearchPool.Construct();
		RAD_OUT_OF_MEM(ps);
		ps->Initialize(m_paks, m_fs, m_media);
		if (ps->OpenSearch(m_path.c_str, m_ext.c_str))
		{
			m_search = ps;
			return true;
		}
		ps->Release();
	}
	return false;
}

bool CSearch::MyOnZeroReferences()
{
	HFileSystem temp(m_fs);
	InterfaceComponent<FileSystem>(temp)->m_searchPool.Destroy(this);
	return true; // handled.
}

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(FileSystem, file.FileSystem);

FileSystem::FileSystem() : AtomicRefCount(1),
	m_enabledMedia(0),
	m_maxSectorSize(0),
	m_init(false)
{
}

inline void FileSystem::AssertInit() const
{
	RAD_VERIFY_MSG(m_init, "File system not initialized!");
}

void FileSystem::Initialize(int enabledMedia, U32 version)
{
	if (version != Version) throw BadInterfaceVersionException();
	m_enabledMedia = enabledMedia;
	m_cddvdPath = "1:";
	m_cddvdPathSlash = "1:/";
	m_hddPath = "9:";
	m_hddPathSlash = "9:/";
	m_mod = "default_mod_dir";
	m_modPath = "9:/default_mod_dir";
	m_modPathSlash = "9:/default_mod_dir/";

	m_maxSectorSize = 512;

	if (m_enabledMedia & HDD)
	{
		m_maxSectorSize = (FPos)std::max(m_maxSectorSize, file::DeviceSectorSize(m_hddPath.c_str, 0));
	}
	if (m_enabledMedia & CDDVD)
	{
		m_maxSectorSize = (FPos)std::max(m_maxSectorSize, file::DeviceSectorSize(m_cddvdPath.c_str, 0));
	}
	if (m_enabledMedia & Mod)
	{
		m_maxSectorSize = (FPos)std::max(m_maxSectorSize, file::DeviceSectorSize(m_modPath.c_str, 0));
	}

	m_streamInputBufferPool.Create(
		ZFS,
		"fs-stream-buffer-input",
		8
	);

	m_fileDataPool.Create(
		ZFS,
		"fs-file-data",
		8
	);

	m_bufferedDiskIOPool.Create(
		ZFS,
		"fs-buffered-io",
		8
	);

	m_diskFilePool.Create(
		ZFS,
		"fs-disk-file",
		8
	);

	m_pakSearchPool.Create(
		ZFS,
		"fs-pak-search",
		8
	);

	m_diskSearchPool.Create(
		ZFS,
		"fs-disk-search",
		8
	);

	m_searchPool.Create(
		ZFS,
		"fs-search",
		8
	);

	m_init = true;
}

void FileSystem::AddPakFile(const file::HPakFile& pakFile)
{
	AssertInit();
	m_paks.push_back(pakFile);
}

void FileSystem::RemovePakFile(const file::HPakFile& pakFile)
{
	AssertInit();
	RAD_STL_FOREACH(PakList::iterator, it, m_paks)
	{
		if ((*it)->Component() == pakFile->Component())
		{
			m_paks.erase(it); break;
		}
	}
}

void FileSystem::ReleaseAllPakRefs()
{
	AssertInit();
	m_paks.clear();
	STLContainerShrinkToSize(m_paks);
}

HPakFile FileSystem::Pak(int num)
{
	AssertInit();
	RAD_ASSERT(num < (int)m_paks.size());
	return m_paks[num];
}

HSearch FileSystem::OpenSearch(const char *path, const char *extIncludingPeriod, int media)
{
	AssertInit();
	Reference(); // making a search object references us.
	CSearch *search = m_searchPool.Construct();
	RAD_OUT_OF_MEM(search);
	String str;
	FixupPath(path, str);
	String ext = CStr(extIncludingPeriod).Lower();
	search->Initialize(&m_paks, this, int(media & m_enabledMedia));
	if (search->OpenSearch(str.c_str, ext.c_str))
	{
		return search;
	}
	search->Release();
	return 0;
}

HBufferedAsyncIO FileSystem::SafeCreateBufferedIO(
	FPos size,
	FPos alignment,
	Zone &zone
)
{
	HBufferedAsyncIO io = CreateBufferedIO(size, alignment, zone);
	RAD_OUT_OF_MEM(io);
	return io;
}

HBufferedAsyncIO FileSystem::CreateBufferedIO(
	FPos size,
	FPos alignment,
	Zone &zone
)
{
	RAD_ASSERT(size);
	RAD_ASSERT(alignment);

	BufferedDiskIO* io = m_bufferedDiskIOPool.Construct();
	if (!io)
	{
		RAD_OUT_OF_MEM(io);
		return HBufferedAsyncIO();
	}
	RAD_ASSERT(io->m_pending == false);
	io->m_reqDataSize = size;
	Reference();
	io->m_fs = this;

	// size needs to be big enough for the read to straddle sector boundaries (which would make us
	// read an extra sector of data).

	io->m_dataSize = (FPos)std::max(m_maxSectorSize, size);
	io->m_dataSize = (FPos)Align(io->m_dataSize, m_maxSectorSize);
	io->m_dataSize += m_maxSectorSize; // extra sector of data.

	io->m_dataAlignment = (FPos)std::max<FPos>(m_maxSectorSize, alignment);
	io->m_data = AlignedFileBuffer(io->m_dataSize, io->m_dataAlignment, zone);
	RAD_ASSERT(IsAligned(io->m_data, io->m_dataAlignment));
	io->m_alignment = alignment;

	RAD_ASSERT(io->m_pending == false);

	if (!io->m_data)
	{
		io->Release();
		io = 0;
	}

	return io;
}

HStreamInputBuffer FileSystem::SafeCreateStreamBuffer(
	FPos size,
	Zone &zone
)
{
	HStreamInputBuffer ib = CreateStreamBuffer(size, zone);
	RAD_OUT_OF_MEM(ib);
	return ib;
}

HStreamInputBuffer FileSystem::CreateStreamBuffer(
	FPos size,
	Zone &zone
)
{
	RAD_ASSERT(size);

	StreamInputBuffer *ib = m_streamInputBufferPool.Construct();
	if (!ib)
	{
		RAD_OUT_OF_MEM(ib);
		return HStreamInputBuffer();
	}
	Reference();
	ib->m_fs = this;
	ib->m_io = CreateBufferedIO(size, 8, zone);

	if (!ib->m_io)
	{
		ib->Release();
		ib = 0;
	}

	return ib;
}

int FileSystem::FileExists(const char* filename, int media)
{
	AssertInit();
	String path;
	FixupPath(filename, path);

	int found = 0;
	media = media & m_enabledMedia;

	// try loading from Mod?
	if (media & Mod)
	{
		if (file::FileExists((m_modPathSlash + path).c_str, 0))
		{
			found |= Mod;
		}
	}
	// try loading from HDD?
	if (media & HDD)
	{
		if (file::FileExists((m_hddPathSlash + path).c_str, 0))
		{
			found |= HDD;
		}
	}
	// HDD not enabled or doesn't exist.
	// try loading from CDDVD?
	if (media & CDDVD)
	{
		if (file::FileExists((m_cddvdPathSlash + path).c_str, 0))
		{
			found |= CDDVD;
		}
	}
	// HDD and CDDVD failed or not enabled, try paks.
	// try loading from Paks
	if (media & Paks)
	{
		// go in reverse order so paks added last get checked first.
		for (size_t i = m_paks.size(); i > 0; --i)
		{
			if (m_paks[i-1]->FileExists(path.c_str)) 
			{
				found |= Paks;
				break;
			}
		}
	}

	return found;
}

int FileSystem::DeleteFile(const char *filename, int media)
{
	AssertInit();
	String path;
	FixupPath(filename, path);

	int deleted = 0;
	media = media & m_enabledMedia;

	// try loading from Mod?
	if (media & Mod)
	{
		if (file::DeleteFile((m_modPathSlash + path).c_str, 0))
		{
			deleted |= Mod;
		}
	}
	// try loading from HDD?
	if (media & HDD)
	{
		if (file::DeleteFile((m_hddPathSlash + path).c_str, 0))
		{
			deleted |= HDD;
		}
	}
	
	return deleted;
}

bool FileSystem::FileSize(const char *filename, int media, file::FPos &size)
{
	AssertInit();
	RAD_ASSERT(size);
	String path;
	FixupPath(filename, path);

	media = media & m_enabledMedia;

	// try loading from HDD?
	if (media & Mod)
	{
		file::File file;
		if (file.Open((m_modPathSlash + path).c_str, OpenExisting,
			AccessRead, ShareMode(ShareRead|ShareWrite|ShareTemporary),
			FileOptions(0), 0) == Success)
		{
			size = file.Size();
			return true;
		}
	}
	// try loading from HDD?
	if (media & HDD)
	{
		file::File file;
		if (file.Open((m_hddPathSlash + path).c_str, OpenExisting,
			AccessRead, ShareMode(ShareRead|ShareWrite|ShareTemporary),
			FileOptions(0), 0) == Success)
		{
			size = file.Size();
			return true;
		}
	}
	// HDD not enabled or doesn't exist.
	// try loading from CDDVD?
	if (media & CDDVD)
	{
		file::File file;
		if (file.Open((m_cddvdPathSlash + path).c_str, OpenExisting,
			AccessRead, ShareMode(ShareRead|ShareWrite|ShareTemporary),
			FileOptions(0), 0) == Success)
		{
			size = file.Size();
			return true;
		}
	}
	// HDD and CDDVD failed or not enabled.
	// try loading from Paks
	if (media & Paks)
	{
		// go in reverse order so paks added last get checked first.
		for (size_t i = m_paks.size(); i > 0; --i)
		{
			if (m_paks[i-1]->FileSize(path.c_str, size)) return true;
		}
	}

	return false;
}

Result FileSystem::LoadFile(
	const char *filename, 
	int &media, 
	HBufferedAsyncIO& hAsyncIO, 
	const HIONotify &ioComplete,
	FPos alignment,
	Zone &zone,
	bool relativePath
)
{
	AssertInit();

	HFile file;
	Result r = OpenFile(filename, media, file, HIONotify(), relativePath);

	if (r == Success)
	{
		RAD_ASSERT(file);

		media = file->media;
		r = file->Load(hAsyncIO, ioComplete, alignment, zone);
	}

	return r;
}

Result FileSystem::OpenFile(
	const char *filename, 
	int media, 
	HFile &hFile, 
	const HIONotify &ioComplete,
	bool relativePath
)
{
	AssertInit();
	RAD_ASSERT(filename);
	String path;

	media = media & m_enabledMedia;
	Result r = ErrorFileNotFound;

	if (relativePath)
	{
		FixupPath(filename, path);

		// try loading from Mod?
		if (media & Mod)
		{
			r = OpenDiskFile(m_modPathSlash.c_str, path.c_str, Mod, hFile, ioComplete);
		}
		// try loading from HDD?
		if (Success != r && (media & HDD))
		{
			r = OpenDiskFile(m_hddPathSlash.c_str, path.c_str, HDD, hFile, ioComplete);
		}
		// HDD not enabled or doesn't exist.
		// try loading from CDDVD?
		if (Success != r && (media & CDDVD))
		{
			r = OpenDiskFile(m_cddvdPathSlash.c_str, path.c_str, CDDVD, hFile, ioComplete);
		}
		// HDD and CDDVD failed or not enabled, try paks.
		// try loading from Paks
		if (Success != r && (media & Paks))
		{
			// go in reverse order so paks added last get checked first.
			for (size_t i = m_paks.size(); i > 0; --i)
			{
				r = m_paks[i-1]->OpenFile(path.c_str, hFile);
				if (r == Success) 
					break;
			}
		}
	}
	else
	{
		if (media & HDD)
			r = OpenDiskFile("", filename, HDD, hFile, ioComplete);
	}

	if (ioComplete)
		ioComplete->Notify(r);

	return r;
}

Result FileSystem::OpenFileStream(
	const char *path,
	int &media,
	HStreamInputBuffer &stream,
	const HIONotify &ioComplete,
	FPos bufSize,
	Zone &zone,
	bool relativePath
)
{
	RAD_ASSERT(bufSize);

	HFile f;
	Result r = OpenFile(path, media, f, HIONotify(), relativePath);
	if (r < Success)
	{
		if (ioComplete)
		{
			ioComplete->Notify(r);
		}
		return r;
	}

	stream = CreateStreamBuffer(bufSize, zone);
	
	if (stream)
	{
		stream->Bind(f);
		media = f->media;
	}
	else
	{
		r = ErrorOutOfMemory;
	}

	if (ioComplete)
	{
		ioComplete->Notify(r);
	}

	return r;
}

Result FileSystem::OpenDiskFile(
	const char *prefix, 
	const char *filename, 
	int media, 
	HFile &hFile, 
	const HIONotify &ioComplete
)
{
	AssertInit();
	String strFile = String(prefix, RefTag) + String(filename, RefTag);
	if (file::FileExists(strFile.c_str, 0))
	{
		DiskFile *diskFile = m_diskFilePool.Construct();
		RAD_OUT_OF_MEM(diskFile);
		Reference();

		diskFile->m_fs = this;
		diskFile->m_media = media;

		diskFile->m_sectorSize = file::DeviceSectorSize(strFile.c_str, 0);
		RAD_ASSERT(diskFile->m_sectorSize);
		Result r = diskFile->m_file.Open(strFile.c_str, OpenAlways, AccessRead, ShareRead, Async, 0);

		if (r == Success)
		{
			diskFile->m_size = diskFile->m_file.Size();
			diskFile->m_numSectors = (diskFile->m_size + diskFile->m_sectorSize - 1) / diskFile->m_sectorSize;
			hFile = diskFile;
			if (ioComplete)
			{
				ioComplete->Notify(r);
			}
		}
		else
		{
			diskFile->Release();
		}

		return r;
	}
	return ErrorFileNotFound;
}

void *FileSystem::AlignedFileBuffer(FPos size, FPos alignment, Zone &zone)
{
	if (alignment < 8) 
		alignment = 8;
	return zone_malloc(zone, size, 0, alignment);
}

void FileSystem::FixupPath(const char *str, String &path)
{
	RAD_ASSERT(str);
	path.Clear();
	if (str[0] == 0) 
		return;
	String prefix, right;

	// remove leading ../
	while (str[0] == L'.' && str[1] == L'.' && str[2] == L'/')
	{
		str += 3;
	}

	right = CStr(str);

	// process '../' command inside path string.
	while (!right.empty)
	{
		int ofs = right.StrStr("/");
		if (ofs != -1)
		{
			prefix = right.SubStr(0, ofs+1); // include '/'
			right = right.SubStr(ofs+1);

			if (right.length >= 3)
			{
				if (right[0] == '.' && right[1] == '.' && right[2] == '/')
				{
					prefix.Clear();
					// remove leading ../
					while (right.length >= 3 && right[0] == '.' && right[1] == '.' && right[2] == '/')
					{
						right = right.Left(3);
					}
				}
			}

			path += prefix;
		}
		else
		{
			// no more directories.
			path += right;
			break;
		}
	}
}

int FileSystem::RAD_IMPLEMENT_GET(numPaks)
{
	AssertInit();
	return (int)m_paks.size();
}

int FileSystem::RAD_IMPLEMENT_GET(enabledMedia)
{
	AssertInit();
	return m_enabledMedia;
}

void FileSystem::RAD_IMPLEMENT_SET(enabledMedia) (int value)
{
	AssertInit();
	m_enabledMedia = value;
}

const char *FileSystem::RAD_IMPLEMENT_GET(cddvdRoot)
{

	return m_cddvd.c_str;
}

void FileSystem::RAD_IMPLEMENT_SET(cddvdRoot) (const char *value)
{
	AssertInit();

	if(value == 0) {
		value = "";
	}

	m_cddvd = value;
	if (m_cddvd[0] == '/') 
		m_cddvd.Erase(0, 1);

	if (!m_cddvd.empty)
	{
		if (*(m_cddvd.end-1) == L'/')
		{
			m_cddvd.Erase(m_cddvd.length-1);
		}
	}
	if (!m_cddvd.empty)
	{
		m_cddvdPath = String("1:/", RefTag) + m_cddvd;
		m_cddvdPathSlash = m_cddvdPath + '/';
	}
	else
	{
		m_cddvdPath = "1:";
		m_cddvdPathSlash = "1:/";
	}
}

const char *FileSystem::RAD_IMPLEMENT_GET(hddRoot)
{
	AssertInit();
	return m_hdd.c_str;
}

void FileSystem::RAD_IMPLEMENT_SET(hddRoot) (const char *value)
{
	AssertInit();

	if(value == 0 || value[0] == 0)
	{
		value = "";
	}

	m_hdd = value;
	if (m_hdd[0] == '/') 
		m_hdd.Erase(0, 1);
	if (!m_hdd.empty)
	{
		if (*(m_hdd.end-1) == '/')
		{
			m_hdd.Erase(m_hdd.length-1);
		}
	}
	if (!m_hdd.empty)
	{
		m_hddPath = String("9:/") + m_hdd;
		m_hddPathSlash = m_hddPath + '/';
	}
	else
	{
		m_hddPath = "9:";
		m_hddPathSlash = "9:/";
	}
}

const char *FileSystem::RAD_IMPLEMENT_GET(modRoot)
{
	AssertInit();
	return m_mod.c_str;
}

void FileSystem::RAD_IMPLEMENT_SET(modRoot) (const char *value)
{
	AssertInit();

	if(value == 0 || value[0] == 0)
	{
		value = "";
	}

	m_mod = value;
	if (m_mod[0] == '/') 
		m_mod.Erase(0, 1);
	if (!m_mod.empty)
	{
		if (*(m_mod.end-1) == '/')
		{
			m_mod.Erase(m_mod.length-1);
		}
	}
	if (!m_mod.empty)
	{
		m_modPath = String("9:/") + m_mod;
		m_modPathSlash = m_modPath + '/';
	}
	else
	{
		m_modPath = "9:";
		m_modPathSlash = "9:/";
	}
}

} // details
} // file
