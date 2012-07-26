// WinFile.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "WinFile.h"
#include "../TimeDef.h"
#include <direct.h>
#include "../PushSystemMacros.h"

using namespace xtime;

namespace file {

FileSystem::Ref FileSystem::New() {

	String cwd;

	{
		char x[256];
		_getcwd(x, 255);
		x[255] = 0;
		cwd = x;
	}

	cwd.Replace('\\', '/');
	cwd.Lower();
	
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
				path[2] = 0;
				sdvd = path;
				sdvd.Lower();
				break;
			}
		}
	}

	if (!sdvd.empty)
		dvd = sdvd.c_str;

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	return FileSystem::Ref(new WinFileSystem(cwd.c_str, dvd, (AddrSize)si.dwAllocationGranularity));
}

WinFileSystem::WinFileSystem(
	const char *root, 
	const char *dvd,
	AddrSize pageSize
) : FileSystem(root, dvd), m_pageSize(pageSize) {
}

MMFile::Ref WinFileSystem::NativeOpenFile(
	const char *path,
	::Zone &zone,
	FileOptions options
) {
	HANDLE f = CreateFileA(
		path,
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
	);

	if (f == INVALID_HANDLE_VALUE)
		return MMFileRef();

	HANDLE m = CreateFileMapping(
		f,
		0,
		PAGE_READONLY,
		0,
		0,
		0
	);

	if (m == INVALID_HANDLE_VALUE) {
		CloseHandle(f);
		return MMFileRef();
	}

	MMFile::Ref r(new (ZFile) WinMMFile(f, m, m_pageSize));

	if (options & kFileOption_MapEntireFile) {
		MMapping::Ref mm = r->MMap(0, 0, zone);
		if (mm) {
			static_cast<WinMMFile*>(r.get())->m_mm = mm;
			// important otherwise we have a circular reference.
			static_cast<WinMMapping*>(mm.get())->m_file.reset();
		}
	}

	return r;
}

FileSearch::Ref WinFileSystem::NativeOpenSearch(
	const char *path,
	SearchOptions searchOptions,
	FileOptions fileOptions
) {
	return WinFileSearch::New(
		String(path), // not CStr since this string persists outside the scope of this function.
		String(),
		searchOptions
	);
}

bool WinFileSystem::GetFileTime(
	const char *path,
	xtime::TimeDate &td,
	FileOptions options
) {
	RAD_ASSERT(path);
	String nativePath;

	if (options & kFileOption_NativePath) {
		nativePath = CStr(path);
	} else {
		if (!GetNativePath(path, nativePath))
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

		if (::GetFileTime(h, 0, 0, &ft)) {
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

bool WinFileSystem::DeleteFile(
	const char *path,
	FileOptions options
) {
	RAD_ASSERT(path);
	String nativePath;

	if (options & kFileOption_NativePath) {
		nativePath = CStr(path);
	} else {
		if (!GetNativePath(path, nativePath))
			return false;
	}

	return DeleteFileA(nativePath.c_str) != 0;
}

bool WinFileSystem::CreateDirectory(
	const char *path,
	FileOptions options
) {
	RAD_ASSERT(path);
	String nativePath;

	if (options & kFileOption_NativePath) {
		nativePath = CStr(path);
	} else {
		if (!GetNativePath(path, nativePath))
			return false;
	}

	// create intermediate directory structure.

	path = nativePath.c_str;
	for (int i = 0; path[i]; ++i) {
		if (path[i] == '/' && (path[1] != ':' || i > 2)) {
			char dir[256];
			string::ncpy(dir, path, i+1);
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

bool WinFileSystem::DeleteDirectory_r(const char *nativePath) {

	RAD_ASSERT(nativePath);

	String basePath(CStr(nativePath));
	if (basePath.empty)
		return false;

	FileSearch::Ref s = OpenSearch(
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

		while (s->NextFile(name, &fa, 0)) {
			String path = basePath + name;

			if (fa & kFileAttribute_Directory) {
				if (!DeleteDirectory_r(path.c_str))
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

bool WinFileSystem::DeleteDirectory(
	const char *path,
	FileOptions options
) {
	RAD_ASSERT(path);
	String nativePath;

	if (options & kFileOption_NativePath) {
		nativePath = CStr(path);
	} else {
		if (!GetNativePath(path, nativePath))
			return false;
	}

	return DeleteDirectory_r(nativePath.c_str);
}

bool WinFileSystem::NativeFileExists(const char *path) {
	RAD_ASSERT(path);
	return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

///////////////////////////////////////////////////////////////////////////////

WinMMFile::WinMMFile(HANDLE f, HANDLE m, AddrSize pageSize) : m_f(f), m_m(m), m_pageSize(pageSize) {
}

WinMMFile::~WinMMFile() {
	m_mm.reset();

	if (m_m != INVALID_HANDLE_VALUE)
		CloseHandle(m_m);
	if (m_f != INVALID_HANDLE_VALUE)
		CloseHandle(m_f);
}

MMapping::Ref WinMMFile::MMap(AddrSize ofs, AddrSize size, ::Zone &zone) {
	if (size == 0)
		size = this->size.get();

	RAD_ASSERT(ofs+size <= this->size.get());

	AddrSize base = ofs & ~(m_pageSize-1);
	AddrSize padd = ofs - base;
	AddrSize backingSize = (size+padd) - base;

	if (m_mm) {
		// we mapped the whole file just return the window
		const void *data = reinterpret_cast<const U8*>(m_mm->data.get()) + ofs;
		return MMapping::Ref(new (ZFile) WinMMapping(shared_from_this(), 0, data, size, ofs, backingSize, zone));
	}

	const void *pbase = MapViewOfFile(
		m_m,
		FILE_MAP_READ,
		0,
		(DWORD)base,
		(DWORD)(size + padd)
	);

	const void *data = reinterpret_cast<const U8*>(pbase) + padd;
	return MMapping::Ref(new (ZFile) WinMMapping(shared_from_this(), pbase, data, size, ofs, backingSize, zone));
}

AddrSize WinMMFile::RAD_IMPLEMENT_GET(size) {
	return (AddrSize)GetFileSize(m_f, 0);
}

///////////////////////////////////////////////////////////////////////////////

WinMMapping::WinMMapping(
	const MMFile::Ref &file,
	const void *base,
	const void *data,
	AddrSize size,
	AddrSize offset,
	AddrSize backingSize,
	::Zone &zone
) : MMapping(data, size, offset, backingSize, zone), m_file(file), m_base(base) {
}

WinMMapping::~WinMMapping() {
	if (m_base)
		UnmapViewOfFile(m_base);
}

void WinMMapping::Prefetch(AddrSize offset, AddrSize size) {
	RAD_NOT_USED(offset);
	RAD_NOT_USED(size);
}

///////////////////////////////////////////////////////////////////////////////

FileSearch::Ref WinFileSearch::New(
	const String &path,
	const String &prefix,
	SearchOptions options
) {
	String dir, pattern;

	// split the pattern out of the directory
	const char *sz = path.c_str;
	for (int i = path.length - 1; i >= 0; --i) {
		if (sz[i] == '/' || sz[i] == '\\') {
			dir = String(sz, i, string::CopyTag);
			pattern = path.SubStr(i); // include the leading '/'
			break;
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
		if (s->NextState() == kState_Done) {
			delete s;
			s = 0;
		}
	}

	return s ? FileSearch::Ref(s) : FileSearch::Ref();
}

WinFileSearch::WinFileSearch(
	const String &path,
	const String &prefix,
	const String &pattern,
	SearchOptions options
) : m_path(path), 
    m_prefix(prefix), 
	m_pattern(pattern), 
	m_options(options), 
	m_state(kState_Files), 
	m_h(INVALID_HANDLE_VALUE) 
{
}

WinFileSearch::~WinFileSearch() {
	if (m_h != INVALID_HANDLE_VALUE)
		FindClose(m_h);
}

bool WinFileSearch::NextFile(
	String &path,
	FileAttributes *fileAttributes,
	xtime::TimeDate *fileTime
) {
	if (m_subDir) {
		if (m_subDir->NextFile(path, fileAttributes, fileTime))
			return true;
		m_subDir.reset();
	}

	for (;;) {
		if (m_fd.cFileName[0] == 0) {
			if (!FindNextFileA(m_h, &m_fd)) {
				if (NextState() == kState_Done)
					return false;
				RAD_ASSERT(m_fd.cFileName[0] != 0); // nextState() *must* fill this in.
			}
		}

		char kszFileName[256];
		string::cpy(kszFileName, m_fd.cFileName);
		const String kFileName(CStr(kszFileName));
		m_fd.cFileName[0] = 0;

		if (kFileName == "." || kFileName == "..")
			continue;

		if (m_fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (m_state == kState_Dirs) {
				path = m_path + "/" + kFileName;
				m_subDir = New(path + m_pattern, m_prefix + kFileName + "/", m_options);
				if (m_subDir && m_subDir->NextFile(path, fileAttributes, fileTime))
					return true;
				m_subDir.reset();
			}
		} else if(m_state == kState_Files) {

			if (m_prefix.empty) {
				path = String(kFileName, string::CopyTag); // move off of stack.
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
				path.Replace('/', '\\');

			return true;
		}
	}
}

WinFileSearch::State WinFileSearch::NextState() {
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
	} else if (m_state == kState_Dirs) {
		++m_state;
	}
	return (State)m_state;
}

} // file