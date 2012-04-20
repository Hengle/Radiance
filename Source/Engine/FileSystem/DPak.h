// DPak.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "DPakDef.h"
#include "FileSystem.h"
#include <Runtime/ThreadDef.h>

namespace file {

//////////////////////////////////////////////////////////////////////////////////////////
// file::IDPakReader
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IDPakReader, IInterface, file.IDPakReader)

	// Methods

	virtual void Initialize(
		const HFileSystem &fileSystem, 
		thread::IThreadContext *decompressContext = 0, 
		U32 version = Version
	) = 0;

	virtual HPakFile MountPakFile(const HFile &file, const wchar_t *name) = 0;

RAD_INTERFACE_END

//////////////////////////////////////////////////////////////////////////////////////////
// file::IDPakEntry
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IDPakEntry, IFile, file.IDPakEntry)

	// Properties

	RAD_DECLARE_READONLY_PROPERTY(IDPakEntry, tag, const void*);
	RAD_DECLARE_READONLY_PROPERTY(IDPakEntry, tagSize, AddrSize);

protected:

	virtual RAD_DECLARE_GET(tag, const void*) = 0;
	virtual RAD_DECLARE_GET(tagSize, AddrSize) = 0;

RAD_INTERFACE_END

} // file

