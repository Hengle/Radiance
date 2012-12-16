/*! \file EditorModelEditorWidget.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once

#include "../../Types.h"
#include <QtGui/QWidget>
#include "../../Renderer/Mesh.h"
#include "../../Renderer/SkMesh.h"
#include "../../Renderer/Material.h"
#include <Runtime/PushPack.h>

class QTreeWidget;
class QTreeWidgetItem;

namespace tools {
namespace editor {

class GLWidget;
class GLNavWidget;

class RADENG_CLASS ModelEditorWidget : public QWidget {
	Q_OBJECT
public:

	ModelEditorWidget(
		const pkg::Asset::Ref &asset,
		bool editable,
		QWidget *parent = 0
	);
	
	bool Load();
	void Tick(float dt);

protected:

	virtual void resizeEvent(QResizeEvent *event);

private slots:

	void OnRenderGL(GLWidget &src);
	void ItemSelectionChanged();

private:

	void Draw(r::Material::Sort sort);

	ModelEditorWidget();
	virtual ~ModelEditorWidget();

	void Load(int id);
	void DoClose();

	r::SkMesh::Ref m_skModel;
	r::MeshBundle::Ref m_bundle;
	pkg::Asset::Ref m_asset;
	GLNavWidget *m_glw;
	QTreeWidget *m_tree;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
