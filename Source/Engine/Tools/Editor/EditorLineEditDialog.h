// EditorLineEditDialog.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include <QtGui/QDialog>
#include <Runtime/PushPack.h>

class QLineEdit;
class QLabel;

namespace tools {
namespace editor {

class LineEditDialog : public QDialog
{
	Q_OBJECT
public:
	LineEditDialog(
		const QString &caption, 
		const QString &prompt,
		const QString &edit = QString(),
		QWidget *parent = 0
	);

	QLineEdit *LineEdit() const { return m_line; }
	QLabel *Label() const { return m_label; }

private:

	QLineEdit *m_line;
	QLabel *m_label;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
