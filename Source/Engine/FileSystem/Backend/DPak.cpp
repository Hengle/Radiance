// DPak.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "DPak.h"
#undef max
#undef min
#include <limits>
#include <Runtime/PushSystemMacros.h>

using namespace string;

namespace file {
namespace details {

RAD_ZONE_DEC(RADENG_API, ZDPak);
RAD_ZONE_DEF(RADENG_API, ZDPak, "DPak", ZFS);

enum
{
	DPakSig = RAD_FOURCC_LE('D', 'P', 'A', 'K'),
	DPakMagic = 0xA3054028
};

//////////////////////////////////////////////////////////////////////////////////////////
// DPakFileEntry
//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(DPakFileEntry, file.details.DPakFileEntry);

DPakFileEntry::DPakFileEntry() : 
AtomicRefCount(1),
m_lump(0)
{
}

DPakFileEntry::Decoder *DPakFileEntry::DecoderForFilePos(FPos fileOfs)
{
	Decoder *decoder = 0;
	{
		Lock l(m_m);
		FPos bestDist = std::numeric_limits<FPos>::max();
		
		DecoderList::iterator best = m_free.end();

		for (DecoderList::iterator it = m_free.begin(); it != m_free.end(); ++it)
		{
			Decoder *d = *it;
			
			if (d->srcFileOfs > fileOfs)
			{
				if(fileOfs < bestDist)
				{
					best = it;
					bestDist = fileOfs;
				}
			}
			else
			{
				FPos ofs = fileOfs - d->srcFileOfs;
				if (ofs < bestDist)
				{
					bestDist = ofs;
					best = it;
				}
			}
		}

		if (best != m_free.end())
		{
			decoder = *best;
			RAD_ASSERT(decoder->it2 == best);
			m_free.erase(best);
			m_used.push_back(decoder);
			decoder->it2 = --(m_used.end());
		}
	}
	
	if (decoder)
	{
		if (decoder->srcFileOfs > fileOfs)
		{
			// reset the decoder.
			AddrSize unused = 0;
			decoder->decoder.End(0, unused);
			decoder->decoder.Begin();
			decoder->srcFileOfs = 0;
			decoder->curFileOfs = 0;
			decoder->curBufOfs  = 0;
			decoder->pass = 0;
			decoder->ioidx = 0;
			decoder->eof = false;
			decoder->cancel = false;
			for (int i = 0; i < DPakFileEntry::Decoder::NumBufs; ++i)
			{
				decoder->src[i]->Cancel();
				decoder->src[i]->WaitForCompletion();
			}
		}
	}
	else
	{
		DPakFile *dpak = InterfaceComponent<DPakFile>(m_hpak);
		DPakReader *reader = InterfaceComponent<DPakReader>(dpak->m_reader);
		decoder = reader->NewDecoder(dpak, this);
		decoder->decoder.Begin();
		{
			Lock l(m_m);
			m_used.push_back(decoder);
			decoder->it2 = --(m_used.end());
		}
	}

	return decoder;
}

void DPakFileEntry::StartDecoder(Decoder &decoder)
{
	DPakReader *reader = InterfaceComponent<DPakReader>(decoder.file->m_reader);
	reader->StartDecoder(decoder);
}

void DPakFileEntry::FreeDecoder(Decoder &decoder)
{
	DPakReader *reader = InterfaceComponent<DPakReader>(decoder.file->m_reader);
	reader->FreeDecoder(decoder);
}

void DPakFileEntry::FinishDecoder(Decoder &decoder)
{
	if (decoder.result != Success)
	{
		{
			Lock l(m_m);
			m_used.erase(decoder.it2);
		}
		FreeDecoder(decoder);
	}
	else
	{
		Lock l(m_m);
		m_used.erase(decoder.it2);
		m_free.push_back(&decoder);
		decoder.it2 = --(m_free.end());
	}
}

Result DPakFileEntry::Read(const HBufferedAsyncIO &hio, FPos fileOffset, FPos sizeToRead, const HIONotify &ioComplete)
{
	RAD_ASSERT(m_lump);
	RAD_ASSERT(hio);
	BufferedDiskIO *io = InterfaceComponent<BufferedDiskIO>(hio);
	RAD_ASSERT_MSG(io->m_pending == false, "Specified IO object is in use!");
	RAD_ASSERT_MSG(io->m_reqDataSize >= sizeToRead, "Read size is greater than specified IO object buffer size!");

	const void *tag = m_lump->TagData();

	if (tag) // file is compressed
	{
		RAD_ASSERT(m_lump->TagSize() >= sizeof(data_codec::lmp::LOfs));
		
		const data_codec::lmp::LOfs *uncLumpSize = reinterpret_cast<const data_codec::lmp::LOfs*>(tag);
		
		io->m_reqSize = io->m_size = sizeToRead;
		io->m_notify = ioComplete;
		io->m_ofs = 0;
		io->m_pending = true;
		io->SetByteCount(0);
		io->TriggerStatusChange(Pending);

		if (fileOffset >= (FPos)(*uncLumpSize))
		{
			// they're trying to start reading past the end of the file.
			io->TriggerStatusChange(ErrorPartial);
			return ErrorPartial;
		}

		Reference();
		io->m_file = this;

		Decoder *decoder = DecoderForFilePos(fileOffset);
		RAD_ASSERT(decoder);
		decoder->dst = io;
		decoder->dstFileOfs = fileOffset;
		decoder->dstReadSize = sizeToRead;
		decoder->dstBytesRead = 0;

		{
			Lock l(io->m_m);
			io->m_internalIO = decoder;
		}
		
		StartDecoder(*decoder);
		return Pending;
	}
	
	// uncompressed file, go straight to disk.

	DPakFile *file = InterfaceComponent<DPakFile>(m_hpak);

	if (fileOffset >= (FPos)m_lump->Size())
	{
		io->SetByteCount(0);
		io->TriggerStatusChange(ErrorPartial);
		return ErrorPartial; // trying to read past EOF.
	}

	return file->m_file->Read(hio, fileOffset + (FPos)m_lump->Ofs(), sizeToRead, ioComplete);
}

Result DPakFileEntry::Load(
	HBufferedAsyncIO       &asyncIO,
	const HIONotify        &ioComplete,
	FPos alignment,
	Zone &zone
)
{
	DPakReader *reader = InterfaceComponent<DPakReader>(InterfaceComponent<DPakFile>(m_hpak)->m_reader);
	RAD_ASSERT(reader);

	asyncIO = reader->m_fs->CreateBufferedIO(this->size, alignment, zone);
	if (!asyncIO) 
		return ErrorOutOfMemory;

	return Read(asyncIO, 0, this->size, ioComplete);
}

FPos DPakFileEntry::RAD_IMPLEMENT_GET(size)
{
	RAD_ASSERT(m_lump);
	FPos s = 0;
	const void *tag = m_lump->TagData();

	if (tag) // file is compressed
	{
		RAD_ASSERT(m_lump->TagSize() >= sizeof(data_codec::lmp::LOfs));
		const data_codec::lmp::LOfs *uncLumpSize = reinterpret_cast<const data_codec::lmp::LOfs*>(tag);
		s = (FPos)(*uncLumpSize);
	}
	else
	{
		s = (FPos)m_lump->Size();
	}

	return s;
}

const void *DPakFileEntry::RAD_IMPLEMENT_GET(tag)
{
	if (0 == tagSize)
	{
		return 0;
	}

	RAD_ASSERT(m_lump);
	const void *tag = m_lump->TagData();

	if (tag)
	{
		tag = reinterpret_cast<const U8*>(tag) + sizeof(data_codec::lmp::LOfs); // skip our header.
	}

	return tag;
}

AddrSize DPakFileEntry::RAD_IMPLEMENT_GET(tagSize)
{
	RAD_ASSERT(m_lump);
	data_codec::lmp::LOfs s = m_lump->TagSize();
	const void *tag = m_lump->TagData();

	if (tag)
	{
		s -= (data_codec::lmp::LOfs)sizeof(data_codec::lmp::LOfs);
	}

	return s;
}

bool DPakFileEntry::MyOnZeroReferences()
{
	HDPakReader hreader(InterfaceComponent<DPakFile>(m_hpak)->m_reader);
	DPakReader *reader = InterfaceComponent<DPakReader>(hreader);
	{
		Lock l(m_m);
		RAD_STL_FOREACH(DecoderList::iterator, it, m_used)
		{
			reader->CancelDecoder(*(*it));
		}
	}
	
	for (;;)
	{
		{
			Lock l(m_m);
			if (m_used.empty())
			{
				break;
			}
		}
		thread::Sleep();
	}
	while (!m_free.empty())
	{
		Decoder *d = m_free.front();
		reader->m_pakFileEntryDecoderPool.Destroy(d);
		m_free.pop_front();
	}
	reader->m_pakFileEntryPool.Destroy(this);
	return true; // handled.
}

//////////////////////////////////////////////////////////////////////////////////////////
// DPakFile
//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(DPakFile, file.details.DPakFile);

DPakFile::DPakFile() :
AtomicRefCount(1)
{
}

bool DPakFile::Initialize(const HDPakReader &hreader, const HFile &file, const char *name)
{
	RAD_ASSERT(name);

	m_reader = hreader;
	if (name) 
		m_name = name;
	m_file = file;

	DPakReader *reader = InterfaceComponent<DPakReader>(hreader);
	HStreamInputBuffer stream = reader->m_fs->SafeCreateStreamBuffer(256 * 1024);
	stream->Bind(file);

	::stream::InputStream is(stream->buffer);

	if (!m_pak.LoadLumpInfo(
		DPakSig, 
		DPakMagic, 
		is, 
		data_codec::lmp::LittleEndian
	))
	{
		return false;
	}

	// pak is mounted and ready to go.
	return true;
}

HSearch DPakFile::OpenSearch(const char* path, const char* extIncludingPeriod)
{
	RAD_ASSERT(path&&extIncludingPeriod);
	if (extIncludingPeriod[0] == 0 || extIncludingPeriod[1] != L'.') 
		return 0;

	DPakFileSearch *search = InterfaceComponent<DPakReader>(m_reader)->m_fileSearchPool.Construct();
	Reference();
	search->m_pakFile = this;
	search->m_dpakFile = this;
	search->m_lumpNum = 0;
	search->m_path = path;
	search->m_ext = extIncludingPeriod;

	return search;
}

//////////////////////////////////////////////////////////////////////////////////////////
// DPakFile::OpenFile()
//////////////////////////////////////////////////////////////////////////////////////////

Result DPakFile::OpenFile(const char *path, HFile &file)
{
	file.Close();

	const data_codec::lmp::StreamReader::Lump *lump = m_pak.GetByName(path);
	if (!lump)
	{
		return ErrorFileNotFound;
	}

	DPakReader *reader = InterfaceComponent<DPakReader>(m_reader);
	DPakFileEntry *entry = reader->m_pakFileEntryPool.Construct();

	Reference();
	entry->m_hpak = this;
	entry->m_lump = lump;

	file = entry;
	return Success;
}

bool DPakFile::FileExists(const char *path)
{
	const data_codec::lmp::StreamReader::Lump *lump = m_pak.GetByName(path);
	return 0 != lump;
}

bool DPakFile::FileSize(const char *path, FPos& size)
{
	const data_codec::lmp::StreamReader::Lump *lump = m_pak.GetByName(path);
	size = 0;
	if (lump)
	{
		const void *tag = lump->TagData();
		if (tag)
		{
			const data_codec::lmp::LOfs *uncSize = reinterpret_cast<const data_codec::lmp::LOfs*>(tag);
			size = (FPos)(*uncSize);
		}
		else
		{
			size = lump->Size();
		}
	}
	return 0 != lump;
}

const char *DPakFile::RAD_IMPLEMENT_GET(name)
{
	return m_name.c_str;
}

bool DPakFile::MyOnZeroReferences()
{
	HDPakReader hreader(m_reader);
	DPakReader *reader = InterfaceComponent<DPakReader>(hreader);
	reader->CloseFile();
	reader->m_pakFilePool.Destroy(this);
	return true; // handled.
}

//////////////////////////////////////////////////////////////////////////////////////////
// DPakFileSearch
//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(DPakFileSearch, file.details.DPakFileSearch);

DPakFileSearch::DPakFileSearch() :
AtomicRefCount(1),
m_lumpNum(0),
m_dpakFile(0)
{
}

bool DPakFileSearch::NextFile(String &outFilename)
{
	const U32 NumLumps = m_dpakFile->m_pak.NumLumps();
	while (m_lumpNum < NumLumps)
	{
		const data_codec::lmp::StreamReader::Lump *lump = m_dpakFile->m_pak.GetByIndex(m_lumpNum++);
		RAD_ASSERT(lump);
		String name(lump->Name(), RefTag);

		// trivial rejection.
		if (name.length < m_path.length) 
			continue;
		if (name.SubStr(0, m_path.length) != m_path) 
			continue; // doesn't start with our path.

		if (m_ext[1] != '*') // filter the extension.
		{
			char ext[file::MaxExtLen+1];
			file::FileExt(name.c_str, ext, file::MaxExtLen+1);
			if (!cmp(ext, m_ext.c_str.get()))
			{
				outFilename = name;
				return true;
			}
		}
		else
		{
			outFilename = name;
			return true;
		}
	}
	return false;
}

bool DPakFileSearch::MyOnZeroReferences()
{
	HPakFile temp(m_pakFile);
	DPakReader *reader = InterfaceComponent<DPakReader>(m_dpakFile->m_reader);
	reader->m_fileSearchPool.Destroy(this);
	return true; // handled.
}

//////////////////////////////////////////////////////////////////////////////////////////
// DPakReader
//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(DPakReader, file.DPakReader);

DPakReader::DPakReader() : 
AtomicRefCount(1), 
thread::Thread(96*1024), 
m_context(0), 
m_init(false),
m_threadExit(true),
m_openFileCount(0)
{
}

inline void DPakReader::AssertInit() const
{
	RAD_VERIFY_MSG(m_init, "DPakReader not initialized!");
}

void DPakReader::Initialize(
	const HFileSystem &fileSystem, 
	thread::IThreadContext *decompressContext,
	U32 version
)
{
	if (version != Version) 
		throw BadInterfaceVersionException();

	m_fileSearchPool.Create(
		ZDPak,
		"dpk-fs-search",
		32
	);

	m_pakFilePool.Create(
		ZDPak,
		"dpk-pak-files",
		32
	);

	m_pakFileEntryPool.Create(
		ZDPak,
		"dpk-pak-entries",
		32
	);

	m_pakFileEntryDecoderPool.Create(
		ZDPak,
		"dpk-fs-decoders",
		32
	);

	m_fs = fileSystem;
	m_context = decompressContext;
	m_init = true;
}

HPakFile DPakReader::MountPakFile(const HFile &file, const char *name)
{
	RAD_ASSERT(file);

	DPakFile *pakFile = m_pakFilePool.Construct();
	Reference();
	if (!pakFile->Initialize(this, file, name))
	{
		pakFile->Release();
		pakFile = 0;
	}
	else
	{
		OpenFile();
	}

	return pakFile;
}

int DPakReader::ThreadProc()
{
	for (;;)
	{
		m_m.lock();
		m_threadExit = 0 == m_openFileCount;
		bool io = true;
		
		if (!m_decoders.empty())
		{
			DPakFileEntry::DecoderList::iterator it = m_decoders.begin();
			m_m.unlock();

			while (it != m_decoders.end())
			{
				DPakFileEntry::Decoder &d = *(*it);

				bool wait = false;
				bool done = ProcessDecoder(d, wait);
				io = io && wait;

				m_m.lock();
				++it;
				m_m.unlock();

				if (done)
				{
					FinishDecoder(d);
				}
			}

			if (io) // everything is waiting for io.
			{
				// let the io thread run.
				thread::Sleep();
			}
		}
		else
		{
			m_m.unlock();
			if (!m_threadExit)
				m_block.Wait();
		}

		if (m_threadExit) 
			break;
	}
	return 0;
}

bool DPakReader::ProcessDecoder(DPakFileEntry::Decoder &decoder, bool &waitingForIO)
{
	bool done = false;

	if (decoder.cancel)
	{
		decoder.result = ErrorAborted;
		
		// cancel this decoder.
		for (int i = 0; i < DPakFileEntry::Decoder::NumBufs; ++i)
		{
			decoder.src[i]->Cancel();
		}

		done = true;
	}
	else
	{
		// check to see if the active IO pass is complete?
		const HBufferedAsyncIO &srcIO = decoder.src[decoder.pass];
		decoder.result = srcIO->result;

		waitingForIO = true;

		if (decoder.result == Success)
		{
			waitingForIO = false;
			const HFileData fd = decoder.dst->SetupFileData();
			
			if (decoder.pass != decoder.ioidx && !decoder.eof) // don't obliterate our current io block.
			{
				bool eof = true;
				decoder.curFileOfs += srcIO->data->size;

				// start the next IO pass?
				if (decoder.curFileOfs < decoder.curFileSize)
				{
					if (decoder.result != ErrorPartial)
					{
						StartDecoderIO(decoder);
						eof = false;
					}
				}

				decoder.eof = eof;
			}

			// continue to decompress the output data.
			U8 buf[DPakFileEntry::Decoder::BufSize];

			U8 *dst = buf;
			FPos dstOfs = 0;
			FPos dstSize = DPakFileEntry::Decoder::BufSize;
			FPos totalBytesRead = 0;

			const U8 *src = static_cast<const U8*>((const void*)srcIO->data->ptr);
			FPos srcSize = srcIO->data->size - decoder.curBufOfs;

			RAD_ASSERT(decoder.dstBytesRead < decoder.dstReadSize);

			// are we in our data?
			if (decoder.srcFileOfs >= decoder.dstFileOfs)
			{
				RAD_ASSERT((decoder.srcFileOfs-decoder.dstFileOfs) == decoder.dstBytesRead);
				dst = static_cast<U8*>(const_cast<void*>((const void*)fd->ptr)) + decoder.dstBytesRead;
				dstSize = decoder.dstReadSize - decoder.dstBytesRead;
			}

			for (;;)
			{
				AddrSize s = srcSize;
				AddrSize d = dstSize;

				if (decoder.decoder.Decode(src + decoder.curBufOfs, s, dst + dstOfs, d))
				{
					decoder.curBufOfs += (FPos)s;
					srcSize -= (FPos)s;
					dstOfs += (FPos)d;
					dstSize -= (FPos)d;
					totalBytesRead += (FPos)d;

					if (srcSize == 0 || dstSize == 0)
					{
						if (dst == buf) // we are working off the stack, should we copy anything over?
						{
							if (decoder.srcFileOfs + totalBytesRead > decoder.dstFileOfs)
							{
								// we've read into our window of requested data, copy over any overflow.
								FPos overflow = (decoder.srcFileOfs + totalBytesRead) - decoder.dstFileOfs;
								overflow = std::min(overflow, decoder.dstReadSize);
								RAD_ASSERT(overflow <= dstOfs);
								memcpy(const_cast<void*>((const void*)fd->ptr), dst + (dstOfs - overflow), overflow);
								decoder.dstBytesRead = overflow;
								dst = static_cast<U8*>(const_cast<void*>((const void*)fd->ptr)) + decoder.dstBytesRead;
								dstSize = decoder.dstReadSize - decoder.dstBytesRead;
								dstOfs = 0;
							}
							else
							{
								dstSize = DPakFileEntry::Decoder::BufSize;
								dstOfs  = 0;
							}
						}
						else
						{
							// doesn't matter if we're out of source or dest buffer in this case, we're done.
							// (i.e. we have to wait for more source data, or we filled our buffer).
							decoder.dstBytesRead += dstOfs;
							break;
						}

						if (0 == srcSize)
						{
							break; // need more data.
						}
					}
				}
				else
				{
					// cancel this decoder.
					for (int i = 0; i < DPakFileEntry::Decoder::NumBufs; ++i)
					{
						decoder.src[i]->Cancel();
					}
					decoder.result = ErrorCompressionFailure;
					done = true;
					break;
				}
			}

			if (!done)
			{
				decoder.srcFileOfs += totalBytesRead; // the next buffer read will start here.
				decoder.dst->SetByteCount(decoder.dstBytesRead);
				if (decoder.dstBytesRead == decoder.dstReadSize) // we've read all data requested
				{
					// *NOTE* don't cancel the decoder IO. if we did, it would put this object
					// in a weird state, and we'd be unable to re-use it.
					decoder.result = Success;
					done = true;
				}
				else if (0 == srcSize)
				{
					if (decoder.eof)
					{
						decoder.result = ErrorPartial;
						done = true;
					}
					else
					{
						decoder.pass = (decoder.pass+1) & (DPakFileEntry::Decoder::NumBufs-1);
						decoder.curBufOfs = 0;
					}
				}
			}

		}
		else if (decoder.result != Pending)
		{
			// error occured!
			done = true;
		}
	}

	return done;
}

void DPakReader::OpenFile()
{
	UReg count = 0;
	bool exit = false;
	{
		Lock l(m_m);
		count = ++m_openFileCount;
		exit = m_threadExit;
	}
	
	if (exit && (1 == count))
	{
		Join();
		Run(m_context);
	}
}

void DPakReader::CloseFile()
{
	{
		Lock l(m_m);
		RAD_ASSERT(m_openFileCount > 0);
		if (--m_openFileCount == 0)
		{
			m_block.Open(); // let the thread wake up.
		}
	}
}

DPakFileEntry::Decoder *DPakReader::NewDecoder(DPakFile *file, DPakFileEntry *entry)
{
	DPakFileEntry::Decoder *d = m_pakFileEntryDecoderPool.Construct();
	d->file = file;
	d->entry = entry;
	d->pakFileOfs = (FPos)entry->m_lump->Ofs();
	d->curFileOfs = 0;
	d->curBufOfs  = 0;
	d->curFileSize = (FPos)entry->m_lump->Size();
	d->srcFileOfs = 0;
	d->dst = 0;
	for (int i = 0; i < DPakFileEntry::Decoder::NumBufs; ++i)
	{
		d->src[i] = m_fs->CreateBufferedIO(DPakFileEntry::Decoder::BufSize);
		RAD_VERIFY_MSG(d->src[i], "DPakReader: failed to allocate compressed file decoder buffer!");
	}
	d->pass = 0;
	d->ioidx = 0;
	d->result = Pending;
	d->eof = false;
	d->cancel = false;
	return d;
}

void DPakReader::FreeDecoder(DPakFileEntry::Decoder &decoder)
{
	m_pakFileEntryDecoderPool.Destroy(&decoder);
}

void DPakReader::StartDecoder(DPakFileEntry::Decoder &decoder)
{
	if (decoder.curFileOfs == 0 || (decoder.pass != decoder.ioidx && !decoder.eof))
	{
#if defined(RAD_OPT_DEBUG)
		if (decoder.curFileOfs == 0)
		{
			RAD_ASSERT(0 == decoder.pass);
			RAD_ASSERT(0 == decoder.ioidx);
			RAD_ASSERT(0 == decoder.curBufOfs);
			RAD_ASSERT(0 == decoder.dstBytesRead);
		}
#endif
		StartDecoderIO(decoder);
	}
	{
		Lock l(m_m);
		bool b = m_decoders.empty();
		m_decoders.push_back(&decoder);
		decoder.it = --m_decoders.end();
		if (b)
		{
			m_block.Open();
		}
	}
}

void DPakReader::FinishDecoder(DPakFileEntry::Decoder &decoder)
{
	{
		Lock l(m_m);
		m_decoders.erase(decoder.it);
		decoder.it = m_decoders.end();
		if (m_decoders.empty())
		{
			m_block.Close();
		}
		decoder.dst->m_internalIO = 0;
	}
	
	BufferedDiskIO  *io = decoder.dst;
	Result r = decoder.result;

	decoder.entry->FinishDecoder(decoder);
	io->TriggerStatusChange(r);
}

void DPakReader::CancelDecoder(DPakFileEntry::Decoder &decoder)
{
	decoder.cancel = true;
}

void DPakReader::StartDecoderIO(DPakFileEntry::Decoder &decoder)
{
	FPos sizeToRead = std::min<FPos>(DPakFileEntry::Decoder::BufSize, decoder.curFileSize - decoder.curFileOfs);
	RAD_ASSERT(sizeToRead);
	const HBufferedAsyncIO &src = decoder.src[decoder.ioidx];
	decoder.ioidx = (decoder.ioidx+1) & (DPakFileEntry::Decoder::NumBufs-1);
	decoder.result = decoder.file->m_file->Read(src, decoder.curFileOfs + decoder.pakFileOfs, sizeToRead, HIONotify(0));
}

} // details
} // file

