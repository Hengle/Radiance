// File.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "File.h"
#include "../PushSystemMacros.h"

using namespace data_codec::lmp;

namespace file {

RAD_ZONE_DEF(RADRT_API, ZFile, "Files", ZRuntime);
RAD_ZONE_DEF(RADRT_API, ZPakFile, "Files", 0); // this is a dummy zone for memory mapped pak files.

namespace {
	inline bool PathHasAlias(const char *path) {
		RAD_ASSERT(path);
		return (path[0] == '@') && (path[2] == ':');
	}
	inline bool IsAbsPath(const char *path) {
		return PathHasAlias(path);
	}
	inline bool IsRelPath(const char *path) {
		return !IsAbsPath(path);
	}
}

namespace details {

///////////////////////////////////////////////////////////////////////////////

//! Handles searching through mount points/pak files, and performs shadowing.

class DetailsSearch : public FileSearch {
public:

	DetailsSearch(FileSystem &fs) : m_fs(fs), m_paths(fs.m_paths) {
	}

	bool Init(
		const char *path,
		SearchOptions searchOptions,
		FileOptions fileOptions,
		int mask,
		int exclude
	) {
		m_path = path;
		m_root = GetFilePath(path);
		m_searchOptions = searchOptions;
		m_fileOptions = fileOptions;
		m_mask = mask;
		m_exclude = exclude;
		m_it = m_paths.begin();
		OpenSearch();
		return m_s;
	}

	virtual bool NextFile(
		String &path,
		FileAttributes *fileAttributes,
		xtime::TimeDate *fileTime
	) {
		while (m_s) {
			FileSystem::PathMapping::Vec::const_iterator end = m_it;
			--end;

			while (m_s->NextFile(path, fileAttributes, fileTime)) {
				// is this shadowed by a higher precident mount point?
				FileSystem::PathMapping::Vec::const_iterator it;
				for (it = m_paths.begin(); it != end; ++it) {

					const FileSystem::PathMapping &m = *m_it;

					if (m.mask&m_exclude)
						continue;

					if (m.mask&m_mask) {

						String filePath;
						if (!m_root.empty) {
							filePath = m_root + "/" + path;
						} else {
							filePath = path;
						}

						if (m.pak) {
							if (m.pak->FileExists(filePath.c_str))
								break;
						} else {
							String diskPath = m.dir + "/" + filePath;
							String nativePath;
					
							if (m_fs.GetNativePath(diskPath.c_str, nativePath)) {
								if (m_fs.NativeFileExists(nativePath.c_str))
									break;
							}
						}
					}
				}

				if (it == end)
					return true; // not shadowed.
			}

			OpenSearch();
		}
		return false;
	}

private:

	void OpenSearch() {

		m_s.reset();

		for (;m_it != m_paths.end(); ++m_it) {
			
			const FileSystem::PathMapping &m = *m_it;
			if (m.mask&m_exclude)
				continue;
			if (m.mask&m_mask) {
				if (m.pak) {
					m_s = m.pak->OpenSearch(m_path.c_str, m_searchOptions);
				} else {
					String path(m.dir + "/" + m_path);
					String nativePath;
					
					if (m_fs.GetNativePath(path.c_str, nativePath)) {
						m_s = m_fs.NativeOpenSearch(nativePath.c_str, m_searchOptions, m_fileOptions);
					}
				}

				if (m_s) {
					++m_it;
					break;
				}
			}
		}
	}

	String m_path;
	String m_root;
	SearchOptions m_searchOptions;
	FileOptions m_fileOptions;
	int m_mask;
	int m_exclude;

	FileSystem &m_fs;
	FileSearch::Ref m_s;
	FileSystem::PathMapping::Vec m_paths;
	FileSystem::PathMapping::Vec::const_iterator m_it;
};

} // details

///////////////////////////////////////////////////////////////////////////////

FileSystem::FileSystem(const char *root, const char *dvd)
: m_globalMask(kFileMask_Any) {
	RAD_ASSERT(root);

	SetAlias('r', root);
	
	if (dvd)
		SetAlias('d', dvd);
}

void FileSystem::SetAlias(
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
			alias = alias.SubStr(0, alias.numBytes - 1);
		}
	}
}

void FileSystem::AddDirectory(
	const char *path,
	int mask
) {
	RAD_ASSERT(path);
	PathMapping m;
	m.dir = path;
	m.mask = mask;
	m_paths.push_back(m);
}

PakFile::Ref FileSystem::OpenPakFile(
	const char *path,
	::Zone &zone,
	FileOptions options,
	int mask,
	int exclude,
	int *resolved
) {
	MMFile::Ref file = OpenFile(path, zone, options, mask, exclude, resolved);
	if (file)
		return PakFile::Open(file);
	return PakFile::Ref();
}

void FileSystem::AddPakFile(
	const PakFileRef &pakFile,
	int mask
) {
	RAD_ASSERT(pakFile);
	PathMapping m;
	m.pak = pakFile;
	m.mask = mask;
	m_paths.push_back(m);
}

void FileSystem::RemovePakFile(const PakFileRef &pakFile) {
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

void FileSystem::RemovePakFiles() {
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
		if (!GetAbsolutePath(path, spath, mask, exclude, resolved))
			return 0;
	} else {
		spath = CStr(path);
	}

	if (!nativePath) {
		String x;
		if (!GetNativePath(spath.c_str, x))
			return 0;
		spath = x;
	}

	return ::fopen(spath.c_str, mode);
}

MMFile::Ref FileSystem::OpenFile(
	const char *path,
	::Zone &zone,
	FileOptions options,
	int mask,
	int exclude,
	int *resolved
) {
	mask &= m_globalMask;

	if (options & kFileOption_NativePath)
		return NativeOpenFile(path, zone, options);

#if !defined(RAD_OPT_SHIP)
	// backslash characters are illegal in a file path and won't open on multiple platforms.
	for (const char *z = path; *z; ++z) {
		if (*z == '\\')
			return MMFile::Ref();
	}
#endif

	if (IsAbsPath(path)) {
		String nativePath;
		if (!GetNativePath(path, nativePath))
			return MMFile::Ref();
		return NativeOpenFile(nativePath.c_str, zone, options);
	}

	const String kPath(CStr(path));

	RAD_ASSERT_MSG(kPath[0] != '/', "Relative paths should never begin with a '/'!");

	if (resolved)
		*resolved = 0;

	for (PathMapping::Vec::const_iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
		const PathMapping &m = *it;

		if (m.mask&exclude)
			continue;
		if (m.mask&mask) {

			if (m.pak) {
				MMFileRef r = m.pak->OpenFile(kPath.c_str);
				if (r) {
					if (resolved)
						*resolved = m.mask;
					return r;
				}
			} else {
				String spath;
				
				spath = m.dir + "/" + kPath;
			
				if (!GetNativePath(spath.c_str, spath))
					return MMFileRef();
				MMFileRef r = NativeOpenFile(spath.c_str, zone, options);
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

//! Maps an entire file into memory.
MMapping::Ref FileSystem::MapFile(
	const char *path,
	::Zone &zone,
	FileOptions options,
	int mask,
	int exclude,
	int *resolved
) {
	MMFile::Ref file = OpenFile(path, zone, options, mask, exclude, resolved);
	if (!file)
		return MMapping::Ref();
	return file->MMap(0, 0);
}

MMFileInputBuffer::Ref FileSystem::OpenInputBuffer(
	const char *path,
	::Zone &zone,
	AddrSize mappedSize,
	FileOptions options,
	int mask,
	int exclude,
	int *resolved
) {
	MMFile::Ref file = OpenFile(path, zone, options, mask, exclude, resolved);
	if (!file)
		return MMFileInputBuffer::Ref();
	return MMFileInputBuffer::Ref(new (ZFile) MMFileInputBuffer(file, mappedSize, zone));
}

bool FileSystem::GetAbsolutePath(
	const char *path, 
	String &absPath,
	int mask,
	int exclude,
	int *resolved
) {
	RAD_ASSERT(path);
	mask &= m_globalMask;

	if (IsAbsPath(path)) {
		absPath = path;
		return true;
	}

	const String kPath(CStr(path));

	RAD_ASSERT_MSG(kPath[0] != '/', "Relative paths should never begin with a '/'!");

	for (PathMapping::Vec::const_iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
		const PathMapping &m = *it;

		if (m.pak)
			continue;
		if (m.mask&exclude)
			continue;
		if (m.mask&mask) {
		
			absPath = m.dir;
			if (!kPath.empty)
				absPath += "/" + kPath;
			
			if (resolved)
				*resolved = m.mask;
			return true;
		}
	}

	if (resolved)
		*resolved = 0;
	return false;
}

bool FileSystem::GetNativePath(
	const char *path,
	String &nativePath
) {
	RAD_ASSERT(path);

	if (!PathHasAlias(path))
		return false;

	String spath(CStr(path));

#if defined(RAD_OPT_DEBUG)
	boost::array<bool, kAliasMax> m_touched;
	memset(&m_touched[0], 0, sizeof(bool)*kAliasMax);
#endif

	while (PathHasAlias(spath.c_str)) {
		char alias = spath[1];
#if defined(RAD_OPT_DEBUG)
		RAD_ASSERT_MSG(!m_touched[alias], "Expanding recursive file system alias!");
		m_touched[alias] = true;
#endif
		spath = m_aliasTable[alias] + spath.SubStr(3); // include '/' separator.
	}

	nativePath = spath;
	return true;
}

bool FileSystem::ExpandToNativePath(
	const char *path, 
	String &nativePath,
	int mask,
	int exclude,
	int *resolved
) {
	if (!GetAbsolutePath(path, nativePath, mask, exclude, resolved))
		return false;
	return GetNativePath(nativePath.c_str, nativePath);
}

bool FileSystem::FileExists(
	const char *path,
	FileOptions options,
	int mask,
	int exclude,
	int *resolved
) {
	mask &= m_globalMask;

	if (options & kFileOption_NativePath)
		return NativeFileExists(path);

	if (IsAbsPath(path)) {
		String nativePath;
		if (!GetNativePath(path, nativePath))
			return false;
		return NativeFileExists(nativePath.c_str);
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
				if (m.pak->FileExists(kPath.c_str)) {
					if (resolved)
						*resolved = m.mask;
					return true;
				}
			} else {
				String spath = m.dir + "/" + kPath;
				if (!GetNativePath(spath.c_str, spath))
					return false;
				if (NativeFileExists(spath.c_str)) {
					if (resolved)
						*resolved = m.mask;
					return true;
				}
			}
		}
	}

	return false;
}

FileSearch::Ref FileSystem::OpenSearch(
	const char *path,
	SearchOptions searchOptions,
	FileOptions fileOptions,
	int mask,
	int exclude
) {
	if (fileOptions & kFileOption_NativePath)
		return NativeOpenSearch(path, searchOptions, fileOptions);

	if (IsAbsPath(path)) {
		String nativePath;
		if (!GetNativePath(path, nativePath))
			return FileSearch::Ref();
		return NativeOpenSearch(nativePath.c_str, searchOptions, fileOptions);
	}

	mask &= m_globalMask;

	details::DetailsSearch *s = new (ZFile) details::DetailsSearch(*this);
	FileSearch::Ref r(s);

	if (!s->Init(path, searchOptions, fileOptions, mask, exclude))
		return FileSearch::Ref();

	return r;
}

///////////////////////////////////////////////////////////////////////////////

PakFile::Ref PakFile::Open(const MMFile::Ref &file) {
	PakFile::Ref r(new (ZFile) PakFile(file));

	MMFileInputBuffer ib(file, 1*kMeg);
	stream::InputStream is(ib);

	if (!r->m_pak.LoadLumpInfo(kDPakSig, kDPakMagic, is, data_codec::lmp::LittleEndian))
		r.reset();

	return r;
}

PakFile::PakFile(const MMFile::Ref &file) : m_file(file) {
}

MMFile::Ref PakFile::OpenFile(const char *path) {
	MMFile::Ref r;

	const StreamReader::Lump *l = m_pak.GetByName(path);
	if (l)
		r.reset(new (ZFile) MMPakEntry(shared_from_this(), *l));
	return r;
}

FileSearch::Ref PakFile::OpenSearch(
	const char *path,
	SearchOptions searchOptions
) {
	PakSearch *s = new (ZFile) PakSearch(path, searchOptions, shared_from_this());
	FileSearch::Ref r(s);

	String temp;
	if (!s->NextFile(temp, 0, 0))
		return FileSearch::Ref();

	s->Reset();
	return r;
}

bool PakFile::FileExists(const char *path) {
	return m_pak.GetByName(path) != 0;
}

int PakFile::RAD_IMPLEMENT_GET(numLumps) {
	return (int)m_pak.NumLumps();
}

const data_codec::lmp::StreamReader::Lump *PakFile::LumpForIndex(int i) {
	return m_pak.GetByIndex(i);
}

const data_codec::lmp::StreamReader::Lump *PakFile::LumpForName(const char *path) {
	return m_pak.GetByName(path);
}

PakFile::MMPakEntry::MMPakEntry(
	const PakFileRef &pakFile,
	const data_codec::lmp::StreamReader::Lump &lump
) : m_pakFile(pakFile), m_lump(lump) {
}

MMapping::Ref PakFile::MMPakEntry::MMap(AddrSize ofs, AddrSize size, ::Zone &zone) {
	if (size == 0)
		size = ((AddrSize)m_lump.Size()) - ofs;
	RAD_ASSERT(ofs+size <= (AddrSize)m_lump.Size());
	MMapping::Ref mm = m_pakFile->m_file->MMap(
		(AddrSize)m_lump.Ofs() + ofs,
		size,
		zone
	);
	
	if (!mm)
		return MMapping::Ref();
		
	return MMapping::Ref(new (ZFile) MMappedPakEntry(mm, ofs));
}

AddrSize PakFile::MMPakEntry::RAD_IMPLEMENT_GET(size) {
	return (AddrSize)m_lump.Size();
}

PakFile::PakSearch::PakSearch(const char *_path, SearchOptions searchOptions, const PakFile::Ref &pak)
	: m_searchOptions(searchOptions), m_pak(pak), m_idx(0) {

	m_prefix = GetFilePath(_path);
	m_pattern = GetFileName(_path);
}

void PakFile::PakSearch::Reset() {
	m_idx = 0;
}

bool PakFile::PakSearch::NextFile(
	String &path,
	FileAttributes *fileAttributes,
	xtime::TimeDate *fileTime
) {
	const int numLumps = m_pak->numLumps;

	for (; m_idx < numLumps; ++m_idx) {
		const data_codec::lmp::StreamReader::Lump *l = m_pak->LumpForIndex(m_idx);
		if (!l)
			break;

		String lumpPath(CStr(l->Name()));

		if (m_prefix.NCompare(lumpPath, m_prefix.numBytes))
			continue;

		if (!(m_searchOptions & kSearchOption_Recursive)) {
			String name = GetFileName(lumpPath.c_str);
			int len = name.numBytes + m_prefix.numBytes + 1;
			if (len != lumpPath.numBytes)
				continue; // in a sub-directory, not recursing so skip.
		}

		if (PathMatchesExtension(lumpPath.c_str, m_pattern.c_str)) {
			++m_idx;
			path = lumpPath.SubStr(m_prefix.numBytes);
			if (fileAttributes)
				*fileAttributes = kFileAttribute_PakFile;
			if (fileTime)
				memset(fileTime, 0, sizeof(xtime::TimeDate));
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

MMFileInputBuffer::MMFileInputBuffer(
	const MMFile::Ref &file,
	AddrSize mappedSize,
	::Zone &zone
) : m_file(file), m_bufSize((stream::SPos)mappedSize), m_pos(0), m_zone(zone) {
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

			m_mmap = m_file->MMap(m_pos, bufSize, m_zone);
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

RADRT_API String RADRT_CALL GetFileExtension(const char *path) {
	RAD_ASSERT(path);
	String ext;

	int l = string::len(path);
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

RADRT_API String RADRT_CALL SetFileExtension(
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

RADRT_API String RADRT_CALL GetFileName(const char *path) {
	RAD_ASSERT(path);
	
	int l = string::len(path);
	for (int i = l-1; i >= 0; --i) {
		if (path[i] == '/' || path[i] == '\\') {
			return String(&path[i+1]);
		}
	}

	return String(path);
}

RADRT_API String RADRT_CALL GetBaseFileName(const char *path) {
	String filename = GetFileName(path);
	return SetFileExtension(filename.c_str, 0);
}

RADRT_API String RADRT_CALL GetFilePath(const char *path) {
	RAD_ASSERT(path);
	
	int l = string::len(path);
	for (int i = l-1; i >= 0; --i) {
		if (path[i] == '/' || path[i] == '\\') {
			return String(path, i, string::CopyTag);
		}
	}

	return String();
}

RADRT_API bool RADRT_CALL PathMatchesExtension(const char *path, const char *pattern) {
	RAD_ASSERT(path);
	RAD_ASSERT(pattern);

	if (!string::cmp(pattern, "*.*"))
		return true;

	String s = GetFileExtension(path);
	String p = GetFileExtension(pattern);

	return s == p;
}

} // file
