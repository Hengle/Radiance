/*! \file EditorModelEditorWidget.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once

#include "../../Types.h"
#include <QtGui/QWidget>
#include <QtCore/QPoint>
#include "EditorGLNavWidget.h"
#include "../../Renderer/Mesh.h"
#include "../../Renderer/SkMesh.h"
#include "../../Renderer/VtMesh.h"
#include "../../Renderer/Material.h"
#include "../../Renderer/Shader.h"
#include <Runtime/PushPack.h>

class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;
class QLabel;
class QLineEdit;

namespace asset {
class MeshVBLoader;
class MeshMaterialLoader;
}

namespace tools {
namespace editor {

class RADENG_CLASS ModelEditorNavWidget : public GLNavWidget {
	Q_OBJECT
public:
	ModelEditorNavWidget(QWidget *parent = 0);

signals:

	void OnAdjustLightDistance(float distance);
	void OnAdjustLightRadius(float adjust);
	void OnOrbitLight(float x, float y);

protected:

	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);

private:

	enum Mode {
		kMode_None,
		kMode_Orbit,
		kMode_Distance,
		kMode_Radius
	};

	QPoint m_mp;
	Mode m_mode;
};

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
	void OnItemSelectionChanged();
	void OnAdjustLightDistance(float distance);
	void OnAdjustLightRadius(float adjust);
	void OnOrbitLight(float x, float y);
	void OnLightBrightnessChanged(const QString &text);
	void OnLightSpecularExponentChanged(const QString &text);
	void OnLightDiffuseColorChanged(const Vec4 &rgba);
	void OnLightSpecularColorChanged(const Vec4 &rgba);

private:

	pkg::Asset::Ref LoadMaterial(const char *m);

	void Draw(
		r::Material::Sort sort,
		const r::Shader::Uniforms &u
	);

	void DrawLightSphere();
	void DrawWireframe();
	void DrawNormals(bool normals, bool tangents);
	
	void DrawSkaNormals(bool normals, bool tangents);
	void DrawVtmNormals(bool normals, bool tangents);
	void DrawMeshNormals(bool normals, bool tangents);
	
	ModelEditorWidget();
	virtual ~ModelEditorWidget();

	void Load(int id);
	void DoClose();

	Vec3 m_lightDfColor;
	Vec3 m_lightSpColor;
	Vec3 m_lightPos;
	float m_lightRadius;
	float m_lightBrightness;
	float m_lightSpecularExp;

	r::Mesh::Ref m_lightSphere;
	r::SkMesh::Ref m_skModel;
	r::VtMesh::Ref m_vtModel;
	asset::MeshVBLoader *m_bundle;
	pkg::Asset::Ref m_asset;
	pkg::Asset::Ref m_wireframeMat;
	pkg::Asset::Ref m_lightSphereMat;
	pkg::Asset::Ref m_lightRadiusMat;
	ModelEditorNavWidget *m_glw;
	QTreeWidget *m_tree;
	QCheckBox *m_wireframe;
	QCheckBox *m_normals;
	QCheckBox *m_tangents;
	QCheckBox *m_lighting;
	QCheckBox *m_showLightRadius;
	QLineEdit *m_brightness;
	QLineEdit *m_specularExponent;
	float **m_skVerts[2];
};

} // editor
} // tools

#include <Runtime/PopPack.h>
