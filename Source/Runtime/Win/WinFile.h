// WinFile.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../File.h"
#include "../Container/ZoneDeque.h"
#include "WinHeaders.h"
#include "../PushPack.h"

namespace file {

///////////////////////////////////////////////////////////////////////////////

class WinFileSystem : public FileSystem {
public:

	WinFileSystem(
		const char *root,
		const char *dvd,
		AddrSize pageSize
	);

	virtual FileSearchRef OpenSearch(
		const char *path,
		SearchOptions searchOptions,
		FileOptions fileOptions,
		int mask
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

class WinMMFile : public MMFile {
public:

	~WinMMFile();

	virtual MMappingRef MMap(
		AddrSize ofs, 
		AddrSize size,
		::Zone &zone
	);

protected:

	virtual RAD_DECLARE_GET(size, AddrSize);

private:

	friend class WinFileSystem;

	WinMMFile(
		HANDLE f, 
		HANDLE m, 
		AddrSize pageSize
	);

	HANDLE m_f, m_m;
	AddrSize m_pageSize;
	MMapping::Ref m_mm; // only if kFileOption_MapEntireFile is set.
};

///////////////////////////////////////////////////////////////////////////////

class WinMMapping : public MMapping {
public:

	~WinMMapping();

	virtual void Prefetch(
		AddrSize offset,
		AddrSize size
	);

private:

	friend class WinMMFile;
	friend class WinFileSystem;

	WinMMapping(
		const MMFile::Ref &file,
		const void *base,
		const void *data,
		AddrSize size,
		AddrSize offset,
		AddrSize backingSize,
		::Zone &zone
	);

	MMFile::Ref m_file; // keeps the file open.
	const void *m_base;
};

///////////////////////////////////////////////////////////////////////////////

class WinFileSearch : public FileSearch {
public:

	~WinFileSearch();

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

	WinFileSearch(
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

	WIN32_FIND_DATAA m_fd;
	String m_path;
	String m_prefix;
	String m_pattern;
	FileSearch::Ref m_subDir;
	SearchOptions m_options;
	int m_state;
	HANDLE m_h;
};

} // file

#include "../PopPack.h"
