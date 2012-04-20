// WinGLWindow.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../GLWindow.h"
#include <gl/glu.h>

void CheckGLErrors()
{
#if defined(RAD_OPT_DEBUG)
	//
	// did things go as planned?
	//
	bool found=false;
	GLenum err = glGetError();
	while( err != GL_NO_ERROR )
	{
		const char* string = (const char*)gluErrorString(err);
		if (!found) // only dump the first one.
		{
			OutputDebugStringA("GLERROR: ");
			OutputDebugStringA(string);
			OutputDebugStringA("\n");
			found = true;
			break;
		}
		found = true;
		err = glGetError();
	}
	
	if( found )
	{
		__asm { int 3 };
	}
#endif
}

PFNGLISRENDERBUFFERPROC glIsRenderbuffer = 0;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = 0;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = 0;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = 0;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = 0;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv = 0;
PFNGLISFRAMEBUFFERPROC glIsFramebuffer = 0;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = 0;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = 0;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = 0;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = 0;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = 0;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = 0;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv = 0;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = 0;

namespace
{
	PFNGLACTIVETEXTUREARBPROC SetActiveTextureARB;
	PFNGLCLIENTACTIVETEXTUREARBPROC SetClientActiveTextureARB;
}

void glActiveTexture(GLenum texture)
{
	RAD_ASSERT(SetActiveTextureARB);
	SetActiveTextureARB(texture);
}

void glClientActiveTexture(GLenum texture)
{
	RAD_ASSERT(SetClientActiveTextureARB);
	SetClientActiveTextureARB(texture);
}

namespace details {

namespace
{
	bool s_extLoad = false;
	
	void LoadExtensions()
	{
		if (s_extLoad) return;
		SetActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
		SetClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)wglGetProcAddress("glClientActiveTextureARB");
		glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)wglGetProcAddress("glIsRenderbuffer");
		glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
		glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress("glDeleteRenderbuffers");
		glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
		glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorage");
		glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetRenderbufferParameteriv");
		glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)wglGetProcAddress("glIsFramebuffer");
		glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
		glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
		glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
		glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
		glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbuffer");
		glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
		glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)wglGetProcAddress("glGetFramebufferAttachmentParameteriv");
		glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
		s_extLoad = true;
	}
}

GLWindow::GLWindow() : m_hDC(0), m_hGLRC(0), m_hWND(0), m_width(0), m_height(0)
{
}

GLWindow::~GLWindow()
{
	Close();
}

GLState&  GLWindow::BeginFrame()
{
	BindGL();
	return m_state;
}

void GLWindow::EndFrame()
{
	RAD_ASSERT(m_hDC);
	SwapBuffers(m_hDC);
	UnbindGL();
}

void GLWindow::BindGL()
{
	RAD_ASSERT(m_hGLRC);
	RAD_ASSERT(!m_hDC);
	RAD_ASSERT(m_hWND);

	m_cs.lock();
	m_hDC = GetDC(m_hWND);
	wglMakeCurrent(m_hDC, m_hGLRC);
}

void GLWindow::UnbindGL()
{
	RAD_ASSERT(m_hGLRC);
	RAD_ASSERT(m_hDC);
	RAD_ASSERT(m_hWND);

	wglMakeCurrent(m_hDC, 0);
	ReleaseDC(m_hWND, m_hDC);
	m_hDC = 0;
	m_cs.unlock();
}

void GLWindow::Redraw()
{
	RAD_ASSERT(m_hWND);
	RedrawWindow(m_hWND, 0, 0, RDW_INVALIDATE);
}

void GLWindow::InitializeGL()
{
	BindGL();
	LoadExtensions();

	CHECK_GL_ERRORS();

	glDisable( GL_LIGHTING );
	glShadeModel( GL_SMOOTH );
	glEnable( GL_POINT_SMOOTH );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	m_state.SetState(DT_Disable|DWM_Enable|CFM_None|CWM_All|NoArrays, BM_Off);
	m_state.DisableAllTMUs();
	m_state.Commit();

	CHECK_GL_ERRORS();

	UnbindGL();
}

bool GLWindow::Open(::GLWindow* ownerWindow, const wchar_t* title, int x, int y, int width, int height, bool border)
{
	RAD_ASSERT(!m_hWND);
	RAD_ASSERT(!m_hGLRC);
	RAD_ASSERT(!m_hDC);
	
	m_pump.createParms.title = title;
	m_pump.createParms.x = x;
	m_pump.createParms.y = y;
	m_pump.createParms.width = width;
	m_pump.createParms.height = height;
	m_pump.createParms.border = border;
	m_pump.createParms.complete = false;
	m_pump.createParms.success = false;

	m_pump.window = this;
	m_pump.msgWindow = ownerWindow, 
	m_pump.active = false;
	m_pump.Run();

	while (!m_pump.createParms.complete)
	{
		thread::Sleep(1);
	}

	return m_pump.createParms.success;

}

void GLWindow::PostCloseMsg()
{
	PostMessage(m_hWND, WM_CLOSE, 0, 0);
}

void GLWindow::Close()
{
	m_cs.lock();
	if (m_hWND)
	{
		PostCloseMsg();
		m_cs.unlock();

		while (m_pump.active)
		{
			thread::Sleep(100);
		}
	}
	else
	{
		m_cs.unlock();
	}

	m_width = m_height = 0;
}

bool GLWindow::IsOpen()
{
	return m_pump.active;
}

void GLWindow::WaitForClose()
{
	while (IsOpen())
	{
		thread::Sleep(100);
	}
}

void GLWindow::OnMouseDown(int mButton, int keyState, int x, int y)
{
}

void GLWindow::OnMouseUp(int mButton, int keyState, int x, int y)
{
}

void GLWindow::OnMouseMove(int keyState, int x, int y)
{
}

void GLWindow::OnMouseDoubleClick(int mButton, int keyState, int x, int y)
{
}

void GLWindow::OnKeyDown(int keyState, int key, int x, int y)
{
}

void GLWindow::OnKeyUp(int keyState, int key, int x, int y)
{
}

void GLWindow::OnChar(int keyState, int key, int x, int y, bool repeat)
{
}

void GLWindow::OnPaint()
{
}

void GLWindow::Idle()
{
}

void GLWindow::CaptureMouse()
{
	RAD_ASSERT(m_hWND);
	SetCapture(m_hWND);
}

void GLWindow::ReleaseMouse()
{
	ReleaseCapture();
}

int GLWindow::Width()
{
	return m_width;
}

int GLWindow::Height()
{
	return m_height;
}

void GLWindow::SetDefaultViewport()
{
	glViewport(0, 0, Width(), Height());
}

void GLWindow::FocusChange(bool focus)
{
}

struct KEYSTATE
{
	int key;
	int keyState;
};

static int KeyState() // helper
{
	int keyState = 0;
	if (GetKeyState(VK_SHIFT) < 0) keyState |= KeyState_Shift;
	if (GetKeyState(VK_CONTROL) < 0) keyState |= KeyState_Ctrl;
	if (GetKeyState(VK_MENU) < 0) keyState |= KeyState_Alt;
	return keyState;
}

static int KeyStateFromWparam(WPARAM wp)
{
	int keyState = 0;
	if (wp & MK_SHIFT) keyState |= KeyState_Shift;
	if (wp & MK_CONTROL) keyState |= KeyState_Ctrl;
	if (wp & MK_LBUTTON) keyState |= KeyState_LButton;
	if (wp & MK_RBUTTON) keyState |= KeyState_RButton;
	if (wp & MK_MBUTTON) keyState |= KeyState_MButton;
	if (GetKeyState(VK_MENU) < 0) keyState |= KeyState_Alt;
	return keyState;
}

static int TranslateKey(WPARAM wParam, int keyState)
{
	int key = 0;

	switch (wParam)
	{
	case VK_F1: key = Key_F1; break;
	case VK_F2: key = Key_F2; break;
	case VK_F3: key = Key_F3; break;
	case VK_F4: key = Key_F4; break;
	case VK_F5: key = Key_F5; break;
	case VK_F6: key = Key_F6; break;
	case VK_F7: key = Key_F7; break;
	case VK_F8: key = Key_F8; break;
	case VK_F9: key = Key_F9; break;
	case VK_F10: key = Key_F10; break;
	case VK_F11: key = Key_F11; break;
	case VK_F12: key = Key_F12; break;
	case VK_LEFT: key = Key_Left; break;
	case VK_RIGHT: key = Key_Right; break;
	case VK_DOWN: key = Key_Down; break;
	case VK_UP: key = Key_Up; break;
	case VK_PRIOR: key = Key_PageUp; break;
	case VK_NEXT: key = Key_PageDown; break;
	case VK_HOME: key = Key_Home; break;
	case VK_END: key = Key_End; break;
	case VK_INSERT: key = Key_Insert; break;
	case VK_DELETE: key = Key_Delete; break;
	}

	if (!key)
	{
		key = MapVirtualKey((UINT)wParam, 2);
		if (key)
		{
			if (!(keyState&KeyState_Shift)) key = _tolower(key);
		}
	}

	return key;
}

static KEYSTATE TranslateKeyState(WPARAM wParam)
{
	KEYSTATE ks;

	ks.keyState = KeyState();
	ks.key = TranslateKey(wParam, ks.keyState);
	return ks;
}

static POINT WindowMousePos(HWND hWnd)
{
	POINT p;
	RAD_ASSERT(hWnd);
	GetCursorPos(&p);
	ScreenToClient(hWnd, &p);
	return p;
}

LRESULT CALLBACK GLWindow::WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT r = 0;

#pragma warning (disable:4312)
	
	// NOTE: the warning generated here is retarded and can be ignored. According to the the win docs, WindowLongPtr versions are pointer safe
	// for all versions of windows (32/64 bit), so I'm not worried about it.
	GLWindow::MessagePump* pump = (GLWindow::MessagePump*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	::GLWindow* window = 0;

#pragma warning (default:4312)

	if (pump) window = pump->msgWindow;

	switch (nMsg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
	break;
	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_KEYDOWN:
		{
			RAD_ASSERT(window);
			KEYSTATE ks = TranslateKeyState(wParam);
			if (ks.key)
			{
				POINT p = WindowMousePos(hWnd);
				if ((nMsg==WM_SYSKEYUP)||(nMsg==WM_KEYUP))
					window->OnKeyUp(ks.keyState, ks.key, p.x, p.y);
				else
					window->OnKeyDown(ks.keyState, ks.key, p.x, p.y);
			}
		}
	break;
	case WM_SYSCHAR:
	case WM_CHAR:
		{
			RAD_ASSERT(window);
			POINT p = WindowMousePos(hWnd);
			window->OnChar(KeyState(), (int)wParam, p.x, p.y, (lParam&(1<<30)) ? true : false);
		}
	break;
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
		{
			RAD_ASSERT(window);
			int keyState = KeyStateFromWparam(wParam);
			int mButton = 0;

			switch (nMsg)
			{
			case WM_LBUTTONDBLCLK: mButton |= KeyState_LButton; break;
			case WM_RBUTTONDBLCLK: mButton |= KeyState_RButton; break;
			case WM_MBUTTONDBLCLK: mButton |= KeyState_MButton; break;
			}

			int x = LOWORD(lParam);
			int y = HIWORD(lParam);

			// 0-2^16 --> +/- 2^15.
			if (x&(1<<15)) x -= (1<<16);
			if (y&(1<<15)) y -= (1<<16);

			window->OnMouseDoubleClick(mButton, keyState, x, y);
		}
	break;
	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDOWN:
		{
			RAD_ASSERT(window);
			int keyState = KeyStateFromWparam(wParam);
			int mButton = 0;

			switch (nMsg)
			{
			case WM_LBUTTONUP: case WM_LBUTTONDOWN: mButton |= KeyState_LButton; break;
			case WM_RBUTTONUP: case WM_RBUTTONDOWN: mButton |= KeyState_RButton; break;
			case WM_MBUTTONUP: case WM_MBUTTONDOWN: mButton |= KeyState_MButton; break;
			}

			int x = LOWORD(lParam);
			int y = HIWORD(lParam);

			// 0-2^16 --> +/- 2^15.
			if (x&(1<<15)) x -= (1<<16);
			if (y&(1<<15)) y -= (1<<16);

			switch (nMsg)
			{
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
				window->OnMouseUp(mButton, keyState, x, y);
			break;
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
				window->OnMouseDown(mButton, keyState, x, y);
			break;
			}
		}
	break;
	case WM_MOUSEMOVE:
		{
			RAD_ASSERT(window);
			int keyState = KeyState();
			
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);

			// 0-2^16 --> +/- 2^15.
			if (x&(1<<15)) x -= (1<<16);
			if (y&(1<<15)) y -= (1<<16);

			window->OnMouseMove(keyState, x, y);
		}
	break;
	case WM_PAINT:
		{
			RAD_ASSERT(window);
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			window->OnPaint();
		}
	break;
	case WM_DESTROY:
		RAD_ASSERT(pump&&window);
		pump->window->m_cs.lock();
		pump->window->m_hWND = 0;
		RAD_ASSERT_MSG(pump->window->m_hDC==0, "WM_DESTROY during Begin/End Frame!");
		wglDeleteContext(pump->window->m_hGLRC);
		pump->window->m_hGLRC = 0;
		pump->window->m_cs.unlock();
		pump->quit = true;
	break;
	default:
		r = DefWindowProc(hWnd, nMsg, wParam, lParam);
	break;
	}

	return r;
}

GLWindow::MessagePump::MessagePump() : active(false), window(0), msgWindow(0)
{
}

GLWindow::MessagePump::~MessagePump()
{
}

int GLWindow::MessagePump::ThreadProc()
{
	RAD_ASSERT(window && !active);
	active = true;
	quit = false;

	WNDCLASS wc;

	memset(&wc, 0, sizeof(WNDCLASS));
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = L"RAD_GLTOOLWINDOW";
	wc.lpszMenuName = 0;
	wc.style = CS_OWNDC;

	RegisterClass(&wc);

	HWND hWND;
	HGLRC hGLRC;

	hWND = CreateWindow(L"RAD_GLTOOLWINDOW", createParms.title, 
		(createParms.border ? (WS_OVERLAPPED|WS_SYSMENU) : WS_POPUP) | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
		(createParms.x>=0) ? createParms.x : CW_USEDEFAULT,
		(createParms.y>=0) ? createParms.y : CW_USEDEFAULT,
		createParms.width, createParms.height, 0, 0, GetModuleHandle(0), 0);

	if (hWND)
	{
		ShowWindow(hWND, SW_SHOW);

		//SetWindowText(hWND, L"WHAT THE HELL?");
#pragma warning (disable:4244)

		// NOTE: the warning generated here is retarded and can be ignored. According to the the win docs, WindowLongPtr versions are pointer safe
		// for all versions of windows (32/64 bit), so I'm not worried about it.
		SetWindowLongPtr(hWND, GWLP_USERDATA, (LONG_PTR)this);
		SetWindowPos(hWND, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // update window data from SetWindowLongPtr().

#pragma warning (default:4244)

		// select a pixel format for the window.
		PIXELFORMATDESCRIPTOR pfd;
		
		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cDepthBits = 32;
		pfd.cStencilBits = 8;
		pfd.cAlphaBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;

		HDC hdc = GetDC(hWND);

		int pfn;
		pfn = ChoosePixelFormat(hdc, &pfd);
		if (pfn)
		{
			if (SetPixelFormat(hdc, pfn, &pfd))
			{
				hGLRC = wglCreateContext(hdc);
				if (!wglMakeCurrent(hdc, hGLRC))
				{
					wglDeleteContext(hGLRC);
					hGLRC = 0;
				}
				else
				{
					wglMakeCurrent(0, 0);
				}
			}
		}

		ReleaseDC(hWND, hdc);

		if (!hGLRC)
		{
			DestroyWindow(hWND);
			hWND = 0;
		}
	}

	window->m_hWND = hWND;
	window->m_hGLRC = hGLRC;
	window->m_hDC = 0;

	createParms.success = hWND != 0;
	
	if (createParms.success)
	{
		window->InitializeGL();
		createParms.complete = true;

		MSG msg;
		while (!quit)
		{
			//if (GetMessage(&msg, hWND, 0, 0))
			if (PeekMessage(&msg, hWND, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				if (msgWindow) msgWindow->Idle();
				thread::Sleep(1);
			}
		}
	}
	else
	{
		createParms.complete = true;
	}

	active = false;
	return 0;
}

} // details
