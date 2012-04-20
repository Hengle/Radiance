// QtSupport.cpp
// Copyright (c) 2009 Pyramind Labs LLC, All Rights Reserved
// Allows us to use Qt windows in 3DSMAX.

#include <qtwinmigrate/qmfcapp.h>
#include <windows.h>

BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved )
{
    static bool ownApplication = FALSE;

    if ( dwReason == DLL_PROCESS_ATTACH )
        ownApplication = QMfcApp::pluginInstance( hInstance );
    if ( dwReason == DLL_PROCESS_DETACH && ownApplication )
        delete qApp;

    return TRUE;
}
