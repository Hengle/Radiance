/*! \file MapBuilderDebugUI.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#pragma once

#include "../../Types.h"
#include "../../Camera.h"
#include <Runtime/PushPack.h>

class QAction;
class QIcon;

namespace tools {
namespace editor {

class PopupMenu;

} // editor
} // tools

namespace tools {

class RADENG_CLASS MapBuilderDebugUI {
public:

	virtual ~MapBuilderDebugUI() {}

	RAD_DECLARE_PROPERTY(MapBuilderDebugUI, enabled, bool, bool);
	RAD_DECLARE_READONLY_PROPERTY(MapBuilderDebugUI, camera, Camera*);

	virtual void SetDebugMenu(editor::PopupMenu *menu) = 0;

	virtual QAction *AddDebugMenuAction(
		editor::PopupMenu &menu,
		const char *path
	) = 0;

	virtual QAction *AddDebugMenuAction(
		editor::PopupMenu &menu,
		const QIcon &icon,
		const char *path
	) = 0;

protected:

	virtual RAD_DECLARE_GET(enabled, bool) = 0;
	virtual RAD_DECLARE_SET(enabled, bool) = 0;

	virtual RAD_DECLARE_GET(camera, Camera*) = 0;

};

} // tools

#include <Runtime/PopPack.h>
