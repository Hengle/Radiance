// FileDef.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntFile.h"
#include "../String.h"

namespace file {

RAD_ZONE_DEC(RADRT_API, ZFile);

class FileSystem;
typedef boost::shared_ptr<FileSystem> FileSystemRef;

class PakFile;
typedef boost::shared_ptr<PakFile> PakFileRef;

class FileSearch;
typedef boost::shared_ptr<FileSearch> FileSearchRef;

class MMFile;
typedef boost::shared_ptr<MMFile> MMFileRef;

class MMapping;
typedef boost::shared_ptr<MMapping> MMappingRef;

enum {
	RAD_FLAG(kFileMask_Base),
	RAD_FLAG(kFileMask_Mod),
	RAD_FLAG(kFileMask_PakFiles),
	RAD_FLAG(kFileMask_DVD),
	kFileMask_All = 0xffffffff,
	kFileMask_Any = kFileMask_All
};

RAD_BEGIN_FLAGS
	RAD_FLAG(kFileOption_NativePath),
	kFileOptions_None = 0
RAD_END_FLAGS(FileOptions)

RAD_BEGIN_FLAGS
	RAD_FLAG(kFileAttribute_Hidden),
	RAD_FLAG(kFileAttribute_Normal),
	RAD_FLAG(kFileAttribute_ReadOnly),
	RAD_FLAG(kFileAttribute_Directory),
	kFileAttributes_None = 0
RAD_END_FLAGS(FileAttributes)

RAD_BEGIN_FLAGS
	RAD_FLAG(kSearchOption_Recursive),
	RAD_FLAG(kSearchOption_ReturnNativePaths),
	kSearchOptions_None = 0
RAD_END_FLAGS(SearchOptions)

} // file

RAD_IMPLEMENT_FLAGS(file::FileOptions)
RAD_IMPLEMENT_FLAGS(file::FileAttributes)
RAD_IMPLEMENT_FLAGS(file::SearchOptions)
