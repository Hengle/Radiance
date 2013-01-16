// EditorEventRegistry.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <QtGui/qevent.h>

namespace tools {
namespace editor {

enum EventRegistry
{
	EV_LogWindowPrint = QEvent::User,
	EV_ProgressDialog,
	EV_RebuildContentPropertyGrid,
	EV_CookWindowPrint,
	EV_DebugConsoleMenuBuilderConnectWindow
};

} // editor
} // tools
