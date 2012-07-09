// EditorPopupMenu.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorPopupMenu.h"
#include <QtGui/QMenu>

namespace tools {
namespace editor {

PopupMenu::PopupMenu(QWidget *parent)
: QWidget(parent)
{
	m_menu = new (ZEditor) QMenu(this);
}

PopupMenu::~PopupMenu()
{
}

void PopupMenu::AddItem(
	const char *path,
	QObject *receiver,
	const char *member
)
{
	QString s(path);
	QMenu *m = BuildMenu(s);
	if (m)
		m->addAction(s, receiver, member);
}

void PopupMenu::AddItem(
	const QIcon &icon,
	const char *path,
	QObject *receiver,
	const char *member
)
{
	QString s(path);
	QMenu *m = BuildMenu(s);
	if (m)
		m->addAction(icon, s, receiver, member);
}

void PopupMenu::AddSep(const char *path)
{
	if (!path)
	{
		m_menu->addSeparator();
		return;
	}

	QString s(path);
	QMenu *m = BuildMenu(s, false);

	if (m)
		m->addSeparator();
}

QAction *PopupMenu::Exec(const QPoint &p, QAction *action)
{
	return m_menu->exec(p, action);
}

QMenu *PopupMenu::BuildMenu(QString &path, bool trim)
{
	QStringList split = path.split('\n', QString::SkipEmptyParts);

	if (split.count() < (trim ? 2 : 1))
		return m_menu;

	if (trim)
	{
		path = split.back();
		split.pop_back();
	}

	QString root;
	QMenu *m = 0;

	foreach(QString x, split)
	{
		root += x + "||";
		MenuMap::const_iterator it = m_sub.find(root);
		if (it == m_sub.end())
		{
			m = m->addMenu(x);
			m_sub[root] = m;
		}
		else
		{
			m = it->second;
		}
	}

	return m;
}

} // editor
} // tools

#include "moc_EditorPopupMenu.cc"
