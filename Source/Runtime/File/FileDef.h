// FileDef.h
// Platform Agnostic File System Definitions.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntFile.h"
#include "../PushPack.h"


namespace file {

RAD_ZONE_DEC(RADRT_API, ZFile);

// FPos is the maximum offset and byte size for file positions and read/writes.
typedef U32 FPos;

enum
{
	MinFPos = 0,
	MaxFPos = MaxU32
};

// Aliases

enum
{
	AliasRoot = 9,
	AliasHDD = 0,
	AliasCDDVD = 1,
	MaxAliases = 10
};

//////////////////////////////////////////////////////////////////////////////////////////
// Flags and Return Codes
//////////////////////////////////////////////////////////////////////////////////////////

enum Result
{
	// operational result flags
	ErrorPartial = -1, // only some of the bytes were read/written.
	ErrorAborted = -2, // the IO operation was aborted.
	ErrorAccessDenied = -3,
	ErrorFileNotFound = -4,
	ErrorGeneric = -5,
	ErrorExpansionFailed = -6, // ExpandToNative path failed on the requested filepath.
	ErrorInvalidArguments = -7,
	ErrorDriveNotReady = -8,
	ErrorTrayOpen = -9,
	ErrorCompressionFailure = -10,
	ErrorOutOfMemory = -11,
	Success = 0,
	Pending
};

enum ShareMode
{
	// open flags
	RAD_FLAG(ShareRead), // allows others to request read access.
	RAD_FLAG(ShareWrite),      // allows others to request write access.
	RAD_FLAG(ShareTemporary)   // allows others to open with Temporary.
};

enum CreationType
{
	CreateAlways,    // always creates a new file. if the file exists, it is deleted, and the function creates the file as if CreateNew was used.
	CreateNew,       // creates a new file. if the file already exists, the function fails.
	OpenExisting,    // the function fails if the file does not exist.
	OpenAlways,      // opens the file, if it exists. if the file does not exist, the function creates the file as if CreateNew was used.
	TruncateExisting // opens the file and truncates it to a zero length. fails if the file does not exist.
};

enum AccessMode
{
	RAD_FLAG(AccessRead),
	RAD_FLAG(AccessWrite)
};

enum FileOptions
{
	//
	// Asynchronous IO mode.
	//
	// accelerates reads/writes (typically disables buffering on the device). using this flag is
	// not a guarantee of increased IO performance. this flag provides for maximum
	// throughput. the implementation of this option is platform specific,
	// but you must obey the following rules when this option is used:
	//
	// file access (read/write) must start at file byte offsets that are multiples of the devices' sector size.
	// the number of bytes read/written in an operation must be multiples of the devices' sector size.
	// the buffer used must be aligned to a multiple of the devices' sector size.
	//
	// this flag may not provide increased performance on certain devices or under certain circumstances.
	//
	RAD_FLAG(Async)
};

enum FileAttributes
{
	// file attributes
	RAD_FLAG(Hidden),
	RAD_FLAG(Normal),
	RAD_FLAG(ReadOnly),
	RAD_FLAG(Directory)
};

enum SearchFlags
{
	// search flags
	RAD_FLAG(Recursive),
	RAD_FLAG(DirNames),
	RAD_FLAG(FileNames)
};

// legal for FileOptions and SearchFlags!
enum
{
	//
	// tells the system that no path or filename translation is necessary, and the raw string should be used as the
	// full path for the platforms file system.
	//
	RAD_FLAG_BIT(NativePath, 24),

	MiscFlagsAll = NativePath
};

class Search;
class AsyncIO;
class File;

extern RADRT_API const char * const NativePathSeparator;

} // file


#include "../PopPack.h"
