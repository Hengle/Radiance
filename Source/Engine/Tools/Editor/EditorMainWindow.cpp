// EditorMainWindow.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QAction>
#include <QtGui/QMenuBar>
#include "EditorMainWindow.h"
#include "EditorLogWindow.h"
#include "EditorZoneViewWindow.h"
#include "EditorTickState.h"
#include "EditorUtils.h"
#include "EditorGLWidget.h"
#include "EditorCookerDialog.h"
#include "ContentBrowser/EditorContentBrowserWindow.h"
#include "ContentBrowser/EditorContentBrowserView.h"
#include "PropertyGrid/EditorPropertyGrid.h"
#include "../../App.h"
#include "../../Renderer/PC/RBackend.h"
#include "../../Renderer/GL/GLTable.h"
#include "../../Sound/Sound.h"

bool __PostQuitPending();

namespace tools {
namespace editor {

MainWindow *MainWindow::s_instance = 0;

MainWindow *MainWindow::Get()
{
	return s_instance;
}

MainWindow::MainWindow(::App &app, bool firstTime, QWidget *parent) :
QMainWindow(parent),
m_app(&app),
m_firstTime(firstTime),
m_run(false),
m_glBase(0),
m_exitPosted(false),
m_contentBrowser(0),
m_logWinShowHide(0),
m_zoneWinShowHide(0),
m_zoneWin(0),
m_logWin(0),
m_tickEnabled(true),
#if defined(RAD_OPT_WIN)
m_userSettings(
	"editor.user.ini",
	QSettings::IniFormat
)
#else
m_userSettings(
	QSettings::IniFormat,
	QSettings::UserScope,
	QString::fromWCharArray(app.company),
	QString::fromWCharArray(app.title)
)
#endif
{
	setWindowFlags(
		Qt::Window|
		Qt::CustomizeWindowHint|
		Qt::WindowTitleHint|
		Qt::WindowSystemMenuHint|
		Qt::WindowCloseButtonHint|
		Qt::WindowMinMaxButtonsHint
	);
	setEnabled(false);
	setAttribute(Qt::WA_QuitOnClose);
	QDesktopWidget *desktop = QApplication::desktop();
	PercentSize(*this, desktop->screenGeometry(), 0.85f, 0.80f);
	CenterWidget(*this, desktop->screenGeometry());
}

MainWindow::~MainWindow()
{
	s_instance = 0;
}

void MainWindow::BindGL()
{
	m_glBase->makeCurrent();
	m_app->engine->sys->r->ctx = m_glBaseCtx;
}

void MainWindow::UnbindGL()
{
	m_app->engine->sys->r->ctx = r::HContext();
	m_glBase->doneCurrent();
}

bool MainWindow::Init()
{
	setWindowTitle(QString::fromWCharArray(m_app->title) + ": Content Browser");
	m_logWin = new (ZEditor) LogWindow(this);
	m_logWin->setWindowTitle(QString::fromWCharArray(m_app->title) + QString(": Starting Editor..."));

	s_instance = this;

	return true;
}

bool MainWindow::Show()
{
	show();
	m_logWin->show();

	QApplication::setOverrideCursor(Qt::WaitCursor);

	COut(C_Info) << "Initializing OpenGL..." << std::endl;
	QDispatch();

	QGLFormat fmt;
	r::SetQGLFormat(fmt);
	QGLFormat::setDefaultFormat(fmt);

	m_glBase = new (ZEditor) QGLWidget(this);

	m_glBase->makeCurrent();
	m_glBaseCtx = m_app->engine->sys->r.Cast<r::IRBackend>()->BindContext();
	RAD_ASSERT(m_glBaseCtx);
	m_app->engine->sys->r->ctx = m_glBaseCtx;
	BindGL();
	CLEAR_GL_ERRORS();

	m_sound = SoundContext::create(m_app->engine->sys->alDriver);

	m_logWinShowHide = new (ZEditor) QAction("Log Window", this);
	m_logWinShowHide->setCheckable(true);
	m_logWinShowHide->setChecked(true);
	RAD_VERIFY(connect(m_logWinShowHide, SIGNAL(triggered(bool)), SLOT(ShowHideLogWindowTriggered(bool))));
	
	m_zoneWinShowHide = new (ZEditor) QAction("Zone Memory", this);
	m_zoneWinShowHide->setCheckable(true);
	m_zoneWinShowHide->setChecked(false);
	RAD_VERIFY(connect(m_zoneWinShowHide, SIGNAL(triggered(bool)), SLOT(ShowHideZoneWindowTriggered(bool))));

	QAction *openCookerDialog = new (ZEditor) QAction("Cooker..", this);
	RAD_VERIFY(connect(openCookerDialog, SIGNAL(triggered(bool)), SLOT(OpenCookerDialog())));

	QMenuBar *menuBar = new (ZEditor) QMenuBar(this);
	QMenu *toolsMenu = menuBar->addMenu("Tools");
	toolsMenu->addAction(openCookerDialog);
	QMenu *viewMenu = menuBar->addMenu("View");
	viewMenu->addAction(m_logWinShowHide);
	viewMenu->addAction(m_zoneWinShowHide);
	setMenuBar(menuBar);

	return true;
}

void MainWindow::ShowHideLogWindowTriggered(bool checked)
{
	m_logWin->setVisible(checked);
}

void MainWindow::ShowHideZoneWindowTriggered(bool checked)
{
	if (checked)
	{
		m_zoneWin = new (ZEditor) ZoneViewWindow(this);
		m_zoneWin->show();
	}
	else
	{
		if (m_zoneWin)
			m_zoneWin->close();
		m_zoneWin = 0;
	}
}

void MainWindow::OpenCookerDialog()
{
	(new CookerDialog(this))->show();
}

void MainWindow::Run()
{
	if (m_firstTime)
	{
		m_app->Push(TickInit::New());
	}

	m_run = true;
	startTimer(1);
}

void MainWindow::PostInit()
{
	setEnabled(true);
	m_logWin->EnableCloseButton();
	m_logWin->setWindowTitle("System Log");
	m_logWin->hide();
}

void MainWindow::PostLoad()
{
	QApplication::restoreOverrideCursor();
	UnbindGL();

	m_contentBrowser = new (ZEditor) ContentBrowserWindow(WS_Widget, true, true);
	setCentralWidget(m_contentBrowser);
	
	m_app->EditorStart();
}

void MainWindow::timerEvent(QTimerEvent*)
{
	AppTick();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	e->accept();
	emit OnClose(e);
	if (e->isAccepted())
	{
		emit Closing();
		DoClosing();
	}
}

void MainWindow::DoClosing()
{
	setCentralWidget(0);
	m_contentBrowser = 0;
	delete m_logWin;
}

bool MainWindow::CheckExit()
{
	if (m_exitPosted) 
		return true;

	if (__PostQuitPending() || m_app->exit
#if !defined(RAD_OPT_APPLE)
		|| !m_app->wantEditor
#endif
		)
	{
		m_exitPosted = true;
		close();
		return true;
	}

	return false;
}

void MainWindow::AppTick()
{
	BindGL();

	if (CheckExit()) 
		return;

	if (m_tickEnabled)
	{
		float elapsed = m_app->Tick();
		emit OnTick(elapsed);
		ContentBrowserView::Tick(elapsed);
		m_sound->tick(elapsed, true);
	}

	CheckExit();
}

} // editor
} // tools

#include "moc_EditorMainWindow.cc"
