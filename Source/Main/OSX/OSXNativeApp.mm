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

#import <AppKit/AppKit.h>
#import <AppKit/NSWorkspace.h>
#import <AppKit/NSOpenGL.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#if !defined(RAD_OPT_PC_TOOLS)
#import "AppDelegate.h"
#endif

namespace {
r::VidMode VidModeFromCGMode(CGDisplayModeRef cgMode) {
	r::VidMode m;
	m.fullscreen = false;
	
	m.w = (int)CGDisplayModeGetWidth(cgMode);
	m.h = (int)CGDisplayModeGetHeight(cgMode);
	
	m.hz = (int)CGDisplayModeGetRefreshRate(cgMode);
	if (m.hz < 59)
		m.hz = 59;
	
	m.bpp = 8;
	
	CFStringRef px = CGDisplayModeCopyPixelEncoding(cgMode);
	if (CFStringCompare(px, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
		m.bpp = 32;
	} else if (CFStringCompare(px, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
		m.bpp = 16;
	}
	
	return m;
}
	
NSScreen *NSScreenForCGDirectDisplayID(CGDirectDisplayID displayId) {
	int numScreens = (int)[[NSScreen screens] count];
	for (int i = 0; i < numScreens; ++i) {
		NSScreen *s = [[NSScreen screens] objectAtIndex:i];
		NSDictionary *d = [s deviceDescription];
		NSNumber *n = [d objectForKey:@"NSScreenNumber"];
		if (n && ([n unsignedIntValue] == displayId))
			return s;
	}
	return 0;
}
}

namespace details {

#if !defined(RAD_OPT_PC_TOOSL)
	
bool ConfigureWindow(
	int width,
	int height,
	bool fullscreen,
	int screenWidth,
	int screenHeight
);
	
#endif
	
struct DDVars : public DisplayDevice::NativeVars {
	enum FadeState {
		kFadeState_Black,
		kFadeState_Normal
	};
	
	enum FadeTiming {
		kFadeTiming_Default,
		kFadeTiming_Immediate
	};
	
	CGDirectDisplayID displayId;
	CGDisplayModeRef defModeRef;
	CFArrayRef displayModes;
	FadeState fadeState;
	
	
	DDVars() : displayModes(0), defModeRef(0), fadeState(kFadeState_Normal) {
	}
	
	virtual ~DDVars() {
		if (displayModes)
			CFRelease(displayModes);
		if (defModeRef)
			CGDisplayModeRelease(defModeRef);
	}
	
	CGDisplayModeRef FindMatchingDisplayMode(const r::VidMode &mode) {
		RAD_ASSERT(displayModes);
		const int kNumModes = (int)CFArrayGetCount(displayModes);
		
		for (int i = 0; i < kNumModes; ++i) {
			CGDisplayModeRef cgMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(displayModes, (CFIndex)i);
			RAD_ASSERT(cgMode);
			r::VidMode testMode = VidModeFromCGMode(cgMode);
			
			if (testMode.w != mode.w ||
				testMode.h != mode.h ||
				testMode.bpp != mode.bpp ||
				testMode.hz != mode.hz)
				continue;
			
			return cgMode;
		}
		
		return 0;
	}
	
	void FadeIn(FadeTiming timing = kFadeTiming_Default) {
		if (fadeState != DDVars::kFadeState_Normal) {
			fadeState = DDVars::kFadeState_Normal;
			
			CGDisplayFadeReservationToken token;
			while (CGAcquireDisplayFadeReservation(1.f, &token) != kCGErrorSuccess) {}
			
			CGDisplayFade(token, (timing==kFadeTiming_Immediate) ? 0.f : 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, true);
			CGReleaseDisplayFadeReservation(token);
		}
	}
	
	void FadeOut() {
		if (fadeState != DDVars::kFadeState_Black) {
			fadeState = DDVars::kFadeState_Black;
			
			CGDisplayFadeReservationToken token;
			while (CGAcquireDisplayFadeReservation(1.f, &token) != kCGErrorSuccess) {}
			
			CGDisplayFade(token, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, true);
			CGReleaseDisplayFadeReservation(token);
		}
	}
	
	static DDVars *Get(const DisplayDevice::NativeVars::Ref &r) {
		return static_cast<DDVars*>(r.get());
	}
};

NativeApp::NativeApp()
{
}

bool NativeApp::PreInit() {
	
	COut(C_Info) << "Detecting video system..." << std::endl;
	
	CGDirectDisplayID displays[256];
	uint32_t numDisplays;
	
	if ((CGGetActiveDisplayList(256, displays, &numDisplays) != kCGErrorSuccess) || (numDisplays < 1)) {
		COut(C_Error) << "ERROR: CGGetActiveDisplayList failed." << std::endl;
		return false;
	}
	
	for (uint32_t i = 0;  i < numDisplays; ++i) {
		::DisplayDevice::Ref _dd(new (ZEngine) ::DisplayDevice());
		DisplayDevice *dd = &_dd->m_imp;
		
		dd->m_primary = i == 0;
		dd->m_vars.reset(new (ZEngine) DDVars());
		DDVars *ddv = DDVars::Get(dd->m_vars);
		
		ddv->displayId = displays[i];
		CGDisplayModeRef defMode = CGDisplayCopyDisplayMode(ddv->displayId);
		
		ddv->defModeRef = defMode;
		dd->m_defMode = VidModeFromCGMode(defMode);
		dd->m_curMode = dd->m_defMode;
		dd->m_maxMSAA = 0;
		dd->m_maxAnisotropy = 0;
		
		ddv->displayModes = CGDisplayCopyAllDisplayModes(displays[i], 0);
		if (!ddv->displayModes)
			continue;
		
		const int kNumModes = (int)CFArrayGetCount(ddv->displayModes);
		for (int k = 0; k < kNumModes; ++k) {
			CGDisplayModeRef cgMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(ddv->displayModes, (CFIndex)k);
			RAD_ASSERT(cgMode);
			
			uint32_t flags = CGDisplayModeGetIOFlags(cgMode);
			
			if (flags&kDisplayModeStretchedFlag)
				continue; // we don't do this.
			
			r::VidMode m = VidModeFromCGMode(cgMode);
			if (m.bpp < 32)
				continue; // we don't like anything less than 32 bpp.
			
			// don't allow duplicates.
			r::VidModeVec::iterator it;
			for (it = dd->m_vidModes.begin(); it != dd->m_vidModes.end(); ++it) {
				r::VidMode &x = *it;
				
				if (x.w == m.w &&
					x.h == m.h) {
					// matches.
					// prefer we use the same hz as our desktop...
					if (m.hz == dd->m_defMode.hz)
						x.hz = m.hz;
					break;
				}
			}
			
			if (it == dd->m_vidModes.end())
				dd->m_vidModes.push_back(m);
		}
		
		for (r::VidModeVec::const_iterator it = dd->m_vidModes.begin(); it != dd->m_vidModes.end(); ++it) {
			const r::VidMode &m = *it;
			COut(C_Info) << "\t" << (it-dd->m_vidModes.begin()) << ": " << m.w << "x" << m.h << "x" << m.bpp << " @ " << m.hz << "hz" << std::endl;
		}
		
		if (dd->m_vidModes.empty()) {
			if (dd->m_primary) {
				COut(C_Error) << "ERROR: primary display is invalid, initialization failed." << std::endl;
				return false;
			}
			continue;
		}
		
		// create a GL context to find out MSAA and anisotropy limits.
		std::vector<NSOpenGLPixelFormatAttribute> attribs;
		attribs.reserve(128);
		
		attribs.push_back(NSOpenGLPFAMinimumPolicy);
		attribs.push_back(NSOpenGLPFAAccelerated);
		attribs.push_back(NSOpenGLPFAFullScreen);
		attribs.push_back(NSOpenGLPFADoubleBuffer);
		attribs.push_back(NSOpenGLPFAColorSize);
		attribs.push_back(24);
		attribs.push_back(NSOpenGLPFAAlphaSize);
		attribs.push_back(8);
		attribs.push_back(NSOpenGLPFADepthSize);
		attribs.push_back(24);
		attribs.push_back(NSOpenGLPFAStencilSize);
		attribs.push_back(8);
		attribs.push_back(NSOpenGLPFAScreenMask);
		attribs.push_back(CGDisplayIDToOpenGLDisplayMask(ddv->displayId));
		
		
		NSOpenGLPixelFormat *pf = 0;
		
		// we'll find out max MSAA if we can find a pixel format...
		int msaa = 16;
		for (;;) {
			std::vector<NSOpenGLPixelFormatAttribute> x(attribs);
			
			if (msaa > 0) {
				x.push_back(NSOpenGLPFASampleBuffers);
				x.push_back(1);
				x.push_back(NSOpenGLPFASamples);
				x.push_back((NSOpenGLPixelFormatAttribute)msaa);
			}
			
			x.push_back(0);
			
			pf = [[[NSOpenGLPixelFormat alloc] initWithAttributes:&x[0]] autorelease];
			if (pf)
				break;
			
			RAD_ASSERT(msaa > 0);
			
			msaa >>= 1;
		}
		
		dd->m_maxMSAA = msaa;
		
		// make a GL context and figure out max anisotropy.
		NSOpenGLContext *glCtx = [[NSOpenGLContext alloc] initWithFormat: pf shareContext: nil];
		if (!glCtx) {
			COut(C_Warn) << "WARNING: Unable to create an openGL context!" << std::endl;
			if (dd->m_primary) {
				COut(C_Error) << "ERROR: primary display is invalid!" << std::endl;
				return false;
			}
			continue;
		}
		
		[glCtx makeCurrentContext];
		
		const char *glExt = (const char *)glGetString(GL_EXTENSIONS);
		if (glExt && string::strstr(glExt, "GL_EXT_texture_filter_anisotropic")) {
			GLint maxAnisotropy;
			glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
			dd->m_maxAnisotropy = maxAnisotropy;
		}
		
		[NSOpenGLContext clearCurrentContext];
		[glCtx release];
		
		m_displayDevices.push_back(_dd);
		
		if (dd->m_primary)
			m_primaryDisplay = _dd;
	}
	
	return true;
}

bool NativeApp::Initialize() {
	return true;
}

void NativeApp::Finalize() {
}

bool NativeApp::BindDisplayDevice(const ::DisplayDeviceRef &display, const r::VidMode &mode) {
	RAD_ASSERT(display);
	
	bool capture = true;
	if (m_activeDisplay) {
		if (m_activeDisplay.get() != display.get()) {
			ResetDisplayDevice();
		} else {
			capture = false;
		}
	}
	
	DDVars *ddv = DDVars::Get(display->m_imp.m_vars);
	
#if !defined(RAD_OPT_PC_TOOLS)
	NSRect screenRect;
	screenRect.origin.x = 0;
	screenRect.origin.y = 0;
	
	NSUInteger style;
#endif
	
	if (mode.fullscreen) {
		
#if !defined(RAD_OPT_PC_TOOLS)
		if (s_appd->window) {
			[s_appd->window release];
			s_appd->window = 0;
		}
#endif
		
		CGDisplayModeRef cgMode = ddv->FindMatchingDisplayMode(mode);
		
		if (!cgMode) {
			COut(C_Error) << "No native video mode found which matches " << mode.w << "x" << mode.h << "x" << mode.bpp << " @ " << mode.hz << "hz" << std::endl;
			return false;
		}
		
		ddv->FadeOut();
		
		if (capture && (CGCaptureAllDisplaysWithOptions(kCGCaptureNoOptions) != kCGErrorSuccess)) {
			ddv->FadeIn(DDVars::kFadeTiming_Immediate);
			COut(C_Error) << "CGDisplayCapture failed!" << std::endl;
			return false;
		}
		
		if (!display->curVidMode->SameSize(mode)) {
			if (CGDisplaySetDisplayMode(ddv->displayId, cgMode, 0) != kCGErrorSuccess) {
				ddv->FadeIn(DDVars::kFadeTiming_Immediate);
				COut(C_Error) << "CGDisplaySetDisplayMode failed for mode " << mode.w << "x" << mode.h << "x" << mode.bpp << " @ " << mode.hz << "hz" << std::endl;
				return false;
			}
		}
		
#if !defined(RAD_OPT_PC_TOOLS)
		style = NSBorderlessWindowMask;
		screenRect.size.width = mode.w;
		screenRect.size.height = mode.h;
	} else {
		
		NSScreen *screen = NSScreenForCGDirectDisplayID(ddv->displayId);
		if (!screen) {
			COut(C_Error) << "ERROR: unable to find screen!" << std::endl;
			return false;
		}
		
		style = NSTitledWindowMask;
		screenRect = [screen frame];
	}
	
	NSRect windowRect;
	windowRect.size.width = mode.w;
	windowRect.size.height = mode.h;
	windowRect.origin.x = ((short)screenRect.size.width - windowRect.size.width) / 2;
	windowRect.origin.y = ((short)screenRect.size.height - windowRect.size.height) / 2;
	
	if (s_appd->window) {
		NSRect r = [NSWindow frameRectForContentRect: windowRect styleMask: style];
		[s_appd->window setStyleMask: style];
		[s_appd->window setFrame: r display: TRUE];
	} else {
		s_appd->window = [[NSWindow alloc] initWithContentRect: windowRect styleMask: style backing: NSBackingStoreBuffered defer: NO screen: nil];
		
		NSString *title = [NSString stringWithUTF8String: App::Get()->title.get()];
		[s_appd->window setTitle: title];
		[title release];
		
		[s_appd->window setAcceptsMouseMovedEvents: YES];
		[s_appd->window setOpaque: YES];
	}
	
	if (mode.fullscreen) {
		[s_appd->window setLevel: CGShieldingWindowLevel()+1];
	} else {
		[s_appd->window setLevel: NSNormalWindowLevel];
	}
	
	[s_appd->window orderFront: nil];
#else
	}
#endif
	
	display->m_imp.m_curMode = mode;
	m_activeDisplay = display;
	
	return true;
}

void NativeApp::ResetDisplayDevice() {
	if (m_activeDisplay) {
		if (m_activeDisplay->curVidMode->fullscreen) {
			DDVars *ddv = DDVars::Get(m_activeDisplay->m_imp.m_vars);
			
			ddv->FadeOut();
			
			if ([NSOpenGLContext currentContext]) {
				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT);
				[[NSOpenGLContext currentContext] flushBuffer];
				glClear(GL_COLOR_BUFFER_BIT);
				[[NSOpenGLContext currentContext] flushBuffer];
			}
			
			if (!m_activeDisplay->curVidMode->SameSize(m_activeDisplay->m_imp.m_defMode))
				CGDisplaySetDisplayMode(ddv->displayId, ddv->defModeRef, 0);
			
			CGReleaseAllDisplays();
			
#if !defined(RAD_OPT_PC_TOOLS)
			if (s_appd->window) {
				if (App::Get()->exit) {
					[s_appd->window close];
				}
				else {
					[s_appd->window release];
				}
				s_appd->window = 0;
			}
#endif
			
			ddv->FadeIn();
			
		} else {
	
#if !defined(RAD_OPT_PC_TOOLS)
		if (s_appd->window) {
			[s_appd->window release];
			s_appd->window = 0;
		}
#endif
		}
		
		m_activeDisplay->m_imp.m_curMode = m_activeDisplay->m_imp.m_defMode;
		m_activeDisplay.reset();
	}

}

void NativeApp::LaunchURL(const char *sz) {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:sz]]];
}

#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)

struct wGLContext : public GLDeviceContext {
	NSOpenGLContext *glCtx;
	
	wGLContext(NSOpenGLContext *x) : glCtx(x) {
	}
	
	~wGLContext() {
		[NSOpenGLContext clearCurrentContext];
		[glCtx clearDrawable];
		[glCtx release];
	}
	
	virtual void Bind() {
		[glCtx makeCurrentContext];
	}
	
	virtual void Unbind() {
		[NSOpenGLContext clearCurrentContext];
	}
	
	virtual void SwapBuffers() {
		[glCtx flushBuffer];
	}
};

GLDeviceContext::Ref NativeApp::CreateOpenGLContext(const GLPixelFormat &pf) {
	RAD_ASSERT(m_activeDisplay);
	
	DDVars *ddv = DDVars::Get(m_activeDisplay->m_imp.m_vars);
	
	std::vector<NSOpenGLPixelFormatAttribute> attribs;
	attribs.reserve(128);
	
	attribs.push_back(NSOpenGLPFAMinimumPolicy);
	/*if (m_activeDisplay->curVidMode->fullscreen)
		attribs.push_back(NSOpenGLPFAFullScreen);*/
	attribs.push_back(NSOpenGLPFAAccelerated);
	if (pf.doubleBuffer)
		attribs.push_back(NSOpenGLPFADoubleBuffer);
	attribs.push_back(NSOpenGLPFAColorSize);
	attribs.push_back(pf.red+pf.green+pf.blue);
	attribs.push_back(NSOpenGLPFAAlphaSize);
	attribs.push_back(pf.alpha);
	attribs.push_back(NSOpenGLPFADepthSize);
	attribs.push_back(pf.depth);
	attribs.push_back(NSOpenGLPFAStencilSize);
	attribs.push_back(pf.stencil);
	attribs.push_back(NSOpenGLPFAScreenMask);
	attribs.push_back(CGDisplayIDToOpenGLDisplayMask(ddv->displayId));
	
	if (pf.mSamples > 0) {
		attribs.push_back(NSOpenGLPFASampleBuffers);
		attribs.push_back(1);
		attribs.push_back(NSOpenGLPFASamples);
		attribs.push_back((NSOpenGLPixelFormatAttribute)pf.mSamples);
	}
	
	attribs.push_back(0);
	
	NSOpenGLPixelFormat *nspf = 0;
	
	nspf = [[[NSOpenGLPixelFormat alloc] initWithAttributes:&attribs[0]] autorelease];
	if (!nspf) {
		COut(C_Error) << "Failed to create pixel format!" << std::endl;
		return GLDeviceContext::Ref();
	}
	
	NSOpenGLContext *glCtx = [[NSOpenGLContext alloc] initWithFormat: nspf shareContext: nil];
	if (!glCtx) {
		COut(C_Error) << "Failed to OpenGL context!" << std::endl;
		return GLDeviceContext::Ref();
	}
	
	GLDeviceContext::Ref r(new wGLContext(glCtx));
	r->Bind();
	
	if (s_appd->window)
		[glCtx setView: [s_appd->window contentView]];
	
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	r->SwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
	r->SwapBuffers();

	if (m_activeDisplay->curVidMode->fullscreen)
		ddv->FadeIn();
	
	return r;
}
	
#endif

StringTable::LangId NativeApp::RAD_IMPLEMENT_GET(systemLangId) {
	return StringTable::LangId_EN;
}

} // details

