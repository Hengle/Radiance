/*! \file iOSNativeApp.mm
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include "../NativeApp.h"
#include <Engine/App.h>
#include <Engine/COut.h>
#include <Engine/Zones.h>
#include <Engine/Renderer/GL/RBackend.h>
#include <Engine/StringTable.h>
#include "../GL/GLContext.h"

#import "AppDelegate.h"
#import "UIDevice-Hardware.h"

StringTable::LangId AppleGetLangId();

namespace details {

NativeApp::NativeApp() {
	
}

bool NativeApp::PreInit() {

	m_langId = AppleGetLangId();
	
	COut(C_Info) << "Detecting system..." << std::endl;
	
	::DisplayDevice::Ref _dd(new (ZEngine) ::DisplayDevice());
	DisplayDevice *dd = &_dd->m_imp;
	
	r::VidMode vm;
	
	vm.w = s_app->glView->width;
	vm.h = s_app->glView->height;
	vm.hz = 60;
	vm.bpp = 32;
	
	dd->m_defMode = vm;
	dd->m_curMode = vm;
	dd->m_maxMSAA = 4;
	dd->m_maxAnisotropy = 16;
	
	dd->m_vidModes.push_back(vm);
	
	m_primaryDisplay = _dd;
	m_displayDevices.push_back(_dd);
	
	const char *family = 0;
	
	switch ([[UIDevice currentDevice] deviceFamily]) {
		case UIDeviceFamilyiPhone:
			m_deviceFamily = plat::kDeviceFamily_iPhone;
			family = "iPhone";
			break;
		case UIDeviceFamilyiPod:
			m_deviceFamily = plat::kDeviceFamily_iPod;
			family = "iPod";
			break;
		case UIDeviceFamilyiPad:
			m_deviceFamily = plat::kDeviceFamily_iPad;
			family = "iPad";
			break;
		default:
			m_deviceFamily = plat::kDeviceFamily_Uknown;
			family = "Unknown";
			break;
	}
	
	const char *deviceType = 0;
	
	switch([[UIDevice currentDevice] platformType]) {
	
	// iPhone
		case UIDevice1GiPhone:
			m_deviceType = plat::kDeviceType_iPhone1;
			deviceType = "iPhone 1";
			break;
		case UIDevice3GiPhone:
			m_deviceType = plat::kDeviceType_iPhone3;
			deviceType = "iPhone 3G";
			break;
		case UIDevice3GSiPhone:
			m_deviceType = plat::kDeviceType_iPhone3GS;
			deviceType = "iPhone 3GS";
			break;
		case UIDevice4iPhone:
			m_deviceType = plat::kDeviceType_iPhone4;
			deviceType = "iPhone 4";
			break;
		case UIDevice4SiPhone:
			m_deviceType = plat::kDeviceType_iPhone4S;
			deviceType = "iPhone 4S";
			break;
		case UIDevice5iPhone:
			m_deviceType = plat::kDeviceType_iPhone5;
			deviceType = "iPhone 5";
			break;
	
	// iPod
		case UIDevice1GiPod:
			m_deviceType = plat::kDeviceType_iPod1;
			deviceType = "iPod 1";
			break;
		case UIDevice2GiPod:
			m_deviceType = plat::kDeviceType_iPod2;
			deviceType = "iPod 2";
			break;
		case UIDevice3GiPod:
			m_deviceType = plat::kDeviceType_iPod3;
			deviceType = "iPod 3";
			break;
		case UIDevice4GiPod:
			m_deviceType = plat::kDeviceType_iPod4;
			deviceType = "iPod 4";
			break;
			
	// iPad
		case UIDevice1GiPad:
			m_deviceType = plat::kDeviceType_iPad1;
			deviceType = "iPad 1";
			break;
		case UIDevice2GiPad:
			m_deviceType = plat::kDeviceType_iPad2;
			deviceType = "iPad 2";
			break;
		case UIDevice3GiPad:
			m_deviceType = plat::kDeviceType_iPad3;
			deviceType = "iPad 3";
			break;
		
	// default
		default:
			m_deviceType = plat::kDeviceType_Unknown;
			break;
	}
	
	if (m_deviceType == plat::kDeviceType_Unknown) {
		COut(C_Info) << "Device: Unrecognized " << family << std::endl;
	} else {
		COut(C_Info) << "Device: " << deviceType << std::endl;
	}
	
	NSString *nsString = [[UIDevice currentDevice] systemVersion];
	String osVersion([nsString cStringUsingEncoding:NSASCIIStringEncoding]);
	
	int major = 0;
	int minor = 0;
	int rev = 0;
	
	sscanf(osVersion.c_str.get(), "%d.%d.%d", &major, &minor, &rev);
	m_osVersion = RAD_MAKE_OS_VERSION(major, minor, rev);
	
	COut(C_Info) << "iOS Version: " << major << "." << minor << "." << rev << std::endl;
	COut(C_Info) << "Screen Size " << vm.w << "x" << vm.h << "x" << vm.bpp << "x" << vm.hz << std::endl;
		
	return true;
}

bool NativeApp::Initialize() {
	return true;
}

void NativeApp::Finalize() {
}

bool NativeApp::BindDisplayDevice(const ::DisplayDeviceRef &display, const r::VidMode &mode) {
	m_activeDisplay = display;
	return true;
}

void NativeApp::ResetDisplayDevice() {
	m_activeDisplay.reset();
}

void NativeApp::LaunchURL(const char *sz) {
	[[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:sz]]];
}

void NativeApp::SetThrottleFramerate(bool throttle) {
	s_app->throttleFramerate = throttle;
	if (s_app->refreshTimer)
		[s_app startRefresh];
}

struct wGLContext : public GLDeviceContext {
	EAGLView *glCtx;
	
	wGLContext(EAGLView *x) : glCtx(x) {
	}
	
	~wGLContext() {
	}
	
	virtual void Bind() {
		[glCtx bindGL];
	}
	
	virtual void Unbind() {
		[glCtx unbindGL];
	}
	
	virtual void SwapBuffers() {
		[glCtx presentScene];
	}
	
	virtual void BindFramebuffer() {
		[glCtx bindFramebuffer];
	}
};

GLDeviceContext::Ref NativeApp::CreateOpenGLContext(const GLPixelFormat &pf) {
	RAD_ASSERT(m_activeDisplay);
	GLDeviceContext::Ref r(new wGLContext(s_app->glView));
	r->Bind();
	return r;
}

StringTable::LangId NativeApp::RAD_IMPLEMENT_GET(systemLangId) {
	return m_langId;
}

} // details

