// GLNavWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "GLWindow.h"
#include "GLCamera.h"

class GLNavWindow : public GLWindow
{
public:
	GLNavWindow();
	virtual ~GLNavWindow();

	class PaintHandler
	{
	public:
		PaintHandler();
		virtual ~PaintHandler();

		virtual void OnPaint(GLNavWindow& window) = 0;
	};

	PaintHandler* SetPaintHandler(PaintHandler* ph);

	void OnMouseDown(int mButton, int keyState, int x, int y);
	void OnMouseUp(int mButton, int keyState, int x, int y);
	void OnMouseMove(int keyState, int x, int y);
	void OnChar(int keyState, int key, int x, int y, bool repeat);
	void OnPaint();

	GLCamera& Camera();
	const GLCamera& Camera() const;

	static void PopupWindow(const wchar_t *title, int x, int y, int w, int h, bool border, PaintHandler* ph);

private:

	enum MotionState
	{
		None,
		Zooming,
		Moving,
		Rotating
	};

	struct MouseState
	{
		MouseState();

		MotionState mstate;
		int x, y, keys;

		void FindMotionState();
	};

	GLCamera m_cam;
	MouseState m_mState;
	PaintHandler* m_ph;
};
