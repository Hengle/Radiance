/*! \file EditorMapEditorWindow.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_editor
*/

#include RADPCH
#include "EditorMapEditorWindow.h"

namespace tools {
namespace editor {
namespace map_editor {

MapEditorWindow::MapEditorWindow(
	const pkg::Asset::Ref &asset,
	bool editable,
	WidgetStyle style,
	QWidget *parent
) : 
EditorWindow(
	style, 
	EditorWindow::kButton_None, 
	true,
	true, 
	parent
) {
}

MapEditorWindow::~MapEditorWindow() {
}

} // map_editor
} // editor
} // tools

#include "moc_EditorMapEditorWindow.cc"
