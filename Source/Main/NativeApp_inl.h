/*! \file NativeApp_inl.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

inline const r::VidModeVec *DisplayDevice::RAD_IMPLEMENT_GET(vidModes) {
	return m_imp.vidModes;
}

inline r::VidMode DisplayDevice::RAD_IMPLEMENT_GET(curVidMode) {
	return m_imp.curVidMode;
}

inline bool DisplayDevice::IsVidModeSupported(const r::VidMode &mode) {
	return m_imp.IsVidModeSupported(mode);
}

inline void NativeApp::LaunchURL(const char *sz) {
	m_imp.LaunchURL(sz);
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

inline const DisplayDevice::Vec &NativeApp::RAD_IMPLEMENT_GET(displayDevices) {
	return m_imp.displayDevices;
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

#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)
inline NativeDeviceContext::Ref NativeApp::CreateOpenGLContext() {
	return m_imp.CreateOpenGLContext();
}
#endif
