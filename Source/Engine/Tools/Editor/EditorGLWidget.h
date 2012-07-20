// EditorGLWidget.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include "../../Renderer/Renderer.h"
#define __glext_h_
#include <QtOpenGL/QGLContext>
#include <QtOpenGL/QGLWidget>
#undef __glext_h_
#include <Runtime/PushPack.h>

class QWheelEvent;
class QMouseEvent;
class QKeyEvent;

namespace tools {
namespace editor {

class RADENG_CLASS GLWidget : public QGLWidget
{
	Q_OBJECT
public:
	GLWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	virtual ~GLWidget();

	void bindGL(bool makeCurrent = false);
	void unbindGL();

signals:

	void OnRenderGL(GLWidget &src);
	void OnResizeGL(GLWidget &src, int width, int height);
	void OnInitializeGL(GLWidget &src);
	void OnWheelEvent(QWheelEvent *e);
	void OnMouseMoveEvent(QMouseEvent *e);
	void OnMousePressEvent(QMouseEvent *e);
	void OnMouseReleaseEvent(QMouseEvent *e);
	void OnMouseDoubleClickEvent(QMouseEvent *e);
	void OnKeyPressEvent(QKeyEvent *e);
	void OnKeyReleaseEvent(QKeyEvent *e);

protected:
	virtual void renderGL();
	virtual void resizeGL(int width, int height);
	virtual void initializeGL();
	virtual void wheelEvent(QWheelEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void mouseDoubleClickEvent(QMouseEvent *e);
	virtual void keyPressEvent(QKeyEvent *e);
	virtual void keyReleaseEvent(QKeyEvent *e);

private:
	virtual void paintGL();
	r::HContext m_ctx;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
