// WinCrashReporter.h
// Windows crash report.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Vlad Andreev
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "WinHeaders.h"
#include <string>
#include <vector>

#include <psapi.h>
#define _NO_CVCONST_H
#include <dbghelp.h>

#include "../PushPack.h"

namespace fatal {

//////////////////////////////////////////////////////////////////////////////////////////
// Crash reporter class.
//////////////////////////////////////////////////////////////////////////////////////////

class CrashReporter
{
public:
	typedef std::string String;
	typedef std::wstring WString;
	typedef void (*CrashCallback)(const String &);

	static void Initialize( 
		HANDLE procHandle,
		const String &subjectLine, 
		const String &account, 
		const String &server, 
		U16 port = 25,
		const void *PDBData = 0,
		size_t PDBSize = 0);

	static void Initialize( 
		HANDLE procHandle,
		CrashCallback callback,
		const void *PDBData = 0,
		size_t PDBSize = 0);
	static void Finalize();

	static LONG WINAPI ExceptionFilter(__in struct _EXCEPTION_POINTERS* ptrs);

	static bool Initialized()
	{
		return (s_inst != 0);
	}

private:

	CrashReporter(
		HMODULE moduleHandle, 
		HANDLE procHandle, 
		const String &subjectLine, 
		const String &server, 
		const String &user, 
		U16 port);

	CrashReporter(HMODULE moduleHandle, HANDLE procHandle, CrashCallback callback);

	virtual ~CrashReporter();

	void InitSymbols();
	void LoadDbgHelp();
	void CreateTempPDB(const void *data, size_t size);
	void EnumAllModules();

	static BOOL CALLBACK Enumerate(PSTR moduleName, DWORD64 moduleBase, ULONG moduleSize, PVOID userContext);
	LONG WINAPI ExceptionFilterImpl(__in struct _EXCEPTION_POINTERS* ptrs);

	void MailReport(const String &fromAddress
				  , const String &toAddress
				  , const String &subject
				  , const String &body);

	void Error(const String &msg);
	void CheckCode(const char *ret, const char *code = "250");
	void Send(const String &cmd);
	void Read(const String &code);

	static const int recvBufSize = 1024;
	SOCKET m_socket;
	char ret[recvBufSize];

	String m_server, m_account;
	String m_subjectLine;
	U16 m_port;
	HANDLE m_procHandle;
	HMODULE m_moduleHandle;
	String m_fileName, m_exeName;
	DWORD m_imageSize;
	std::vector<DWORD64> m_images;
	String m_tempPath;
	CrashCallback m_callback;

	static CrashReporter *s_inst;

	////////////////////////////////////////////////////////////////////////////////////////
	// DbgHelp exports
	////////////////////////////////////////////////////////////////////////////////////////

	typedef BOOL (__stdcall *StackWalk64Ptr)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID, PREAD_PROCESS_MEMORY_ROUTINE64,
											PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
	typedef PVOID (__stdcall *SymFunctionTableAccess64Ptr)(HANDLE, DWORD64);
	typedef DWORD64 (__stdcall *SymGetModuleBase64Ptr)(HANDLE, DWORD64);
	typedef BOOL (__stdcall *SymCleanupPtr)(HANDLE);
	typedef BOOL (__stdcall *SymFromAddrPtr)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
	typedef BOOL (__stdcall *SymGetLineFromAddr64Ptr)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
	typedef BOOL (__stdcall *SymInitializePtr)(HANDLE, PCTSTR, BOOL);
	typedef DWORD (__stdcall *SymSetOptionsPtr)(DWORD);
	typedef BOOL (__stdcall *GetModuleInformationPtr)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
	typedef BOOL (__stdcall *SymUnloadModule64Ptr)(HANDLE, DWORD64);
	typedef DWORD (__stdcall *SymLoadModule64Ptr)(HANDLE, HANDLE, PSTR, PSTR, DWORD64, DWORD);
	typedef BOOL (__stdcall *EnumerateLoadedModules64Ptr)(HANDLE, PENUMLOADED_MODULES_CALLBACK64, PVOID);
	typedef BOOL (__stdcall *SymSetSearchPathPtr)(HANDLE, PSTR);
	typedef BOOL (__stdcall *SymGetSearchPathPtr)(HANDLE, PSTR, DWORD);
	typedef DWORD (__stdcall *SymGetOptionsPtr)(VOID);

	static StackWalk64Ptr StackWalk64;
	static SymCleanupPtr SymCleanup;
	static SymFromAddrPtr SymFromAddr;
	static SymFunctionTableAccess64Ptr SymFunctionTableAccess64;
	static SymGetModuleBase64Ptr SymGetModuleBase64;
	static SymGetLineFromAddr64Ptr SymGetLineFromAddr64;
	static SymInitializePtr SymInitialize;
	static SymSetOptionsPtr SymSetOptions;
	static GetModuleInformationPtr GetModuleInformation;
	static SymUnloadModule64Ptr SymUnloadModule64;
	static SymLoadModule64Ptr SymLoadModule64;
	static EnumerateLoadedModules64Ptr EnumerateLoadedModules64;
	static SymSetSearchPathPtr SymSetSearchPath;
	static SymGetSearchPathPtr SymGetSearchPath;
	static SymGetOptionsPtr SymGetOptions;

	HMODULE m_dbghelp;
};

} // fatal

#include "../PopPack.h"