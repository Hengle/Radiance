// EditorTickState.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorTickState.h"
#include "EditorMainWindow.h"
#include "EditorLogWindow.h"
#include "EditorUtils.h"
#include "../../App.h"
#include "../../Renderer/GL/RBackend.h"
#include <QtGui/QMessageBox.h>

namespace tools {
namespace editor {

///////////////////////////////////////////////////////////////////////////////
// Initialize
///////////////////////////////////////////////////////////////////////////////

::App::Tickable::Ref TickInit::New()
{
	return ::App::Tickable::Ref(new (ZEditor) TickInit());
}

TickInit::TickInit() :
::App::Tickable(Priority),
m_result(false)
{
	Run();
}

int TickInit::Tick(::App &app, float dt, const xtime::TimeSlice &time, int flags)
{
	if (firstTick)
	{
		RAD_VERIFY(app.engine->sys->r.Cast<r::IRBackend>()->VidBind());
		if (!app.engine->sys->r.Cast<r::IRBackend>()->CheckCaps())
		{
			MainWindow *m = MainWindow::Get();
			
			QMessageBox::critical(
			  m,
			  "Error",
			  "Unsupported video card detected!"
			);
			
			m_result = false;
			QApplication::restoreOverrideCursor();
			m->logWindow->ExitAfterDialog();
			return TickPop;
		}
	}

	if (exited)
	{
		MainWindow *m = MainWindow::Get();
		if (!m_result)
		{
			QApplication::restoreOverrideCursor();
			m->logWindow->ExitAfterDialog();
		}
		else
		{
			m->PostInit();
			app.Push(TickLoad::New());
		}
		return TickPop;
	}

	return TickNext;
}

int TickInit::ThreadProc()
{
	m_result = App::Get()->Initialize();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Load
///////////////////////////////////////////////////////////////////////////////

::App::Tickable::Ref TickLoad::New()
{
	return ::App::Tickable::Ref(new (ZEditor) TickLoad());
}

TickLoad::TickLoad() :
::App::Tickable(Priority),
m_result(false),
m_d(0)
{
}

// hi honey can I help you program??.../string /input = gstring global
int TickLoad::Tick(::App &app, float dt, const xtime::TimeSlice &time, int flags)
{
	if (firstTick)
	{
		m_d = new (ZEditor) ProgressDialog(
			"Initializing...",
			QString(),
			QString(),
			0,
			0,
			MainWindow::Get()
		);
		m_d->setMinimumDuration(500);
		Run();
	}

	if (exited)
	{
		m_d->close();
		MainWindow::Get()->PostLoad();
		return TickPop;
	}

	return TickNext;
}

int TickLoad::ThreadProc()
{
	App::Get()->engine->sys->packages->DiscoverPackages(*m_d);
	return 0;
}

} // editor
} // tools
