/*! \file EditorParticleEditorWindow.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once

#include "EditorTypes.h"
#include "EditorWindow.h"
#include "EditorParticleEditorWidget.h"
#include <QtGui/QWidget>
#include <QtGui/QIcon>
#include <Runtime/PushPack.h>

class QResizeEvent;

namespace tools {
namespace editor {

class RADENG_CLASS ParticleEditorWindow : public EditorWindow {
	Q_OBJECT
public:

	ParticleEditorWindow(
		const pkg::Asset::Ref &asset,
		bool editable,
		WidgetStyle style,
		QWidget *parent
	);

	bool Load();

protected:

	virtual void OnTickEvent(float dt);

private:

	ParticleEditorWidget *m_w;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
