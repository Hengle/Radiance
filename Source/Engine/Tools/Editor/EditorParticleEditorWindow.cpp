/*! \file EditorParticleEditorWindow.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#include RADPCH
#include "EditorParticleEditorWindow.h"

namespace tools {
namespace editor {

ParticleEditorWindow::ParticleEditorWindow(
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
	m_w = new (ZEditor) ParticleEditorWidget(
		asset,
		editable,
		this
	);

	SetCenterWidget(m_w);
	CenterParent(0.85f, 0.85f);
}

void ParticleEditorWindow::OnTickEvent(float dt) {
	m_w->Tick(dt);
}

bool ParticleEditorWindow::Load() {
	return m_w->Load();
}

} // editor
} // tools

#include "moc_EditorParticleEditorWindow.cc"
