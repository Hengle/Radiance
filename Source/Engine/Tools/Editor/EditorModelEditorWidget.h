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
#include "../../Renderer/VtMesh.h"
#include "../../Renderer/Material.h"
#include <Runtime/PushPack.h>

class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;

namespace asset {
class MeshVBLoader;
class MeshMaterialLoader;
}

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

	pkg::Asset::Ref LoadMaterial(const char *m);

	void Draw(r::Material::Sort sort);
	void DrawWireframe();
	void DrawNormals(bool normals, bool tangents);
	
	void DrawSkaNormals(bool normals, bool tangents);
	void DrawVtmNormals(bool normals, bool tangents);
	void DrawMeshNormals(bool normals, bool tangents);
	
	ModelEditorWidget();
	virtual ~ModelEditorWidget();

	void Load(int id);
	void DoClose();

	r::SkMesh::Ref m_skModel;
	r::VtMesh::Ref m_vtModel;
	asset::MeshVBLoader *m_bundle;
	pkg::Asset::Ref m_asset;
	pkg::Asset::Ref m_wireframeMat;
	pkg::Asset::Ref m_normalMat;
	pkg::Asset::Ref m_tangentMat;
	GLNavWidget *m_glw;
	QTreeWidget *m_tree;
	QCheckBox *m_wireframe;
	QCheckBox *m_normals;
	QCheckBox *m_tangents;
	float **m_skVerts[2];
};

} // editor
} // tools

#include <Runtime/PopPack.h>
