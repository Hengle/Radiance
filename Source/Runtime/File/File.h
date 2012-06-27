// File.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "FileDef.h"
#include "../TimeDef.h"
#include "../Container/ZoneMap.h"
#include <stdio.h>

namespace file {

/*! Memory mapped file system.

	All paths in the file system are platform agnostic UTF8 paths. '/' is the directory separator.
	
	- Valid paths
		- Relative Path 
			A path that does not start with an alias.
			- Example: /base/characters/model.3dx
		- Absolute Path
			A path that begins with an alias.
			- Example: r:/base/characters/model.3dx

	- Aliases defined by default.
		- r: The directory containing the executable (root directory).
		- w: The working directory that was set when the file system was created.
		- d: DVD drive (if one exists)

	\par Resolving Paths

	- Absolute Paths
		An absolute path contains some alias relative path in the form of alias:/dir/file.
		The alias referenced by the path is expanded recursively to produce the native path
		used by the file system backend.

	- Relative Paths
		A relative path is a path that does not have any reference to an alias and has the form:
		/dir/file.

		The order in which addPakFile() and addDirectory() are called defines the precidence
		used in locating a file using a relative path. The first instance of a file located
		in a directory or pak file is used by a relative path.

		\sa addPakFile(), addDirectory()

	\note

	The file system is intended to be case-sensative. Resolving a path in a pak file is a
	case-sensative operation. While Linux, Mac, and iOS all have case-sensative file systems,
	Windows does not. When using free-files it is possible to make mistakes in casing of file 
	names that won't hurt you on windows builds.
*/
class FileSystem : public boost::noncopyable {
public:
	typedef FileSystemRef Ref;

	//! Creates a file system object.
	static Ref create();

	virtual ~FileSystem();

	//! Sets the value of an alias.
	void setAlias(
		const char *name,
		const char *path
	);

	//! Gets the value of the specified alias.
	String alias(const char *name);

	//! Adds a directory to be used when resolving relative paths.
	/*! \sa addPakFile() */
	void addDirectory(
		const char *path, 
		int mask = kFileMask_All
	);

	//! Opens a pak file.
	PakFileRef openPakFile(
		const char *path, 
		FileOptions options = kFileOption_None,
		int mask = kFileMask_Any
	) const;

	//! Adds a pak file to be used when resolving relative paths.
	/*! \sa addDirectory() */
	void addPakFile(
		const PakFileRef &file, 
		int mask = kFileMask_All
	);

	//! Removes a pak file from the resolvers list for relative paths. 
	/*! \sa addPakFile() */
	void removePakFile(const PakFileRef &file);
	//! Removes all pak files from the resolvers list for relative paths. */
	/*! \sa addPakFile() */
	void removePakFiles();

	//! Opens a FILE* to a file.
	/*! If the path specified is a relative path then the path is expanded using
		getAbsolutePath(). The path is then expanded to a native path with
		getNativePath() and passed to the stdio function fopen(). 
		
		\sa getAbsolutePath() 
	*/
	FILE *fopen(
		const char *path,
		const char *mode,
		FileOptions options = kFileOption_None,
		int mask = kFileMask_Any,
		int exclude = 0,
		int *resolved = 0
	);

	//! Opens a memory mapped file.
	MMFileRef openFile(
		const char *path,
		FileOptions options = kFileOption_None,
		int mask = kFileMask_Any,
		int *resolved = 0
	);

	//! Opens a wild-card file search.
	FileSearchRef openSearch(
		const char *path,
		FileOptions options = kFileOption_None,
		int mask = kFileMask_Any
	);

	//! Expands a relative path into an absolute path.
	/*! \param path An relative path of the form alias:/path. 
		\param exclude Any directories matching any bits in the exclude mask will not 
		       be used.
		\param resolved optional parameter that contains the mask of the path that was
		       used to resolve the path.
	*/
	bool getAbsolutePath(
		const char *path, 
		String &expanded,
		int mask = kFileMask_Any,
		int exclude = 0,
		int *resolved = 0
	);

	//! Expands an absolute path to a native path that can be used by stdio and other
	//! OS level functions.
	/*! \note Only absolute paths are accepted by this function. Using a relative path
		will fail. */
	bool getNativePath(
		const char *path,
		String &nativePath
	);

	//! Gets the TimeDate object for the specified file.
	/*! \note Only absolute paths are accepted by this function. Using a relative path
		will fail. */
	bool getFileTime(
		const char *path,
		xtime::TimeDate &td,
		FileOptions options = kFileOption_None
	);

	//! Deletes the specified file.
	/*! \note Only absolute paths are accepted by this function. Using a relative path
		will fail. */
	bool deleteFile(
		const char *path,
		FileOptions options = kFileOption_None
	);

	//! Deletes the specified directory.
	/*! \note Only absolute paths are accepted by this function. Using a relative path
		will fail. */
	bool deleteDirectory(
		const char *path,
		FileOptions options = kFileOption_None
	);

	//! Global mask used on any file operations that take a mask.
	/*! The value of this field does not effect addDirectory() or addPakFile() */
	RAD_DECLARE_PROPERTY(FileSystem, globalMask, int, int);

	//! Gets the system memory mapping page size.
	RAD_DECLARE_READONLY_PROPERTY(FileSystem, pageSize, AddrSize);

protected:

	FileSystem(
		const char *root,
		const char *cwd,
		const char *dvd
	);

	virtual RAD_DECLARE_GET(pageSize, int) = 0;

private:

	RAD_DECLARE_GET(globalMask, int);
	RAD_DECLARE_SET(globalMask, int);

	int m_globalMask;

};

//! Memory Mapped File
class MMFile : public boost::noncopyable {
public:
	typedef MMFileRef Ref;

	//! Maps the specified file data into user address space.
	virtual MMappingRef mmap(
		AddrSize ofs, 
		AddrSize size
	) = 0;

	//! The size of the file.
	RAD_DECLARE_READONLY_PROPERTY(MMFile, size, AddrSize);

protected:

	MMFile();

	virtual RAD_DECLARE_GET(size, AddrSize) = 0;

};

//! Memory mapping object.
class MMapping : public boost::noncopyable {
public:
	typedef MMappingRef Ref;

	//! Notifies the operating system that the specified memory will be
	//! used and should be prefetched into memory.
	/*! \notes This function may not improve performance or may be ignored
		by the OS. Equivelent to madvise(WILLNEED). */
	void prefetch(
		AddrSize offset,
		AddrSize size
	);

	//! Returns a pointer to the mapped file data.
	RAD_DECLARE_READONLY_PROPERTY(MMapping, data, const void*);
	//! Returns the size of the mapping window.
	RAD_DECLARE_READONLY_PROPERTY(MMapping, size, AddrSize);

protected:

	MMapping(
		const void *data,
		AddrSize size
	);

private:

	RAD_DECLARE_GET(data, const void*);
	RAD_DECLARE_GET(size, AddrSize);

	const void *m_data;
	AddrSize m_size;
};

// Helper functions

//! Sets the extension of a file.
/*! If null is specified for extension then the extension is removed from the file. */
RADRT_API String RADRT_CALL setFileExtension(
	const char *file, 
	const char *ext
);

//! Returns the file name without any path components.
RADRT_API String RADRT_CALL getFileName(const char *path);

//! Returns the file name without any extension or path components.
RADRT_API String RADRT_CALL getBaseFileName(const char *path);

//! Returns the file path without the file name itself.
RADRT_API String RADRT_CALL getFilePath(const char *path);

} // file
