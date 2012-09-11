/*! \file EditorMapEditorObject.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_editor
*/

#pragma once

#include "EditorMapEditorTypes.h"
#include <QtCore/QObject>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {
namespace map_editor {

class ObjectRenderer;

/////////////////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS EditorObject : public QObject {
	Q_OBJECT
public:

	EditorObject(QObject *parent = 0);
	virtual ~EditorObject();

	virtual ObjectRenderer *Renderer() const = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS PersistentObject : EditorObject {
	Q_OBJECT
public:

	PersistentObject(QObject *parent = 0);
	virtual ~PersistentObject();
};

} // map_editor
} // editor
} // tools

#include <Runtime/PopPack.h>
