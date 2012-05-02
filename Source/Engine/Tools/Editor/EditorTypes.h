// EditorTypes.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include <QtCore/Qt>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

enum WidgetStyle {
	WS_Dialog,
	WS_Window,
	WS_Widget
};

inline Qt::WindowType WindowTypeForStyle(WidgetStyle style) {

	Qt::WindowType type = Qt::Widget;

	switch (style) {
		case WS_Dialog:
			type = Qt::Dialog;
			break;
		case WS_Window:
			type = Qt::Window;
			break;
		default:
			break;
	}

	return type;
}

} // editor
} // tools

#include <Runtime/PopPack.h>
