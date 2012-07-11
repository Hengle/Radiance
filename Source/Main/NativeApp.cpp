/*! \file NativeApp.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include RADPCH
#include "NativeApp.h"
#include "StdVidModes.h" // s_stdVidModes
#include <Runtime/StringBase.h>

bool DisplayDevice::SelectVidMode(r::VidMode &mode) {
	return false;
}

NativeApp::NativeApp(int argc, const char **argv) : m_argc(argc), m_argv(argv) {
	m_mainThreadId = thread::ThreadId();
}

bool NativeApp::FindArg(const char *arg) {
	for (int i = 0; i < m_argc; ++i) {
		if (!string::icmp(arg, m_argv[i])) 
			return true;
	}
	return false;
}

const char *NativeApp::ArgArg(const char *arg) {
	for (int i = 0; i < m_argc-1; ++i) {
		if (!string::icmp(arg, m_argv[i]))
			return m_argv[i+1];
	}
	return 0;
}
