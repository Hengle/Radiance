// File.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "FileDef.h"
#include "../Stream.h"
#include "../TimeDef.h"
#include "../Container/ZoneVector.h"
#include "../DataCodec/LmpReader.h"
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
			- Example: @r:/base/characters/model.3dx

	- Aliases defined by default.
		- r: The directory containing the executable (root directory).
		- w: The working directory that was set when the file system was created.
		- d: DVD drive (if one exists)

	\par Resolving Paths

	- Absolute Paths
		An absolute path contains some alias relative path in the form of @[letter/number]:/dir/file.
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
		char name,
		const char *path
	);

	//! Gets the value of the specified alias.
	string::String alias(char name);

	//! Adds a directory to be used when resolving relative paths.
	/*! \sa addPakFile() */
	void addDirectory(
		const char *path, 
		int mask = kFileMask_All
	);

	//! Opens a pak file.
	PakFileRef openPakFile(
		const char *path, 
		FileOptions options = kFileOptions_None,
		int mask = kFileMask_Any,
		int exclude = 0,
		int *resolved = 0
	);

	//! Adds a pak file to be used when resolving relative paths.
	/*! \sa addDirectory() */
	void addPakFile(
		const PakFileRef &pakFile, 
		int mask = kFileMask_All
	);

	//! Removes a pak file from the resolvers list for relative paths. 
	/*! \sa addPakFile() */
	void removePakFile(const PakFileRef &pakFile);
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
		FileOptions options = kFileOptions_None,
		int mask = kFileMask_Any,
		int exclude = 0,
		int *resolved = 0
	);

	//! Opens a memory mapped file.
	MMFileRef openFile(
		const char *path,
		FileOptions options = kFileOptions_None,
		int mask = kFileMask_Any,
		int exclude = 0,
		int *resolved = 0
	);

	//! Opens a wild-card file search.
	virtual FileSearchRef openSearch(
		const char *path,
		SearchOptions searchOptions = kSearchOption_Recursive,
		FileOptions fileOptions = kFileOptions_None,
		int mask = kFileMask_Any
	) = 0;

	//! Expands a relative path into an absolute path.
	/*! \param path A relative path of the form alias:/path. 
		\param exclude Any directories matching any bits in the exclude mask will not 
		       be used.
		\param resolved optional parameter that contains the mask of the path that was
		       used to resolve the path.

		\note The returned absolute path will only reference a disk path. You cannot access 
		pak files via absolute paths.
	*/
	bool getAbsolutePath(
		const char *path, 
		string::String &absPath,
		int mask = kFileMask_Any,
		int exclude = 0,
		int *resolved = 0
	);

	//! Expands an absolute path to a native path that can be used by stdio and other
	//! OS level functions.
	bool getNativePath(
		const char *path,
		string::String &nativePath
	);

	//! Returns true if the file exists.
	/*! The location that the file was found in is returned by the optional \em resolved
		parameter. */
	bool fileExists(
		const char *path,
		FileOptions options = kFileOptions_None,
		int mask = kFileMask_Any,
		int exclude = 0,
		int *resolved = 0
	);

	//! Gets the TimeDate object for the specified file.
	/*! \note Only absolute paths are accepted by this function. Using a relative path
		will fail. */
	virtual bool getFileTime(
		const char *path,
		xtime::TimeDate &td,
		FileOptions options = kFileOptions_None
	) = 0;

	//! Deletes the specified file.
	/*! \note Only absolute paths are accepted by this function. Using a relative path
		will fail. */
	virtual bool deleteFile(
		const char *path,
		FileOptions options = kFileOptions_None
	) = 0;

	//! Creates the specified directory.
	/*! \note Only absolute paths are accepted by this function. Using a relative path
		will fail. */
	virtual bool createDirectory(
		const char *path,
		FileOptions options = kFileOptions_None
	) = 0;

	//! Deletes the specified directory.
	/*! \note Only absolute paths are accepted by this function. Using a relative path
		will fail. */
	virtual bool deleteDirectory(
		const char *path,
		FileOptions options = kFileOptions_None
	) = 0;

	//! Global mask used on any file operations that take a mask.
	/*! The value of this field does not effect addDirectory() or addPakFile() */
	RAD_DECLARE_PROPERTY(FileSystem, globalMask, int, int);

protected:

	FileSystem(
		const char *root,
		const char *dvd
	);

	virtual bool nativeFileExists(const char *path) = 0;

	virtual MMFileRef nativeOpenFile(
		const char *path,
		FileOptions options
	) = 0;

private:

	RAD_DECLARE_GET(globalMask, int);
	RAD_DECLARE_SET(globalMask, int);

	enum {
		kAliasMax = 255
	};

	struct PathMapping {
		typedef zone_vector<PathMapping, ZFileT>::type Vec;
		PakFileRef pak;
		string::String dir;
		int mask;
	};
	
	PathMapping::Vec m_paths;
	boost::array<string::String, kAliasMax> m_aliasTable;
	int m_globalMask;
};

//! Memory Mapped File
class MMFile : public boost::noncopyable, public boost::enable_shared_from_this<MMFile> {
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
	virtual void prefetch(
		AddrSize offset,
		AddrSize size
	) = 0;

	//! Returns a pointer to the mapped file data.
	RAD_DECLARE_READONLY_PROPERTY(MMapping, data, const void*);
	//! Returns the size of the mapping window.
	RAD_DECLARE_READONLY_PROPERTY(MMapping, size, AddrSize);
	//! Returns the offset into the file this mapping object represents.
	RAD_DECLARE_READONLY_PROPERTY(MMapping, offset, AddrSize);

protected:

	MMapping(
		const void *data,
		AddrSize size,
		AddrSize offset
	);

private:

	RAD_DECLARE_GET(data, const void*);
	RAD_DECLARE_GET(size, AddrSize);
	RAD_DECLARE_GET(offset, AddrSize);

	const void *m_data;
	AddrSize m_size;
	AddrSize m_offset;
};

//! File search
class FileSearch : public boost::noncopyable {
public:
	typedef FileSearchRef Ref;

	virtual bool nextFile(
		string::String &path,
		FileAttributes *fileAttributes = 0,
		xtime::TimeDate *fileTime = 0
	) = 0;

protected:

	FileSearch();
};

//! Pak file.
class PakFile : public boost::noncopyable, public boost::enable_shared_from_this<PakFile> {
public:
	typedef PakFileRef Ref;

	MMFileRef openFile(const char *path);
	bool fileExists(const char *path);

	const data_codec::lmp::StreamReader::Lump *lumpForIndex(int i);
	const data_codec::lmp::StreamReader::Lump *lumpForName(const char *path);

	RAD_DECLARE_READONLY_PROPERTY(PakFile, numLumps, int);

private:

	friend class FileSystem;
	friend class MMPakEntry;

	class MMPakEntry : public MMFile {
	public:

		MMPakEntry(
			const PakFileRef &pakFile,
			const data_codec::lmp::StreamReader::Lump &lump
		);

		virtual MMappingRef mmap(
			AddrSize ofs, 
			AddrSize size
		);

	protected:

		virtual RAD_DECLARE_GET(size, AddrSize);

	private:

		PakFileRef m_pakFile;
		const data_codec::lmp::StreamReader::Lump &m_lump;
	};

	static Ref open(const MMFileRef &file);

	PakFile(const MMFileRef &file);

	RAD_DECLARE_GET(numLumps, int);

	MMFileRef m_file;
	data_codec::lmp::StreamReader m_pak;

};

//! Stream InputBuffer for MMFile's
class MMFileInputBuffer : public boost::noncopyable, public stream::IInputBuffer {
public:
	typedef MMFileInputBufferRef Ref;
	
	MMFileInputBuffer(
		const MMFile::Ref &file,
		AddrSize mappedSize = 8*Meg
	);

	virtual stream::SPos Read(void *buf, stream::SPos numBytes, UReg *errorCode);
	virtual bool SeekIn(stream::Seek seekType, stream::SPos ofs, UReg* errorCode);
	virtual stream::SPos InPos() const;
	virtual stream::SPos Size()  const;

	virtual UReg InCaps() const;
	virtual UReg InStatus() const;

private:

	MMFile::Ref m_file;
	MMapping::Ref m_mmap;
	stream::SPos m_pos;
	stream::SPos m_bufSize;
};

//! Stream InputBuffer for FILE*
class FILEInputBuffer : public boost::noncopyable, public stream::IInputBuffer {
public:
	typedef FILEInputBufferRef Ref;

	FILEInputBuffer(FILE *fp);

	virtual stream::SPos Read(void *buff, stream::SPos numBytes, UReg *errorCode);
	virtual bool SeekIn(stream::Seek seekType, stream::SPos ofs, UReg* errorCode);
	virtual stream::SPos InPos() const;
	virtual stream::SPos Size() const;

	virtual UReg InCaps() const;
	virtual UReg InStatus() const;

private:

	FILE *m_fp;
	stream::SPos m_pos;
};

//! Stream OutputBuffer for FILE*
class FILEOutputBuffer : public boost::noncopyable, public stream::IOutputBuffer {
public:
	typedef FILEOutputBufferRef Ref;

	FILEOutputBuffer(FILE *fp);

	virtual stream::SPos Write(const void* buff, stream::SPos numBytes, UReg* errorCode);
	virtual bool SeekOut(stream::Seek seekType, stream::SPos ofs, UReg* errorCode);
	virtual stream::SPos OutPos() const;
	virtual void Flush();

	virtual UReg OutCaps() const;
	virtual UReg OutStatus() const;

private:

	stream::SPos Size() const;

	FILE *m_fp;
	stream::SPos m_pos;
};

// Helper functions

//! Gets the file extension, including the '.'
RADRT_API string::String RADRT_CALL getFileExtension(const char *path);

//! Sets the extension of a file.
/*! If null is specified for extension then the extension is removed from the file. 
	\param ext The extension to set, including the '.' */
RADRT_API string::String RADRT_CALL setFileExtension(
	const char *path, 
	const char *ext
);

//! Returns the file name without any path components.
RADRT_API string::String RADRT_CALL getFileName(const char *path);

//! Returns the file name without any extension or path components.
RADRT_API string::String RADRT_CALL getBaseFileName(const char *path);

//! Returns the file path without the file name itself.
RADRT_API string::String RADRT_CALL getFilePath(const char *path);

} // file

#include "File.inl"
