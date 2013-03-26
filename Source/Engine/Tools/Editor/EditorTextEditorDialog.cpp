// EditorTextEditorDialogs.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorTextEditorDialog.h"
#include <QtGui/QTextEdit>
#include <QtGui/QKeyEvent>

namespace tools {
namespace editor {

TextEditorDialogTextEdit::TextEditorDialogTextEdit(QWidget *parent) : QTextEdit(parent) {
}

TextEditorDialogTextEdit::TextEditorDialogTextEdit(const QString &text, QWidget *parent) : QTextEdit(text, parent) {
}

bool TextEditorDialogTextEdit::event(QEvent *e) {
	if ((e->type() == QEvent::KeyPress) ||
		(e->type() == QEvent::KeyRelease)) {
		QKeyEvent *key = static_cast<QKeyEvent*>(e);
		if (key->key() == Qt::Key_Enter) {
			key->ignore();
			return false;
		}
	}

	return QTextEdit::event(e);
}

TextEditorDialog::TextEditorDialog(
	const QString &text,
	QWidget *parent
) : EditorWindow(
WS_Dialog, 
EditorWindow::kButton_OKCancel|EditorWindow::kButton_DefaultOK,
false,
false, 
parent, 
Qt::WindowSystemMenuHint|Qt::WindowCloseButtonHint) {

	m_textEdit = new (ZEditor) TextEditorDialogTextEdit(text, this);
	SetCenterWidget(m_textEdit);
}

} // editor
} // tools

#include "moc_EditorTextEditorDialog.cc"
