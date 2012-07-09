// EditorMapEditorWindow.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorMapEditorWindow.h"
#include "../EditorMainWindow.h"
#include "../EditorUtils.h"
#include "../../../Packages/Packages.h"
#include <QtGui/QDesktopWidget>

namespace tools {
namespace editor {

void MapEditorWindow::LaunchEditor(int assetId)
{
	(new (ZEditor) MapEditorWindow())->Load(assetId);
}

MapEditorWindow::MapEditorWindow()
: QWidget(0, 
	Qt::Window|
	Qt::CustomizeWindowHint|
	Qt::WindowTitleHint|
	Qt::WindowSystemMenuHint|
	Qt::WindowMinMaxButtonsHint|
	Qt::WindowCloseButtonHint
)
{
	setAttribute(Qt::WA_DeleteOnClose);
	QDesktopWidget *desktop = QApplication::desktop();
	PercentSize(*this, desktop->screenGeometry(), 0.85f, 0.80f);
	CenterWidget(*this, desktop->screenGeometry());
	RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(OnClose(QCloseEvent*)), SLOT(MainWinClose(QCloseEvent*))));
}

MapEditorWindow::~MapEditorWindow()
{
}

void MapEditorWindow::Load(int id)
{
	show();

	pkg::Package::Entry::Ref ref = Packages()->FindEntry(id);
	if (ref)
	{
		setWindowTitle(QString("Map Editor: ") + ref->name.get());
	}
}

void MapEditorWindow::closeEvent(QCloseEvent *e)
{
	e->accept(); // assume we can close.
	emit OnClose(e);
	if (e->isAccepted())
	{
		emit Closing();
		DoClose();
	}
}

void MapEditorWindow::MainWinClose(QCloseEvent *e)
{
	e->setAccepted(close());
}

void MapEditorWindow::DoClose()
{
}

} // editor
} // tools

#include "moc_EditorMapEditorWindow.cc"
