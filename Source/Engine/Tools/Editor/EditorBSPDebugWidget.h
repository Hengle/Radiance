// EditorBSPDebugWidget.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include "EditorGLNavWidget.h"
#include "EditorPopupMenu.h"
#include "EditorProgressDialog.h"
#include "../../Packages/Packages.h"
#include "../../Assets/MapAsset.h"
#include "../../World/MapBuilder/MapBuilderDebugUI.h"
#include <Runtime/PushPack.h>

class QWidget;
class QKeyEvent;

namespace tools {
namespace editor {

class RADENG_CLASS BSPDebugWidget : public GLNavWidget, public MapBuilderDebugUI {
	Q_OBJECT
public:

	BSPDebugWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	virtual ~BSPDebugWidget();

	void DebugMap(int id);

	virtual void SetDebugMenu(PopupMenu *menu);

	virtual QAction *AddDebugMenuAction(
		PopupMenu &menu,
		const char *path
	);

	virtual QAction *AddDebugMenuAction(
		PopupMenu &menu,
		const QIcon &icon,
		const char *path
	);

protected:

	virtual void renderGL();
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);

	virtual RAD_DECLARE_GET(enabled, bool) {
		return m_enabled;
	}

	virtual RAD_DECLARE_SET(enabled, bool);

	virtual RAD_DECLARE_GET(camera, Camera*) { 
		return GLNavWidget::camera;
	}

private slots:

	void OnTick(float dt);
	void OnMenuItem();

private:

	pkg::Asset::Ref m_asset;
	asset::MapAsset::Ref m_map;
	float m_time, m_dt;
	bool m_loaded;
	bool m_enabled;
	PopupMenu *m_menu;
	ProgressDialog *m_progress;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
