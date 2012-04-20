// EditorSearchLineWidget.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include <QtGui/QWidget>
#include <Runtime/PushPack.h>

class QLineEdit;

namespace tools {
namespace editor {

class SearchLineWidget : public QWidget
{
	Q_OBJECT
public:
	SearchLineWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	virtual QSize sizeHint() const;

	RAD_DECLARE_PROPERTY(SearchLineWidget, text, QString, QString);
	
public slots:

	void clear();

signals:

	void textChanged(const QString &text);
	void returnPressed();

private:

	RAD_DECLARE_GET(text, QString);
	RAD_DECLARE_SET(text, QString);

	QLineEdit *m_lineEdit;

};

} // editor
} // tools

#include <Runtime/PopPack.h>
