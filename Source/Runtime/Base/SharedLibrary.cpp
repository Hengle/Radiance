// SharedLibrary.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "SharedLibrary.h"

#if defined(RAD_OPT_WINX)
#include "../Win/WinHeaders.h"
#elif defined(RAD_OPT_LINUX) || defined(RAD_OPT_APPLE)
#include "../String.h"
#include "Utils.h"
#include <dlfcn.h>
#include <sstream>
#define DLOPEN
using namespace string;
#else
#error RAD_ERROR_UNSUP_PLAT
#endif

bool SharedLibrary::Load(const char *nativeFilename, bool reportErrors)
{
	RAD_ASSERT(!Loaded());
	
#if defined(RAD_OPT_WINX)
	UINT x = SetErrorMode(reportErrors ? 0 : SEM_NOOPENFILEERRORBOX);
	m_h = LoadLibraryExA(nativeFilename, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
	SetErrorMode(x);
#elif defined(DLOPEN)
	dlerror();
	m_h = dlopen(nativeFilename, RTLD_NOW|RTLD_GLOBAL);
	
#if defined(RAD_OPT_PC)
	if (!m_h && reportErrors)
	{
		MessageBox("dlopen(): failed", RAD_SS("Failed to load " << nativeFilename << ". Error: " << dlerror()), MBStyleOk);
	}
#endif
#endif

	return m_h != 0;
}

void SharedLibrary::Unload()
{
	if (m_h)
	{
#if defined(RAD_OPT_WINX)
		FreeModule((HMODULE)m_h);
#elif defined(DLOPEN)
		dlclose(m_h);
#endif
		m_h = 0;
	}
}

void *SharedLibrary::ProcAddress(const char* procName)
{
#if defined(RAD_OPT_WINX)
	return (void*)GetProcAddress((HMODULE)m_h, procName);
#elif defined(DLOPEN)
	return dlsym(m_h, procName);
#endif
}

