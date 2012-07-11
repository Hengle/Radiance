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
#include <Runtime/PushPack.h>

class RADENG_CLASS DisplayDevice {
public:
	typedef boost::shared_ptr<DisplayDevice> Ref;
	typedef zone_vector<Ref, ZEngineT>::type Vec;

	bool IsVidModeSupported(const r::VidMode &mode);
	bool SelectVidMode(r::VidMode &mode);

	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, vidModes, const r::VidModeVec*);
	RAD_DECLARE_READONLY_PROPERTY(DisplayDevice, curVidMode, r::VidMode);

private:

	friend class details::DisplayDevice;

	DisplayDevice();

	RAD_DECLARE_GET(vidModes, const r::VidModeVec*);
	RAD_DECLARE_GET(curVidMode, r::VidMode);

	details::DisplayDevice m_imp;
}; 

class RADENG_CLASS NativeApp : public boost::shared_ptr<NativeApp> {
public:

	NativeApp(int argc, const char **argv);

	virtual bool PreInit() = 0;
	virtual bool Initialize() = 0;
	virtual void Finalize() = 0;
	virtual bool Run() = 0;
	virtual void NotifyBackground(bool background) = 0;
	virtual void PostInputEvent(const InputEvent &e) = 0;
	virtual float Tick() = 0;

	bool BindDisplayDevice(const DisplayDevice::Ref &display, const r::VidMode &mode);
	void ResetDisplayDevice();
	void LaunchURL(const char *sz);
	
#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)
	NativeDeviceContext::Ref CreateOpenGLContext();
#endif

	RAD_DECLARE_READONLY_PROPERTY(NativeApp, systemLanguage, StringTable::LangId);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, displayDevices, const DisplayDevice::Vec&);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, mainThreadId, thread::Id);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, argc, int);
	RAD_DECLARE_READONLY_PROPERTY(NativeApp, argv, const char**);

private:
		
	RAD_DECLARE_GET(systemLanguage, StringTable::LangId);
	RAD_DECLARE_GET(displayDevices, const DisplayDevice::Vec&);
	RAD_DECLARE_GET(mainThreadId, thread::Id);
	RAD_DECLARE_GET(argc, int);
	RAD_DECLARE_GET(argv, const char**);

	details::NativeApp m_imp;
	thread::Id m_mainThreadId;
	int m_argc;
	const char **m_argv;
};

#include <Runtime/PopPack.h>
#include "NativeApp_inl.h"
