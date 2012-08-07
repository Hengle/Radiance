// DPak.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <Runtime/Interface/ComponentBuilder.h>
#include <Runtime/Thread/Interlocked.h>
#include <Runtime/String.h>
#include <Runtime/DataCodec/LmpReader.h>
#include <Runtime/DataCodec/ZLib.h>
#include <Runtime/Thread.h>
#include <Runtime/Thread/Locks.h>
#include <Runtime/Base/ObjectPool.h>
#include "../FileSystem.h"
#include "../DPak.h"
#include "FileSystem.h"

#include <list>
#include <deque>

namespace file {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// DPakFileEntry
//////////////////////////////////////////////////////////////////////////////////////////

struct DPakFile;
struct DPakFileEntry :
	public IDPakEntry,
	private AtomicRefCount
{
	DPakFileEntry();

	struct Decoder;
	typedef std::list<DPakFileEntry::Decoder*> DecoderList;

	struct Decoder : InternalIO
	{
		enum 
		{
			NumBufs = 2,
			// NOTE: if you change BufSize be-ware it's also used
			// on the stack, and the decode thread has a 96k stack.
			BufSize = 64 * 1024
		};
		data_codec::zlib::Decoder decoder;
		DecoderList::iterator it, it2;
		DPakFile *file;
		DPakFileEntry *entry;
		HBufferedAsyncIO src[NumBufs];
		BufferedDiskIO  *dst;
		Result result;
		FPos pakFileOfs; // file offset in pak.
		FPos curFileOfs; // current position in the compressed file.
		FPos curFileSize;
		FPos curBufOfs; // current offset in compressed buffer.
		FPos srcFileOfs; // first byte in "src" is at this offset in the decompressed file.
		FPos dstFileOfs;
		FPos dstReadSize;
		FPos dstBytesRead;
		bool eof;
		U8 pass;
		U8 ioidx;
		volatile bool cancel;
		virtual void CancelInternalIO() { cancel = true; }
	};

	HPakFile m_hpak;
	const data_codec::lmp::StreamReader::Lump *m_lump;
	DecoderList m_used;
	DecoderList m_free;
	Mutex m_m;

	Decoder *DecoderForFilePos(FPos fileOfs);
	void StartDecoder(Decoder &decoder);
	void FreeDecoder(Decoder &decoder);
	void FinishDecoder(Decoder &decoder);

	virtual Result Read(
		const HBufferedAsyncIO &io, 
		FPos fileOffset, 
		FPos sizeToRead, 
		const HIONotify &ioComplete
	);

	virtual Result Load(
		HBufferedAsyncIO       &asyncIO,
		const HIONotify        &ioComplete,
		FPos alignment,
		Zone &zone
	);

	virtual RAD_DECLARE_GET(size, FPos);
	virtual RAD_DECLARE_GET(media, int) { return file::Paks; }
	virtual RAD_DECLARE_GET(tag, const void*);
	virtual RAD_DECLARE_GET(tagSize, AddrSize);

	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, MyOnZeroReferences(), this);
	RAD_INTERFACE_MAP_BEGIN(DPakFileEntry)
		RAD_INTERFACE_MAP_ADD(IDPakEntry)
	RAD_INTERFACE_MAP_END

	bool MyOnZeroReferences();
};

//////////////////////////////////////////////////////////////////////////////////////////
// DPakFile
//////////////////////////////////////////////////////////////////////////////////////////

struct DPakFile :
	public IPakFile,
	private AtomicRefCount
{
	HDPakReader m_reader;
	string::String m_name;
	HFile m_file;
	data_codec::lmp::StreamReader m_pak;

	DPakFile();

	bool Initialize(const HDPakReader &reader, const HFile &file, const char *name);

	virtual HSearch OpenSearch(const char* path, const char* extIncludingPeriod);
	virtual Result OpenFile(const char *path, HFile &file);
	virtual bool FileExists(const char* path);
	virtual bool FileSize(const char* path, file::FPos& size);

	virtual RAD_DECLARE_GET(name, const char*);

	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, MyOnZeroReferences(), this);
	RAD_INTERFACE_MAP_BEGIN(DPakFile)
		RAD_INTERFACE_MAP_ADD(IPakFile)
	RAD_INTERFACE_MAP_END

	bool MyOnZeroReferences();
};

//////////////////////////////////////////////////////////////////////////////////////////
// DPakPakFileSearch
//////////////////////////////////////////////////////////////////////////////////////////

struct DPakFileSearch :
	public ISearch,
	private AtomicRefCount
{
	HPakFile m_pakFile;
	DPakFile *m_dpakFile;
	U32 m_lumpNum;
	string::String m_path;
	string::String m_ext;

	DPakFileSearch();

	virtual bool NextFile(string::String &outFilename);
	
	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, MyOnZeroReferences(), this);
	RAD_INTERFACE_MAP_BEGIN(DPakFileSearch)
		RAD_INTERFACE_MAP_ADD(ISearch)
	RAD_INTERFACE_MAP_END

	bool MyOnZeroReferences();
};

//////////////////////////////////////////////////////////////////////////////////////////
// DPakReader
//////////////////////////////////////////////////////////////////////////////////////////

struct DPakReader : 
public IDPakReader,
public thread::Thread,
private AtomicRefCount
{
	bool m_init;
	HFileSystem m_fs;
	thread::IThreadContext *m_context;
	ThreadSafeObjectPool<DPakFile> m_pakFilePool;
	ThreadSafeObjectPool<DPakFileSearch> m_fileSearchPool;
	ThreadSafeObjectPool<DPakFileEntry::Decoder> m_pakFileEntryDecoderPool;
	ThreadSafeObjectPool<DPakFileEntry> m_pakFileEntryPool;
	Mutex m_m;
	thread::Gate m_block;
	DPakFileEntry::DecoderList m_decoders;
	volatile UReg m_openFileCount;
	volatile bool m_threadExit;

	DPakReader();
	~DPakReader();

	void AssertInit() const;

	//////////////////////////////////////////////////////////////////////////////////////////
	// file::IDPakReader
	//////////////////////////////////////////////////////////////////////////////////////////

	virtual void Initialize(
		const HFileSystem &fileSystem, 
		thread::IThreadContext *decompressContext,
		U32 version
	);

	virtual HPakFile MountPakFile(const HFile &file, const char *name);
	virtual int ThreadProc();
	bool ProcessDecoder(DPakFileEntry::Decoder &decoder, bool &waitingForIO);

	void OpenFile();
	void CloseFile();
	DPakFileEntry::Decoder *NewDecoder(DPakFile *file, DPakFileEntry *entry);
	void FreeDecoder(DPakFileEntry::Decoder &decoder);
	void StartDecoder(DPakFileEntry::Decoder &decoder);
	void FinishDecoder(DPakFileEntry::Decoder &decoder);
	void CancelDecoder(DPakFileEntry::Decoder &decoder);
	void StartDecoderIO(DPakFileEntry::Decoder &decoder);
	
	//////////////////////////////////////////////////////////////////////////////////////////
	// IInterface
	//////////////////////////////////////////////////////////////////////////////////////////

	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, NULL, this);
	RAD_INTERFACE_MAP_BEGIN(DPakReader)
		RAD_INTERFACE_MAP_ADD(IDPakReader)
	RAD_INTERFACE_MAP_END

};

} // details
} // file
