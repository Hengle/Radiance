/*! \file OSXNativeApp.h
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

enum OSXVersion {
	kOSXVersion_10_7 = RAD_MAKE_OS_VERSION(10, 7, 0),
	kOSXVersion_10_8 = RAD_MAKE_OS_VERSION(10, 8, 0),
	kOSXVersion_MountainLion = kOSXVersion_10_8
};

namespace details {

class SystemVersion {
public:

	static int Major() {
		return (int)Get().m_major;
	}
	
	static int Minor() {
		return (int)Get().m_minor;
	}
	
	static int Revision() {
		return (int)Get().m_revision;
	}
	
	static int Version() {
		const SystemVersion &v = Get();
		return (int)RAD_MAKE_OS_VERSION(v.m_major, v.m_minor, v.m_revision);
	}
	
	static bool GEqual(int version) {
		return Version() >= version;
	}
	
	static bool Less(int version) {
		return Version() < version;
	}
	
	static bool HasGameCenter() {
		return GEqual(kOSXVersion_10_8);
	}
	
private:

	SystemVersion();
	SystemVersion(const OSXVersion &);

	S32 m_major;
	S32 m_minor;
	S32 m_revision;
	
	static const SystemVersion &Get();
};

class NativeApp;

class RADENG_CLASS DisplayDevice {
public:
	struct NativeVars {
		typedef boost::shared_ptr<NativeVars> Ref;
		virtual ~NativeVars() {}
	};
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
		return m_primary;
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
	NativeVars::Ref m_vars;
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
		return plat::kDeviceFamily_PC;
	}
	
	RAD_DECLARE_GET(deviceType, plat::DeviceType) {
		return plat::kDeviceType_PC;
	}
	
	RAD_DECLARE_GET(osType, plat::OSType) {
		return plat::kOSType_OSX;
	}
	
	RAD_DECLARE_GET(osVersion, int) {
		return SystemVersion::Version();
	}

	::DisplayDeviceRef m_primaryDisplay;
	::DisplayDeviceRef m_activeDisplay;
	::DisplayDeviceVec m_displayDevices;
	StringTable::LangId m_langId;
	bool m_useDisplayCapture;
};

} // details

#include <Runtime/PopPack.h>
