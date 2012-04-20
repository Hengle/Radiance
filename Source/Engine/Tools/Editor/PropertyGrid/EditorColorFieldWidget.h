// EditorColorFieldWidget.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include <QtGui/QWidget>
#include <QtGui/QColor>
#include <Runtime/PushPack.h>

class QLineEdit;
class QPaintEvent;
class QFocusEvent;

namespace tools {
namespace editor {

class RADENG_CLASS ColorFieldWidget : public QWidget
{
	Q_OBJECT
public:
	ColorFieldWidget(QWidget *parent = 0);

	QString Color() const { return m_color; }
	void SetColor(const QString &color);

signals:

	void CloseEditor();

private slots:

	void BrowseClicked();
	void EditLineFinished();

private:
	
	virtual void focusInEvent(QFocusEvent *);

	QString m_color;
	QLineEdit *m_edit;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
