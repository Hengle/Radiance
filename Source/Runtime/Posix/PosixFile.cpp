/*! \file PosixFile.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#include RADPCH
#include "PosixFile.h"
#include "../TimeDef.h"
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dirent.h>
#include <fnmatch.h>
#include "../PushSystemMacros.h"

using namespace xtime;

// In the Main project.
#if defined(RAD_OPT_APPLE)
String AppleGetBundlePath();
#endif

namespace file {

FileSystem::Ref FileSystem::New() {

	String cwd;

#if defined(RAD_OPT_IOS) || (defined(RAD_OPT_OSX) && defined(RAD_OPT_SHIP))
	cwd = AppleGetBundlePath();
#else
	{
		char x[256];
		getcwd(x, 255);
		x[255] = 0;
		cwd = x;
	}
#endif
	
	AddrSize pageSize = (AddrSize)sysconf(_SC_PAGE_SIZE);
	
	return FileSystem::Ref(new PosixFileSystem(cwd.c_str, 0, pageSize));
}

PosixFileSystem::PosixFileSystem(
	const char *root, 
	const char *dvd,
	AddrSize pageSize
) : FileSystem(root, dvd), m_pageSize(pageSize) {
}

MMFile::Ref PosixFileSystem::NativeOpenFile(
	const char *path,
	::Zone &zone,
	FileOptions options
) {

	int fd = open(path, O_RDONLY, 0);
	if (fd == -1)
		return MMFile::Ref();

	MMFile::Ref r(new (ZFile) PosixMMFile(fd, m_pageSize));

	if (options & kFileOption_MapEntireFile) {
		MMapping::Ref mm = r->MMap(0, 0, zone);
		if (mm) {
			static_cast<PosixMMFile*>(r.get())->m_mm = mm;
			// important otherwise we have a circular reference.
			static_cast<PosixMMapping*>(mm.get())->m_file.reset();
		}
	}

	return r;
}

FileSearch::Ref PosixFileSystem::NativeOpenSearch(
	const char *path,
	SearchOptions searchOptions,
	FileOptions fileOptions
) {
	return PosixFileSearch::New(
		String(path), // not CStr since this string persists outside the scope of this function.
		String(),
		searchOptions
	);
}

bool PosixFileSystem::GetFileTime(
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

	struct stat s;
	
	if (::stat(nativePath.c_str, &s) != 0)
		return false;
	
	struct tm t;
	localtime_r(&s.st_mtime, &t);

	td.dayOfMonth = (U8)t.tm_mday;
	td.month      = (U8)t.tm_mon;
	td.year       = (U16)t.tm_year;
	td.hour       = (U8)t.tm_hour;
	td.minute     = (U8)t.tm_min;
	td.second     = (U8)t.tm_sec;
	td.dayOfWeek  = (U8)t.tm_wday;
	td.millis     = 0;

	return true;
}

bool PosixFileSystem::DeleteFile(
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

	return unlink(nativePath.c_str) == 0;
}

bool PosixFileSystem::CreateDirectory(
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
		if ((path[i] == '/') && (i > 2)) {
			char dir[256];
			string::ncpy(dir, path, i+1);
			if (mkdir(dir, 0777) == -1) {
				if (errno != EEXIST)
					return false;
			}
		}
	}

	if (nativePath[0] != '/' || (nativePath.length > 2)) {
		if (mkdir(nativePath.c_str, 0777) == -1) {
			if (errno != EEXIST)
				return false;
		}
	}

	return true;
}

bool PosixFileSystem::DeleteDirectory_r(const char *nativePath) {

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
				if (unlink(path.c_str) != 0) {
					std::cerr << "ERROR: unable to remove file: " << path.c_str.get() << std::endl;
				}
			}
		}

		s.reset();
	}

	if (unlink(nativePath)) {
		std::cerr << "ERROR: unable to remove directory: " << nativePath << std::endl;
	}
	
	return true;
}

bool PosixFileSystem::DeleteDirectory(
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

bool PosixFileSystem::NativeFileExists(const char *path) {
	RAD_ASSERT(path);
	struct stat s;
	return ::stat(path, &s) == 0;
}

///////////////////////////////////////////////////////////////////////////////

PosixMMFile::PosixMMFile(int fd, AddrSize pageSize) : m_fd(fd), m_pageSize(pageSize) {
}

PosixMMFile::~PosixMMFile() {
	if (m_fd != -1)
		close(m_fd);
}

MMapping::Ref PosixMMFile::MMap(AddrSize ofs, AddrSize size, ::Zone &zone) {
	if (size == 0 || (size > this->size.get()))
		size = this->size.get();
	RAD_ASSERT(size > 0);
	RAD_ASSERT(ofs+size <= this->size.get());

	AddrSize base = ofs & ~(m_pageSize-1);
	AddrSize padd = ofs - base;
	AddrSize mappedSize = size+padd;

	if (m_mm) {
		// we mapped the whole file just return the window
		const void *data = reinterpret_cast<const U8*>(m_mm->data.get()) + ofs;
		return MMapping::Ref(new (ZFile) PosixMMapping(shared_from_this(), 0, data, size, ofs, 0, zone));
	}
	
	const void *pbase = mmap(
		0,
		(size_t)mappedSize,
		PROT_READ,
		MAP_PRIVATE,
		m_fd,
		(off_t)base
	);
	
	if (!pbase)
		return MMapping::Ref();

	const void *data = reinterpret_cast<const U8*>(pbase) + padd;
	return MMapping::Ref(new (ZFile) PosixMMapping(
		shared_from_this(),
		pbase,
		data,
		size,
		ofs,
		mappedSize,
		zone
	));
}

AddrSize PosixMMFile::RAD_IMPLEMENT_GET(size) {
	RAD_ASSERT(m_fd != -1);
	struct stat s;
	if (fstat(m_fd, &s) == 0)
		return (AddrSize)s.st_size;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

PosixMMapping::PosixMMapping(
	const MMFile::Ref &file,
	const void *base,
	const void *data,
	AddrSize size,
	AddrSize offset,
	AddrSize mappedSize,
	::Zone &zone
) : MMapping(data, size, offset, mappedSize, zone), m_file(file), m_base(base) {
}

PosixMMapping::~PosixMMapping() {
	if (m_base)
		munmap((void*)m_base, this->mappedSize);
}

void PosixMMapping::Prefetch(AddrSize offset, AddrSize size) {
}

///////////////////////////////////////////////////////////////////////////////

FileSearch::Ref PosixFileSearch::New(
	const String &path,
	const String &prefix,
	SearchOptions options
) {
	String dir, pattern;

	// split the pattern out of the directory
	const char *sz = path.c_str;
	for (int i = path.length - 1; i >= 0; --i) {
		if (sz[i] == '/') {
			dir = String(sz, i, string::CopyTag);
			pattern = path.SubStr(i+1); // no leading /
			break;
		}
	}

	if (dir.empty || pattern.empty)
		return FileSearch::Ref();

	PosixFileSearch *s = new (ZFile) PosixFileSearch(dir, prefix, pattern, options);

	s->m_sdir = opendir(dir.c_str);
	if (!s->m_sdir) {
		delete s;
		return FileSearch::Ref();
	}
	
	s->m_cur = new (ZFile) dirent;
	s->m_cur->d_name[0] = 0;
	
	// must see if a file exists that matches our pattern (prime the search).
	for (;;) {
		struct dirent *next;
		if ((readdir_r((DIR*)s->m_sdir, s->m_cur, &next) != 0) || (next == 0)) {
			::closedir((DIR*)s->m_sdir);
			s->m_sdir = 0;
			break;
		}
		if (next->d_type == DT_REG) {
			if (fnmatch(pattern.c_str, &next->d_name[0], 0) != FNM_NOMATCH)
				break;
		}
	}
	
	if (!s->m_sdir) {
		if (s->NextState() == kState_Done) {
			delete s;
			return FileSearch::Ref();
		}
	}

	return FileSearch::Ref(s);
}

PosixFileSearch::PosixFileSearch(
	const String &path,
	const String &prefix,
	const String &pattern,
	SearchOptions options
) : m_path(path), 
    m_prefix(prefix), 
	m_pattern(pattern), 
	m_options(options), 
	m_state(kState_Files), 
	m_sdir((DIR*)0),
	m_cur(0)
{
	if (m_pattern.length > 1) {
		String x = m_pattern.Right(2);
		if (x == ".*") {
			// fnmatch will require an extension with *.* unlike windows.
			// change to *
			m_pattern = m_pattern.Left(m_pattern.length - 2);
		}
	}
}

PosixFileSearch::~PosixFileSearch() {
	if (m_sdir)
		::closedir((DIR*)m_sdir);
	if (m_cur)
		delete m_cur;
}

bool PosixFileSearch::NextFile(
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
		if (m_cur->d_name[0] == 0) {
			for (;;) {
				struct dirent *next;
				if ((readdir_r((DIR*)m_sdir, m_cur, &next) != 0) || (next == 0)) {
					if (NextState() == kState_Done)
						return false;
					RAD_ASSERT(m_cur->d_name[0] != 0); // nextState() *must* fill this in.
					break;
				}
				if (next->d_type == DT_REG) {
					if (fnmatch(m_pattern.c_str, &next->d_name[0], 0) != FNM_NOMATCH)
						break;
				} else {
					break;
				}
			}
		}

		char kszFileName[256];
		string::cpy(kszFileName, &m_cur->d_name[0]);
		const String kFileName(CStr(kszFileName));
		m_cur->d_name[0] = 0;

		if (kFileName == "." || kFileName == "..")
			continue;

		if (m_cur->d_type == DT_DIR) {
			if (m_state == kState_Dirs) {
				path = m_path + "/" + kFileName;
				m_subDir = New(path + "/" + m_pattern, m_prefix + kFileName + "/", m_options);
				if (m_subDir && m_subDir->NextFile(path, fileAttributes, fileTime))
					return true;
				m_subDir.reset();
			} else if (m_options & kSearchOption_Directories) {
				if (m_prefix.empty) {
					path = String(kFileName, string::CopyTag); // move off of stack.
				} else {
					path = m_prefix + kFileName;
				}

				if (fileAttributes)
					*fileAttributes = kFileAttribute_Directory;

				if (fileTime) {
					struct stat s;
				
					if (::stat(m_cur->d_name, &s) == 0) {
						struct tm t;
						localtime_r(&s.st_mtime, &t);

						fileTime->dayOfMonth = (U8)t.tm_mday;
						fileTime->month      = (U8)t.tm_mon;
						fileTime->year       = (U16)t.tm_year;
						fileTime->hour       = (U8)t.tm_hour;
						fileTime->minute     = (U8)t.tm_min;
						fileTime->second     = (U8)t.tm_sec;
						fileTime->dayOfWeek  = (U8)t.tm_wday;
						fileTime->millis     = 0;
					}
				}

				return true;	
			}
		} else if((m_cur->d_type == DT_REG) && (m_state == kState_Files)) {

			if (m_prefix.empty) {
				path = String(kFileName, string::CopyTag); // move off of stack.
			} else {
				path = m_prefix + kFileName;
			}

			if (fileAttributes)
				*fileAttributes = kFileAttribute_Normal;

			if (fileTime) {
				struct stat s;
				
				if (::stat(m_cur->d_name, &s) == 0) {
					struct tm t;
					localtime_r(&s.st_mtime, &t);

					fileTime->dayOfMonth = (U8)t.tm_mday;
					fileTime->month      = (U8)t.tm_mon;
					fileTime->year       = (U16)t.tm_year;
					fileTime->hour       = (U8)t.tm_hour;
					fileTime->minute     = (U8)t.tm_min;
					fileTime->second     = (U8)t.tm_sec;
					fileTime->dayOfWeek  = (U8)t.tm_wday;
					fileTime->millis     = 0;
				}
			}

			return true;
		}
	}
}

PosixFileSearch::State PosixFileSearch::NextState() {
	if (m_state == kState_Files) {
		++m_state;
		// move onto directories? (recursive)
		if (!(m_options & kSearchOption_Recursive))
			++m_state; // skip directory state.

		if (m_state == kState_Dirs) {
			if (m_sdir)
				closedir((DIR*)m_sdir);
			String path(m_path);
			m_sdir = opendir(m_path.c_str);
			if (m_sdir == 0) {
				++m_state;
			} else {
				struct dirent *next;
				if ((readdir_r((DIR*)m_sdir, m_cur, &next) != 0) || (next == 0)) {
					closedir((DIR*)m_sdir);
					++m_state;
				}
			}
		}
	} else if (m_state == kState_Dirs) {
		++m_state;
	}
	return (State)m_state;
}

} // file
