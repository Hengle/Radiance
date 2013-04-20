/*! \file WinNativeApp.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include RADPCH
#include "../NativeApp.h"
#include <Engine/App.h>
#include <Engine/COut.h>
#include <Engine/Engine.h>
#include <Engine/Zones.h>
#include <Engine/Input.h>
#include <Runtime/Base.h>
#include <Runtime/File.h>
#include <Runtime/Thread.h>
#include <Runtime/Time.h>
#include <Runtime/Win/WinHeaders.h>
#include <Runtime/Runtime.h>
#include <ShellAPI.h>
#include "../../../../Source/VSProjects/VS10/resource.h"
#include <tchar.h>

#if defined(RAD_OPT_GL)
#include <Engine/Renderer/GL/RBackend.h>
#include <Engine/Renderer/GL/GLTable.h>
#include <Engine/Renderer/GL/wglext.h>
#endif

#if !defined(RAD_OPT_PC_TOOLS)
#include "WinKeys.h"
#endif

namespace {
enum {
	kRequiredDMFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT,
	kMaxMultiSample = 32
};
r::VidMode VidModeFromDevMode(const DEVMODEA &dm) {
	RAD_ASSERT((dm.dmFields&kRequiredDMFields) == kRequiredDMFields);
	r::VidMode m;
	m.w = dm.dmPelsWidth;
	m.h = dm.dmPelsHeight;
	m.bpp = dm.dmBitsPerPel;
	m.fullscreen = false;
	m.hz = 60;

	if (dm.dmFields & DM_DISPLAYFREQUENCY)
		m.hz = dm.dmDisplayFrequency;

	if (m.hz < 59)
		m.hz = 59;

	return m;
}
}

#if !defined(RAD_OPT_PC_TOOLS)

namespace {

HDC s_hDC=0;
HWND s_hWnd=0;
HINSTANCE s_hInstance=0;
}

bool ConfigureWindow(
	int width,
	int height,
	bool fullscreen,
	int screenWidth,
	int screenHeight
);
#else
namespace {
HINSTANCE s_hInstance=0;
}

#endif

namespace details {

struct DDVars : public DisplayDevice::NativeVars {
	DISPLAY_DEVICEA dd;
	DEVMODEA dm;

	static DDVars *Get(const DisplayDevice::NativeVars::Ref &r) {
		return static_cast<DDVars*>(r.get());
	}
};

NativeApp::NativeApp()
{
}

inline HGLRC myWglCreateContex(HDC dc) {
	HGLRC wglRC = 0;
	__try {
		wglRC = wglCreateContext(dc);
	} __except(EXCEPTION_CONTINUE_EXECUTION) {
	}
	return wglRC;
}


bool NativeApp::PreInit() {
	
	COut(C_Info) << "Detecting video system..." << std::endl;

#define HACK_DISABLE_GL
#if defined(RAD_OPT_GL) && !defined(HACK_DISABLE_GL)

	{
		WNDCLASSEXA clex;
		memset(&clex, 0, sizeof(WNDCLASSEXA));

		clex.cbSize = sizeof(WNDCLASSEXA);
		clex.style = CS_OWNDC;
		clex.lpfnWndProc = DefWindowProc;
		clex.hInstance = s_hInstance;
		clex.lpszClassName = "rad_openGL_ini";

		RegisterClassExA(&clex);
	}

	HWND wglWnd = CreateWindowA(
		"rad_openGL_ini", 
		"", 
		WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
		0,
		0,
		100,
		100,
		0,
		0,
		s_hInstance,
		0
	);

	if (!wglWnd) {
		COut(C_Error) << "ERROR: Unable to create device window!" << std::endl;
		return false;
	}

	HDC wglDC = GetDC(wglWnd);

	{
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cAlphaBits = 8;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int pf = ChoosePixelFormat(wglDC, &pfd);
		if (pf < 1) {
			ReleaseDC(wglWnd, wglDC);
			DestroyWindow(wglWnd);
			COut(C_Error) << "ERROR: Unable to bind device window pixel format!" << std::endl;
			return false;
		}

		DescribePixelFormat(wglDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

		if (!SetPixelFormat(wglDC, pf, &pfd)) {
			COut(C_Error) << "ERROR: Unable to bind device window pixel format!" << std::endl;
			return false;
		}
	}

	HGLRC wglRC = myWglCreateContex(wglDC);
	
	if (!wglRC) {
		COut(C_Error) << "ERROR: Unable to create device window context!" << std::endl;
		return false;
	}

	wglMakeCurrent(wglDC, wglRC);
	
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
	if (!wglGetExtensionsStringARB) {
		wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
		if (!wglGetExtensionsStringARB) {
			wglMakeCurrent(0, 0);
			wglDeleteContext(wglRC);
			ReleaseDC(wglWnd, wglDC);
			DestroyWindow(wglWnd);
			COut(C_Error) << "Unable to find wglGetExtensionsStringARB" << std::endl;
			return false;
		}
	}

	const char *wglStrings = wglGetExtensionsStringARB(wglDC);

	if (wglStrings)
		COut(C_Info) << "wglGetExtensionsStringARB: " << wglStrings << std::endl;

	if (!wglStrings || !string::strstr(wglStrings, "WGL_ARB_pixel_format")) {
		wglMakeCurrent(0, 0);
		wglDeleteContext(wglRC);
		ReleaseDC(wglWnd, wglDC);
		DestroyWindow(wglWnd);
		COut(C_Error) << "WGL_ARB_pixel_format is required but not found." << std::endl;
		return false;
	}

	bool multiSample = false;
	if (string::strstr(wglStrings, "WGL_ARB_multisample"))
		multiSample = true;

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");

#endif

	DISPLAY_DEVICEA wdd;
	for (int i = 0; ; ++i) {
		wdd.cb = sizeof(DISPLAY_DEVICEA);
		if (!EnumDisplayDevicesA(0, (DWORD)i, &wdd, 0))
			break;
		if (!(wdd.StateFlags&DISPLAY_DEVICE_ACTIVE))
			continue;
		
		::DisplayDevice::Ref _dd(new (ZEngine) ::DisplayDevice());
		DisplayDevice *dd = &_dd->m_imp;

		dd->m_vars.reset(new (ZEngine) DDVars());
		DDVars *ddv = DDVars::Get(dd->m_vars);

		memcpy(&ddv->dd, &wdd, sizeof(DISPLAY_DEVICEA));

		if (wdd.StateFlags&DISPLAY_DEVICE_PRIMARY_DEVICE) {
			dd->m_primary = true;
		} else {
			dd->m_primary = false;
		}

		DEVMODEA dm;
		dm.dmSize = sizeof(DEVMODEA);

		BOOL r = EnumDisplaySettingsA(ddv->dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm);

		if (!r || ((dm.dmFields&kRequiredDMFields) != kRequiredDMFields)) {
			if (dd->m_primary) {
				// this is kinda bad
				COut(C_Error) << "ERROR: failed to enumerate primary display, initialization failed." << std::endl;
				return false;
			}

			continue;
		}

		if (!(dm.dmFields & DM_POSITION)) {
			// dmPosition is not valid, meaning we cannot identify this as secondary monitor...
			if (dd->m_primary) {
				dm.dmPosition.x = 0;
				dm.dmPosition.y = 0;
				dm.dmFields |= DM_POSITION;
			} else {
				continue; // this monitor cannot be identified.
			}
		}

		string::ncpy((char*)dm.dmDeviceName, wdd.DeviceName, 32);
		memcpy(&ddv->dm, &dm, sizeof(DEVMODEA));
		dd->m_defMode = VidModeFromDevMode(dm);
		dd->m_curMode = dd->m_defMode;
		dd->m_maxMSAA = 0;
		dd->m_maxAnisotropy = 0;

		// enumerate valid display modes.
		COut(C_Info) << "Display " << i << ": '" << ddv->dd.DeviceName << " - " << ddv->dd.DeviceString << "':" << std::endl;

		for (int k = 0; ; ++k) {
			dm.dmSize = sizeof(DEVMODEA);
			if (!EnumDisplaySettingsA(ddv->dd.DeviceName, k, &dm))
				break;
			if ((dm.dmFields&kRequiredDMFields) != kRequiredDMFields)
				continue;
			if (dm.dmBitsPerPel < 32)
				continue;
			r::VidMode m = VidModeFromDevMode(dm);

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
				COut(C_Error) << "ERROR: primary display '" << ddv->dd.DeviceName << "' is invalid, initialization failed." << std::endl;
				return false;
			}
			continue;
		}

#if defined(RAD_OPT_GL) && !defined(HACK_DISABLE_GL)
		if (multiSample) {

			int maxSamples = 0;

			int reqAttribs[9] = {
				WGL_SUPPORT_OPENGL_ARB,
				WGL_DRAW_TO_WINDOW_ARB,
				WGL_ACCELERATION_ARB,
				WGL_PIXEL_TYPE_ARB,
				WGL_COLOR_BITS_ARB,
				WGL_ALPHA_BITS_ARB,
				WGL_DEPTH_BITS_ARB,
				WGL_STENCIL_BITS_ARB,
				WGL_SAMPLES_ARB
			};

			int attribs[9];

			// Enumerate all pixel formats, find highest multi-sample count
			for (int k = 1; ; ++k) { // PF's are 1 based
				if (!wglGetPixelFormatAttribivARB(wglDC, k, PFD_MAIN_PLANE, 9, reqAttribs, attribs))
					break;
				if (attribs[0] != TRUE)
					continue;
				if (attribs[1] != TRUE)
					continue;
				if (attribs[2] != WGL_FULL_ACCELERATION_ARB)
					continue;
				if (attribs[3] != WGL_TYPE_RGBA_ARB)
					continue;
				if (attribs[4] < 24)
					continue;
				if (attribs[5] < 8)
					continue;
				if (attribs[6] < 24)
					continue;
				if (attribs[7] < 8)
					continue;

				if (maxSamples < attribs[8])
					maxSamples = attribs[8];

				if (maxSamples >= kMaxMultiSample)
					break;
			}

			dd->m_maxMSAA = maxSamples;
		}

		const char *glExt = (const char *)glGetString(GL_EXTENSIONS);
		if (glExt && string::strstr(glExt, "GL_EXT_texture_filter_anisotropic")) {
			GLint maxAnisotropy;
			glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
			dd->m_maxAnisotropy = maxAnisotropy;
		}
		
#endif

		m_displayDevices.push_back(_dd);

		if (dd->m_primary)
			m_primaryDisplay = _dd;
	}

	if (!m_primaryDisplay) {
		COut(C_Error) << "ERROR: no primary display detected, initialization failed!" << std::endl;
		return false;
	}

	return true;
}

bool NativeApp::Initialize() {
	return true;
}

void NativeApp::Finalize() {
}

bool NativeApp::BindDisplayDevice(const ::DisplayDeviceRef &display, const r::VidMode &mode) {
	RAD_VERIFY(!m_activeDisplay);
	RAD_ASSERT(display);

	int screenWidth, screenHeight;

	if (mode.fullscreen) {
		DDVars *vars = DDVars::Get(display->m_imp.m_vars);
		DEVMODEA dm;
		memcpy(&dm, &vars->dm, sizeof(DEVMODEA));

		dm.dmPelsWidth = mode.w;
		dm.dmPelsHeight = mode.h;

		if (mode.hz) {
			dm.dmFields |= DM_DISPLAYFREQUENCY;
			dm.dmDisplayFrequency = mode.hz;
		}

		if (ChangeDisplaySettingsA(&dm, 0) != DISP_CHANGE_SUCCESSFUL) {
			ChangeDisplaySettings(0, 0);
			return false;
		}

		screenWidth = mode.w;
		screenHeight = mode.h;
	} else {
		screenWidth = display->m_imp.m_defMode.w;
		screenHeight = display->m_imp.m_defMode.h;
	}

#if !defined(RAD_OPT_PC_TOOLS)
	ConfigureWindow(
		mode.w,
		mode.h,
		mode.fullscreen,
		screenWidth,
		screenHeight
	);

	ShowWindow(s_hWnd, SW_SHOW);
	UpdateWindow(s_hWnd);
#endif

	display->m_imp.m_curMode = mode;
	m_activeDisplay = display;

	return true;
}

void NativeApp::ResetDisplayDevice() {
	if (m_activeDisplay) {
		if (m_activeDisplay->curVidMode->fullscreen)
			ChangeDisplaySettings(0, 0);
		m_activeDisplay->m_imp.m_curMode = m_activeDisplay->m_imp.m_defMode;
		m_activeDisplay.reset();
	}
}

void NativeApp::LaunchURL(const char *sz) {
	RAD_ASSERT(sz);
	ShellExecuteA(0, "open", sz, 0, 0, SW_SHOWNORMAL);
}

void NativeApp::SetThrottleFramerate(bool throttle) {
}

#if defined(RAD_OPT_GL) && !defined(RAD_OPT_PC_TOOLS)

struct wGLContext : public GLDeviceContext {
	HGLRC glrc;

	~wGLContext() {
		wglMakeCurrent(0, 0);
		wglDeleteContext(glrc);
	}

	virtual void Bind() {
		wglMakeCurrent(s_hDC, glrc);
	}

	virtual void Unbind() {
		wglMakeCurrent(0, 0);
	}

	virtual void SwapBuffers() {
		::SwapBuffers(s_hDC);
	}
};

GLDeviceContext::Ref NativeApp::CreateOpenGLContext(const GLPixelFormat &pf) {
	RAD_ASSERT(m_activeDisplay);

	if (!s_hWnd)
		return GLDeviceContext::Ref();

	if (!s_hDC)
		s_hDC = GetDC(s_hWnd);

	HWND wglWnd = CreateWindowA(
		"rad_openGL_ini", 
		"", 
		WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
		0,
		0,
		100,
		100,
		0,
		0,
		s_hInstance,
		0
	);

	if (!wglWnd) {
		COut(C_Error) << "ERROR: Unable to create device window!" << std::endl;
		return GLDeviceContext::Ref();
	}

	HDC wglDC = GetDC(wglWnd);

	{
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cAlphaBits = 8;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int pf = ChoosePixelFormat(wglDC, &pfd);
		if (pf < 1) {
			ReleaseDC(wglWnd, wglDC);
			DestroyWindow(wglWnd);
			COut(C_Error) << "ERROR: Unable to bind device window pixel format!" << std::endl;
			return GLDeviceContext::Ref();
		}

		DescribePixelFormat(wglDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

		if (!SetPixelFormat(wglDC, pf, &pfd)) {
			COut(C_Error) << "ERROR: Unable to bind device window pixel format!" << std::endl;
			return GLDeviceContext::Ref();
		}
	}

	HGLRC wglRC = wglCreateContext(wglDC);
	if (!wglRC) {
		COut(C_Error) << "ERROR: Unable to create device window context!" << std::endl;
		return GLDeviceContext::Ref();
	}

	wglMakeCurrent(wglDC, wglRC);

	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
	if (!wglGetExtensionsStringARB) {
		wglMakeCurrent(0, 0);
		wglDeleteContext(wglRC);
		ReleaseDC(wglWnd, wglDC);
		DestroyWindow(wglWnd);
		COut(C_Error) << "Unable to find wglGetExtensionsStringARB" << std::endl;
		return GLDeviceContext::Ref();
	}

	const char *wglStrings = wglGetExtensionsStringARB(wglDC);

	if (!wglStrings || !string::strstr(wglStrings, "WGL_ARB_pixel_format")) {
		wglMakeCurrent(0, 0);
		wglDeleteContext(wglRC);
		ReleaseDC(wglWnd, wglDC);
		DestroyWindow(wglWnd);
		COut(C_Error) << "WGL_ARB_pixel_format is required but not found." << std::endl;
		return GLDeviceContext::Ref();
	}

	bool multiSample = false;
	if (string::strstr(wglStrings, "WGL_ARB_multisample"))
		multiSample = true;

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

	int pfi = -1;

	for (int i = 0; i < 2; ++i) {
		std::vector<int> attribs;
		attribs.reserve(100);

		attribs.push_back(WGL_DRAW_TO_WINDOW_ARB);
		attribs.push_back(TRUE);

		attribs.push_back(WGL_ACCELERATION_ARB);
		attribs.push_back(WGL_FULL_ACCELERATION_ARB);

		attribs.push_back(WGL_SUPPORT_OPENGL_ARB);
		attribs.push_back(TRUE);
	
		attribs.push_back(WGL_DOUBLE_BUFFER_ARB);
		attribs.push_back((pf.doubleBuffer) ? TRUE : FALSE);

		attribs.push_back(WGL_PIXEL_TYPE_ARB);
		attribs.push_back(WGL_TYPE_RGBA_ARB);
	
		attribs.push_back(WGL_COLOR_BITS_ARB);
		attribs.push_back(pf.red+pf.green+pf.blue);

		attribs.push_back(WGL_RED_BITS_ARB);
		attribs.push_back(pf.red);

		attribs.push_back(WGL_GREEN_BITS_ARB);
		attribs.push_back(pf.green);

		attribs.push_back(WGL_BLUE_BITS_ARB);
		attribs.push_back(pf.blue);

		attribs.push_back(WGL_ALPHA_BITS_ARB);
		attribs.push_back(pf.alpha);

		attribs.push_back(WGL_DEPTH_BITS_ARB);
		attribs.push_back(pf.depth);

		if (i == 0 && multiSample && pf.mSamples > 0) {
			attribs.push_back(WGL_SAMPLE_BUFFERS_ARB);
			attribs.push_back(1);
			attribs.push_back(WGL_SAMPLES_ARB);
			attribs.push_back(pf.mSamples);
		}

		attribs.push_back(0);

		int x;
		UINT num;
		
		if (wglChoosePixelFormatARB(s_hDC, &attribs[0], 0, 1, &x, &num)) {
			// may have failed because we requested MSAA but it's unsupported
			if (num > 0) {
				pfi = x;
				break;
			}

			DDVars *dd = DDVars::Get(m_activeDisplay->m_imp.m_vars);
			COut(C_Warn) << "WARNING: Requested multisample format doesn't exist on '" << dd->dd.DeviceName << " - " << dd->dd.DeviceString << "', trying non-multisample mode..." << std::endl;
		}
	}

	// Done with temp window.

	wglMakeCurrent(0, 0);
	wglDeleteContext(wglRC);
	ReleaseDC(wglWnd, wglDC);
	DestroyWindow(wglWnd);

	if (pfi == -1) {
		COut(C_Error) << "ERROR: unable to find pixel format for requested mode!" << std::endl;
		return GLDeviceContext::Ref();
	}

	PIXELFORMATDESCRIPTOR pfd;
	if (DescribePixelFormat(s_hDC, pfi, sizeof(PIXELFORMATDESCRIPTOR), &pfd)) {
		if (SetPixelFormat(s_hDC, pfi, &pfd)) {
			HGLRC glrc = wglCreateContext(s_hDC);
			if (glrc) {
				wGLContext *wglc = new (ZEngine) wGLContext();
				wglc->glrc = glrc;
				wglc->Bind();
				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT);
				::SwapBuffers(s_hDC);
				glClear(GL_COLOR_BUFFER_BIT);
				::SwapBuffers(s_hDC);
				return GLDeviceContext::Ref(wglc);
			}
		}
	}

	DDVars *dd = DDVars::Get(m_activeDisplay->m_imp.m_vars);
	COut(C_Error) << "ERROR: unable to create openGL context (pfi=" << pfi << ") on '" << dd->dd.DeviceName << " - " << dd->dd.DeviceString << "'!" << std::endl;
	return GLDeviceContext::Ref();
}
#endif

StringTable::LangId NativeApp::RAD_IMPLEMENT_GET(systemLangId) {
	StringTable::LangId id = StringTable::LangId_EN;
	
	LANGID winId = GetUserDefaultUILanguage();
	
	switch (winId&0xff) {
		case LANG_CHINESE:
			id = StringTable::LangId_CH;
			break;
		case LANG_FRENCH:
			id = StringTable::LangId_FR;
			break;
		case LANG_GERMAN:
			id = StringTable::LangId_GR;
			break;
		case LANG_ITALIAN:
			id = StringTable::LangId_IT;
			break;
		case LANG_JAPANESE:
			id = StringTable::LangId_JP;
			break;
		case LANG_RUSSIAN:
		case LANG_UKRAINIAN:
			id = StringTable::LangId_RU;
			break;
		case LANG_SPANISH:
			id = StringTable::LangId_SP;
			break;
		default:
			break;
	}
	
	return id;
}

} // details

#if defined(RAD_OPT_PC_TOOLS)

void SetNativeWinInstance(HINSTANCE hInstance) {
	s_hInstance = hInstance;
}

#else

ATOM MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK MyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

bool ConfigureWindow(
	int width,
	int height,
	bool fullscreen,
	int screenWidth,
	int screenHeight
) {
	DWORD style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	int x = 0;
	int y = 0;

	if (!fullscreen) {
		style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

		RECT rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = (LONG)width;
		rect.bottom = (LONG)height;

		AdjustWindowRect(&rect,	style, false);

		width = (int)(rect.right - rect.left);
		height = (int)(rect.bottom - rect.top);

		// center the window
		x = (screenWidth - width) / 2;
		y = (screenHeight - height) / 2;
	}

	App *app = App::Get();

	if (!s_hWnd) {
		s_hWnd = CreateWindow(
			_T("rad_game_win"), 
			CStr(app->title.get()).ToWChar().c_str,
			style,
			x,
			y,
			width,
			height,
			0,
			0,
			s_hInstance,
			0
		);

		return s_hWnd != 0;
	}

	// adjust existing window style.
	SetWindowLongPtr(s_hWnd, GWL_STYLE, (LONG_PTR)style);
	SetWindowPos(s_hWnd, 0, x, y, width, height, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
	return true;
}

int NativeWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, int argc, const char **argv, int nCmdShow) {

//	ChangeDisplaySettings(0, 0);

	s_hInstance = hInstance;

	rt::Initialize();
	RAD_DEBUG_ONLY(file::EnforcePortablePaths(false));

	COut(C_Info) << "NativeWinMain..." << std::endl;
	COut(C_Info) << "echo command line: ";

	for (int i = 0; i < argc; ++i) {
		COut(C_Info) << argv[i] << " ";
	}

	COut(C_Info) << std::endl;

	App *app = App::Get(argc, argv);

	if (!app->PreInit()) {
		MessageBoxA(0, "Initialization failed! See log.txt for details.", "Error", MB_OK);
		return 1;
	}

	if (!app->DoLauncher())
		return 0;

	MyRegisterClass(hInstance);

	if (!app->InitWindow()) {
		app->ResetDisplayDevice();
		MessageBoxA(0, "Initialization failed! See log.txt for details.", "Error", MB_OK);
		return 1;
	}

	// after pre-init was called, we must have set a video mode and created a window.
	if (!s_hWnd) {
		COut(C_Error) << "Windowing system was not initialized (Developer note your custom PreInit method must call BindDisplayDevice() before returning!" << std::endl;
		MessageBoxA(0, "Windowing system was not initialized! See log.txt for details.", "Error", MB_OK);
		return 1;
	}

	if (!app->engine->sys->r->ctx.get()) {
		COut(C_Error) << "Rendering system was not initialized (Developer note your custom PreInit method must call set the rendering context before returning!" << std::endl;
		MessageBoxA(0, "Rendering system was not initialized! See log.txt for details.", "Error", MB_OK);
		return 1;
	}

	r::HRBackend rb = app->engine->sys->r.Cast<r::IRBackend>();

	if (!rb->VidBind()) {
		app->ResetDisplayDevice();
		MessageBoxA(0, "Failed to bind rendering device! See log.txt for details.", "Error", MB_OK);
		return 1;
	}

	if (!rb->CheckCaps()) {
		app->ResetDisplayDevice();
		MessageBoxA(0, "Unsupported video card detected! See log.txt for details.", "Error", MB_OK);
		return 1;
	}

	if (!(app->Initialize() && app->Run())) {
		app->ResetDisplayDevice();
		MessageBoxA(0, "Initialization failed! See log.txt for details.", "Error", MB_OK);
		return 1;
	}

	MSG m;
	while (!app->exit) {
		while (PeekMessage(&m, s_hWnd, 0, 0, PM_REMOVE)) {
			DispatchMessage(&m);
		}
		app->Tick();
		thread::Sleep();
	}

	app->ResetDisplayDevice();
	app->Finalize();

	ReleaseDC(s_hWnd, s_hDC);
	DestroyWindow(s_hWnd);

	App::DestroyInstance();
	
	rt::Finalize();
	return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEX clex;
	memset(&clex, 0, sizeof(WNDCLASSEX));

	clex.cbSize = sizeof(WNDCLASSEX);
	clex.style = CS_DBLCLKS | CS_OWNDC;
	clex.lpfnWndProc = MyWndProc;
	clex.hInstance = hInstance;
	clex.hbrBackground = (HBRUSH)NULL_BRUSH;
	clex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
	clex.hIconSm = LoadIcon(clex.hInstance, MAKEINTRESOURCE(IDI_SMALLICON));
	clex.hCursor = LoadCursor(NULL, IDC_ARROW);
	clex.lpszClassName = _T("rad_game_win");

	return RegisterClassEx(&clex);
}

LRESULT CALLBACK MyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	App *app = App::Get();

	if (app->exit) // on our way out.
		return DefWindowProc(hWnd, message, wParam, lParam);

	InputEvent e;
	
	switch (message) {
	case WM_ACTIVATEAPP:
		App::Get()->NotifyBackground(wParam ? true : false);
		if (wParam) {
			if (app->activeDisplay.get() && app->activeDisplay->curVidMode->fullscreen) {
				ShowWindow(s_hWnd, SW_MINIMIZE);
			}
		} else {
			if (app->activeDisplay.get() && app->activeDisplay->curVidMode->fullscreen) {
				ShowWindow(s_hWnd, SW_NORMAL);
			}
		}
		break;
	case WM_CLOSE:
		App::Get()->exit = true;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_LBUTTONDOWN:
		e.type = InputEvent::T_MouseDown;
		e.data[0] = LOWORD(lParam);
		e.data[1] = HIWORD(lParam);
		e.data[2] = kMouseButton_Left;
		e.time = xtime::ReadMilliseconds();
		app->PostInputEvent(e);
		break;
	case WM_LBUTTONUP:
		e.type = InputEvent::T_MouseUp;
		e.data[0] = LOWORD(lParam);
		e.data[1] = HIWORD(lParam);
		e.data[2] = kMouseButton_Left;
		e.time = xtime::ReadMilliseconds();
		app->PostInputEvent(e);
		break;
	case WM_MBUTTONDOWN:
		e.type = InputEvent::T_MouseDown;
		e.data[0] = LOWORD(lParam);
		e.data[1] = HIWORD(lParam);
		e.data[2] = kMouseButton_Middle;
		e.time = xtime::ReadMilliseconds();
		app->PostInputEvent(e);
		break;
	case WM_MBUTTONUP:
		e.type = InputEvent::T_MouseUp;
		e.data[0] = LOWORD(lParam);
		e.data[1] = HIWORD(lParam);
		e.data[2] = kMouseButton_Middle;
		e.time = xtime::ReadMilliseconds();
		app->PostInputEvent(e);
		break;
	case WM_RBUTTONDOWN:
		e.type = InputEvent::T_MouseDown;
		e.data[0] = LOWORD(lParam);
		e.data[1] = HIWORD(lParam);
		e.data[2] = kMouseButton_Right;
		e.time = xtime::ReadMilliseconds();
		app->PostInputEvent(e);
		break;
	case WM_RBUTTONUP:
		e.type = InputEvent::T_MouseUp;
		e.data[0] = LOWORD(lParam);
		e.data[1] = HIWORD(lParam);
		e.data[2] = kMouseButton_Right;
		e.time = xtime::ReadMilliseconds();
		app->PostInputEvent(e);
		break;
	case WM_MOUSEMOVE:
		e.type = InputEvent::T_MouseMove;
		e.data[0] = LOWORD(lParam);
		e.data[1] = HIWORD(lParam);
		e.data[2] = 0;

		if (wParam & MK_LBUTTON)
			e.data[2] |= kMouseButton_Left;
		if (wParam & MK_MBUTTON)
			e.data[2] |= kMouseButton_Middle;
		if (wParam & MK_RBUTTON)
			e.data[2] |= kMouseButton_Right;

		e.time = xtime::ReadMilliseconds();
		app->PostInputEvent(e);
		break;
	case WM_MOUSEWHEEL:
		e.type = InputEvent::T_MouseWheel;
		e.data[0] = LOWORD(lParam);
		e.data[1] = HIWORD(lParam);
		e.data[2] = HIWORD(wParam); // TODO scaling?
		e.time = xtime::ReadMilliseconds();
		app->PostInputEvent(e);
		break;
	case WM_KEYDOWN:
		e.type = InputEvent::T_KeyDown;
		e.repeat = LOWORD(lParam) > 1;
		e.data[0] = TranslateVKey((int)wParam, (int)lParam);
		e.time = xtime::ReadMilliseconds();
		app->PostInputEvent(e);
		break;
	case WM_KEYUP:
		e.type = InputEvent::T_KeyUp;
		e.repeat = false;
		e.data[0] = TranslateVKey((int)wParam, (int)lParam);
		e.time = xtime::ReadMilliseconds();
		app->PostInputEvent(e);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

#endif
