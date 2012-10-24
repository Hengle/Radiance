/*! \file NativeApp_inl.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

inline bool DisplayDevice::RAD_IMPLEMENT_GET(primary) {
	return m_imp.primary;
}

inline const r::VidModeVec *DisplayDevice::RAD_IMPLEMENT_GET(vidModes) {
	return m_imp.vidModes;
}

inline const r::VidMode *DisplayDevice::RAD_IMPLEMENT_GET(curVidMode) {
	return m_imp.curVidMode;
}

inline const r::VidMode *DisplayDevice::RAD_IMPLEMENT_GET(defVidMode) {
	return m_imp.defVidMode;
}

inline int DisplayDevice::RAD_IMPLEMENT_GET(maxMSAA) {
	return m_imp.maxMSAA;
}

inline int DisplayDevice::RAD_IMPLEMENT_GET(maxAnisotropy) {
	return m_imp.maxAnisotropy;
}

///////////////////////////////////////////////////////////////////////////////

inline void NativeApp::LaunchURL(const char *sz) {
	m_imp.LaunchURL(sz);
}

inline bool NativeApp::PreInit() {
	return m_imp.PreInit();
}

inline bool NativeApp::Initialize() {
	return m_imp.Initialize();
}

inline void NativeApp::Finalize() {
	m_imp.Finalize();
}

inline bool NativeApp::BindDisplayDevice(const DisplayDevice::Ref &display, const r::VidMode &mode) {
	return m_imp.BindDisplayDevice(display, mode);
}

inline void NativeApp::ResetDisplayDevice() {
	m_imp.ResetDisplayDevice();
}

inline StringTable::LangId NativeApp::RAD_IMPLEMENT_GET(systemLangId) {
	return m_imp.systemLangId;
}

inline thread::Id NativeApp::RAD_IMPLEMENT_GET(mainThreadId) {
	return m_mainThreadId;
}

inline int NativeApp::RAD_IMPLEMENT_GET(argc) {
	return m_argc;
}

inline const char **NativeApp::RAD_IMPLEMENT_GET(argv) {
	return m_argv;
}

inline plat::DeviceFamily NativeApp::RAD_IMPLEMENT_GET(deviceFamily) {
	return m_imp.deviceFamily;
}

inline plat::DeviceType NativeApp::RAD_IMPLEMENT_GET(deviceType) {
	return m_imp.deviceType;
}

inline plat::OSType NativeApp::RAD_IMPLEMENT_GET(osType) {
	return m_imp.osType;
}

inline int NativeApp::RAD_IMPLEMENT_GET(osVersion) {
	return m_imp.osVersion;
}

inline const DisplayDevice::Ref &NativeApp::RAD_IMPLEMENT_GET(primaryDisplay) {
	return m_imp.primaryDisplay;
}

inline const DisplayDevice::Ref &NativeApp::RAD_IMPLEMENT_GET(activeDisplay) {
	return m_imp.activeDisplay;
}

inline const DisplayDevice::Vec &NativeApp::RAD_IMPLEMENT_GET(displayDevices) {
	return m_imp.displayDevices;
}

inline void NativeApp::RAD_IMPLEMENT_SET(throttleFramerate) (bool throttle) {
	if (throttle != m_throttle) {
		m_throttle = throttle;
		m_imp.SetThrottleFramerate(throttle);
	}
}

inline bool NativeApp::RAD_IMPLEMENT_GET(throttleFramerate) {
	return m_throttle;
}

#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)
inline GLDeviceContext::Ref NativeApp::CreateOpenGLContext(const GLPixelFormat &pf) {
	return m_imp.CreateOpenGLContext(pf);
}
#endif
