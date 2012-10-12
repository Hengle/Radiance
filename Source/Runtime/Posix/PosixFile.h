/*! \file PosixFile.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#pragma once

#include "../File.h"
#include "../Container/ZoneDeque.h"
#include "../PushPack.h"

struct dirent;

namespace file {

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS PosixFileSystem : public FileSystem {
public:

	PosixFileSystem(
		const char *root,
		const char *dvd,
		AddrSize pageSize
	);

	virtual bool GetFileTime(
		const char *path,
		xtime::TimeDate &td,
		FileOptions options
	);

	virtual bool DeleteFile(
		const char *path,
		FileOptions options
	);

	virtual bool CreateDirectory(
		const char *path,
		FileOptions options
	);

	virtual bool DeleteDirectory(
		const char *path,
		FileOptions options
	);

protected:

	virtual bool NativeFileExists(const char *path);
	
	virtual MMFileRef NativeOpenFile(
		const char *path,
		::Zone &zone,
		FileOptions options
	);

	virtual FileSearchRef NativeOpenSearch(
		const char *path,
		SearchOptions searchOptions,
		FileOptions options
	);

private:

	bool DeleteDirectory_r(const char *nativePath);

	AddrSize m_pageSize;
};

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS PosixMMFile : public MMFile {
public:

	~PosixMMFile();

	virtual MMappingRef MMap(
		AddrSize ofs, 
		AddrSize size,
		::Zone &zone
	);

protected:

	virtual RAD_DECLARE_GET(size, AddrSize);

private:

	friend class PosixFileSystem;

	PosixMMFile(
		int fd,
		AddrSize pageSize
	);

	int m_fd;
	AddrSize m_pageSize;
	MMapping::Ref m_mm; // only if kFileOption_MapEntireFile is set.
};

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS PosixMMapping : public MMapping {
public:

	~PosixMMapping();

	virtual void Prefetch(
		AddrSize offset,
		AddrSize size
	);

private:

	friend class PosixMMFile;
	friend class PosixFileSystem;

	PosixMMapping(
		const MMFile::Ref &file,
		const void *base,
		const void *data,
		AddrSize size,
		AddrSize offset,
		AddrSize mappedSize,
		AddrSize backingSize,
		::Zone &zone
	);

	MMFile::Ref m_file; // keeps the file open.
	const void *m_base;
	AddrSize m_mappedSize;
};

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS PosixFileSearch : public FileSearch {
public:

	~PosixFileSearch();

	static FileSearch::Ref New(
		const String &path,
		const String &prefix,
		SearchOptions options
	);

	virtual bool NextFile(
		String &path,
		FileAttributes *fileAttributes,
		xtime::TimeDate *fileTime
	);

private:

	PosixFileSearch(
		const String &path,
		const String &prefix,
		const String &pattern,
		SearchOptions options
	);

	enum State {
		kState_Files,
		kState_Dirs,
		kState_Done
	};

	State NextState();

	String m_path;
	String m_prefix;
	String m_pattern;
	FileSearch::Ref m_subDir;
	SearchOptions m_options;
	int m_state;
	void *m_sdir;
	struct dirent *m_cur;
};

} // file

#include "../PopPack.h"
