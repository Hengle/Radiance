// WinCrashReporter.cpp
// Windows crash reporter.
// Copyright (c) 2005-2010 Pyramind Labs LLC, All Rights Reserved
// Author: Vlad Andreev
// See Radiance/LICENSE for licensing terms.

#include "../Base.h"
#include "WinCrashReporter.h"
#include "../File.h"

#include <winsock.h>
#include <sstream>
#include <iostream>
#include <fstream>

namespace fatal {

void CrashReporter::Initialize(HANDLE procHandle,
							   const String &subjectLine,
							   const String &account,
							   const String &server,
							   U16 port,
							   const void *PDBData,
							   size_t PDBSize)
{
	RAD_ASSERT(!s_inst);
	s_inst = new CrashReporter(GetModuleHandle(NULL), procHandle, subjectLine, account, server, port);

	// Must be called from here (these depend on static callbacks, which need s_inst)
	s_inst->CreateTempPDB(PDBData, PDBSize);
	s_inst->InitSymbols();
	SetUnhandledExceptionFilter(ExceptionFilter);
}

void CrashReporter::Initialize(HANDLE procHandle,
							   CrashCallback callback,
							   const void *PDBData,
							   size_t PDBSize)
{
	RAD_ASSERT(!s_inst);
	s_inst = new CrashReporter(GetModuleHandle(NULL), procHandle, callback);

	// Must be called from here (these depend on static callbacks, which need s_inst)
	s_inst->CreateTempPDB(PDBData, PDBSize);
	s_inst->InitSymbols();
	SetUnhandledExceptionFilter(ExceptionFilter);
}

CrashReporter::CrashReporter(HMODULE moduleHandle,
							 HANDLE procHandle,
							 const String &subjectLine,
							 const String &account,
							 const String &server,
							 U16 port)
: m_server(server)
, m_account(account)
, m_subjectLine(subjectLine)
, m_port(port)
, m_moduleHandle(moduleHandle)
, m_procHandle(procHandle)
, m_callback(0)
{
	LoadDbgHelp();
}

CrashReporter::CrashReporter(HMODULE moduleHandle, HANDLE procHandle, CrashCallback callback)
: m_moduleHandle(moduleHandle)
, m_procHandle(procHandle)
, m_callback(callback)
{
	LoadDbgHelp();
}

CrashReporter::~CrashReporter()
{
	if (m_dbghelp)
	{
		std::vector<DWORD64>::iterator it = s_inst->m_images.begin();
		for (; it != s_inst->m_images.end(); ++it)
		{
			SymUnloadModule64(s_inst->m_procHandle, *it);
		}
		SymCleanup(s_inst->m_procHandle);
		FreeModule(m_dbghelp);
	}
}

void CrashReporter::CreateTempPDB(const void *data, size_t size)
{
	// Get the temp path.
	const DWORD pathBufferSize = 1024;
	static char pathBuffer[pathBufferSize];
	DWORD bufferLength = GetTempPathA(pathBufferSize, pathBuffer);
	if (bufferLength > pathBufferSize)
	{
		// GetTempPath() failed, just use the current directory.
		pathBuffer[0] = 0;
	}
	m_tempPath = pathBuffer;

	// Get the name of the currently executing image.  Presumably that's the client app.
	char path[_MAX_PATH];
	::GetModuleFileNameA(GetModuleHandle(NULL), path, sizeof(path));

	// Figure out filenames.
	char file[_MAX_FNAME], ext[_MAX_EXT];
	_splitpath_s(path, NULL, 0, NULL, 0, file, sizeof(file), ext, sizeof(ext));
	m_exeName = String(file) + ".exe";

	if (!data || !m_dbghelp)
	{
		return;
	}
	m_fileName = String(pathBuffer) + file + ".pdb";

	// The .pdb will need to be timestamped to match the executable, so get that info now.
	HANDLE exeHandle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	FILETIME creationTime, accessTime, writeTime;
	GetFileTime(exeHandle, &creationTime, &accessTime, &writeTime);
	CloseHandle(exeHandle);

	// Write the pdb.
	HANDLE pdbHandle = CreateFileA(m_fileName.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
	DWORD numWritten;
	WriteFile(pdbHandle, data, DWORD(size), &numWritten, NULL);
	SetFileTime(pdbHandle, &creationTime, &accessTime, &writeTime);
	CloseHandle(pdbHandle);
}

void CrashReporter::InitSymbols()
{
	if (!m_dbghelp)
	{
		return;
	}

	// Set DbgHelp options.
	DWORD symOptions = SymGetOptions();
	symOptions |= SYMOPT_DEBUG | SYMOPT_LOAD_LINES;
	SymSetOptions(symOptions);
	if (SymInitialize(m_procHandle, NULL, FALSE))
	{
		char searchPath[4096];
		SymGetSearchPath(m_procHandle, searchPath, 4096);
		String newSearchPath = m_tempPath + ";" + String(searchPath);
		SymSetSearchPath(m_procHandle, PSTR(newSearchPath.c_str()));
		EnumAllModules();
	}
}

void CrashReporter::LoadDbgHelp()
{
    m_dbghelp = LoadLibraryA("dbghelp.dll");
    if (m_dbghelp)
	{
        String functionName;

        StackWalk64 = (StackWalk64Ptr)GetProcAddress(m_dbghelp, "StackWalk64");
        SymFunctionTableAccess64 = (SymFunctionTableAccess64Ptr)GetProcAddress(m_dbghelp, "SymFunctionTableAccess64");
        SymGetModuleBase64 = (SymGetModuleBase64Ptr)GetProcAddress(m_dbghelp, "SymGetModuleBase64");
        SymCleanup = (SymCleanupPtr)GetProcAddress(m_dbghelp, "SymCleanup");
        SymFromAddr = (SymFromAddrPtr)GetProcAddress(m_dbghelp, "SymFromAddr");
        SymGetLineFromAddr64 = (SymGetLineFromAddr64Ptr)GetProcAddress(m_dbghelp, "SymGetLineFromAddr64");
        SymInitialize = (SymInitializePtr)GetProcAddress(m_dbghelp, "SymInitialize");
        SymSetOptions = (SymSetOptionsPtr)GetProcAddress(m_dbghelp, "SymSetOptions");

		SymUnloadModule64 = (SymUnloadModule64Ptr)GetProcAddress(m_dbghelp, "SymUnloadModule64");
		SymLoadModule64 = (SymLoadModule64Ptr)GetProcAddress(m_dbghelp, "SymLoadModule64");

		EnumerateLoadedModules64 = (EnumerateLoadedModules64Ptr)GetProcAddress(m_dbghelp, "EnumerateLoadedModules64");
		SymSetSearchPath = (SymSetSearchPathPtr)GetProcAddress(m_dbghelp, "SymSetSearchPath");
		SymGetSearchPath = (SymGetSearchPathPtr)GetProcAddress(m_dbghelp, "SymGetSearchPath");
		SymGetOptions = (SymGetOptionsPtr)GetProcAddress(m_dbghelp, "SymGetOptions");
    }
}

void CrashReporter::EnumAllModules()
{
	EnumerateLoadedModules64(m_procHandle, (PENUMLOADED_MODULES_CALLBACK64)Enumerate, NULL);
	if (m_fileName != "")
	{
		DeleteFileA(m_fileName.c_str());
	}
}

void CrashReporter::Finalize()
{
	delete s_inst;
	s_inst = 0;
}

BOOL CALLBACK CrashReporter::Enumerate(PSTR moduleName, DWORD64 moduleBase, ULONG moduleSize, PVOID userContext)
{
	DWORD64 imageBase = SymLoadModule64(
		s_inst->m_procHandle,
		NULL,
		moduleName,
		NULL,
		moduleBase,
		moduleSize);

	s_inst->m_images.push_back(imageBase);
	return TRUE;
}

LONG WINAPI CrashReporter::ExceptionFilter(struct _EXCEPTION_POINTERS* ptrs)
{
	std::stringstream reportStream;
	HANDLE hThread;
	DuplicateHandle(s_inst->m_procHandle, GetCurrentThread(), s_inst->m_procHandle, &hThread, 0, false, DUPLICATE_SAME_ACCESS );

	// SMTP mandates Unix-style line endings, and callbacks will want Windows-style endings.
	String CR = (s_inst->m_callback) ? "\n" : "\r\n";

	if (s_inst->m_dbghelp)
	{
		static STACKFRAME64 frame;
		memset(&frame, 0, sizeof(frame));

		// Set up the frame
		frame.AddrPC.Offset = ptrs->ContextRecord->Eip;
		frame.AddrPC.Mode = AddrModeFlat;
		frame.AddrStack.Offset = ptrs->ContextRecord->Esp;
		frame.AddrStack.Mode = AddrModeFlat;
		frame.AddrFrame.Offset = ptrs->ContextRecord->Ebp;
		frame.AddrFrame.Mode = AddrModeFlat;
		BOOL ret;

		DWORD64 offsetFromSymbol = 0;

		do
		{
			ret = StackWalk64(IMAGE_FILE_MACHINE_I386,
						s_inst->m_procHandle,
						hThread,
						&frame,
						(VOID *)ptrs->ContextRecord,
						NULL,
						SymFunctionTableAccess64,
						SymGetModuleBase64,
						NULL);
			reportStream.setf(std::ios_base::hex, std::ios::basefield);
			reportStream << "EIP: " << frame.AddrPC.Offset << " ESP: " << frame.AddrStack.Offset << " Ret: " << frame.AddrReturn.Offset << CR;

			char symbolData[4096];
			SYMBOL_INFO *symbol = (SYMBOL_INFO *)((void *)symbolData);
			symbol->SizeOfStruct = sizeof(symbol);
			symbol->MaxNameLen = 4096 - sizeof(symbol);

			// get function symbol info
			if (SymFromAddr(s_inst->m_procHandle, frame.AddrPC.Offset, &offsetFromSymbol, symbol))
			{
				reportStream << "-> " << symbol->Name << "+0x" << offsetFromSymbol << CR;

				reportStream.setf(std::ios_base::dec, std::ios::basefield);
				IMAGEHLP_LINE64 lineInfo;
				lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				DWORD column;
				if (SymGetLineFromAddr64(s_inst->m_procHandle, frame.AddrPC.Offset, &column, &lineInfo))
				{
					reportStream << lineInfo.FileName << "[line " << lineInfo.LineNumber << ", offset " << column << "]" << CR;
				}
			}
			else
			{
				reportStream << "[no symbol information]" << CR;
			}

			reportStream << CR;
		} while (ret);
	}
	else
	{
		reportStream << "Dbghelp.dll not found.  Symbol information and stack trace not available." << CR;
	}

	// Send off the report.
	if (s_inst->m_callback)
	{
		s_inst->m_callback(reportStream.str().c_str());
	}
	else
	{
		s_inst->MailReport(s_inst->m_account, s_inst->m_account, s_inst->m_subjectLine, reportStream.str().c_str());
	}

	CloseHandle( hThread );
	MessageBoxA(NULL, (s_inst->m_exeName + " has caused an exception and will have to be shutdown.").c_str(), "UNHANDLED EXCEPTION", MB_OK);
	return EXCEPTION_EXECUTE_HANDLER;
}

//////////////////////////////////////////////////////////////////////////////////////////
// SMTP handling
//////////////////////////////////////////////////////////////////////////////////////////

void CrashReporter::MailReport(const String &fromAddress
                             , const String &toAddress
                             , const String &subject
                             , const String &body)
{
	WSADATA wsaData;

	WSAStartup(MAKEWORD(2,2), &wsaData);
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_socket == INVALID_SOCKET)
	{
		std::stringstream ss;
		ss << "Socket error: " << WSAGetLastError();
		Error(ss.str().c_str());
	}


	hostent* smtpServer = gethostbyname(m_server.c_str());
	if (!smtpServer)
	{
		throw ::exception(("Unable to resolve " + m_server).c_str());
	}
	char *serverIp = inet_ntoa (*(struct in_addr *)*smtpServer->h_addr_list);

	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(serverIp);
	clientService.sin_port = htons(m_port);

	if (connect(m_socket, (SOCKADDR *)&clientService, sizeof(clientService)) == SOCKET_ERROR)
	{
		Error("Failed to connect to SMTP server.");
	}

	Read("220");
	Send("HELO TestApp\r\n");
	Read("250");

	Send("MAIL FROM: " + fromAddress + "\r\n");
	Read("250");

	Send("RCPT TO: " + toAddress + "\r\n");
	Read("250");

	Send("DATA\r\n");
	Read("354");

	String message = "Subject: " + subject + "\r\n"
						+ "From: " + fromAddress + "\r\n"
						+ "To: " + toAddress + "\r\n\r\n"
						+ body + "\r\n\r\n.\r\n";
	Send(message);
	Read("250");
	Send("QUIT");

	closesocket(m_socket);
	WSACleanup();
}

void CrashReporter::Error(const String &msg)
{
	MessageBoxA(NULL, msg.c_str(), "Crash Report", MB_OK);
	WSACleanup();
}

////////////////////////////////////////////////////////////////////////////////////////
// Checks the returned numeric.  Success is usually 250,
// although DATA returns 354.
////////////////////////////////////////////////////////////////////////////////////////

void CrashReporter::CheckCode(const char *ret, const char *code)
{
	if (ret[0] != code[0] || ret[1] != code[1] || ret[2] != code[2])
	{
		Error(String("Server error: ") + ret);
	}
}

void CrashReporter::Send(const String &cmd)
{
	if (send(m_socket, cmd.c_str(), (int)cmd.length(), 0) == SOCKET_ERROR)
	{
		std::stringstream ss;
		ss << "Socket error: " << WSAGetLastError();
		Error(ss.str().c_str());
	}
}

void CrashReporter::Read(const String &code)
{
	memset(ret, 0, recvBufSize);
	if (recv(m_socket, ret, recvBufSize, 0) == SOCKET_ERROR)
	{
		std::stringstream ss;
		ss << "Socket error: " << WSAGetLastError();
		Error(ss.str().c_str());
	}
	CheckCode(ret, code.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////
// DbgHelp exports
////////////////////////////////////////////////////////////////////////////////////////

CrashReporter::StackWalk64Ptr CrashReporter::StackWalk64;
CrashReporter::SymCleanupPtr CrashReporter::SymCleanup;
CrashReporter::SymFromAddrPtr CrashReporter::SymFromAddr;
CrashReporter::SymFunctionTableAccess64Ptr CrashReporter::SymFunctionTableAccess64;
CrashReporter::SymGetModuleBase64Ptr CrashReporter::SymGetModuleBase64;
CrashReporter::SymGetLineFromAddr64Ptr CrashReporter::SymGetLineFromAddr64;
CrashReporter::SymInitializePtr CrashReporter::SymInitialize;
CrashReporter::SymSetOptionsPtr CrashReporter::SymSetOptions;
CrashReporter::GetModuleInformationPtr CrashReporter::GetModuleInformation;
CrashReporter::SymUnloadModule64Ptr CrashReporter::SymUnloadModule64;
CrashReporter::SymLoadModule64Ptr CrashReporter::SymLoadModule64;
CrashReporter::EnumerateLoadedModules64Ptr CrashReporter::EnumerateLoadedModules64;
CrashReporter::SymSetSearchPathPtr CrashReporter::SymSetSearchPath;
CrashReporter::SymGetSearchPathPtr CrashReporter::SymGetSearchPath;
CrashReporter::SymGetOptionsPtr CrashReporter::SymGetOptions;

CrashReporter *CrashReporter::s_inst = 0;

} // fatal

