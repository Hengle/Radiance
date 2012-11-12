// EditorPIEWidget.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include "EditorGLWidget.h"
#include "../../Game/Game.h"
#include <Runtime/PushPack.h>

class QWidget;
class QKeyEvent;

namespace tools {
namespace editor {

class RADENG_CLASS PIEWidget : public GLWidget, public IToolsCallbacks {
	Q_OBJECT
public:

	PIEWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	virtual ~PIEWidget();

	void RunMap(int id);

	// IToolsCallback
	virtual void SwapBuffers() {
		swapBuffers();
	}

protected:

	virtual void renderGL();
	virtual void wheelEvent(QWheelEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void mouseDoubleClickEvent(QMouseEvent *e);
	virtual void keyPressEvent(QKeyEvent *event);
	virtual void keyReleaseEvent(QKeyEvent *event);

private slots:

	void OnTick(float dt);

private:

	void CenterCursor();

	float m_dt;
	Game::Ref m_game;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
