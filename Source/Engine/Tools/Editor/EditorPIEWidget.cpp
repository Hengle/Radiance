// EditorPIEWidget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorMainWindow.h"
#include "EditorPIEWidget.h"
#include "EditorUtils.h"
#include "../../Packages/Packages.h"
#include "../../Persistence.h"
#include "../../App.h"
#include "../../Engine.h"
#include <QtCore/QPoint>
#include <QtGui/QCursor>

namespace tools {
namespace editor {

PIEWidget::PIEWidget(QWidget *parent, Qt::WindowFlags f) : 
GLWidget(parent, f), m_dt(0.f) {
	RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(Closing()), SLOT(close())));
	RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(OnTick(float)), SLOT(OnTick(float))));
}

PIEWidget::~PIEWidget() {
	if (m_game)
		m_game->NotifySaveState();
}

void PIEWidget::RunMap(int id, GameUIMode uiMode) {
	m_game = Game::New(uiMode);
	m_game->SetViewport(0, 0, width(), height());
	m_game->EnableProgressIndicator(this);

	if (m_game->LoadEntry()) {
		m_game->LoadMap(id, 0, world::kUD_Slot, true, true);
		m_game->Tick(0.001f, this);
		if (m_game->quit)
			close();
	}
}

void PIEWidget::wheelEvent(QWheelEvent *e) {
	if (m_game)
		PostInputEvent(e, *m_game.get());
}

void PIEWidget::mouseMoveEvent(QMouseEvent *e) {
	if (m_game)
		PostInputEvent(e, *m_game.get(), false, true);
}

void PIEWidget::mousePressEvent(QMouseEvent *e) {
	if (m_game)
		PostInputEvent(e, *m_game.get(), true, false);
}

void PIEWidget::mouseReleaseEvent(QMouseEvent *e) {
	if (m_game)
		PostInputEvent(e, *m_game.get(), false, false);
}

void PIEWidget::mouseDoubleClickEvent(QMouseEvent *e) {
	if (m_game)
		PostInputEvent(e, *m_game.get(), true, false);
}

void PIEWidget::keyPressEvent(QKeyEvent *e) {
	if (m_game)
		PostInputEvent(e, *m_game.get(), true);
}

void PIEWidget::keyReleaseEvent(QKeyEvent *e) {
	if (m_game)
		PostInputEvent(e, *m_game.get(), false);
}

void PIEWidget::CenterCursor() {
	int x = width()/2;
	int y = height()/2;
	QPoint p = mapToGlobal(QPoint(x, y));
	QCursor::setPos(p);
}

void PIEWidget::OnTick(float dt) {
	m_dt = dt;
	if (m_game) {
		updateGL(); // renderGL immediately.
		if (m_game->quit)
			close();
	}
}

void PIEWidget::renderGL() {
	if (m_game) {
		m_game->SetViewport(0, 0, width(), height());
		m_game->Tick(m_dt, this);
		m_dt = 0.f;
	} else {
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
}

} // editor
} // tools

#include "moc_EditorPIEWidget.cc"
