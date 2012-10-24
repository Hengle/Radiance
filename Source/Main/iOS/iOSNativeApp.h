/*! \file OSXNativeApp.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#pragma once

#include "../IntMain.h"
#include "../NativeAppDef.h"
#include "../GL/GLContext.h"

#include <Runtime/PushPack.h>

class NativeApp;
class DisplayDevice;

namespace details {

class NativeApp;

class RADENG_CLASS DisplayDevice {
private:
	friend class NativeApp;
	friend class ::DisplayDevice;

	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, primary, bool);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, vidModes, const r::VidModeVec*);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, curVidMode, const r::VidMode*);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, defVidMode, const r::VidMode*);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, maxMSAA, int);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, maxAnisotropy, int);

	RAD_DECLARE_GET(primary, bool) {
		return true;
	}

	RAD_DECLARE_GET(vidModes, const r::VidModeVec*) {
		return &m_vidModes;
	}

	RAD_DECLARE_GET(curVidMode, const r::VidMode*) {
		return &m_curMode;
	}
	
	RAD_DECLARE_GET(defVidMode, const r::VidMode*) {
		return &m_defMode;
	}

	RAD_DECLARE_GET(maxMSAA, int) {
		return m_maxMSAA;
	}

	RAD_DECLARE_GET(maxAnisotropy, int) {
		return m_maxAnisotropy;
	}

	r::VidMode m_curMode;
	r::VidMode m_defMode;
	r::VidModeVec m_vidModes;
	int m_maxMSAA;
	int m_maxAnisotropy;
	bool m_primary;
}; 

class RADENG_CLASS NativeApp {
private:
	friend class ::NativeApp;

	NativeApp();

	bool PreInit();
	bool Initialize();
	void Finalize();

	bool BindDisplayDevice(const ::DisplayDeviceRef &display, const r::VidMode &mode);
	void ResetDisplayDevice();
	void LaunchURL(const char *sz);
	void SetThrottleFramerate(bool throttle);

#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)
	GLDeviceContext::Ref CreateOpenGLContext(const GLPixelFormat &pf);
#endif

	RAD_DECLARE_READONLY_PROPERTY(NativeApp, systemLangId, StringTable::LangId);

	RAD_DECLARE_READONLY_PROPERTY(NativeApp, primaryDisplay, const ::DisplayDeviceRef&);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, activeDisplay, const ::DisplayDeviceRef&);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, displayDevices, const ::DisplayDeviceVec&);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, deviceFamily, plat::DeviceFamily);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, deviceType, plat::DeviceType);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, osType, plat::OSType);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, osVersion, int);
	
	RAD_DECLARE_GET(systemLangId, StringTable::LangId);
	
	RAD_DECLARE_GET(primaryDisplay, const ::DisplayDeviceRef&) {
		return m_primaryDisplay;
	}

	RAD_DECLARE_GET(activeDisplay, const ::DisplayDeviceRef&) {
		return m_activeDisplay;
	}

	RAD_DECLARE_GET(displayDevices, const ::DisplayDeviceVec&) {
		return m_displayDevices;
	}
	
	RAD_DECLARE_GET(deviceFamily, plat::DeviceFamily) {
		return m_deviceFamily;
	}
	
	RAD_DECLARE_GET(deviceType, plat::DeviceType) {
		return m_deviceType;
	}
	
	RAD_DECLARE_GET(osType, plat::OSType) {
		return plat::kOSType_iOS;
	}
	
	RAD_DECLARE_GET(osVersion, int) {
		return m_osVersion;
	}

	::DisplayDeviceRef m_primaryDisplay;
	::DisplayDeviceRef m_activeDisplay;
	::DisplayDeviceVec m_displayDevices;
	plat::DeviceFamily m_deviceFamily;
	plat::DeviceType m_deviceType;
	int m_osVersion;
	StringTable::LangId m_langId;
};

} // details

#include <Runtime/PopPack.h>
