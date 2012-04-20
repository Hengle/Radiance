// WinGLWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <Runtime/Thread/Thread.h>
#include <Runtime/Thread/Locks.h>

class GLWindow;

namespace details {

class GLWindow
{
public:
	GLWindow();
	~GLWindow();

	void Redraw();
	bool Open(::GLWindow* msgWindow, const wchar_t* title, int x, int y, int width, int height, bool border);
	void Close();
	bool IsOpen();
	void WaitForClose();
	GLState& BeginFrame();
	void EndFrame();
	void CaptureMouse();
	void ReleaseMouse();
	int Width();
	int Height();
	void SetDefaultViewport();

	void OnMouseDown(int mButton, int keyState, int x, int y);
	void OnMouseUp(int mButton, int keyState, int x, int y);
	void OnMouseMove(int keyState, int x, int y);
	void OnMouseDoubleClick(int mButton, int keyState, int x, int y);
	void OnKeyDown(int keyState, int key, int x, int y);
	void OnKeyUp(int keyState, int key, int x, int y);
	void OnChar(int keyState, int key, int x, int y, bool repeat);
	void OnPaint();
	void FocusChange(bool focus);
	void Idle();
	void PostCloseMsg();

private:

	class MessagePump : public thread::Thread
	{
	public:
		MessagePump();
		~MessagePump();

		::GLWindow* msgWindow;
		GLWindow* window;
		volatile bool active;
		volatile bool quit;
		
		struct
		{
			const wchar_t* title;
			int x, y, width, height;
			bool border;
			bool complete;
			bool success;
		} createParms;

	protected:

		virtual int ThreadProc();
	};

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

	int m_width, m_height;
	volatile HDC m_hDC;
	volatile HGLRC m_hGLRC;
	volatile HWND m_hWND;
	MessagePump m_pump;
	boost::mutex m_cs;
	GLState m_state;

	void InitializeGL();

	void BindGL();
	void UnbindGL();
};

} // details
