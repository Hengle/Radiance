// EditorPopupMenu.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include <QtGui/QWidget>
#include <Runtime/Container/ZoneMap.h>

class QMenu;
class QIcon;
class QPoint;
class QAction;

namespace tools {
namespace editor {

class RADENG_CLASS PopupMenu : public QWidget
{
	Q_OBJECT
public:
	PopupMenu(QWidget *parent = 0);
	virtual ~PopupMenu();

	void AddItem(
		const char *path,
		QObject *receiver,
		const char *member
	);

	void AddItem(
		const QIcon &icon,
		const char *path,
		QObject *receiver,
		const char *member
	);

	void AddSep(const char *path = 0);

	QAction *Exec(const QPoint &p, QAction *action = 0);

private:

	typedef zone_map<QString, QMenu*, ZEditorT>::type MenuMap;

	QMenu *BuildMenu(QString &path, bool trim = true);

	MenuMap m_sub;
	QMenu *m_menu;
};

} // editor
} // tools
