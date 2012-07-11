/*! \file QtAppMain.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include RADPCH
#include <Runtime/Base.h>
#include <Runtime/Runtime.h>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <Engine/Tools/Editor/EditorMainWindow.h>
#include <Engine/COut.h>
#include <Engine/App.h>
#include <Engine/Engine.h>
#include <Engine/Zones.h>
#include <Engine/Renderer/PC/RBackend.h>

#undef qApp

RADENG_API int RADENG_CALL QtAppMain(int argc, const char **argv) {

	rt::Initialize();
	RAD_DEBUG_ONLY(file::EnforcePortablePaths(false));

	QApplication qApp(argc, (char**)argv);

	COut(C_Info) << "QtAppMain..." << std::endl;
	COut(C_Info) << "echo command line: ";

	for (int i = 0; i < argc; ++i) {
		COut(C_Info) << argv[i] << " ";
	}

	COut(C_Info) << std::endl;

	App *app = App::Get();

	QCoreApplication::setOrganizationName(app->company.get());
	QCoreApplication::setOrganizationDomain(app->website.get());
	QCoreApplication::setApplicationName(app->title.get());

	if (!app->PreInit())
		return 1;

	tools::editor::MainWindow *mainWin = new (ZEditor) tools::editor::MainWindow(*app);

	if (!(mainWin->Init() && mainWin->Show()))
		return 1;

	r::HRBackend rb = app->engine->sys->r.Cast<r::IRBackend>();
	RAD_VERIFY(rb);
	if (!rb->VidBind()) {
		QMessageBox::critical(0, "Error", "Failed to initialize OpenGL!");
		return 1;
	}

	// NOTE: normally before CheckCaps() is called you must set the context on the renderer.
	// MainWindow::Show() called previously binds the GL context and sets the renderer state for us.

	if (!rb->CheckCaps()) {
		QMessageBox::critical(0, "Error", "Unsupported graphics card detected!");
		return 1;
	}

	if (!app->Initialize())
		return 1;

	app->Run();
	mainWin->Run();
	int r = qApp.exec();

	mainWin->glBase->makeCurrent();
	app->engine->VidReset();

	delete mainWin;

	App::DestroyInstance();

	rt::Finalize();
	return r;
}

