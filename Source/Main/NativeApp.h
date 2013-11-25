/*! \file NativeApp.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#pragma once

#include "IntMain.h"
#include "NativeAppDef.h"
#include "NativeDeviceContext.h"
#include "NativeAppBackend.h"

#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)
	#include "GL/GLContext.h"
#endif

#include <Runtime/PushPack.h>

class RADENG_CLASS DisplayDevice {
public:
	typedef boost::shared_ptr<DisplayDevice> Ref;
	typedef zone_vector<Ref, ZEngineT>::type Vec;

	RAD_BEGIN_FLAGS
		RAD_FLAG(kMatchDisposition_Upsize),
		RAD_FLAG(kMatchDisposition_Downsize),
		RAD_FLAG(kMatchDisposition_AllowAspectChange),
		RAD_FLAG(kMatchDisposition_AllowAspect4x3),
		RAD_FLAG(kMatchDisposition_AllowAspect16x9),
		RAD_FLAG(kMatchDisposition_AllowAspect16x10),
		kMatchDisposition_Any = kMatchDisposition_AllowAspectChange|kMatchDisposition_AllowAspect4x3|
			kMatchDisposition_AllowAspect16x9|kMatchDisposition_AllowAspect16x10
	RAD_END_FLAGS(MatchDisposition)

	bool IsVidModeSupported(const r::VidMode &mode);
	bool MatchVidMode(
		r::VidMode &mode,
		MatchDisposition disposition
	);

	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, primary, bool);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, vidModes, const r::VidModeVec*);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, curVidMode, const r::VidMode*);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, defVidMode, const r::VidMode*);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, maxMSAA, int);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, maxAnisotropy, int);

private:

	friend class details::DisplayDevice;
	friend class details::NativeApp;

	DisplayDevice() {}

	RAD_DECLARE_GET(primary, bool);
	RAD_DECLARE_GET(vidModes, const r::VidModeVec*);
	RAD_DECLARE_GET(curVidMode, const r::VidMode*);
	RAD_DECLARE_GET(defVidMode, const r::VidMode*);
	RAD_DECLARE_GET(maxMSAA, int);
	RAD_DECLARE_GET(maxAnisotropy, int);

	details::DisplayDevice m_imp;
}; 

class RADENG_CLASS NativeApp {
public:

	NativeApp(int argc, const char **argv);

	virtual bool PreInit();
	virtual bool Initialize();
	virtual void Finalize();
	virtual bool Run() = 0;
	virtual void NotifyBackground(bool background) = 0;
	virtual void PostInputEvent(const InputEvent &e) = 0;
	virtual float Tick() = 0;

	// this is hacky for now
#if !defined(RAD_OPT_PC_TOOLS)
	void PlayFullscreenMovie(const char *path) { m_imp.PlayFullscreenMovie(path); }
	virtual void MovieFinished() {}
#endif

	bool BindDisplayDevice(const DisplayDevice::Ref &display, const r::VidMode &mode);
	void ResetDisplayDevice();
	void LaunchURL(const char *sz);
	bool FindArg(const char *arg);
	const char *ArgArg(const char *arg);
	
#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)
	GLDeviceContext::Ref CreateOpenGLContext(const GLPixelFormat &pf);
#endif

	RAD_DECLARE_READONLY_PROPERTY(NativeApp, systemLangId, StringTable::LangId);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, mainThreadId, thread::Id);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, argc, int);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, argv, const char**);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, deviceFamily, plat::DeviceFamily);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, deviceType, plat::DeviceType);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, osType, plat::OSType);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, osVersion, int);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, primaryDisplay, const DisplayDevice::Ref&);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, activeDisplay, const DisplayDevice::Ref&);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, displayDevices, const DisplayDevice::Vec&);
	RAD_DECLARE_PROPERTY(NativeApp, throttleFramerate, bool, bool);

private:
		
	RAD_DECLARE_GET(systemLangId, StringTable::LangId);
	RAD_DECLARE_GET(mainThreadId, thread::Id);
	RAD_DECLARE_GET(argc, int);
	RAD_DECLARE_GET(argv, const char**);
	RAD_DECLARE_GET(deviceFamily, plat::DeviceFamily);
	RAD_DECLARE_GET(deviceType, plat::DeviceType);
	RAD_DECLARE_GET(osType, plat::OSType);
	RAD_DECLARE_GET(osVersion, int);
	RAD_DECLARE_GET(primaryDisplay, const DisplayDevice::Ref&);
	RAD_DECLARE_GET(activeDisplay, const DisplayDevice::Ref&);
	RAD_DECLARE_GET(displayDevices, const DisplayDevice::Vec&);
	RAD_DECLARE_GET(throttleFramerate, bool);
	RAD_DECLARE_SET(throttleFramerate, bool);
	
	details::NativeApp m_imp;
	thread::Id m_mainThreadId;
	int m_argc;
	const char **m_argv;
	bool m_throttle;
};

#include <Runtime/PopPack.h>

RAD_IMPLEMENT_FLAGS(DisplayDevice::MatchDisposition)

#include "NativeApp_inl.h"
