// FileSystemReflectMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#if !defined(RAD_OPT_NO_REFLECTION)
#if 0

#include <Runtime/Interface/ReflectInterface.h>
#include "FileSystem.h"

//////////////////////////////////////////////////////////////////////////////////////////
// file::Info
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_ENUM_NAMESPACE("file::Info", file, Info)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)
	RADREFLECT_ENUM_VALUE_NAMED("Version", file::Version)
RADREFLECT_END_NAMESPACE(RADNULL_API, file, Info)

//////////////////////////////////////////////////////////////////////////////////////////
// file::MediaFlags
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_ENUM_NAMESPACE("file::MediaFlags", file, MediaFlags)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)
	RADREFLECT_ENUM_FLAGS_ATTRIBUTE
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, CDDVD)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, HDD)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, Mod)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, Paks)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, Physical)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, AllMedia)
RADREFLECT_END_NAMESPACE(RADNULL_API, file, MediaFlags)

//////////////////////////////////////////////////////////////////////////////////////////
// file::Result
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_ENUM_NAMESPACE("file::Result", file, Result)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, Success)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, Pending)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorPartial)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorAborted)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorAccessDenied)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorFileNotFound)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorGeneric)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorExpansionFailed)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorInvalidArguments)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorDriveNotReady)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorTrayOpen)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorCompressionFailure)
	RADREFLECT_NAMESPACED_ENUM_VALUE(file, ErrorOutOfMemory)
RADREFLECT_END_NAMESPACE(RADNULL_API, file, Result)

//////////////////////////////////////////////////////////////////////////////////////////
// file::IFileSystem
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_INTERFACE_NAMESPACE("file::IFileSystem", 
									 file, 
									 IFileSystem, 
									 IInterface)

	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	// void Initialize(U32 version, int enabledMedia)
	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_ARG("enabledMedia", int)
		RADREFLECT_DEFAULT_ARG("version", U32, file::Version)
	RADREFLECT_END_METHOD(Initialize)

	// void AddPakFile(const HPakFile& pakFile)
	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_ARG("pakFile", const file::HPakFile&)
	RADREFLECT_END_METHOD(AddPakFile)
	
	// void RemovePakFile(const HPakFile& pakFile)
	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_ARG("pakFile", const file::HPakFile&)
	RADREFLECT_END_METHOD(RemovePakFile)

	// void ReleaseAllPakRefs()
	RADREFLECT_METHOD(void, ReleaseAllPakRefs)

	// HPakFile Pak(int num)
	RADREFLECT_BEGIN_METHOD(file::HPakFile)
		RADREFLECT_ARG("num", int)
	RADREFLECT_END_METHOD(Pak)

	// HFileSearch OpenSearch(const char* path, const char* extIncludingPeriod, int media)
	RADREFLECT_BEGIN_METHOD(file::HSearch)
		RADREFLECT_ARG("path", const wchar_t*)
		RADREFLECT_ARG("extIncludingPeriod", const wchar_t*)
		RADREFLECT_ARG("media", int)
	RADREFLECT_END_METHOD(OpenSearch)

	// virtual HStreamInputBuffer CreateStreamBuffer(FPos size)
	RADREFLECT_BEGIN_METHOD(file::HStreamInputBuffer)
		RADREFLECT_ARG("size", file::FPos)
	RADREFLECT_END_METHOD(CreateStreamBuffer)

	// virtual HStreamInputBuffer SafeCreateStreamBuffer(FPos size)
	RADREFLECT_BEGIN_METHOD(file::HStreamInputBuffer)
		RADREFLECT_ARG("size", file::FPos)
	RADREFLECT_END_METHOD(SafeCreateStreamBuffer)

	// virtual HBufferedAsyncIO CreateBufferedIO(FPos size, int alignment = 0)
	RADREFLECT_BEGIN_METHOD(file::HBufferedAsyncIO)
		RADREFLECT_ARG("size", file::FPos)
		RADREFLECT_DEFAULT_ARG("alignment", file::FPos, 0)
	RADREFLECT_END_METHOD(CreateBufferedIO)

	// virtual HBufferedAsyncIO CreateBufferedIO(FPos size, int alignment = 0)
	RADREFLECT_BEGIN_METHOD(file::HBufferedAsyncIO)
		RADREFLECT_ARG("size", file::FPos)
		RADREFLECT_DEFAULT_ARG("alignment", file::FPos, 0)
	RADREFLECT_END_METHOD(SafeCreateBufferedIO)

	// Result LoadFile(const char* path, int media, HBufferedAsyncIO& hAsyncIO, int alignment = 0)
	RADREFLECT_BEGIN_METHOD(file::Result)
		RADREFLECT_ARG("path", const wchar_t*)
		RADREFLECT_ARG("media", int)
		RADREFLECT_ARG("hAsyncIO", file::HBufferedAsyncIO&)
			RADREFLECT_RTLI_ATTRIBUTE_OUT
		RADREFLECT_ARG("ioComplete", const file::HIONotify&)
		RADREFLECT_DEFAULT_ARG("alignment", file::FPos, 0)
	RADREFLECT_END_METHOD(LoadFile)

	// Result OpenFile(const char *path, int media, HFile &file)
	RADREFLECT_BEGIN_METHOD(file::Result)
		RADREFLECT_ARG("path", const wchar_t*)
		RADREFLECT_ARG("media", int)
		RADREFLECT_ARG("file", file::HFile&)
			RADREFLECT_RTLI_ATTRIBUTE_OUT
		RADREFLECT_ARG("ioComplete", const file::HIONotify&)
	RADREFLECT_END_METHOD(OpenFile)

	// bool FileExists(const char* path, int media)
	RADREFLECT_BEGIN_METHOD(bool)
		RADREFLECT_ARG("path", const wchar_t*)
		RADREFLECT_ARG("media", int)
	RADREFLECT_END_METHOD(FileExists)

	// bool FileSize(const char* path, int media, FPos& size)
	RADREFLECT_BEGIN_METHOD(bool)
		RADREFLECT_ARG("path", const wchar_t*)
		RADREFLECT_ARG("media", int)
		RADREFLECT_ARG("size", file::FPos&)
			RADREFLECT_RTLI_ATTRIBUTE_OUT
	RADREFLECT_END_METHOD(FileSize)

	// Properties

	RADREFLECT_PROPERTY_GET(numPaks, int)
	RADREFLECT_PROPERTY_GET(enabledMedia, int)
	RADREFLECT_PROPERTY(cddvdRoot, const wchar_t*, const wchar_t*)
	RADREFLECT_PROPERTY(hddRoot, const wchar_t*, const wchar_t*)
	RADREFLECT_PROPERTY(modRoot, const wchar_t*, const wchar_t*)

RADREFLECT_END_INTERFACE_NAMESPACE(RADNULL_API, 
								   file, 
								   IFileSystem, 
								   "file::HFileSystem", 
								   HFileSystem)

//////////////////////////////////////////////////////////////////////////////////////////
// file::IFileData
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_INTERFACE_NAMESPACE("file::IFileData", 
									 file, 
									 IFileData, 
									 IInterface)

	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	// Properties
	RADREFLECT_PROPERTY_GET(size, file::FPos)
	RADREFLECT_PROPERTY_GET(ptr, const void*)

RADREFLECT_END_INTERFACE_NAMESPACE(RADNULL_API, 
								   file, 
								   IFileData, 
								   "file::HFileData", 
								   HFileData)

//////////////////////////////////////////////////////////////////////////////////////////
// file::IIONotify
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_INTERFACE_NAMESPACE("file::IIONotify", 
									 file, 
									 IIONotify, 
									 IInterface)
	
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	// void Notify(Result result)
	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_ARG("result", file::Result)
	RADREFLECT_END_METHOD(Notify)

RADREFLECT_END_INTERFACE_NAMESPACE(RADNULL_API, 
								   file, 
								   IIONotify, 
								   "file::HIONotify", 
								   HIONotify)

//////////////////////////////////////////////////////////////////////////////////////////
// file::IAsyncIO
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_INTERFACE_NAMESPACE("file::IAsyncIO", 
									 file, 
									 IAsyncIO, 
									 IInterface)

	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	// void Cancel()
	RADREFLECT_METHOD(void, Cancel)

	// void WaitForCompletion()
	RADREFLECT_METHOD(void, WaitForCompletion)

	// Properties
	RADREFLECT_PROPERTY_GET(result, file::Result)
	RADREFLECT_PROPERTY_GET(ioSize, file::FPos)
	RADREFLECT_PROPERTY_GET(byteCount, file::FPos)

RADREFLECT_END_INTERFACE_NAMESPACE(RADNULL_API, 
								   file, 
								   IAsyncIO, 
								   "file::HAsyncIO", 
								   HAsyncIO)

//////////////////////////////////////////////////////////////////////////////////////////
// file::IBufferedAsyncIO
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_INTERFACE_NAMESPACE("file::IBufferedAsyncIO", 
									 file, 
									 IBufferedAsyncIO, 
									 file::IAsyncIO)

	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	// Properties
	RADREFLECT_PROPERTY_GET(data, file::HFileData)
	RADREFLECT_PROPERTY_GET(bufferSize, file::FPos)
	RADREFLECT_PROPERTY_GET(alignment, file::FPos)
	
RADREFLECT_END_INTERFACE_NAMESPACE(RADNULL_API, 
								   file, 
								   IBufferedAsyncIO, 
								   "file::HBufferedAsyncIO", 
								   HBufferedAsyncIO)

//////////////////////////////////////////////////////////////////////////////////////////
// file::IStreamInputBuffer
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_INTERFACE_NAMESPACE("file::IStreamInputBuffer", 
									 file, 
									 IStreamInputBuffer, 
									 IInterface)

	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	// void Bind(const HFile &file)
	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_ARG("file", const file::HFile&)	
	RADREFLECT_END_METHOD(Bind)

RADREFLECT_END_INTERFACE_NAMESPACE(RADNULL_API, 
								   file, 
								   IStreamInputBuffer, 
								   "file::HStreamInputBuffer", 
								   HStreamInputBuffer)

//////////////////////////////////////////////////////////////////////////////////////////
// file::IFile
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_INTERFACE_NAMESPACE("file::IFile", 
									 file, 
									 IFile, 
									 IInterface)

	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	// Methods

	RADREFLECT_BEGIN_METHOD(file::Result)
		RADREFLECT_ARG("io", const file::HBufferedAsyncIO&)
		RADREFLECT_ARG("fileOffset", file::FPos)
		RADREFLECT_ARG("sizeToRead", file::FPos)
		RADREFLECT_ARG("ioComplete", const file::HIONotify&)
	RADREFLECT_END_METHOD(Read)

	// Properties
	RADREFLECT_PROPERTY_GET(size, file::FPos)

RADREFLECT_END_INTERFACE_NAMESPACE(RADNULL_API, 
								   file, 
								   IFile, 
								   "file::HFile", 
								   HFile)

//////////////////////////////////////////////////////////////////////////////////////////
// file::IFileSearch
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_INTERFACE_NAMESPACE("file::ISearch", 
									 file, 
									 ISearch, 
									 IInterface)

	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	// bool NextFile(pml::string::string<>& outFilename)
	RADREFLECT_BEGIN_METHOD(bool)
		RADREFLECT_ARG("outFilename", string::WString&)
			RADREFLECT_RTLI_ATTRIBUTE_OUT
	RADREFLECT_END_METHOD(NextFile)

RADREFLECT_END_INTERFACE_NAMESPACE(RADNULL_API, 
								   file, 
								   ISearch, 
								   "file::HSearch", 
								   HSearch)

//////////////////////////////////////////////////////////////////////////////////////////
// file::IPakFile
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_INTERFACE_NAMESPACE("file::IPakFile", 
									 file, 
									 IPakFile, 
									 IInterface)

	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)

	// HFileSearch OpenSearch(const char* path, const char* extIncludingPeriod)
	RADREFLECT_BEGIN_METHOD(file::HSearch)
		RADREFLECT_ARG("path", const wchar_t*)
		RADREFLECT_ARG("extIncludingPeriod", const wchar_t*)
	RADREFLECT_END_METHOD(OpenSearch)

	// Result OpenFile(const wchar_t *path, HFile &file) = 0;
	RADREFLECT_BEGIN_METHOD(file::Result)
		RADREFLECT_ARG("path", const wchar_t*)
		RADREFLECT_ARG("file", file::HFile&)
			RADREFLECT_RTLI_ATTRIBUTE_OUT
	RADREFLECT_END_METHOD(OpenFile)

	// bool FileExists(const char* path)
	RADREFLECT_BEGIN_METHOD(bool)
		RADREFLECT_ARG("path", const wchar_t*)
	RADREFLECT_END_METHOD(FileExists)

	// bool FileSize(const char* path, file::FPos& size)
	RADREFLECT_BEGIN_METHOD(bool)
		RADREFLECT_ARG("path", const wchar_t*)
		RADREFLECT_ARG("size", file::FPos&)
			RADREFLECT_RTLI_ATTRIBUTE_OUT
	RADREFLECT_END_METHOD(FileSize)

	// Properties
	RADREFLECT_PROPERTY_GET(name, const wchar_t*)

RADREFLECT_END_INTERFACE_NAMESPACE(RADNULL_API, 
								   file, 
								   IPakFile, 
								   "file::HPakFile", 
								   HPakFile)

#endif
#endif // !defined(RAD_OPT_NO_REFLECTION)
