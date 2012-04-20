// EditorContentImportFieldWidget.h
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
namespace content_property_details {

class RADENG_CLASS ContentImportFieldWidget : public QWidget
{
	Q_OBJECT
public:
	ContentImportFieldWidget(QWidget *parent = 0);

	QString Filter() const { return m_filter; }
	void SetFilter(const QString &filter) { m_filter = filter; }
	QString Path() const { return m_path; }
	void SetPath(const QString &path);
	
signals:

	void CloseEditor();

private slots:

	void BrowseClicked();
	void EditLineFinished();

private:
	
	virtual void focusInEvent(QFocusEvent *);

	QString m_filter;
	QString m_path;
	QLineEdit *m_edit;
};

} // content_property_details
} // editor
} // tools

#include <Runtime/PopPack.h>
