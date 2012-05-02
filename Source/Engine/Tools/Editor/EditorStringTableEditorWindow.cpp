// EditorStringTableEditorWindow.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorStringTableEditorWindow.h"
#include <QtGui/QHBoxLayout>

namespace tools {
namespace editor {

StringTableEditorWindow::StringTableEditorWindow(
	const pkg::Asset::Ref &asset,
	bool editable,
	WidgetStyle style,
	QWidget *parent
) : 
EditorWindow(
	style, 
	EditorWindow::BTN_OK|EditorWindow::BTN_DefaultOK, 
	true,
	false, 
	parent
) {

	QHBoxLayout *layout = new (ZEditor) QHBoxLayout();
	StringTableWidget *widget = new (ZEditor) StringTableWidget(
		asset,
		editable,
		this
	);

	layout->addWidget(widget);

	SetCenterLayout(layout);

	CenterParent(0.65f, 0.65f);
}

} // editor
} // tools

#include "moc_EditorStringTableEditorWindow.cc"
