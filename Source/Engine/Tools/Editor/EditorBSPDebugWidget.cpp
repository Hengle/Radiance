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
		QMessageBox::critical(this, "Error", QString("Error loading map %1").arg(m_asset->path.get()));
		close();
		return;
	}

	m_asset = asset;
}

void BSPDebugWidget::SetDebugMenu(PopupMenu *menu) {
	m_menu = menu;
}

void BSPDebugWidget::mousePressEvent(QMouseEvent *e) {
	if (e->button() == Qt::RightButton) {
		if (m_loaded && m_menu)
			m_menu->Exec(e->globalPos());
		return;
	}

	GLNavWidget::mousePressEvent(e);
}

void BSPDebugWidget::OnTick(float dt) {
	if (m_loaded) {
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
				QMessageBox::critical(this, "Error", "Error loading map.");
				m_progress->close();
				m_progress = 0;
				close();
			}
		}
	}
}

void BSPDebugWidget::OnMenuItem() {
	QAction *action = qobject_cast<QAction*>(QWidget::sender());
	if (action)
		m_map->OnDebugMenu(action->data());
}

void BSPDebugWidget::renderGL() {
	if (m_loaded) {
		if (m_enabled && (m_dt > 0.f))
			m_map->DebugDraw(m_time, m_dt);
		m_dt = 0.f;
	} else {
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
}

} // editor
} // tools

#include "moc_EditorBSPDebugWidget.cc"
