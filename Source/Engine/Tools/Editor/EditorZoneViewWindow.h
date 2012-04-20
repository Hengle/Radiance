// EditorZoneViewWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include <QtGui/QWidget>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

class QTreeWidget;
class QTreeWidgetItem;
class QCloseEvent;

namespace tools {
namespace editor {

class RADENG_CLASS ZoneViewWindow : public QWidget
{
	Q_OBJECT
public:

	ZoneViewWindow(QWidget *parent=0);
	virtual ~ZoneViewWindow();

protected:

	virtual void closeEvent(QCloseEvent *e);

private slots:

	void OnMainWindowClose();

private:

	typedef zone_map<const Zone*, QTreeWidgetItem*, ZEditorT>::type ZoneMap;

	void Update(const Zone &zone);

	QTreeWidget *m_tree;
	ZoneMap m_items;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
