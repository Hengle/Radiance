// FileSystemDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <Runtime/InterfaceDef.h>
#include <Runtime/FileDef.h>

namespace file {

enum Info { Version = (1 << 16) | (0) }; // 1.0

enum MediaFlags
{
	RAD_FLAG(CDDVD), // Enable CDDVD media.
	RAD_FLAG(HDD),   // Enable HDD media.
	RAD_FLAG(Mod),   // Enable Mod media.
	RAD_FLAG(Paks),  // Enable Pak files.
	Physical = CDDVD | HDD | Mod,
	AllMedia = Physical | Paks
};

RAD_DECLARE_INTERFACE(HFileSystem, IFileSystem);
RAD_DECLARE_INTERFACE(HAsyncIO, IAsyncIO);
RAD_DECLARE_INTERFACE(HBufferedAsyncIO, IBufferedAsyncIO);
RAD_DECLARE_INTERFACE(HFileData, IFileData);
RAD_DECLARE_INTERFACE(HFile, IFile);
RAD_DECLARE_INTERFACE(HStreamInputBuffer, IStreamInputBuffer);
RAD_DECLARE_INTERFACE(HSearch, ISearch);
RAD_DECLARE_INTERFACE(HPakFile, IPakFile);
RAD_DECLARE_INTERFACE(HIONotify, IIONotify);

} // file

#if !defined(RAD_OPT_NO_REFLECTION)
#if 0
RADREFLECT_DECLARE(RADNULL_API, ::file::Info)
RADREFLECT_DECLARE(RADNULL_API, ::file::MediaFlags)
RADREFLECT_DECLARE(RADNULL_API, ::file::Result)
RADREFLECT_DECLARE_INTERFACE(RADNULL_API, ::file::IFileSystem, ::file::HFileSystem)
RADREFLECT_DECLARE_INTERFACE(RADNULL_API, ::file::IFileData, ::file::HFileData)
RADREFLECT_DECLARE_INTERFACE(RADNULL_API, ::file::IFile, ::file::HFile)
RADREFLECT_DECLARE_INTERFACE(RADNULL_API, ::file::IAsyncIO, ::file::HAsyncIO)
RADREFLECT_DECLARE_INTERFACE(RADNULL_API, ::file::IBufferedAsyncIO, ::file::HBufferedAsyncIO)
RADREFLECT_DECLARE_INTERFACE(RADNULL_API, ::file::IStreamInputBuffer, ::file::HStreamInputBuffer)
RADREFLECT_DECLARE_INTERFACE(RADNULL_API, ::file::ISearch, ::file::HSearch)
RADREFLECT_DECLARE_INTERFACE(RADNULL_API, ::file::IPakFile, ::file::HPakFile)
RADREFLECT_DECLARE_INTERFACE(RADNULL_API, ::file::IIONotify, ::file::HIONotify)
#endif
#endif // !defined(RAD_OPT_NO_REFLECTION)
