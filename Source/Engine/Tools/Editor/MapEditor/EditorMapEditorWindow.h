// EditorMapEditorWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include <QtGui/QWidget>
#include <QtOpenGL/QGLContext>
#include <QtOpenGL/QGLWidget>
#include <QtGui/QCloseEvent>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class RADENG_CLASS MapEditorWindow : public QWidget
{
	Q_OBJECT
public:

	static void LaunchEditor(int assetId);

signals:

	void OnClose(QCloseEvent *e);
	void Closing();

protected:

	virtual void closeEvent(QCloseEvent *e);

private slots:

	void MainWinClose(QCloseEvent *e);

private:

	void Load(int id);
	void DoClose();

	MapEditorWindow();
	virtual ~MapEditorWindow();
};

} // editor
} // tools

#include <Runtime/PopPack.h>
