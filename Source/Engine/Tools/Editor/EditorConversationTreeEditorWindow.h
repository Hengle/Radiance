/*! \file EditorConversationTreeEditorWindow.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup editor
*/

#pragma once

#include "EditorWindow.h"
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class RADENG_CLASS ConversationTreeEditorWindow : public EditorWindow {
	Q_OBJECT
public:

	ConversationTreeEditorWindow(
		const pkg::Asset::Ref &asset,
		bool editable,
		WidgetStyle style,
		QWidget *parent
	);

};

} // editor
} // tools

#include <Runtime/PopPack.h>
