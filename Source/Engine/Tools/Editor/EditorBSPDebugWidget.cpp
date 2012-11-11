// EditorBSPDebugWidget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorMainWindow.h"
#include "EditorBSPDebugWidget.h"
#include "EditorUtils.h"
#include "../../Packages/Packages.h"
#include "../../Persistence.h"
#include "../../App.h"
#include "../../Engine.h"
#include <QtCore/QPoint>
#include <QtGui/QCursor>
#include <QtGui/QMessageBox>
#include <QtGui/QAction>

namespace tools {
namespace editor {

BSPDebugWidget::BSPDebugWidget(QWidget *parent, Qt::WindowFlags f) : 
GLNavWidget(parent, f), m_menu(0), m_time(0.f), m_dt(0.f), m_loaded(false), m_enabled(false), m_progress(0) {
	RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(Closing()), SLOT(close())));
	RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(OnTick(float)), SLOT(OnTick(float))));

	SetFreeMode();
	GLNavWidget::kbSpeed = 15.f;
	GLNavWidget::mouseSpeed = 0.5f;
	GLNavWidget::camera->fov = 90.f;
}

BSPDebugWidget::~BSPDebugWidget() {
}

void BSPDebugWidget::DebugMap(int id) {
	pkg::Asset::Ref asset = Packages()->Asset(id, pkg::Z_Unique);
	if (!asset) {
		QMessageBox::critical(this, "Error", "Unable to find map asset!");
		close();
		return;
	}

	m_progress = new (ZEditor) ProgressDialog(
		"Compiling",
		QString(),
		QString(),
		0,
		0,
		this
	);
	m_progress->setMinimumDuration(500);

	m_map = asset::MapAsset::Cast(asset);
	m_map->SetProgressIndicator(*m_progress);
	m_map->SetDebugUI(*this);

	int r = asset->Process(
		xtime::TimeSlice(100),
		pkg::P_Load
	);

	if (r < pkg::SR_Success) {
		QMessageBox::critical(this, "Error", QString("Error loading map %1").arg(asset->path.get()));
		close();
		return;
	}

	m_asset = asset;
}

void BSPDebugWidget::SetDebugMenu(PopupMenu *menu) {
	m_menu = menu;
}

QAction *BSPDebugWidget::AddDebugMenuAction(
	PopupMenu &menu,
	const char *path
) {
	return menu.AddAction(path, this, SLOT(OnMenuItem()));
}

QAction *BSPDebugWidget::AddDebugMenuAction(
	PopupMenu &menu,
	const QIcon &icon,
	const char *path
) {
	return menu.AddAction(icon, path, this, SLOT(OnMenuItem()));
}

void BSPDebugWidget::mousePressEvent(QMouseEvent *e) {
	if (e->button() == Qt::RightButton) {
		if (m_enabled && m_menu) {
			m_menu->Exec(e->globalPos());
			if (!m_enabled) {
				delete m_menu;
				m_menu = 0;
			}
		}
		return;
	}

	GLNavWidget::mousePressEvent(e);
}

void BSPDebugWidget::mouseMoveEvent(QMouseEvent *e) {
	if (e->button() == Qt::RightButton)
		return;
	GLNavWidget::mouseMoveEvent(e);
}

void BSPDebugWidget::OnTick(float dt) {
	if (m_enabled) {
		if (m_progress && m_progress->isVisible())
			m_progress->hide();
	} else {
		if (m_progress && !m_progress->isVisible())
			m_progress->show();
	}

	if (m_loaded || m_enabled) {
		if (m_enabled) {
			m_time += dt;
			m_dt = dt;
			GLNavWidget::TickCamera(dt);
			updateGL();
		}
	} else if (m_asset) {
		int r = m_asset->Process(
			xtime::TimeSlice(100),
			pkg::P_Load
		);

		if (r == pkg::SR_Success) {
			m_loaded = true;
			m_progress->close();
			m_progress = 0;
		} else {
			if (r < pkg::SR_Success) {
				m_asset.reset();
				m_map.reset();
				m_enabled = false;
				QMessageBox::critical(this, "Error", "Error loading map.");
				m_progress->close();
				m_progress = 0;
				close();
			}
		}
	}
}

void BSPDebugWidget::RAD_IMPLEMENT_SET(enabled) (bool value) {
	if (m_enabled != value)
		m_enabled = value;
}

void BSPDebugWidget::OnMenuItem() {
	QAction *action = qobject_cast<QAction*>(QWidget::sender());
	if (action)
		m_map->OnDebugMenu(action->data());
}

void BSPDebugWidget::renderGL() {
	if (m_loaded || m_enabled) {
		if (m_enabled && (m_dt > 0.f)) {
			m_map->DebugDraw(m_time, m_dt, rect());
			if (!m_enabled && m_menu) {
				delete m_menu;
				m_menu = 0;
			}
		}
		m_dt = 0.f;
	} else {
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
}

} // editor
} // tools

#include "moc_EditorBSPDebugWidget.cc"
