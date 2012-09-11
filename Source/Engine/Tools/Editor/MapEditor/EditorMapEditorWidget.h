/*! \file EditorMapEditorWidget.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_editor
*/

#pragma once

#include "../EditorTypes.h"
#include <QtGui/QWidget>
#include <QtOpenGL/QGLContext>
#include <QtOpenGL/QGLWidget>
#include <QtGui/QCloseEvent>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {
namespace map_editor {

class RADENG_CLASS MapEditorWidget : public QWidget {
	Q_OBJECT
public:

};

} // map_editor
} // editor
} // tools

#include <Runtime/PopPack.h>
