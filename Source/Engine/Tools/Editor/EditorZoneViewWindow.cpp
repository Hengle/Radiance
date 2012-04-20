// EditorZoneViewWindow.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorZoneViewWindow.h"
#include "EditorMainWindow.h"
#include "EditorUtils.h"
#include <QtGui/QGridLayout>
#include <QtGui/QTreeWidget>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QHeaderView>
#include <QtGui/QAction>

#undef small

namespace tools {
namespace editor {

ZoneViewWindow::ZoneViewWindow(QWidget *parent) : 
QWidget(
	0, 
	Qt::Window|
	Qt::CustomizeWindowHint|
	Qt::WindowTitleHint|
	Qt::WindowSystemMenuHint|
	Qt::WindowMinMaxButtonsHint|
	Qt::WindowCloseButtonHint
)
{
	setWindowTitle("Zone Memory");
	resize(517, 353);

	QGridLayout *l = new (ZEditor) QGridLayout(this);
	m_tree = new QTreeWidget(this);
	m_tree->setSelectionMode(QAbstractItemView::NoSelection);
	m_tree->header()->hide();
	l->addWidget(m_tree, 0, 0);

	setAttribute(Qt::WA_DeleteOnClose);

	if (parent)
		CenterWidget(*this, *parent);

	RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(Closing()), SLOT(OnMainWindowClose())));

	Update(ZRuntime);
	Update(ZUnknown); // any other root level zones here
}

ZoneViewWindow::~ZoneViewWindow()
{
}

void ZoneViewWindow::closeEvent(QCloseEvent *e)
{
	MainWindow::Get()->zoneWinShowHideAction->setChecked(false);
	e->accept();
}

void ZoneViewWindow::OnMainWindowClose()
{
	close();
}

void ZoneViewWindow::Update(const Zone &zone)
{
	SizeBuffer buf[5];
	QString self;
	FormatSize(buf[0], zone.numBytes);
	FormatSize(buf[1], zone.overhead);
	FormatSize(buf[2], zone.small);
	FormatSize(buf[3], zone.large);
	FormatSize(buf[4], zone.high);
	self.sprintf("Self: B(%s), O(%s), A(%d), S(%s), L(%s), H(%s)", 
		buf[0], buf[1], zone.count.get(), buf[2], buf[3], buf[4]);

	QString total;
	FormatSize(buf[0], zone.totalBytes);
	FormatSize(buf[1], zone.totalOverhead);
	FormatSize(buf[2], zone.totalSmall);
	FormatSize(buf[3], zone.totalLarge);
	FormatSize(buf[4], zone.totalHigh);
	total.sprintf("Total: B(%s), O(%s), A(%d), S(%s), L(%s), H(%s)", 
		buf[0], buf[1], zone.totalCount.get(), buf[2], buf[3], buf[4]);

	QString title = "<" + QString(zone.name) + ">: " + total + "|" + self;

	QTreeWidgetItem *item;
	ZoneMap::iterator it = m_items.find(&zone);

	if (it == m_items.end())
	{
		QTreeWidgetItem *parent = 0;

		if (zone.parent)
		{
			it = m_items.find(zone.parent);
			RAD_ASSERT(it != m_items.end());
			parent = it->second;
		}

		item = new QTreeWidgetItem(QStringList(title));

		if (parent)
			parent->addChild(item);
		else
			m_tree->addTopLevelItem(item);

		m_items[&zone] = item;
	}
	else
	{
		item = it->second;
		item->setData(0, Qt::DisplayRole, title);
	}

	for (Zone *z = zone.head; z; z = z->next)
		Update(*z);
}

} // editor
} // tools

#include "moc_EditorZoneViewWindow.cc"
