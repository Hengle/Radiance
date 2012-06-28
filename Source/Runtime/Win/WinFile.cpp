// WinFile.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "WinFile.h"
#include "../TimeDef.h"
#include <direct.h>

using namespace string;
using namespace xtime;

namespace file {

FileSystem::Ref FileSystem::create() {

	String cwd;

	{
		char x[256];
		_getcwd(x, 255);
		x[255] = 0;
		cwd = x;
	}

	cwd.replace('\\', '/');
	
	// Find DVD if there is one

	String sdvd;
	const char *dvd = 0;

	DWORD logicalDriveBits = GetLogicalDrives();
	for (int i = 0; i < 26; ++i) {
		int bit = 1 << i;
		if (logicalDriveBits & bit) {
			char path[4] = {'A' + i, ':', '\\', 0};
			if (GetDriveTypeA(path) == DRIVE_CDROM) {
				path[0] = 'a' + i;
				path[2] = '/';
				sdvd = path;
				break;
			}
		}
	}

	if (!sdvd.empty)
		dvd = sdvd.c_str;

	return FileSystem::Ref(new WinFileSystem(cwd.c_str, dvd));
}

WinFileSystem::WinFileSystem(const char *root, const char *dvd) : FileSystem(root, dvd) {
}

MMFileRef WinFileSystem::openFile(
	const char *path,
	FileOptions options,
	int mask,
	int *resolved
) {
	return MMFileRef();
}

FileSearch::Ref WinFileSystem::openSearch(
	const char *path,
	SearchOptions searchOptions,
	FileOptions fileOptions,
	int mask
) {
	String nativePath;
	if (!getNativePath(path, nativePath))
		return FileSearchRef();

	return WinFileSearch::create(
		nativePath,
		String(),
		searchOptions
	);
}

bool WinFileSystem::getFileTime(
	const char *path,
	xtime::TimeDate &td,
	FileOptions options
) {
	RAD_ASSERT(path);
	String nativePath;

	if (options & kFileOption_NativePath) {
		nativePath = CStr(path);
	} else {
		if (!getNativePath(path, nativePath))
			return false;
	}

	HANDLE h = CreateFileA(
		nativePath.c_str, 
		GENERIC_READ, 
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
	);

	if (h != INVALID_HANDLE_VALUE) {
		FILETIME ft;
		SYSTEMTIME st;

		if (GetFileTime(h, 0, 0, &ft)) {
			FileTimeToLocalFileTime( &ft, &ft );
			FileTimeToSystemTime( &ft, &st );

			td.dayOfMonth = (U8)st.wDay;
			td.month = (U8)st.wMonth;
			td.year = (U16)st.wYear;
			td.hour = (U8)st.wHour;
			td.minute = (U8)st.wMinute;
			td.second = (U8)st.wSecond;
			td.dayOfWeek = (U8)st.wDayOfWeek;
			td.millis = (U16)st.wMilliseconds;

			CloseHandle(h);
		} else {
			CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
	}

	return h != INVALID_HANDLE_VALUE;
}

bool WinFileSystem::deleteFile(
	const char *path,
	FileOptions options
) {
	RAD_ASSERT(path);
	String nativePath;

	if (options & kFileOption_NativePath) {
		nativePath = CStr(path);
	} else {
		if (!getNativePath(path, nativePath))
			return false;
	}

	return DeleteFileA(nativePath.c_str) != 0;
}

bool WinFileSystem::createDirectory(
	const char *path,
	FileOptions options
) {
	RAD_ASSERT(path);
	String nativePath;

	if (options & kFileOption_NativePath) {
		nativePath = CStr(path);
	} else {
		if (!getNativePath(path, nativePath))
			return false;
	}

	// create intermediate directory structure.

	path = nativePath.c_str;
	for (int i = 0; path[i]; ++i) {
		if (path[i] == '/' && (path[1] != ':' || i > 2)) {
			char dir[256];
			ncpy(dir, path, i+1);
			if (!CreateDirectoryA(dir, 0)) {
				if (GetLastError() != ERROR_ALREADY_EXISTS)
					return false;
			}
		}
	}

	if (nativePath[1] != ':' || (nativePath.length > 2)) {
		if (!CreateDirectoryA(nativePath.c_str, 0)) {
			if (GetLastError() != ERROR_ALREADY_EXISTS)
				return false;
		}
	}

	return true;
}

bool WinFileSystem::deleteDirectory_r(const char *nativePath) {

	RAD_ASSERT(nativePath);

	String basePath(CStr(nativePath));
	if (basePath.empty)
		return false;

	FileSearch::Ref s = openSearch(
		(basePath + "/*.*").c_str, 
		kSearchOption_Recursive,
		kFileOption_NativePath, 
		0
	);
	
	if (s) {
		if (*(basePath.end.get()-1) != '/')
			basePath += '/';

		String name;
		FileAttributes fa;

		while (s->nextFile(name, &fa, 0)) {
			String path = basePath + name;

			if (fa & kFileAttribute_Directory) {
				if (!deleteDirectory_r(path.c_str))
					return false;
			} else {
				if (!DeleteFileA(path.c_str))
					return false;
			}
		}

		s.reset();
	}

	return RemoveDirectoryA(nativePath) != 0;
}

bool WinFileSystem::deleteDirectory(
	const char *path,
	FileOptions options
) {
	RAD_ASSERT(path);
	String nativePath;

	if (options & kFileOption_NativePath) {
		nativePath = CStr(path);
	} else {
		if (!getNativePath(path, nativePath))
			return false;
	}

	return deleteDirectory_r(nativePath.c_str);
}

bool WinFileSystem::nativeFileExists(const char *path) {
	RAD_ASSERT(path);
	return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

///////////////////////////////////////////////////////////////////////////////

FileSearch::Ref WinFileSearch::create(
	const string::String &path,
	const string::String &prefix,
	SearchOptions options
) {
	String dir, pattern;

	// split the pattern out of the directory
	const char *sz = path.c_str;
	for (int i = path.length - 1; i >= 0; --i) {
		if (sz[i] == '/' || sz[i] == '\\') {
			dir = String(sz, i, CopyTag);
			pattern = path.substr(i); // include the leading '/'
		}
	}

	if (dir.empty || pattern.empty)
		return FileSearch::Ref();

	WinFileSearch *s = new (ZFile) WinFileSearch(dir, prefix, pattern, options);

	s->m_h = FindFirstFileA(
		path.c_str,
		&s->m_fd
	);

	if (s->m_h == INVALID_HANDLE_VALUE) {
		if (s->nextState() == kState_Done) {
			delete s;
			s = 0;
		}
	}

	return s ? FileSearch::Ref(s) : FileSearch::Ref();
}

WinFileSearch::WinFileSearch(
	const string::String &path,
	const string::String &prefix,
	const string::String &pattern,
	SearchOptions options
) : m_path(path), 
    m_prefix(prefix), 
	m_pattern(pattern), 
	m_options(options), 
	m_state(kState_Files), 
	m_h(INVALID_HANDLE_VALUE) 
{
	m_prefix += "/";
}

WinFileSearch::~WinFileSearch() {
	if (m_h != INVALID_HANDLE_VALUE)
		FindClose(m_h);
}

bool WinFileSearch::nextFile(
	string::String &path,
	FileAttributes *fileAttributes,
	xtime::TimeDate *fileTime
) {
	if (m_subDir) {
		if (m_subDir->nextFile(path, fileAttributes, fileTime))
			return true;
		m_subDir.reset();
	}

	for (;;) {
		if (m_fd.cFileName[0] == 0) {
			if (!FindNextFileA(m_h, &m_fd)) {
				if (nextState() == kState_Done)
					return false;
				RAD_ASSERT(m_fd.cFileName[0] != 0); // nextState() *must* fill this in.
			}
		}

		const String kFileName(CStr(m_fd.cFileName));
		m_fd.cFileName[0] = 0;

		if (kFileName == "." || kFileName == "..")
			continue;

		if (m_fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (m_state == kState_Dirs) {
				path = m_path + "/" + kFileName;
				m_subDir = create(path + m_pattern, m_prefix + kFileName + "/", m_options);
				if (m_subDir->nextFile(path, fileAttributes, fileTime))
					return true;
				m_subDir.reset();
			}
		} else if(m_state == kState_Files) {

			if (m_prefix.empty) {
				path = kFileName;
			} else {
				path = m_prefix + kFileName;
			}

			if (fileAttributes) {
				*fileAttributes = kFileAttributes_None;

				if (m_fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
					*fileAttributes |= kFileAttribute_Hidden;
				if (m_fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
					*fileAttributes |= kFileAttribute_ReadOnly;
				if (m_fd.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
					*fileAttributes |= kFileAttribute_Normal;
			}

			if (fileTime) {
				SYSTEMTIME tm;
				FileTimeToSystemTime(&m_fd.ftLastWriteTime, &tm);

				fileTime->dayOfMonth = (U8)tm.wDay;
				fileTime->month = (U8)tm.wMonth;
				fileTime->year = (U16)tm.wYear;
				fileTime->hour = (U8)tm.wHour;
				fileTime->minute = (U8)tm.wMinute;
				fileTime->second = (U8)tm.wSecond;
				fileTime->dayOfWeek = (U8)tm.wDayOfWeek;
				fileTime->millis = (U16)tm.wMilliseconds;
			}

			if (m_options & kSearchOption_ReturnNativePaths)
				path.replace('/', '\\');

			return true;
		}
	}
}

WinFileSearch::State WinFileSearch::nextState() {
	if (m_state == kState_Files) {
		++m_state;
		// move onto directories? (recursive)
		if (!(m_options & kSearchOption_Recursive))
			++m_state; // skip directory state.

		if (m_state == kState_Dirs) {
			if (m_h != INVALID_HANDLE_VALUE)
				FindClose(m_h);
			String path(m_path + "/*.*");
			m_h = FindFirstFileA(path.c_str, &m_fd);
			if (m_h == INVALID_HANDLE_VALUE)
				++m_state;
		}
	}
	return (State)m_state;
}

} // file