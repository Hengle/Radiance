/*! \file EditorModelEditorWindow.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once

#include "EditorTypes.h"
#include "EditorWindow.h"
#include "EditorModelEditorWidget.h"
#include <QtGui/QWidget>
#include <QtGui/QIcon>
#include <Runtime/PushPack.h>

class QResizeEvent;

namespace tools {
namespace editor {

class RADENG_CLASS ModelEditorWindow : public EditorWindow {
	Q_OBJECT
public:

	ModelEditorWindow(
		const pkg::Asset::Ref &asset,
		bool editable,
		WidgetStyle style,
		QWidget *parent
	);

	bool Load();

protected:

	virtual void OnTickEvent(float dt);

private:

	ModelEditorWidget *m_w;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
