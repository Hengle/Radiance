// GLNavWindow.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "GLNavWindow.h"
#include "Texture.h"

GLNavWindow::PaintHandler::PaintHandler()
{
}

GLNavWindow::PaintHandler::~PaintHandler()
{
}

GLNavWindow::GLNavWindow() : m_ph(0)
{
	
}

GLNavWindow::~GLNavWindow()
{
}

GLNavWindow::PaintHandler* GLNavWindow::SetPaintHandler(PaintHandler* ph)
{
	PaintHandler* old = m_ph;
	m_ph = ph;
	return old;
}

GLNavWindow::MouseState::MouseState()
{
	mstate = None;
	keys = 0;
	x = 0;
	y = 0;
}

void GLNavWindow::MouseState::FindMotionState()
{
	if (keys & KeyState_MButton)
	{
		mstate = Moving;
	}
	else if (keys & KeyState_RButton)
	{
		mstate = Rotating;
	}
	else
	{
		mstate = None;
	}
}

void GLNavWindow::OnMouseDown(int mButton, int keyState, int x, int y)
{
	m_mState.keys = keyState;
	m_mState.x = x;
	m_mState.y = y;
	m_mState.FindMotionState();

	GLWindow::OnMouseDown(mButton, keyState, x, y);
}

void GLNavWindow::OnMouseUp(int mButton, int keyState, int x, int y)
{
	m_mState.mstate = None;
	GLWindow::OnMouseUp(mButton, keyState, x, y);
}

const float MouseMoveSpeed = 0.65f;
const float MouseRotateSpeed = 1.0f / 100.0f;

void GLNavWindow::OnMouseMove(int keyState, int x, int y)
{
	GLCamera& cam = Camera();

	switch (m_mState.mstate)
	{
	case Moving:
		{
			GLVec3 lft = cam.Left();
			GLVec3 up  = cam.Up();
			GLVec3 pos = cam.Pos();

			pos += ((float)(m_mState.x - x) * lft * MouseMoveSpeed) + ((float)(y - m_mState.y) * up * MouseMoveSpeed);
			cam.SetPos(pos);
			Redraw();
		}
	break;
	case Rotating:
		{
			cam.Rotate(GLQuat(GLZAxis, (x - m_mState.x) * MouseRotateSpeed));
			cam.Rotate(GLQuat(cam.Left(), (y - m_mState.y) * MouseRotateSpeed));
			Redraw();
		}
	break;
	}

	m_mState.x = x;
	m_mState.y = y;

	GLWindow::OnMouseMove(keyState, x, y);
}

const float KeyRotateSpeed = math::Constants<float>::PI() / 50.0f;
const float KeyMoveSpeed = 6.0f;
const float KeyMoveSlowSpeed = 0.6f;

void GLNavWindow::OnChar(int keyState, int key, int x, int y, bool repeat)
{
	GLCamera& cam = Camera();
	float move;

	switch (key)
	{
	case 'a':
	case 'A':
		move = (key=='a') ? KeyMoveSpeed : KeyMoveSlowSpeed;
		//cam.Rotate(GLQuat(-GLZAxis, KeyRotateSpeed));
		cam.SetPos(cam.Pos() + (-cam.Left() * move));
		Redraw();
	break;
	case 'd':
	case 'D':
		move = (key=='d') ? KeyMoveSpeed : KeyMoveSlowSpeed;
		//cam.Rotate(GLQuat(GLZAxis, KeyRotateSpeed));
		cam.SetPos(cam.Pos() + (cam.Left() * move));
		Redraw();
	break;
	case 'w':
	case 'W':
		{
			move = (key=='w') ? KeyMoveSpeed : KeyMoveSlowSpeed;
			GLVec3 frw = cam.Forward();
			//frw[2] = 0.0f;
			//frw.Normalize();
			cam.SetPos(cam.Pos() + (-frw * move));
			Redraw();
		}
	break;
	case 's':
	case 'S':
		{
			move = (key=='s') ? KeyMoveSpeed : KeyMoveSlowSpeed;
			GLVec3 frw = cam.Forward();
			//frw[2] = 0.0f;
			//frw.Normalize();
			cam.SetPos(cam.Pos() + (frw * move));
			Redraw();
		}
	break;
	case 'c':
		{
			PostCloseMsg();
		}
	break;
	}
}

GLCamera& GLNavWindow::Camera()
{
	return m_cam;
}

const GLCamera& GLNavWindow::Camera() const
{
	return m_cam;
}

//#define TEST_PAINT

#if defined(TEST_PAINT)
Texture s_testTex("test2.tga", true);
#endif

void GLNavWindow::OnPaint()
{
	if (m_ph)
	{
		m_ph->OnPaint(*this);
	}
	else
	{

#if !defined(TEST_PAINT)

		GLWindow::OnPaint();

#else

		GLState& ctx = BeginFrame();
		s_testTex.Load();
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		
		const GLCamera& cam = Camera();
		cam.Bind(ctx);

		ctx.SetState(DT_Disable|CFM_None|CWM_All|NoArrays, BMS_SrcAlpha|BMD_Zero);

		ctx.DisableAllTMUs();
		ctx.SetTMUState(0, TEM_Replace|TCA_Off);
		ctx.SetTMUTexture(0, s_testTex.GLTex());
		ctx.Commit();
		
		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(15.0f, 10.0f, 10.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(15.0f, -10.0f, 10.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(15.0f, -10.0f, -10.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(15.0f, 10.0f, -10.0f);
		glEnd();
		EndFrame();

#endif
	}
}

void GLNavWindow::PopupWindow(const wchar_t *title, int x, int y, int w, int h, bool border, PaintHandler* ph)
{
	GLNavWindow win;
	
	if (win.Open(title, x, y, w, h, border))
	{
		//win.Camera().SetPos(GLVec3(0, 0, 300));
		win.SetPaintHandler(ph);
		win.Redraw();
		win.WaitForClose();
	}
}
