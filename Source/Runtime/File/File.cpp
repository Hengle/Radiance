// File.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "File.h"

using namespace string;
using namespace data_codec::lmp;

namespace file {

RAD_ZONE_DEF(RADRT_API, ZFile, "Files", ZRuntime);

namespace {
	inline bool pathHasAlias(const char *path) {
		RAD_ASSERT(path);
		return (path[0] == '@') && (path[2] == ':');
	}
	inline bool isAbsPath(const char *path) {
		return pathHasAlias(path);
	}
	inline bool isRelPath(const char *path) {
		return !isAbsPath(path);
	}
	inline void validatePath(const char *path) {
#if defined(RAD_OPT_DEBUG)
		for (int i = 0; path[i]; ++i) {
			RAD_ASSERT_MSG(path[i] != '\\', "Invalid directory seperator"); // invalid directory seperator
		}
#endif
	}
}

FileSystem::FileSystem(const char *root, const char *dvd)
: m_globalMask(kFileMask_Any) {
	RAD_ASSERT(root);

	setAlias('r', root);
	
	if (dvd)
		setAlias('d', dvd);
}

void FileSystem::setAlias(
	char name,
	const char *path
) {
	RAD_ASSERT(path);
	
	String &alias = m_aliasTable[name];

	alias = path;

	if (!alias.empty) {
		char trailing = *(alias.end.get()-1);
		if (trailing == '/' || trailing == '\\') {
			// drop the /
			alias = alias.substr(0, alias.length - 1);
		}
	}
}

void FileSystem::addDirectory(
	const char *path,
	int mask
) {
	RAD_ASSERT(path);
	PathMapping m;
	m.dir = path;
	m.mask = mask;
	m_paths.push_back(m);
}

PakFile::Ref FileSystem::openPakFile(
	const char *path,
	FileOptions options,
	int mask,
	int exclude,
	int *resolved
) {
	MMFile::Ref file = openFile(path, options, mask, exclude, resolved);
	if (file)
		return PakFile::open(file);
	return PakFile::Ref();
}

void FileSystem::addPakFile(
	const PakFileRef &pakFile,
	int mask
) {
	RAD_ASSERT(pakFile);
	PathMapping m;
	m.pak = pakFile;
	m.mask = mask;
	m_paths.push_back(m);
}

void FileSystem::removePakFile(const PakFileRef &pakFile) {
	RAD_ASSERT(pakFile);
	for (PathMapping::Vec::iterator it = m_paths.begin(); it != m_paths.end();) {
		const PathMapping &path = *it;
		if (path.pak && path.pak.get() == pakFile.get()) {
			it = m_paths.erase(it);
		} else {
			++it;
		}
	}
}

void FileSystem::removePakFiles() {
	for (PathMapping::Vec::iterator it = m_paths.begin(); it != m_paths.end();) {
		const PathMapping &path = *it;
		if (path.pak) {
			it = m_paths.erase(it);
		} else {
			++it;
		}
	}
}

FILE *FileSystem::fopen(
	const char *path,
	const char *mode,
	FileOptions options,
	int mask,
	int exclude,
	int *resolved
) {
	RAD_ASSERT(path);
	RAD_ASSERT(mode);

	String spath;
	bool nativePath = (options&kFileOption_NativePath) ? true : false;

	if (!nativePath) {
		if (!getAbsolutePath(path, spath, mask, exclude, resolved))
			return 0;
	} else {
		spath = CStr(path);
	}

	if (!nativePath) {
		String x;
		if (!getNativePath(spath.c_str, x))
			return 0;
		spath = x;
	}

	return ::fopen(spath.c_str, mode);
}

MMFile::Ref FileSystem::openFile(
	const char *path,
	FileOptions options,
	int mask,
	int exclude,
	int *resolved
) {
	mask &= m_globalMask;

	if (options & kFileOption_NativePath)
		return nativeOpenFile(path, options);

	if (isAbsPath(path)) {
		String nativePath;
		if (!getNativePath(path, nativePath))
			return MMFile::Ref();
		return nativeOpenFile(nativePath.c_str, options);
	}

	const String kPath(CStr(path));

	RAD_ASSERT_MSG(kPath[0] != '/', "Relative paths should never being with a '/'!");

	if (resolved)
		*resolved = 0;

	for (PathMapping::Vec::const_iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
		const PathMapping &m = *it;

		if (m.mask&exclude)
			continue;
		if (m.mask&mask) {

			if (m.pak) {
				MMFileRef r = m.pak->openFile(kPath.c_str);
				if (r) {
					if (resolved)
						*resolved = m.mask;
					return r;
				}
			} else {
				String spath;
				
				spath = m.dir + "/" + kPath;
			
				if (!getNativePath(spath.c_str, spath))
					return MMFileRef();
				MMFileRef r = nativeOpenFile(spath.c_str, options);
				if (r) {
					if (resolved)
						*resolved = m.mask;
					return r;
				}
			}
		}
	}

	return MMFile::Ref();
}

bool FileSystem::getAbsolutePath(
	const char *path, 
	String &absPath,
	int mask,
	int exclude,
	int *resolved
) {
	RAD_ASSERT(path);
	mask &= m_globalMask;

	if (isAbsPath(path)) {
		absPath = path;
		return true;
	}

	const String kPath(CStr(path));

	RAD_ASSERT_MSG(kPath[0] != '/', "Relative paths should never being with a '/'!");

	for (PathMapping::Vec::const_iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
		const PathMapping &m = *it;

		if (m.pak)
			continue;
		if (m.mask&exclude)
			continue;
		if (m.mask&mask) {
		
			absPath = m.dir + "/" + kPath;
			
			if (resolved)
				*resolved = m.mask;
			return true;
		}
	}

	if (resolved)
		*resolved = 0;
	return false;
}

bool FileSystem::getNativePath(
	const char *path,
	String &nativePath
) {
	RAD_ASSERT(path);

	if (!pathHasAlias(path))
		return false;

	String spath(CStr(path));

#if defined(RAD_OPT_DEBUG)
	boost::array<bool, kAliasMax> m_touched;
	memset(&m_touched[0], 0, sizeof(bool)*kAliasMax);
#endif

	while (pathHasAlias(spath.c_str)) {
		char alias = spath[1];
#if defined(RAD_OPT_DEBUG)
		RAD_ASSERT_MSG(!m_touched[alias], "Expanding recursive file system alias!");
		m_touched[alias] = true;
#endif
		spath = m_aliasTable[alias] + spath.substr(3); // include '/' separator.
	}

	nativePath = spath;
	return true;
}

bool FileSystem::fileExists(
	const char *path,
	FileOptions options,
	int mask,
	int exclude,
	int *resolved
) {
	mask &= m_globalMask;

	if (options & kFileOption_NativePath)
		return nativeFileExists(path);

	if (isAbsPath(path)) {
		String nativePath;
		if (!getNativePath(path, nativePath))
			return false;
		return nativeFileExists(nativePath.c_str);
	}

	const String kPath(CStr(path));

	if (resolved)
		*resolved = 0;

	for (PathMapping::Vec::const_iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
		const PathMapping &m = *it;

		if (m.mask&exclude)
			continue;
		if (m.mask&mask) {

			if (m.pak) {
				if (m.pak->fileExists(kPath.c_str)) {
					if (resolved)
						*resolved = m.mask;
					return true;
				}
			} else {
				String spath = m.dir + kPath;
				if (!getNativePath(spath.c_str, spath))
					return false;
				if (nativeFileExists(spath.c_str)) {
					if (resolved)
						*resolved = m.mask;
					return true;
				}
			}
		}
	}

	return false;
}
///////////////////////////////////////////////////////////////////////////////

PakFile::Ref PakFile::open(const MMFile::Ref &file) {
	PakFile::Ref r(new (ZFile) PakFile(file));

	MMFileInputBuffer ib(file, 1*Meg);
	stream::InputStream is(ib);

	if (!r->m_pak.LoadLumpInfo(kDPakSig, kDPakMagic, is, data_codec::lmp::LittleEndian))
		r.reset();

	return r;
}

PakFile::PakFile(const MMFile::Ref &file) : m_file(file) {
}

MMFile::Ref PakFile::openFile(const char *path) {
	MMFile::Ref r;

	const StreamReader::Lump *l = m_pak.GetByName(path);
	if (l)
		r.reset(new (ZFile) MMPakEntry(shared_from_this(), *l));
	return r;
}

bool PakFile::fileExists(const char *path) {
	return m_pak.GetByName(path) != 0;
}

int PakFile::RAD_IMPLEMENT_GET(numLumps) {
	return (int)m_pak.NumLumps();
}

const data_codec::lmp::StreamReader::Lump *PakFile::lumpForIndex(int i) {
	return m_pak.GetByIndex(i);
}

const data_codec::lmp::StreamReader::Lump *PakFile::lumpForName(const char *path) {
	return m_pak.GetByName(path);
}

PakFile::MMPakEntry::MMPakEntry(
	const PakFileRef &pakFile,
	const data_codec::lmp::StreamReader::Lump &lump
) : m_pakFile(pakFile), m_lump(lump) {
}

MMapping::Ref PakFile::MMPakEntry::mmap(AddrSize ofs, AddrSize size) {
	RAD_ASSERT(ofs+size <= (AddrSize)m_lump.Size());
	return m_pakFile->m_file->mmap(
		(AddrSize)m_lump.Ofs() + ofs,
		size
	);
}

AddrSize PakFile::MMPakEntry::RAD_IMPLEMENT_GET(size) {
	return (AddrSize)m_lump.Size();
}

///////////////////////////////////////////////////////////////////////////////

MMFileInputBuffer::MMFileInputBuffer(
	const MMFile::Ref &file,
	AddrSize mappedSize
) : m_file(file), m_bufSize(mappedSize), m_pos(0) {
	RAD_ASSERT(mappedSize);
}

stream::SPos MMFileInputBuffer::Read(void *buf, stream::SPos numBytes, UReg *errorCode) {

	U8 *bytes = reinterpret_cast<U8*>(buf);
	stream::SPos bytesRead = 0;
	const stream::SPos kFileSize = Size();
	stream::SPos bytesLeftInFile = kFileSize - m_pos;

	while ((numBytes > 0) && (bytesLeftInFile > 0)) {

		if (m_mmap) {
			// see if we have a valid window to read.
			stream::SPos size = (stream::SPos)m_mmap->size.get();
			stream::SPos offset = (stream::SPos)m_mmap->offset.get();

			if ((m_pos < offset) || (m_pos >= offset+size)) { // no more data left in this mapping
				m_mmap.reset();
			}
		}

		// generate a new mapping?
		if (!m_mmap) {
			AddrSize bufSize = m_bufSize;
			if (bufSize > bytesLeftInFile)
				bufSize = bytesLeftInFile;

			m_mmap = m_file->mmap(m_pos, bufSize);
			if (!m_mmap)
				break; // error.
		}

		// copy
		stream::SPos mmSize = (stream::SPos)m_mmap->size.get();
		stream::SPos mmBase = (stream::SPos)m_mmap->offset.get();
		stream::SPos mmOffset = m_pos - mmBase;
		stream::SPos mmR = mmSize - mmOffset;

		RAD_ASSERT(mmR > 0);

		if (mmR > numBytes)
			mmR = numBytes;
		if (mmR > bytesLeftInFile)
			mmR = bytesLeftInFile;

		const U8 *src = reinterpret_cast<const U8*>(m_mmap->data.get()) + mmOffset;
		memcpy(bytes, src, mmR);
		bytes += mmR;
		bytesRead += mmR;
		m_pos += mmR;
		numBytes -= mmR;
		bytesLeftInFile -= mmR;
	}

	stream::SetErrorCode(errorCode, (numBytes>0)?stream::ErrorUnderflow:stream::Success);
	return bytesRead;
}

bool MMFileInputBuffer::SeekIn(stream::Seek seekType, stream::SPos ofs, UReg* errorCode) {
	bool b = stream::CalcSeekPos(seekType, ofs, m_pos, Size(), &ofs);
	if (b) {
		m_pos = ofs;
		stream::SetErrorCode(errorCode, stream::Success);
	} else {
		SetErrorCode(errorCode, stream::ErrorBadSeekPos);
	}
	return b;
}

///////////////////////////////////////////////////////////////////////////////

FILEInputBuffer::FILEInputBuffer(FILE *fp) : m_fp(fp) {
	RAD_ASSERT(fp);
	m_pos = (stream::SPos)ftell(fp);
}

stream::SPos FILEInputBuffer::Read(void *buff, stream::SPos numBytes, UReg *errorCode) {
	fseek(m_fp, m_pos, SEEK_SET);
	stream::SPos read = (stream::SPos)fread(buff, 1, numBytes, m_fp);
	stream::SetErrorCode(errorCode, (read<numBytes) ? stream::Success : stream::ErrorUnderflow);
	m_pos += read;
	return read;
}

bool FILEInputBuffer::SeekIn(stream::Seek seekType, stream::SPos ofs, UReg* errorCode) {
	bool b = stream::CalcSeekPos(seekType, ofs, m_pos, Size(), &ofs);
	if (b) {
		m_pos = ofs;
		stream::SetErrorCode(errorCode, stream::Success);
	} else {
		SetErrorCode(errorCode, stream::ErrorBadSeekPos);
	}
	return b;
}

stream::SPos FILEInputBuffer::Size()  const {
	size_t x = ftell(m_fp);
	fseek(m_fp, 0, SEEK_END);
	size_t s = ftell(m_fp);
	fseek(m_fp, (long)x, SEEK_SET);
	return (stream::SPos)s;
}

///////////////////////////////////////////////////////////////////////////////

FILEOutputBuffer::FILEOutputBuffer(FILE *fp) : m_fp(fp) {
	RAD_ASSERT(fp);
	m_pos = (stream::SPos)ftell(fp);
}

stream::SPos FILEOutputBuffer::Write(const void* buff, stream::SPos numBytes, UReg* errorCode) {
	fseek(m_fp, m_pos, SEEK_SET);
	stream::SPos write = (stream::SPos)fwrite(buff, 1, numBytes, m_fp);
	SetErrorCode(errorCode, (write<numBytes) ? stream::ErrorUnderflow : stream::Success );
	m_pos += write;
	return write;
}

bool FILEOutputBuffer::SeekOut(stream::Seek seekType, stream::SPos ofs, UReg* errorCode) {
	bool b = stream::CalcSeekPos(seekType, ofs, m_pos, Size(), &ofs);
	if (b) {
		m_pos = ofs;
		stream::SetErrorCode(errorCode, stream::Success);
	} else {
		SetErrorCode(errorCode, stream::ErrorBadSeekPos);
	}
	return b;
}

stream::SPos FILEOutputBuffer::Size()  const {
	size_t x = ftell(m_fp);
	fseek(m_fp, 0, SEEK_END);
	size_t s = ftell(m_fp);
	fseek(m_fp, (long)x, SEEK_SET);
	return (stream::SPos)s;
}

///////////////////////////////////////////////////////////////////////////////

RADRT_API String RADRT_CALL getFileExtension(const char *path) {
	RAD_ASSERT(path);
	String ext;

	int l = len(path);
	if (l > 0) {
		// find the '.'
		for (int i = l-1; i >= 0; --i) {
			if (path[i] == '.') {
				ext = &path[i];
				break;
			}
		}
	}

	return ext;
}

RADRT_API String RADRT_CALL setFileExtension(
	const char *path, 
	const char *ext
) {
	RAD_ASSERT(path);
	String newPath;

#if defined(RAD_OPT_DEBUG)
	if (ext && ext[0]) {
		RAD_ASSERT_MSG(ext[0] == '.', "Extensions must start with a period!");
	}
#endif

	for (int i = 0; path[i] && (path[i] != '.'); ++i) {
		newPath += path[i];
	}

	if (ext) {
		for (int i = 0; ext[i]; ++i) {
			newPath += ext[i];
		}
	}

	return newPath;
}

RADRT_API String RADRT_CALL getFileName(const char *path) {
	RAD_ASSERT(path);
	
	int l = len(path);
	for (int i = l-1; i >= 0; --i) {
		if (path[i] == '/' || path[i] == '\\') {
			return String(&path[i+1]);
		}
	}

	return String(path);
}

RADRT_API String RADRT_CALL getBaseFileName(const char *path) {
	String filename = getFileName(path);
	return setFileExtension(filename.c_str, 0);
}

RADRT_API String RADRT_CALL getFilePath(const char *path) {
	RAD_ASSERT(path);
	
	int l = len(path);
	for (int i = l-1; i >= 0; --i) {
		if (path[i] == '/' || path[i] == '\\') {
			return String(path, i, CopyTag);
		}
	}

	return String();
}

} // file
