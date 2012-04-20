// GLWindow.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

inline GLWindow::GLWindow() {}
inline GLWindow::~GLWindow() {}
inline bool GLWindow::Open(const wchar_t* title, int x, int y, int width, int height, bool border) { return m_imp.Open(this, title, x, y, width, height, border); }
inline void GLWindow::Redraw() { m_imp.Redraw(); }
inline void GLWindow::Close() { m_imp.Close(); } 
inline void GLWindow::WaitForClose() { m_imp.WaitForClose(); }
inline bool GLWindow::IsOpen() { m_imp.IsOpen(); }
inline GLState& GLWindow::BeginFrame() { return m_imp.BeginFrame(); } 
inline void GLWindow::EndFrame() { m_imp.EndFrame(); } 
inline void GLWindow::CaptureMouse() { m_imp.CaptureMouse(); }
inline void GLWindow::ReleaseMouse() { m_imp.ReleaseMouse(); }
inline int GLWindow::Width() { return m_imp.Width(); }
inline int GLWindow::Height() { return m_imp.Height(); }
inline void GLWindow::SetDefaultViewport() { return m_imp.SetDefaultViewport(); }

inline void GLWindow::OnMouseDown(int mButton, int keyState, int x, int y) { m_imp.OnMouseDown(mButton, keyState, x, y); }
inline void GLWindow::OnMouseUp(int mButton, int keyState, int x, int y) { m_imp.OnMouseUp(mButton, keyState, x, y); }
inline void GLWindow::OnMouseMove(int keyState, int x, int y) { m_imp.OnMouseMove(keyState, x, y); }
inline void GLWindow::OnMouseDoubleClick(int mButton, int keyState, int x, int y) { m_imp.OnMouseDoubleClick(mButton, keyState, x, y); }
inline void GLWindow::OnKeyDown(int keyState, int key, int x, int y) { m_imp.OnKeyDown(keyState, key, x, y); }
inline void GLWindow::OnKeyUp(int keyState, int key, int x, int y) { m_imp.OnKeyUp(keyState, key, x, y); }
inline void GLWindow::OnChar(int keyState, int key, int x, int y, bool repeat) { m_imp.OnChar(keyState, key, x, y, repeat); }
inline void GLWindow::OnPaint() { m_imp.OnPaint(); }
inline void GLWindow::FocusChange(bool focus) { m_imp.FocusChange(focus); }
inline void GLWindow::Idle() { m_imp.Idle(); }
inline void GLWindow::PostCloseMsg() { m_imp.PostCloseMsg(); }

