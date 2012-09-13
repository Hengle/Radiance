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

namespace tools {
namespace editor {

class PopupMenu;

} // editor
} // tools

namespace tools {

class RADENG_CLASS MapBuilderDebugUI {
public:

	virtual ~MapBuilderDebugUI() {}

	RAD_DECLARE_PROPERTY(MapBuilderDebugUI, enable, bool, bool);
	RAD_DECLARE_READONLY_PROPERTY(MapBuilderDebugUI, camera, Camera*);

	virtual void SetDebugMenu(editor::PopupMenu *menu) = 0;

protected:

	virtual RAD_DECLARE_GET(enable, bool) = 0;
	virtual RAD_DECLARE_SET(enable, bool) = 0;

	virtual RAD_DECLARE_GET(camera, Camera*) = 0;

};

} // tools

#include <Runtime/PopPack.h>
