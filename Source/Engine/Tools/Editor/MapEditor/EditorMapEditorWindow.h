/*! \file EditorMapEditorWindow.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_editor
*/

#pragma once

#include "../EditorWindow.h"
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {
namespace map_editor {

class RADENG_CLASS MapEditorWindow : public EditorWindow {
	Q_OBJECT
public:

	MapEditorWindow(
		const pkg::Asset::Ref &asset,
		bool editable,
		WidgetStyle style,
		QWidget *parent
	);

	virtual ~MapEditorWindow();
};

} // map_editor
} // editor
} // tools

#include <Runtime/PopPack.h>
