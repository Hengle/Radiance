// File.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "File.h"

using namespace string;

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

	m_aliasTable['r'] = root;

	if (dvd)
		m_aliasTable['d'] = dvd;
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

PakFileRef FileSystem::openPakFile(
	const char *path,
	FileOptions options,
	int mask
) {
	return PakFileRef();
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

	for (PathMapping::Vec::const_iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
		const PathMapping &m = *it;

		if (m.pak)
			continue;
		if (m.mask&exclude)
			continue;
		if (m.mask&mask) {
			const String kPath(CStr(path));

			if (kPath[0] != '/') {
				absPath = m.dir + "/" + kPath;
			} else {
				absPath = m.dir + kPath;
			}

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
		spath = m_aliasTable[alias] + spath.substr(3);
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
				// TODO: Handle Pak Files
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
