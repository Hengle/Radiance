// EditorTextEditorDialogs.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorWindow.h"
#include <Runtime/PushPack.h>

class QIcon;
class QTextEdit;

namespace tools {
namespace editor {

class RADENG_CLASS TextEditorDialog : public EditorWindow {
	Q_OBJECT
public:

	TextEditorDialog(
		const QString &text,
		QWidget *parent = 0
	);

	RAD_DECLARE_READONLY_PROPERTY(TextEditorDialog, textEdit, QTextEdit*);

private:

	RAD_DECLARE_GET(textEdit, QTextEdit*) { return m_textEdit; }

	QTextEdit *m_textEdit;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
