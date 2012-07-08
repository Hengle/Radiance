// File.h
// Platform Agnostic File System
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntFile.h"
#include "FileDef.h"
#include "../TimeDef.h"
#include "../ThreadDef.h"
#include <stdio.h>
#include "../PushPack.h"


namespace file {

RADRT_API FILE *RADRT_CALL wfopen(const wchar_t *path, const wchar_t *mode);

// Strings are passed as wchar_t*'s because using a string container will break when
// used across dll's.

//////////////////////////////////////////////////////////////////////////////////////////
// NOTES
//
// The file system operates on platform agnostic file paths. These paths are of the form
// "alias" + "path". The alias is a number, 0 through MaxAliases-1, followed by
// a colon and a forward slash ':/'. This number corresponds to an alias table, and
// the alias is replaced by the string assigned to that alias.
//
// Path and filenames can only contain lowercase characters a-z,
// digits 0-9, and '_' '.' '!'. The '/' character is used to seperate directories.
//
// Aliases enabled by default:
//
// '9', which corresponds to the directory that the application resides in.
// '0' corresponds to the primary hard disk (on windows it's the disk that windows
// is installed on).
// '1' corresponds to the primary dvd/cdrom drive.
//
// Aliases are fully platform specific (except the '/' directory separator is converted).
// It is not recommended to set your own aliases except in cases where they rely on the
// built in ones:
//
// SetAlias(15, "9:/base/somedir");
//
// Windows:
//
// SetAlias(3, "c:/games/somegame");
//
// Examples:
//
// 9:/base/pak0.pak (good)
// 9:^%$/[]/pak0.pak (bad)
//
//////////////////////////////////////////////////////////////////////////////////////////

// subject to MAX alias length

RADRT_API const wchar_t *RADRT_CALL Alias(UReg aliasNumber);
RADRT_API void RADRT_CALL SetAlias(UReg aliasNumber, const wchar_t *string);

//////////////////////////////////////////////////////////////////////////////////////////
// In a debug build you can toggle the enforcement of portable paths via
// EnforcePortablePaths(). When you do this, the path must not exceed the requirements
// of the crappiest platform supported. Path checking is not performed in release builds.
//
// Example: On the PS2 DVD only 8.3 with a maximum of 32 directories deep (or maybe it was
// less) is supported. By coding with this enabled, you are guaranteed to be able to
// run your application on all the platforms supported (from the file system perspective).
//
// When this feature is enabled, only alias's 9, 0, and 1 are available for use by
// paths and other aliases, and ALL user defined aliases MUST use one of them.
//
// Example:
//
// SetAlias(12, "9:/base") // portable, no error
// SetAlias(12, "c:/dir/dir2") // not portable, will cause an assertion
//
// All the limits like path length filename length, extension length, directory name
// length and depth is dependant on the target platform. Running with this enforcement
// emulates the most restrictive platform.
//
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_DEBUG)

RADRT_API bool RADRT_CALL EnforcePortablePaths(bool enforce = true);
RADRT_API bool RADRT_CALL EnforcePortablePathsEnabled();

#endif

RADRT_API bool RADRT_CALL FilePathIsValid(const wchar_t *string);

//////////////////////////////////////////////////////////////////////////////////////////
// Native path expansion.
//
// You can use these functions to convert Radiance file paths into fully native file paths
// for the target system.
//
// Return true if expansion was successful, false if the path was malformed.
//////////////////////////////////////////////////////////////////////////////////////////


RADRT_API bool RADRT_CALL ExpandToNativePath(
	const wchar_t *portablePath,
	wchar_t *nativePath,
	UReg nativePathBufferSize
);

// returns number of characters that would be written to buffer via ExpandPath if
// successful (excluding null terminator), or 0 if error (or path is a null string).

RADRT_API UReg RADRT_CALL ExpandToNativePathLength(
	const wchar_t *portablePath
);

//////////////////////////////////////////////////////////////////////////////////////////
// Alias expansion.
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL ExpandAliases(
	const wchar_t *portablePath,
	wchar_t *expandedPath,
	UReg expandedPathBufferSize
);

RADRT_API UReg RADRT_CALL ExpandAliasesLength(
	const wchar_t *portablePath
);

// Returns the sector size of the device that the given file path resides on. Note this
// doesn't have to be a filename, it can be a path, or an alias.

RADRT_API FPos RADRT_CALL DeviceSectorSize(
	const wchar_t *portablePath,
	int flags
);

//////////////////////////////////////////////////////////////////////////////////////////
// File/Directory deletion.
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL DeleteFile(
	const wchar_t *portablePath,
	int flags
);

RADRT_API bool RADRT_CALL CreateDirectory(
	const wchar_t *portablePath,
	int flags
);

RADRT_API bool RADRT_CALL DeleteDirectory(
	const wchar_t *portablePath,
	int flags
);

//////////////////////////////////////////////////////////////////////////////////////////
// Misc
//////////////////////////////////////////////////////////////////////////////////////////

// test the existance of a file or directory.

RADRT_API bool RADRT_CALL FileExists(
	const wchar_t *portablePath,
	int flags
);

// returns the modified time of a file.

RADRT_API bool RADRT_CALL FileTime(
	const wchar_t *portablePath,
	xtime::TimeDate *td,
	int flags
);

// returned with a '.'

RADRT_API void RADRT_CALL FileExt(
	const wchar_t *path,
	wchar_t *ext,
	UReg extBufferSize
);

// returned with a '.'

RADRT_API UReg RADRT_CALL FileExtLength(
	const wchar_t *path
);

RADRT_API void RADRT_CALL SetFileExt(
	const wchar_t *path,
	const wchar_t *extWithPeriod,
	wchar_t *newPath,
	UReg newPathBufferSize
);

RADRT_API UReg RADRT_CALL SetFileExtLength(
	const wchar_t *path,
	const wchar_t *extWithPeriod
);

//! Returns the path component of the filename.
RADRT_API UReg RADRT_CALL FilePathNameLength(
	const wchar_t *path
);

RADRT_API void RADRT_CALL FilePathName(
	const wchar_t *path,
	wchar_t *pathName,
	UReg pathNameBufferSize
);

//! Returns the length of the path returned by file::FileBaseName()
RADRT_API UReg RADRT_CALL FileBaseNameLength(
	const wchar_t *path
);

//! Returns the base name of a file, without extension or directory components
RADRT_API void RADRT_CALL FileBaseName(
	const wchar_t *path,
	wchar_t *basePath,
	UReg basePathBufferSize
);

} // file


#include "../PopPack.h"
#include "Backend.h"
#include "../PushPack.h"


namespace file {

class Search : private boost::noncopyable
{
public:
	Search();
	~Search();

	bool Open(
		const wchar_t *directory,
		const wchar_t *extWithPeriod,
		SearchFlags flags
	);

	bool NextFile(
		wchar_t *filenameBuffer,
		UReg filenameBufferSize,
		FileAttributes *fileFlags = 0,
		xtime::TimeDate *fileTime = 0
	);

	void Close();
	bool IsValid();

private:

	details::Search m_imp;
};

class AsyncIO : private boost::noncopyable
{
public:
	AsyncIO();
	virtual ~AsyncIO();

	file::Result Result() const;

	//
	// Cancel is asynchronous. To ensure that the IO is canceled
	// call WaitForCompletion() or wait until Result() != Pending.
	//
	void Cancel();
	bool IsCancelled();
	bool WaitForCompletion(U32 timeout = thread::Infinite) const;
	FPos ByteCount() const;
	void SetByteCount(FPos count);

	//
	// Sets the IO status and triggers processing of the
	// completion callback if the status changed from pending
	// to another state. Once the callback is processed, threads
	// blocking in WaitForCompletion() will become active.
	//
	// This call must be synchronized by users of the IO object
	// and should not be invoked if the object is being used by
	// an IO operation, as the completion of that operation will
	// trigger the completion.
	//
	// If force is true, then completion notification is triggered
	// regardless of status.
	//

	void TriggerStatusChange(
		file::Result result,
		bool force = false
	);

protected:

	virtual void OnComplete(file::Result result);

private:

	details::AsyncIO m_imp;

	friend class File;
	friend class details::AsyncIO;
	friend class details::File;
};

class File : private boost::noncopyable
{
public:
	File();
	~File();

	//
	// the file must be opened in Async mode to use asynchronous IO, otherwise all
	// operations are blocking.
	//
	// unless otherwise noted, if the AsyncIO object on any function is NULL, the function is
	// blocking, even in Async mode.
	//
	// all functions returns Success, or an ERROR code (see File.h). A function that
	// takes an AsyncIO object may return Pending, in which case the IO object must
	// be queried for the operations results. As stated above, if no IO object is
	// given, then the function is blocking and Pending will never be returned.
	//

	Result Open(
		const wchar_t *filename,
		CreationType creationType,
		AccessMode accessMode,
		ShareMode shareMode,
		FileOptions fileOptions,
		AsyncIO *io
	);

	Result Close(AsyncIO* io=0);

	//
	// if you opened the file in Accelerated mode, beware the restrictions!
	//
	// filePos determines the file position to start the operation from.
	//
	// bytesRead/bytesWritten can be null.
	//
	Result Read(
		void *buffer,
		FPos bytesToRead,
		FPos *bytesRead,
		FPos filePos,
		AsyncIO *io
	);

	Result Write(
		const void *buffer,
		FPos bytesToWrite,
		FPos *bytesWritten,
		FPos filePos,
		AsyncIO *io
	);

	FPos Size() const;
	FPos SectorSize() const;
	bool CancelIO();
	bool Flush();

	// Allocate a sector aligned buffer for reading the specified #
	// of bytes using async file mode. Returns buffer, and a valid IO size
	// to request for file operations.
	// Free buffer with AlignedFree().
	void *IOMalloc(AddrSize size, AddrSize *ioSize, Zone &zone = ZFile);
	void *SafeIOMalloc(AddrSize size, AddrSize *ioSize, Zone &zone = ZFile);

	static void IOFree(void *p);

private:

	details::File m_imp;
};

} // file


#include "../PopPack.h"
#include "File.inl"
