/*! \file WinNativeApp.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include "../NativeApp.h"
#include <Engine/App.h>
#include <Engine/COut.h>
#include <Engine/Engine.h>
#include <Engine/Zones.h>
#include <Engine/Input.h>
#include <Engine/Renderer/PC/RBackend.h>
#include <Runtime/Base.h>
#include <Runtime/File.h>
#include <Runtime/Thread.h>
#include <Runtime/Time.h>
#include <Runtime/Runtime.h>

#if defined(RAD_OPT_GL)
#include <Engine/Renderer/GL/GLTable.h>
#endif

#import <AppKit/NSWorkspace.h>
#import <Foundation/Foundation.h>

namespace details {

struct DDVars : public DisplayDevice::NativeVars {
	static DDVars *Get(const DisplayDevice::NativeVars::Ref &r) {
		return static_cast<DDVars*>(r.get());
	}
};

NativeApp::NativeApp()
{
}

bool NativeApp::PreInit() {
	
	COut(C_Info) << "Detecting video system..." << std::endl;

	return true;
}

bool NativeApp::Initialize() {
	return true;
}

void NativeApp::Finalize() {
}

bool NativeApp::BindDisplayDevice(const ::DisplayDeviceRef &display, const r::VidMode &mode) {
	return true;
}

void NativeApp::ResetDisplayDevice() {

}

void NativeApp::LaunchURL(const char *sz) {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:sz]]];
}

#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)

struct wGLContext : public GLDeviceContext {
	
};

GLDeviceContext::Ref NativeApp::CreateOpenGLContext(const GLPixelFormat &pf) {
	return GLDeviceContext::Ref();
}
	
#endif

StringTable::LangId NativeApp::RAD_IMPLEMENT_GET(systemLangId) {
	return StringTable::LangId_EN;
}

} // details

