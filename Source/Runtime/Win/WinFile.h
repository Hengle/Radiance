// WinFile.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../File.h"
#include "../Container/ZoneDeque.h"
#include "WinHeaders.h"

namespace file {

///////////////////////////////////////////////////////////////////////////////

class WinFileSystem : public FileSystem {
public:

	WinFileSystem(
		const char *root,
		const char *dvd,
		AddrSize pageSize
	);

	virtual FileSearchRef openSearch(
		const char *path,
		SearchOptions searchOptions,
		FileOptions fileOptions,
		int mask
	);

	virtual bool getFileTime(
		const char *path,
		xtime::TimeDate &td,
		FileOptions options
	);

	virtual bool deleteFile(
		const char *path,
		FileOptions options
	);

	virtual bool createDirectory(
		const char *path,
		FileOptions options
	);

	virtual bool deleteDirectory(
		const char *path,
		FileOptions options
	);

protected:

	virtual bool nativeFileExists(const char *path);
	virtual MMFileRef nativeOpenFile(const char *path);

private:

	bool deleteDirectory_r(const char *nativePath);

	AddrSize m_pageSize;
};

///////////////////////////////////////////////////////////////////////////////

class WinMMFile : public MMFile {
public:

	~WinMMFile();

	virtual MMappingRef mmap(
		AddrSize ofs, 
		AddrSize size
	);

protected:

	virtual RAD_DECLARE_GET(size, AddrSize);

private:

	friend class WinFileSystem;

	WinMMFile(HANDLE f, HANDLE m, AddrSize pageSize);

	HANDLE m_f, m_m;
	AddrSize m_pageSize;
};

///////////////////////////////////////////////////////////////////////////////

class WinMMapping : public MMapping {
public:

	~WinMMapping();

	virtual void prefetch(
		AddrSize offset,
		AddrSize size
	);

private:

	friend class WinMMFile;

	WinMMapping(
		const MMFile::Ref &file,
		const void *base,
		const void *data,
		AddrSize size,
		AddrSize offset
	);

	MMFile::Ref m_file; // keeps the file open.
	const void *m_base;
};

///////////////////////////////////////////////////////////////////////////////

class WinFileSearch : public FileSearch {
public:

	~WinFileSearch();

	static FileSearch::Ref create(
		const string::String &path,
		const string::String &prefix,
		SearchOptions options
	);

	virtual bool nextFile(
		string::String &path,
		FileAttributes *fileAttributes,
		xtime::TimeDate *fileTime
	);

private:

	WinFileSearch(
		const string::String &path,
		const string::String &prefix,
		const string::String &pattern,
		SearchOptions options
	);

	enum State {
		kState_Files,
		kState_Dirs,
		kState_Done
	};

	State nextState();

	WIN32_FIND_DATAA m_fd;
	string::String m_path;
	string::String m_prefix;
	string::String m_pattern;
	FileSearch::Ref m_subDir;
	SearchOptions m_options;
	int m_state;
	HANDLE m_h;
};

} // file
