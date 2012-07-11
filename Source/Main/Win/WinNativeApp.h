/*! \file WinNativeApp.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#pragma once

#include "../IntMain.h"
#include "../NativeAppDef.h"

#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)
	#include "../GL/GLContext.h"
#endif

#include <Runtime/PushPack.h>

class NativeApp;
class DisplayDevice;

namespace details {

class RADENG_CLASS DisplayDevice {
private:
	friend class ::DisplayDevice;

	bool IsVidModeSupported(const r::VidMode &mode);

	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, vidModes, const r::VidModeVec*);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, curVidMode, r::VidMode);

	RAD_DECLARE_GET(vidModes, const r::VidModeVec*);
	RAD_DECLARE_GET(curVidMode, r::VidMode);

	r::VidModeVec m_vidModes;
}; 

class RADENG_CLASS NativeApp {
private:
	friend class ::NativeApp;

	NativeApp();

	bool BindDisplayDevice(const ::DisplayDeviceRef &display, const r::VidMode &mode);
	void ResetDisplayDevice();
	void LaunchURL(const char *sz);

#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)
	NativeDeviceContext::Ref CreateOpenGLContext();
#endif

	RAD_DECLARE_READONLY_PROPERTY(NativeApp, systemLanguage, StringTable::LangId);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, displayDevices, const ::DisplayDeviceVec&);

	RAD_DECLARE_GET(systemLanguage, StringTable::LangId);
	RAD_DECLARE_GET(displayDevices, const ::DisplayDeviceVec&);

	::DisplayDeviceVec m_displayDevices;

#if defined(RAD_OPT_PC_TOOLS)
	bool m_quit;
#endif
};

} // details

#include <Runtime/PopPack.h>
