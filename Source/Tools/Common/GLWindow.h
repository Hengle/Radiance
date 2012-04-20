// GLWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "RGL.h"

#if defined(RAD_OPT_WIN)
	#include "win/WinGLWindow.h"
#else
	#error "GLWindow: unsupported platform!"
#endif

enum
{
	RAD_FLAG(KeyState_LButton),
	RAD_FLAG(KeyState_RButton),
	RAD_FLAG(KeyState_MButton),
	RAD_FLAG(KeyState_Ctrl),
	RAD_FLAG(KeyState_Alt),
	RAD_FLAG(KeyState_Shift),

	Key_F1 = 0,
	Key_F2,
	Key_F3,
	Key_F4,
	Key_F5,
	Key_F6,
	Key_F7,
	Key_F8,
	Key_F9,
	Key_F10,
	Key_F11,
	Key_F12,
	Key_Left,
	Key_Right,
	Key_Up,
	Key_Down,
	Key_PageUp,
	Key_PageDown,
	Key_Home,
	Key_End,
	Key_Insert,
	Key_Delete
};

class GLWindow
{
public:
	GLWindow();
	virtual ~GLWindow();

	bool Open(const wchar_t* title, int x, int y, int width, int height, bool border);
	void Close();
	void PostCloseMsg(); // must be called instead of Close() when called from an event handler.
	void WaitForClose();
	bool IsOpen();
	void CaptureMouse();
	void ReleaseMouse();
	int Width();
	int Height();
	void SetDefaultViewport();
	void Redraw();

	GLState& BeginFrame();
	void EndFrame(); // does a swap buffers.

	virtual void OnMouseDown(int mButton, int keyState, int x, int y);
	virtual void OnMouseUp(int mButton, int keyState, int x, int y);
	virtual void OnMouseMove(int keyState, int x, int y);
	virtual void OnMouseDoubleClick(int mButton, int keyState, int x, int y);
	virtual void OnKeyDown(int keyState, int key, int x, int y);
	virtual void OnKeyUp(int keyState, int key, int x, int y);
	virtual void OnChar(int keyState, int key, int x, int y, bool repeat);
	virtual void OnPaint();
	virtual void FocusChange(bool focus);
	virtual void Idle(); // idle is called all the time during message pumping.

private:

	details::GLWindow m_imp;
};

#include "GLWindow.inl"