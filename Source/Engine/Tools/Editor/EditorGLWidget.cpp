// EditorGLWidget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorMainWindow.h"
#include "EditorGLWidget.h"
#include "EditorUtils.h"
#include "../../Renderer/PC/RBackend.h"
#include "../../Renderer/GL/GLTable.h"

namespace tools {
namespace editor {

GLWidget::GLWidget(QWidget *parent, Qt::WindowFlags f) :
QGLWidget(parent, MainWindow::Get()->glBase
#if defined(RAD_OPT_WIN)
	, f
#endif
)
{
#if !defined(RAD_OPT_WIN)
	setWindowFlags(f);
#endif
	RAD_VERIFY(isSharing());
}

GLWidget::~GLWidget()
{
	bindGL(true);
	m_ctx.Close();
	unbindGL();
}

void GLWidget::bindGL(bool makeCurrent)
{
	if (makeCurrent)
		this->makeCurrent();

	if (!MainWindow::Get())
		return;

	if (!m_ctx)
		m_ctx = Renderer().Cast<r::IRBackend>()->CreateContext(NativeDeviceContext::Ref());

	Renderer()->ctx = m_ctx;
}

void GLWidget::unbindGL()
{
	if (MainWindow::Get())
		Renderer()->ctx = r::HContext();
	doneCurrent();
}

void GLWidget::paintGL()
{
	bindGL();
	r::gl.Color4f(1, 1, 1, 1, true); // Qt can mess with our Color state behind our back.
	renderGL();
}

void GLWidget::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	emit OnResizeGL(*this, width, height);
}

void GLWidget::renderGL()
{
	emit OnRenderGL(*this);
}

void GLWidget::initializeGL()
{
	CLEAR_GL_ERRORS();
	m_ctx.Close();
	bindGL();
	glViewport(0, 0, width(), height());
	emit OnInitializeGL(*this);
}

void GLWidget::wheelEvent(QWheelEvent *e)
{
	emit OnWheelEvent(e);
}

void GLWidget::mouseMoveEvent(QMouseEvent *e)
{
	emit OnMouseMoveEvent(e);
}

void GLWidget::mousePressEvent(QMouseEvent *e)
{
	emit OnMousePressEvent(e);
}

void GLWidget::mouseReleaseEvent(QMouseEvent *e)
{
	emit OnMouseReleaseEvent(e);
}

void GLWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
	emit OnMouseDoubleClickEvent(e);
}

void GLWidget::keyPressEvent(QKeyEvent *e)
{
	emit OnKeyPressEvent(e);
}

void GLWidget::keyReleaseEvent(QKeyEvent *e)
{
	emit OnKeyReleaseEvent(e);
}

} // editor
} // tools

#include "moc_EditorGLWidget.cc"
