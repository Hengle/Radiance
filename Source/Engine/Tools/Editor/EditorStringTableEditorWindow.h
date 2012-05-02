// EditorStringTableEditorWindow.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorWindow.h"
#include "EditorStringTableWidget.h"
#include <Runtime/PushPack.h>

class QCloseEvent;

namespace tools {
namespace editor {

class RADENG_CLASS StringTableEditorWindow : public EditorWindow {
	Q_OBJECT
public:

	StringTableEditorWindow(
		const pkg::Asset::Ref &asset,
		bool editable,
		WidgetStyle style,
		QWidget *parent
	);

};

} // editor
} // tools

#include <Runtime/PopPack.h>
