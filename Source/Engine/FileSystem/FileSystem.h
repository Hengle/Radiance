// FileSystem.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <Runtime/Interface.h>
#include "FileSystemDef.h"
#include <Runtime/StreamDef.h>
#include <Runtime/String.h>
#include <Runtime/PushPack.h>

namespace file {

//////////////////////////////////////////////////////////////////////////////////////////
// file::IFileSystem
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IFileSystem, IInterface, file.IFileSystem)

	// Methods

	virtual void Initialize(int enabledMedia, U32 version = Version) = 0;

	// paks added last have precident over paks added first.
	virtual void AddPakFile(const HPakFile &pakFile) = 0;
	virtual void RemovePakFile(const HPakFile &pakFile) = 0;
	virtual void ReleaseAllPakRefs() = 0;
	virtual HPakFile Pak(int num) = 0;
	
	virtual HSearch OpenSearch(
		const char *path, 
		const char *extIncludingPeriod, 
		int media
	) = 0;

	virtual HStreamInputBuffer CreateStreamBuffer(
		FPos size, 
		Zone &zone = ZFile
	) = 0;

	//
	// Same as CreateStreamBuffer() except if memory allocation fails
	// the app is terminated.
	//
	virtual HStreamInputBuffer SafeCreateStreamBuffer(
		FPos size, 
		Zone &zone = ZFile
	) = 0;

	virtual HBufferedAsyncIO CreateBufferedIO(
		FPos size, 
		FPos alignment = 8,
		Zone &zone = ZFile
	) = 0;

	//
	// Same as CreateBufferedIO except that if memory allocation fails the app
	// is terminated.
	//
	virtual HBufferedAsyncIO SafeCreateBufferedIO(
		FPos size, 
		FPos alignment = 8,
		Zone &zone = ZFile
	) = 0;

	virtual Result LoadFile(
		const char          *path,
		int                    &media,
		HBufferedAsyncIO       &asyncIO,
		const HIONotify        &ioComplete,
		FPos alignment = 8,
		Zone &zone = ZFile,
		bool relativePath = true
	) = 0;

	virtual Result OpenFile(
		const char *path, 
		int media, 
		HFile &file, 
		const HIONotify &ioComplete,
		bool relativePath = true
	) = 0;

	virtual Result OpenFileStream(
		const char *path,
		int &media,
		HStreamInputBuffer &stream,
		const HIONotify &ioComplete,
		FPos bufSize = 4*Kilo,
		Zone &zone = ZFile,
		bool relativePath = true
	) = 0;

	// returns the media where the file was found.
	virtual int FileExists(const char *path, int media) = 0;
	virtual bool FileSize(const char *path, int media, FPos& size) = 0;

	// returns the media the file was deleted from.
	virtual int DeleteFile(const char *path, int media) = 0;

	// Properties

	RAD_DECLARE_READONLY_PROPERTY(IFileSystem, numPaks, int);
	RAD_DECLARE_PROPERTY(IFileSystem, enabledMedia, int, int);
	RAD_DECLARE_PROPERTY(IFileSystem, cddvdRoot, const char*, const char*);
	RAD_DECLARE_PROPERTY(IFileSystem, hddRoot, const char*, const char*);
	RAD_DECLARE_PROPERTY(IFileSystem, modRoot, const char*, const char*);

protected:

	// Property Accessors

	virtual RAD_DECLARE_GET(numPaks, int) = 0;
	virtual RAD_DECLARE_GET(enabledMedia, int) = 0;
	virtual RAD_DECLARE_SET(enabledMedia, int) = 0;
	virtual RAD_DECLARE_GET(cddvdRoot, const char*) = 0;
	virtual RAD_DECLARE_SET(cddvdRoot, const char*) = 0;
	virtual RAD_DECLARE_GET(hddRoot, const char*) = 0;
	virtual RAD_DECLARE_SET(hddRoot, const char*) = 0;
	virtual RAD_DECLARE_GET(modRoot, const char*) = 0;
	virtual RAD_DECLARE_SET(modRoot, const char*) = 0;

RAD_INTERFACE_END

//////////////////////////////////////////////////////////////////////////////////////////
// file::IFileData
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IFileData, IInterface, file.IFileData)

	// Properties

	RAD_DECLARE_READONLY_PROPERTY(IFileData, size, FPos);
	RAD_DECLARE_READONLY_PROPERTY(IFileData, ptr, const void *);

protected:

	// Property Accessors

	virtual RAD_DECLARE_GET(size, FPos) = 0;
	virtual RAD_DECLARE_GET(ptr, const void *) = 0;

RAD_INTERFACE_END

//////////////////////////////////////////////////////////////////////////////////////////
// file::IIONotify
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IIONotify, IInterface, file.IIONotify)

	virtual void Notify(Result result) = 0;

RAD_INTERFACE_END

//////////////////////////////////////////////////////////////////////////////////////////
// file::IAsyncIO
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IAsyncIO, IInterface, file.IAsyncIO)

	// Methods

	virtual void Cancel() = 0;
	virtual void WaitForCompletion() = 0;
	
	// Property Accessors

	RAD_DECLARE_READONLY_PROPERTY(IAsyncIO, result, Result);
	RAD_DECLARE_READONLY_PROPERTY(IAsyncIO, ioSize, FPos);
	RAD_DECLARE_READONLY_PROPERTY(IAsyncIO, byteCount, FPos);
	
protected:

	// Property Accessors

	virtual RAD_DECLARE_GET(result, Result) = 0;
	virtual RAD_DECLARE_GET(ioSize, FPos) = 0;
	virtual RAD_DECLARE_GET(byteCount, FPos) = 0;

RAD_INTERFACE_END

//////////////////////////////////////////////////////////////////////////////////////////
// file::IBufferedAsyncIO
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IBufferedAsyncIO, IAsyncIO, file.IBufferedAsyncIO)

	// Property Accessors

	//
	// This data field must be reaquired after each operation that uses
	// the buffered IO object. This field gives access to how much data was
	// read, and the buffer.
	//
	RAD_DECLARE_READONLY_PROPERTY(IBufferedAsyncIO, data, HFileData);
	RAD_DECLARE_READONLY_PROPERTY(IBufferedAsyncIO, bufferSize, FPos);
	RAD_DECLARE_READONLY_PROPERTY(IBufferedAsyncIO, alignment, int);
	
protected:

	// Property Accessors

	virtual RAD_DECLARE_GET(data, HFileData) = 0;
	virtual RAD_DECLARE_GET(bufferSize, FPos) = 0;
	virtual RAD_DECLARE_GET(alignment, FPos) = 0;

RAD_INTERFACE_END

//////////////////////////////////////////////////////////////////////////////////////////
// file::IStream
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IStreamInputBuffer, IInterface, file.IStreamInputBuffer)

	// Methods

	virtual void Bind(const HFile &file) = 0;

	// Property Accessors

	RAD_DECLARE_READONLY_PROPERTY(IStreamInputBuffer, buffer, ::stream::IInputBuffer&);
	
protected:

	// Property Accessors

	virtual RAD_DECLARE_GET(buffer, ::stream::IInputBuffer&) = 0;

RAD_INTERFACE_END

//////////////////////////////////////////////////////////////////////////////////////////
// file::IFile
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IFile, IInterface, file.IFile)

	// Methods

	// For maximum read speed, request file offsets that are aligned to the
	// buffered io object alignment.

	virtual Result Read(const HBufferedAsyncIO &io, FPos fileOffset, FPos sizeToRead, const HIONotify &ioComplete) = 0;

	virtual Result Load(
		HBufferedAsyncIO       &asyncIO,
		const HIONotify        &ioComplete,
		FPos alignment = 8,
		Zone &zone = ZFile
	) = 0;

	// Property Accessors

	RAD_DECLARE_READONLY_PROPERTY(IFile, size, FPos);
	RAD_DECLARE_READONLY_PROPERTY(IFile, media, int);
	
protected:

	// Property Accessors

	virtual RAD_DECLARE_GET(size, FPos) = 0;
	virtual RAD_DECLARE_GET(media, int) = 0;

RAD_INTERFACE_END

//////////////////////////////////////////////////////////////////////////////////////////
// file::ISearch
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(ISearch, IInterface, file.ISearch)

	virtual bool NextFile(string::String &outFilename) = 0;

RAD_INTERFACE_END

//////////////////////////////////////////////////////////////////////////////////////////
// file::IPakFile
//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IPakFile, IInterface, file.IPakFile)

	// Methods

	virtual HSearch OpenSearch(const char *path, const char *extIncludingPeriod) = 0;
	virtual Result OpenFile(const char *path, HFile &file) = 0;
	virtual bool FileExists(const char *path) = 0;
	virtual bool FileSize(const char *path, file::FPos &size) = 0;

	// Properties

	RAD_DECLARE_READONLY_PROPERTY(IPakFile, name, const char*);

protected:

	virtual RAD_DECLARE_GET(name, const char*) = 0;

RAD_INTERFACE_END

} // file

#include "../../Runtime/PopPack.h"
