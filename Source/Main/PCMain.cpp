// PCMain.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Application entry point for pc platforms.
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if defined(RAD_OPT_PC)

#include <Runtime/Base.h>
#include <Runtime/String.h>
#include <Runtime/Thread.h>
#include <Runtime/Runtime.h>
#include <Engine/App.h>
#include <Engine/Input.h>
#include <Engine/Persistence.h>
#include <Engine/Renderer/PC/RBackend.h>
#include <Engine/StringTable.h>
#include <Runtime/StringBase.h>

#if defined(RAD_OPT_DEBUG)
#include <Runtime/File.h>
#endif

#if defined(RAD_OPT_PC_TOOLS)
#include <QtGui/QApplication>
#endif

#if !defined(RAD_OPT_PC_TOOLS) || defined(RAD_OPT_APPLE)
#include <SDL/SDL_main.h>
#else
#define _SDL_main_h
#endif

#include <SDL/SDL.h>
#undef MessageBox

#if defined(RAD_OPT_WIN)
#include <Shellapi.h>
#if !defined(RAD_TARGET_GOLDEN)
#include <VLD/vld.h> // VLD only in non-golden builds.
#endif
#if defined(RAD_OPT_PC_TOOLS)
#include <process.h>
#endif
//#define WIN_CRASH_REPORT
#if defined(WIN_CRASH_REPORT)
#include "../Runtime/Win/WinCrashReporter.h"
#include <stdio.h>
namespace {
	void CrashReport(const fatal::WinCrashReporter::String &msg)
	{
		FILE *fp = fopen("crashlog.txt", "a+t");
		if (fp)
		{
			fprintf("-------------\n%s", msg.c_str());
			fclose(fp);
		}
	}
}
#endif
#endif

namespace {

bool PreInit(App &app)
{
	if (SDL_Init(0) == -1)
	{
		throw exception(0, RAD_SS("SDL_Init failed: " << SDL_GetError()).c_str());
	}

	return app.PreInit();
}

bool Initialize(App &app)
{
	bool result = true;

	r::HRBackend rb = app.engine->sys->r.Cast<r::IRBackend>();
	RAD_VERIFY(rb);

	if (!rb->VidBind())
	{
		MessageBox(L"Error", L"Failed to initialize OpenGL.", MBStyleOk);
		return false;
	}

	if (!rb->CheckCaps())
	{
		MessageBox(L"Error", L"Unsupported Graphics Card Detected.", MBStyleOk);
		return false;
	}

	if (!app.Initialize())
		return false;

	app.Run();
	return true;
}

void Finalize(App &app)
{
	app.Finalize();
}

bool s_postQuit = false;

#if defined(RAD_OPT_PC_TOOLS)
bool s_editor = false;
#endif

}

void __PostQuit()
{
	s_postQuit = true;
}

bool __PostQuitPending()
{
	return s_postQuit;
}

bool CloudStorage::Enabled()
{
	return false;
}

CloudFile::Vec CloudStorage::Resolve(const char *name)
{
	return CloudFile::Vec();
}

void CloudStorage::ResolveConflict(const CloudFile::Ref &version)
{
}

bool CloudStorage::StartDownloadingLatestVersion(const char *name)
{
	return true;
}

CloudFile::Status CloudStorage::FileStatus(const char *name)
{
	return CloudFile::Ready;
}

void NativeApp::LaunchURL(const char *sz) {
#if defined(RAD_OPT_WIN)
	ShellExecuteA(0, "open", sz, 0, 0, SW_SHOWNORMAL);
#endif
}

#if defined(RAD_OPT_PC_TOOLS)
#include "../Engine/Tools/Editor/EditorMainWindow.h"
#include "../Engine/Tools/Editor/EditorLogWindow.h"
#include <boost/thread/mutex.hpp>

typedef boost::mutex Mutex;
typedef boost::lock_guard<Mutex> Lock;
Mutex s_qtMutex;
thread::Id s_mainThreadId;

thread::Id __QMainThreadId()
{
	return s_mainThreadId;
}

bool EditorInit(App &app, bool first)
{
	tools::editor::MainWindow *m = new (ZEditor) tools::editor::MainWindow(app, first);
	return m->Init() && m->Show();
}

bool EditorRun(App &app)
{
	tools::editor::MainWindow *m = tools::editor::MainWindow::Get();
	RAD_ASSERT(m);
	m->Run();
	qApp->exec();
	m->glBase->makeCurrent();
	app.engine->VidReset();
	delete m;
	return true;
}

void EditorShutdown(App &app)
{
}

void SyncEditorMode(App &app)
{
#if defined(RAD_OPT_APPLE)
	app.m_editor = true;
#else
	app.m_editor = app.wantEditor;
#endif
}
#endif

bool SDLInit(int argc, char *argv[], App &app)
{
	COut(C_Info) << "SDLInit()..." << std::endl;

	r::VidMode vidMode(1024, 768, 32, 60, false);
	app.engine->sys->r.Cast<r::IRBackend>()->SetVidMode(vidMode);

	SDL_WM_SetCaption(string::Shorten(app.title).c_str(), string::Shorten(app.title).c_str());

	return true;
}

bool SDLRun(App &app)
{
	SDL_Event e;

	while (!__PostQuitPending() && !app.exit
#if defined(RAD_OPT_PC_TOOLS)
		&& !app.wantEditor
#endif
		)
	{
		while (SDL_PollEvent(&e))
		{
			InputEvent i;
			i.touch = 0;
			i.time = xtime::ReadMilliseconds();

			switch (e.type)
			{
				case SDL_QUIT:
					__PostQuit();
					break;
				case SDL_KEYDOWN:
					i.type = InputEvent::T_KeyDown;
					i.data[0] = e.key.keysym.sym;
					app.PostInputEvent(i);
					break;
				case SDL_KEYUP:
					i.type = InputEvent::T_KeyUp;
					i.data[0] = e.key.keysym.sym;
					app.PostInputEvent(i);
					break;
				case SDL_MOUSEBUTTONDOWN:
					{
						i.type = InputEvent::T_MouseDown;
						i.data[0] = e.button.x;
						i.data[1] = e.button.y;
						i.data[2] = SDL_BUTTON(e.button.button);
						app.PostInputEvent(i);
					}
					break;
				case SDL_MOUSEBUTTONUP:
					{
						i.type = InputEvent::T_MouseUp;
						i.data[0] = e.button.x;
						i.data[1] = e.button.y;
						i.data[2] = SDL_BUTTON(e.button.button);
						app.PostInputEvent(i);
					}
					break;
				case SDL_MOUSEMOTION:
					i.type = InputEvent::T_MouseMove;
					i.data[0] = e.motion.x;
					i.data[1] = e.motion.y;
					i.data[2] = e.motion.state;
					app.PostInputEvent(i);
					break;
			}
		}

		app.Tick();
		thread::Sleep();
	}

	return true;
}

void SDLShutdown(App &app)
{
	app.engine->VidReset();
	SDL_Quit();
}

void Run(int argc, char *argv[], App &app)
{
#if defined (RAD_OPT_PC_TOOLS)
	SyncEditorMode(app);
#if defined(RAD_OPT_APPLE)
	s_editor = true;
#else
	s_editor = app.wantEditor;
#endif
#endif

	if (!PreInit(app))
	{
		return;
	}

#if defined (RAD_OPT_PC_TOOLS)
	if (s_editor)
	{
		if (!EditorInit(app, true))
		{
			Finalize(app);
			return;
		}
	}
	else
	{
#endif

	if (!SDLInit(argc, argv, app))
	{
		Finalize(app);
		return;
	}

	Initialize(app);

#if defined(RAD_OPT_PC_TOOLS)
	}
#endif

#if defined (RAD_OPT_PC_TOOLS)

#if defined(RAD_OPT_APPLE)
	EditorRun(app);
#else
	for(;;)
	{
		if (s_editor != app.wantEditor)
		{
			s_editor = app.wantEditor;
			app.SwitchEditorMode(s_editor);
			if (s_editor)
			{
				SDLShutdown(app);
				if (!EditorInit(app, false))
				{
					break;
				}
			}
			else
			{
				EditorShutdown(app);
				if (!SDLInit(argc, argv, app))
				{
					break;
				}
			}
			SyncEditorMode(app);
			app.engine->VidBind();
		}

		if (s_editor)
		{
			if (!EditorRun(app))
			{
				break;
			}
		}
		else
		{
			if (!SDLRun(app))
			{
				break;
			}
		}

		// if we didn't break out of event processing cause of editor toggle
		// then we want to exit.
		if (__PostQuitPending() || app.exit || app.wantEditor == app.editor) 
			break;
	}
#endif

#else
	SDLRun(app);
#endif

	Finalize(app);

#if defined(RAD_OPT_PC_TOOLS)
	if (s_editor)
	{
		EditorShutdown(app);
	}
	else
#endif
	{
		SDLShutdown(app);
	}
}

namespace
{
	int s_argc = 0;
	const char **s_argv = 0;
}

int __Argc() { return s_argc; }
const char **__Argv() { return s_argv; }

#if defined(RAD_OPT_APPLE)
int __HasQt()
{
#if defined(RAD_OPT_PC_TOOLS)
	return 1;
#else
	return 0;
#endif
}
#endif

#if defined(RAD_OPT_WIN) && defined(RAD_OPT_PC_TOOLS)
namespace {
std::string ExeName()
{
	char name[256];
	int x = ::GetModuleFileNameA(0, name, 255);
	name[x] = 0; // terminate.
	return name;
}
bool IsDotCom()
{
	std::string x = ExeName();
	return x.length() > 4 && !::stricmp(&x[x.length()-4], ".com");
}
void SpawnSelf()
{
	char str[256];
	::strcpy(str, ExeName().c_str());
	str[::strlen(str)-4] = 0;
	::strcat(str, ".exe");
	::_spawnl(_P_DETACH, str, str, NULL);
}
}
#endif

extern "C" int main(int argc, char *argv[])
{
	rt::Initialize();
	RAD_DEBUG_ONLY(file::EnforcePortablePaths(false));
#if defined(RAD_OPT_PC_TOOLS)
	s_mainThreadId = thread::ThreadId();
	QApplication qapp(argc, argv);
#endif
	s_argc = argc;
	s_argv = (const char **)argv;

#if defined(RAD_OPT_WIN) && defined(RAD_OPT_PC_TOOLS)
	if (argc == 1 && IsDotCom())
	{
		SpawnSelf();
		rt::Finalize();
		return 0;
	}
#endif

	COut(C_Info) << "main..." << std::endl;
	COut(C_Info) << "echo command line: ";

	for (int i = 0; i < argc; ++i)
	{
		COut(C_Info) << argv[i] << " ";
	}

	COut(C_Info) << std::endl;

	App *app = App::Get();

#if defined(WIN_CRASH_REPORT)
	fatal::WinCrashReporter::Initialize(
		::GetCurrentProcess(),
		CrashReport
	);
#endif

#if defined(RAD_OPT_PC_TOOLS)
	QCoreApplication::setOrganizationName(string::Shorten(app->company).c_str());
	QCoreApplication::setOrganizationDomain(string::Shorten(app->website).c_str());
	QCoreApplication::setApplicationName(string::Shorten(app->title).c_str());
#endif

	try
	{
		Run(argc, argv, *app);
	}
	catch (exception &e)
	{
		std::string msg(RAD_SS("unhandled exception '" << e.type() << "' occurred." << std::endl << "Message: " << (e.what() ? e.what() : "No message provided.")));
		COut(C_Info) << msg << std::endl;
		RAD_FAIL_EX(msg.c_str());
	}

#if defined(WIN_CRASH_REPORT)
	fatal::WinCrashReporter::Shutdown();
#endif

	delete app;

	rt::Finalize();
	return 0;
}

#endif

