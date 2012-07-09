// EditorTextEditorDialogs.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorTextEditorDialog.h"
#include <QtGui/QTextEdit>

namespace tools {
namespace editor {

TextEditorDialog::TextEditorDialog(
	const QString &text,
	QWidget *parent
) : EditorWindow(
WS_Dialog, 
EditorWindow::BTN_OKCancel|EditorWindow::BTN_DefaultOK,
false,
false, 
parent, 
Qt::WindowSystemMenuHint|Qt::WindowCloseButtonHint) {

	m_textEdit = new (ZEditor) QTextEdit(text, this);
	SetCenterWidget(m_textEdit);
}

} // editor
} // tools

#include "moc_EditorTextEditorDialog.cc"
