// EditorFilePathFieldWidget.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include <QtGui/QWidget>
#include <Runtime/PushPack.h>

class QLineEdit;
class QPaintEvent;
class QFocusEvent;

namespace tools {
namespace editor {

class RADENG_CLASS FilePathFieldWidget : public QWidget
{
	Q_OBJECT
public:
	FilePathFieldWidget(QWidget *parent = 0);

	QString Filter() const { return m_filter; }
	void SetFilter(const QString &filter) { m_filter = filter; }
	QString Prefix() const { return m_prefix; }
	void SetPrefix(const QString &prefix);

	QString Path() const { return m_path; }
	void SetPath(const QString &path);

signals:

	void CloseEditor();

private slots:

	void BrowseClicked();
	void EditLineFinished();

private:
	
	virtual void focusInEvent(QFocusEvent *);

	bool ValidatePath(const QString &s);

	QString m_prefix;
	QString m_filter;
	QString m_path;
	QString m_nativePrefix;
	QLineEdit *m_edit;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
