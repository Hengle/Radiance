// EditorSkModelEditorWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include <QtGui/QWidget>
#include <QtGui/QCloseEvent>
#include "../../Renderer/SkMesh.h"
#include "../../Renderer/Material.h"
#include <Runtime/PushPack.h>

class QTreeWidget;
class QTreeWidgetItem;

namespace tools {
namespace editor {

class GLWidget;
class GLNavWidget;

class RADENG_CLASS SkModelEditorWindow : public QWidget
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
	void OnRenderGL(GLWidget &src);
	void OnResizeGL(GLWidget &src, int width, int height);
	void OnInitializeGL(GLWidget &src);
	void OnTick(float dt);
	void ItemSelectionChanged();

private:

	void Draw(r::Material::Sort sort);

	SkModelEditorWindow();
	virtual ~SkModelEditorWindow();

	void Load(int id);
	void DoClose();

	r::SkMesh::Ref m_mesh;
	GLNavWidget *m_glw;
	QTreeWidget *m_tree;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
