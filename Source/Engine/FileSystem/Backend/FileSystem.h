// FileSystem.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Opts.h"
#include <Runtime/Interface/ComponentBuilder.h>
#include <Runtime/File.h>
#include <Runtime/Thread/Interlocked.h>
#include <Runtime/String.h>
#include <Runtime/Stream.h>
#include <Runtime/Base/ObjectPool.h>
#include "../FileSystem.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <vector>

#include <Runtime/PushSystemMacros.h>

namespace file {
namespace details {

RAD_ZONE_DEC(RADENG_API, ZFS);

typedef boost::mutex Mutex;
typedef boost::lock_guard<Mutex> Lock;
typedef std::vector<HPakFile> PakList;

//////////////////////////////////////////////////////////////////////////////////////////
// FileData
//////////////////////////////////////////////////////////////////////////////////////////

struct FileData :
	public file::IFileData,
	private AtomicRefCount
{
	void * volatile m_data;
	void * volatile m_realData;
	FPos  m_size;
	HFileSystem m_fs;

	FileData();

	RAD_DECLARE_GET(size, FPos);

	RAD_DECLARE_GET(ptr, const void*);

	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, MyOnZeroReferences(), this)
	RAD_INTERFACE_MAP_BEGIN(FileData)
		RAD_INTERFACE_MAP_ADD(IFileData)
	RAD_INTERFACE_MAP_END

	bool MyOnZeroReferences();
};

//////////////////////////////////////////////////////////////////////////////////////////
// BufferedDiskIO
//////////////////////////////////////////////////////////////////////////////////////////

struct StreamInputBuffer :
	public IStreamInputBuffer,
	public ::stream::IInputBuffer,
	private AtomicRefCount
{
	HFileSystem m_fs;
	HFile m_file;
	HBufferedAsyncIO m_io;
	FPos m_pos; // actual position in file (buffered).
	FPos m_ofs; // offset into buffer.
	FPos m_spos; // this is the stream position, returned by InPos().
	bool m_valid;

	StreamInputBuffer();

	void Bind(const HFile &file);
	::stream::SPos Read(void *buff, ::stream::SPos numBytes, UReg *errorCode);
	bool SeekIn(::stream::Seek seekType, ::stream::SPos ofs, UReg *errorCode);
	::stream::SPos InPos() const;
	::stream::SPos Size()  const;

	UReg InCaps() const;
	UReg InStatus() const;

	RAD_DECLARE_GET(buffer, ::stream::IInputBuffer&);
	
	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, MyOnZeroReferences(), this)
	RAD_INTERFACE_MAP_BEGIN(StreamInputBuffer)
		RAD_INTERFACE_MAP_ADD(IStreamInputBuffer)
	RAD_INTERFACE_MAP_END

	bool MyOnZeroReferences();
};

//////////////////////////////////////////////////////////////////////////////////////////
// BufferedDiskIO
//////////////////////////////////////////////////////////////////////////////////////////

struct InternalIO
{
	virtual void CancelInternalIO() = 0;
};

struct BufferedDiskIO :
	public IBufferedAsyncIO,
	public file::AsyncIO,
	private AtomicRefCount
{
	HFile m_file;
	HFileSystem m_fs;
	HFileData m_fd;
	HIONotify m_notify;
	InternalIO *m_internalIO;
	Mutex m_m;
	FPos m_ofs;
	FPos m_dataSize;
	FPos m_reqDataSize;
	FPos m_alignment;
	FPos m_dataAlignment;
	FPos m_size;
	FPos m_reqSize;
	void * volatile m_data;
	volatile bool m_pending;

	BufferedDiskIO();

	void Cancel();
	void WaitForCompletion();
	virtual void OnComplete(file::Result result); // from AsyncIO.
	HFileData SetupFileData();

	RAD_DECLARE_GET(byteCount, FPos);
	RAD_DECLARE_GET(result, file::Result);
	RAD_DECLARE_GET(data, HFileData);
	RAD_DECLARE_GET(ioSize, FPos);
	RAD_DECLARE_GET(bufferSize, FPos);
	RAD_DECLARE_GET(alignment, FPos);

	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, MyOnZeroReferences(), this)
	RAD_INTERFACE_MAP_BEGIN(BufferedDiskIO)
		RAD_INTERFACE_MAP_ADD(IBufferedAsyncIO)
		RAD_INTERFACE_MAP_ADD(IAsyncIO)
	RAD_INTERFACE_MAP_END

	bool MyOnZeroReferences();
};

//////////////////////////////////////////////////////////////////////////////////////////
// DiskFile
//////////////////////////////////////////////////////////////////////////////////////////

struct DiskFile :
	public file::IFile,
	private AtomicRefCount
{
	file::File m_file;
	HFileSystem m_fs;
	FPos m_sectorSize;
	FPos m_size;
	FPos m_numSectors;
	int m_media;

	DiskFile();
	
	virtual Result Read(const HBufferedAsyncIO &io, FPos fileOffset, FPos sizeToRead, const HIONotify &ioComplete);
	
	virtual Result Load(
		HBufferedAsyncIO       &asyncIO,
		const HIONotify        &ioComplete,
		FPos alignment,
		Zone &zone
	);

	RAD_DECLARE_GET(size, FPos);
	RAD_DECLARE_GET(media, int) { return m_media; }

	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, MyOnZeroReferences(), this)
	RAD_INTERFACE_MAP_BEGIN(DiskFile)
		RAD_INTERFACE_MAP_ADD(IFile)
	RAD_INTERFACE_MAP_END

	bool MyOnZeroReferences();
};

//////////////////////////////////////////////////////////////////////////////////////////
// PakSearch
//////////////////////////////////////////////////////////////////////////////////////////

struct PakSearch :
	public ISearch,
	private AtomicRefCount
{
	HSearch m_search;
	HFileSystem m_fs;
	PakList* m_paks;
	string::String m_path, m_ext;
	PakList::iterator m_last;
	int m_media;
	
	PakSearch();

	void Initialize(PakList *pakList, const HFileSystem& fs, int media);
	bool OpenSearch(const char* path, const char* extIncludingPeriod);
	bool NextFile(string::String& outFilename);

	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, MyOnZeroReferences(), this)
	RAD_INTERFACE_MAP_BEGIN(PakSearch)
		RAD_INTERFACE_MAP_ADD(ISearch)
	RAD_INTERFACE_MAP_END

	bool MyOnZeroReferences();
};

//////////////////////////////////////////////////////////////////////////////////////////
// DiskSearch
//////////////////////////////////////////////////////////////////////////////////////////

struct DiskSearch : 
	public ISearch,
	private AtomicRefCount
{
	file::Search m_search;
	HFileSystem m_fs;

	DiskSearch();
	
	bool OpenSearch(const char* path, const char* extIncludingPeriod);
	bool NextFile(string::String& outFilename);

	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, MyOnZeroReferences(), this)
	RAD_INTERFACE_MAP_BEGIN(DiskSearch)
		RAD_INTERFACE_MAP_ADD(ISearch)
	RAD_INTERFACE_MAP_END

	bool MyOnZeroReferences();
};

//////////////////////////////////////////////////////////////////////////////////////////
// CSearch
//////////////////////////////////////////////////////////////////////////////////////////

// named CSearch cause file:: already has a Search class.

struct CSearch :
	public ISearch,
	private AtomicRefCount
{
	HSearch m_search;
	HFileSystem m_fs;
	int m_media;
	int m_curMedia;
	PakList* m_paks;
	string::String m_path, m_ext;

	CSearch();

	void Initialize(PakList* pakList, const HFileSystem& fs, int media);
	bool OpenSearch(const char *path, const char *extIncludingPeriod);
	bool NextFile(string::String& outFilename);
	bool SelectNextMedia();
	bool MediaSearch();

	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, MyOnZeroReferences(), this)
	RAD_INTERFACE_MAP_BEGIN(CSearch)
		RAD_INTERFACE_MAP_ADD(ISearch)
	RAD_INTERFACE_MAP_END

	bool MyOnZeroReferences();
};

//////////////////////////////////////////////////////////////////////////////////////////
// FileSystem
//////////////////////////////////////////////////////////////////////////////////////////

struct FileSystem : 
public IFileSystem,
private AtomicRefCount
{
	int m_enabledMedia;
	bool m_init;
	FPos m_maxSectorSize;
	PakList m_paks;
	string::String m_cddvd, m_hdd, m_mod;
	string::String m_cddvdPath, m_hddPath, m_modPath;
	string::String m_cddvdPathSlash, m_hddPathSlash, m_modPathSlash;

	ThreadSafeObjectPool<StreamInputBuffer> m_streamInputBufferPool;
	ThreadSafeObjectPool<FileData> m_fileDataPool;
	ThreadSafeObjectPool<BufferedDiskIO> m_bufferedDiskIOPool;
	ThreadSafeObjectPool<DiskFile> m_diskFilePool;
	ThreadSafeObjectPool<PakSearch> m_pakSearchPool;
	ThreadSafeObjectPool<DiskSearch> m_diskSearchPool;
	ThreadSafeObjectPool<CSearch> m_searchPool;

	FileSystem();

	void AssertInit() const;

	//////////////////////////////////////////////////////////////////////////////////////////
	// IFileSystem
	//////////////////////////////////////////////////////////////////////////////////////////

	virtual void Initialize(int enabledMedia, U32 version);
	virtual void AddPakFile(const file::HPakFile& pakFile);
	virtual void RemovePakFile(const file::HPakFile& pakFile);
	virtual void ReleaseAllPakRefs();
	virtual HPakFile Pak(int num);

	virtual HSearch OpenSearch(
		const char* path, 
		const char* extIncludingPeriod, 
		int media
	);

	virtual HBufferedAsyncIO CreateBufferedIO(
		FPos size, 
		FPos alignment,
		Zone &zone
	);

	virtual HBufferedAsyncIO SafeCreateBufferedIO(
		FPos size, 
		FPos alignment,
		Zone &zone
	);

	virtual HStreamInputBuffer CreateStreamBuffer(
		FPos size,
		Zone &zone
	);

	virtual HStreamInputBuffer SafeCreateStreamBuffer(
		FPos size,
		Zone &zone
	);

	virtual int FileExists(
		const char *path, 
		int media
	);

	virtual int DeleteFile(
		const char *path, 
		int media
	);

	virtual bool FileSize(
		const char *path, 
		int media, 
		file::FPos& size
	);

	virtual Result LoadFile(
		const char *filename, 
		int &media, 
		file::HBufferedAsyncIO& hAsyncIO, 
		const HIONotify &ioComplete, 
		FPos alignment,
		Zone &zone,
		bool relativePath
	);

	virtual Result OpenFile(
		const char *path, 
		int media, 
		file::HFile &file, 
		const HIONotify &ioComplete,
		bool relativePath
	);

	virtual Result OpenFileStream(
		const char *path,
		int &media,
		HStreamInputBuffer &stream,
		const HIONotify &ioComplete,
		FPos bufSize,
		Zone &zone,
		bool relativePath
	);

	Result OpenDiskFile(
		const char *prefix, 
		const char *filename, 
		int media, 
		file::HFile &hAsyncIO, 
		const HIONotify &ioComplete
	);

	void *AlignedFileBuffer(FPos size, FPos alignment, Zone &zone);

	void FixupPath(const char *str, string::String &path);
	
	RAD_DECLARE_GET(numPaks, int);
	RAD_DECLARE_GET(enabledMedia, int);
	RAD_DECLARE_SET(enabledMedia, int);
	RAD_DECLARE_GET(cddvdRoot, const char*);
	RAD_DECLARE_SET(cddvdRoot, const char*);
	RAD_DECLARE_GET(hddRoot, const char*);
	RAD_DECLARE_SET(hddRoot, const char*);
	RAD_DECLARE_GET(modRoot, const char*);
	RAD_DECLARE_SET(modRoot, const char*);

	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, NULL, this)
	RAD_INTERFACE_MAP_BEGIN(FileSystem)
		RAD_INTERFACE_MAP_ADD(IFileSystem)
	RAD_INTERFACE_MAP_END
};

} // details
} // file

#include <Runtime/PopSystemMacros.h>
