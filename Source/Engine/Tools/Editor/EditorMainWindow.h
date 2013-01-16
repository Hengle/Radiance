// EditorMainWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include "../../Renderer/Renderer.h"
#include "../../Sound/SoundDef.h"
#include "EditorGLWidget.h"
#include <QtGui/QMainWindow>
#include <QtGui/QMdiArea>
#include <QtCore/QSettings>
#include <QtGui/QCloseEvent>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

class App;
class QAction;
class QMenu;

namespace tools {
namespace editor {

class LogWindow;
class ZoneViewWindow;
class ContentBrowserWindow;
class DebugConsoleMenuBuilder;

class RADENG_CLASS MainWindow : public QMainWindow {
	Q_OBJECT
public:

	static MainWindow *Get();

	MainWindow(::App &app, QWidget *parent=0);
	virtual ~MainWindow();

	bool Init();
	bool Show();
	void Run();
	void PostInit();
	void PostLoad();
	void BindGL();
	void UnbindGL();
	void ShowContentBrowser(bool show=true);

	RAD_DECLARE_PROPERTY(MainWindow, tickEnabled, bool, bool);
	RAD_DECLARE_READONLY_PROPERTY(MainWindow, logWindow, LogWindow*);
	RAD_DECLARE_READONLY_PROPERTY(MainWindow, app, ::App*);
	RAD_DECLARE_READONLY_PROPERTY(MainWindow, run, bool);
	RAD_DECLARE_READONLY_PROPERTY(MainWindow, glBase, QGLWidget*);
	RAD_DECLARE_READONLY_PROPERTY(MainWindow, mdiArea, QMdiArea*);
	RAD_DECLARE_READONLY_PROPERTY(MainWindow, userPrefs, QSettings*);
	RAD_DECLARE_READONLY_PROPERTY(MainWindow, soundContext, SoundContext*);
	RAD_DECLARE_READONLY_PROPERTY(MainWindow, contentBrowser, ContentBrowserWindow*);
	RAD_DECLARE_READONLY_PROPERTY(MainWindow, logWinShowHideAction, QAction*);
	RAD_DECLARE_READONLY_PROPERTY(MainWindow, zoneWinShowHideAction, QAction*);

signals:

	void OnTick(float elapsed);
	void OnClose(QCloseEvent *e);
	void Closing();

protected:

	virtual void timerEvent(QTimerEvent*);
	virtual void closeEvent(QCloseEvent*);

private slots:

	void ShowHideLogWindowTriggered(bool checked);
	void ShowHideZoneWindowTriggered(bool checked);
	void OpenCookerDialog();

private:

	void AppTick();
	bool CheckExit();
	void DoClosing();
	void UpdateDebugServers();

	RAD_DECLARE_GET(logWindow, LogWindow*) { return m_logWin; }
	RAD_DECLARE_GET(app, ::App*) { return m_app; }
	RAD_DECLARE_GET(run, bool) { return m_run; }
	RAD_DECLARE_GET(tickEnabled, bool) { return m_tickEnabled; }
	RAD_DECLARE_SET(tickEnabled, bool) { m_tickEnabled = value; }
	RAD_DECLARE_GET(userPrefs, QSettings*) { return &m_userSettings; }
	RAD_DECLARE_GET(glBase, QGLWidget*) { return m_glBase; }
	RAD_DECLARE_GET(contentBrowser, ContentBrowserWindow*) { return m_contentBrowser; }
	RAD_DECLARE_GET(logWinShowHideAction, QAction*) { return m_logWinShowHide; }
	RAD_DECLARE_GET(zoneWinShowHideAction, QAction*) { return m_zoneWinShowHide; }
	RAD_DECLARE_GET(soundContext, SoundContext*) { return m_sound.get(); }

	RAD_DECLARE_GET(mdiArea, QMdiArea*) {
		return qobject_cast<QMdiArea*>(centralWidget());
	}

	QGLWidget *m_glBase;
	r::HContext m_glBaseCtx;
	SoundContextRef m_sound;
	LogWindow *m_logWin;
	ZoneViewWindow *m_zoneWin;
	ContentBrowserWindow *m_contentBrowser;
	QAction *m_logWinShowHide;
	QAction *m_zoneWinShowHide;
	QMenu *m_dbgServersMenu;
	DebugConsoleMenuBuilder *m_dbgServersMenuBuilder;
	::App *m_app;
	mutable QSettings m_userSettings;
	bool m_run;
	bool m_exitPosted;
	bool m_tickEnabled;

	static MainWindow *s_instance;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
