// Shell.h
// Copyright (c) 2009 Pyramind Labs LLC, All Rights Reserved
// 3DS Max Plugin Shell (Allows dynamic unloading of the real plugin).

#pragma once

typedef bool ( __stdcall * PluginQueryUnload)(void); // Shell calls this to see if it can unload.
typedef void *(__stdcall * PluginCreate)(void); // Shell calls this to create your object.

#if !defined(SHELL_DEFS_ONLY)

#if !defined(PLUGIN_CLASS_ID) || !defined(PLUGIN_TITLE) || !defined(PLUGIN_DLL) || !defined(PLUGIN_CLASS_NAME)
	#error "You must define PLUGIN_CLASS_ID, PLUGIN_TITLE, PLUGIN_DLL, PLUGIN_CLASS_NAME before including this file!"
#endif

#undef UNICODE
#include "max.h"

#if defined(SHELL_STANDALONE)

extern "C" void * __stdcall MAX_PluginCreate(void);

#else

static HMODULE g_hModule = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved) 
{
	switch (fdwReason)
	{
	case DLL_THREAD_DETACH:
		{
			if (g_hModule)
			{
				PluginQueryUnload unload = (PluginQueryUnload)GetProcAddress(g_hModule, "_MAX_PluginQueryUnload@0");
				if (unload && unload())
				{
					FreeLibrary(g_hModule);
					MessageBox(0, "DLL released, safe to recompile.", "Shell - "PLUGIN_TITLE, MB_OK);
					g_hModule = 0;
				}
			}
			break;
		}
	case DLL_THREAD_ATTACH:
	case DLL_PROCESS_ATTACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	
	return (TRUE);
}

#endif

extern "C" __declspec( dllexport ) const TCHAR* LibDescription() 
{
	return PLUGIN_TITLE;
}

extern "C" __declspec( dllexport ) int LibNumberClasses() 
{
	return 1;
}

class ExporterClassDesc : public ClassDesc 
{
public:
	ExporterClassDesc()
	{
	}

	virtual ~ExporterClassDesc()
	{
	}

	int IsPublic() { return 1; }
	void *Create(BOOL loading) 
	{ 
#if defined(SHELL_STANDALONE)
		return MAX_PluginCreate();
#else
		if (!g_hModule)
		{
			g_hModule = LoadLibraryA("plugins\\"PLUGIN_DLL);
		}

		DWORD err = GetLastError();

		if (g_hModule)
		{
			PluginCreate instFunc = (PluginCreate)GetProcAddress(g_hModule, "_MAX_PluginCreate@0");
			if (instFunc)
			{
				return instFunc();
			}
		}
		
		return NULL;
#endif
	} 

	const TCHAR*	ClassName() { return PLUGIN_CLASS_NAME; }
	SClass_ID	SuperClassID() { return SCENE_EXPORT_CLASS_ID; } 
	Class_ID	ClassID() { return PLUGIN_CLASS_ID; }
	const TCHAR* Category() { return "Standard"; }
};

static ExporterClassDesc expClass;

extern "C" __declspec( dllexport ) ClassDesc* LibClassDesc(int i) 
{
	switch(i) {
	case 0: return &expClass;
	default: return 0;
	}
}

extern "C" __declspec( dllexport ) ULONG LibVersion() 
{
	return VERSION_3DSMAX;
}

// Let the plug-in register itself for deferred loading
extern "C" __declspec( dllexport ) ULONG CanAutoDefer()
{
#if defined(SHELL_STANDALONE)
	return 0;
#else
	return 1;
#endif
}

#endif