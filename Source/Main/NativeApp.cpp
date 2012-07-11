/*! \file NativeApp.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include RADPCH
#include "NativeApp.h"
#include "StdVidModes.h" // s_stdVidModes

NativeApp::NativeApp(int argc, const char **argv) : m_argc(argc), m_argv(argv) {
	m_mainThreadId = thread::ThreadId();
}

bool DisplayDevice::SelectVidMode(r::VidMode &mode) {
	return false;
}
