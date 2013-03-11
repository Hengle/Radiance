/*! \file EditorConversationTreeEditorWindow.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup editor
*/

#include RADPCH
#include "EditorConversationTreeEditorWindow.h"
#include "EditorConversationTreeEditorWidget.h"
#include <QtGui/QHBoxLayout>

namespace tools {
namespace editor {

ConversationTreeEditorWindow::ConversationTreeEditorWindow(
	const pkg::Asset::Ref &asset,
	bool editable,
	WidgetStyle style,
	QWidget *parent
) : 
EditorWindow(
	style, 
	EditorWindow::kButton_Close|EditorWindow::kButton_DefaultClose, 
	true,
	false, 
	parent
) {

	QHBoxLayout *layout = new (ZEditor) QHBoxLayout();
	ConversationTreeEditorWidget *widget = new (ZEditor) ConversationTreeEditorWidget(
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

#include "moc_EditorConversationTreeEditorWindow.cc"

