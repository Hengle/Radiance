// WinFile.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../File.h"
#include "../Container/ZoneDeque.h"
#include "WinHeaders.h"

namespace file {

class WinFileSystem : public FileSystem {
public:

	WinFileSystem(
		const char *root,
		const char *dvd
	);

	virtual MMFileRef openFile(
		const char *path,
		FileOptions options,
		int mask,
		int *resolved
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

private:

	bool deleteDirectory_r(const char *nativePath);

};

class WinMMFile : public MMFile {
public:

	virtual MMappingRef mmap(
		AddrSize ofs, 
		AddrSize size
	);
};

class WinMMapping : public MMapping {
public:

	WinMMapping();

	virtual void prefetch(
		AddrSize offset,
		AddrSize size
	);

};

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
