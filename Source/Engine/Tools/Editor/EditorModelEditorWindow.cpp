/*! \file EditorModelEditorWindow.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#include RADPCH
#include "EditorModelEditorWindow.h"

namespace tools {
namespace editor {

ModelEditorWindow::ModelEditorWindow(
	const pkg::Asset::Ref &asset,
	bool editable,
	WidgetStyle style,
	QWidget *parent
) : EditorWindow(
	style,
	EditorWindow::kButton_OK|EditorWindow::kButton_DefaultOK, 
	true,
	true, 
	parent
) {
	m_w = new (ZEditor) ModelEditorWidget(
		asset,
		editable,
		this
	);

	SetCenterWidget(m_w);
	CenterParent(0.85f, 0.85f);
}

void ModelEditorWindow::OnTickEvent(float dt) {
	m_w->Tick(dt);
}

bool ModelEditorWindow::Load() {
	return m_w->Load();
}

} // editor
} // tools

#include "moc_EditorModelEditorWindow.cc"
