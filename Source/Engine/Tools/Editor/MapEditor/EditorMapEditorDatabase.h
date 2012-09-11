/*! \file EditorMapEditorDatabase.h
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

class RADENG_CLASS EditorDatabase : public QObject {
	Q_OBJECT
public:

	EditorDatabase(QObject *parent = 0);
	virtual ~EditorDatabase();

	void Load(const pkg::AssetRef &asset);

signals:

	void OnLoadingError();
	void OnLoadingComplete();

};

} // map_editor
} // editor
} // tools

#include <Runtime/PopPack.h>
