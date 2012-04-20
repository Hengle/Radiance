// Plugin.cpp
// Copyright (c) 2009 Pyramind Labs LLC, All Rights Reserved

#include "Plugin.h"
#include <qtwinmigrate/qwinwidget.h>
#include <qmessagebox.h>

int Plugin::s_busy = 0;

void Plugin::ShowAbout(HWND hWnd)
{
	QWinWidget win(hWnd);
    win.showCentered();
    QMessageBox::about( &win, "About Radiance Scene Exporter", "Radiance Scene Exporter Version 1.0\nCopyright (C) 2009 Pyramind Labs, All Rights Reserved" );
}

int Plugin::DoExport(const TCHAR *name,ExpInterface *ei,Interface *ip, BOOL suppressPrompts, DWORD options)
{
	++s_busy;
	m_i  = ip;
	m_ei = ei;

	--s_busy;
	return 1;
}

extern "C" __declspec(dllexport) void * __stdcall MAX_PluginCreate()
{
	return new Plugin();
}

extern "C" __declspec(dllexport) bool __stdcall MAX_PluginQueryUnload()
{
	return Plugin::QueryUnload(); 
}

#if defined(SHELL_STANDALONE)
	#include "Shell.cpp"
#endif