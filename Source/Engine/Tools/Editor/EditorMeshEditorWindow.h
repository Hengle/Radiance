// EditorMeshEditorWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include <QtGui/QWidget>
#include <QtGui/QCloseEvent>
#include "../../Renderer/Mesh.h"
#include "../../Renderer/Material.h"
#include <Runtime/PushPack.h>

class QTreeWidget;
class QTreeWidgetItem;

namespace tools {
namespace editor {

class GLWidget;
class GLNavWidget;

class RADENG_CLASS MeshEditorWindow : public QWidget {
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
	void OnRenderGL(GLWidget &src);
	void OnResizeGL(GLWidget &src, int width, int height);
	void OnInitializeGL(GLWidget &src);
	void OnTick(float dt);

private:

	void Draw(r::Material::Sort sort);

	MeshEditorWindow();
	virtual ~MeshEditorWindow();

	void Load(int id);
	void DoClose();

	r::MeshBundle::Ref m_bundle;
	GLNavWidget *m_glw;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
