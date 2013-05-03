/*! \file EditorModelEditorWidget.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#include RADPCH
#include "EditorModelEditorWidget.h"
#include "EditorMainWindow.h"
#include "EditorUtils.h"
#include "EditorGLNavWidget.h"
#include "EditorColorPicker.h"
#include "../../Assets/MeshParser.h"
#include "../../Assets/MeshMaterialLoader.h"
#include "../../Assets/MeshVBLoader.h"
#include "../../Assets/SkModelParser.h"
#include "../../Assets/VtModelParser.h"
#include "../../Renderer/GL/GLState.h"
#include "../../Renderer/GL/GLTexture.h"
#include "../../Renderer/Shader.h"
#include "../../Renderer/Material.h"
#include "../../Renderer/SkMesh.h"
#include "../../Renderer/Mesh.h"
#include "../../Assets/SkMaterialLoader.h"
#include "../../Assets/VtMaterialLoader.h"
#include "../../Assets/MaterialParser.h"
#include "../../SkAnim/SkAnim.h"
#include "../../SkAnim/SkControllers.h"
#include <QtGui/QGridLayout>
#include <QtGui/QSplitter>
#include <QtGui/QWidget>
#include <QtGui/QBoxLayout.h>
#include <QtGui/QFormLayout.h>
#include <QtGui/QGroupBox.h>
#include <QtGui/QCheckBox.h>
#include <QtGui/QTreeWidget>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QDoubleValidator>
#include <Runtime/Base/SIMD.h>
#include <Runtime/Math.h>

#if defined(RAD_OPT_DEBUG)
#define VALIDATE_SIMD_SKIN
#endif

#if defined(VALIDATE_SIMD_SKIN)
const SIMDDriver *SIMD_ref_bind();
#endif

using namespace r;

namespace tools {
namespace editor {

static const float s_kNormalLen = 10.f;

ModelEditorNavWidget::ModelEditorNavWidget(QWidget *parent) : 
GLNavWidget(parent),
m_mode(kMode_None) {
}

void ModelEditorNavWidget::mouseMoveEvent(QMouseEvent *e) {
	if (m_mode == kMode_None) {
		GLNavWidget::mouseMoveEvent(e);
		return;
	}

	QPoint pos = e->pos();
	float dx = pos.x() - m_mp.x();
	float dy = pos.y() - m_mp.y();

	switch (m_mode) {
	case kMode_Orbit:
		emit OnOrbitLight(dx, dy);
		break;
	case kMode_Distance:
		emit OnAdjustLightDistance(dy);
		break;
	case kMode_Radius:
		emit OnAdjustLightRadius(dy);
		break;
	}

	m_mp = pos;
}

void ModelEditorNavWidget::mousePressEvent(QMouseEvent *e) {
	m_mp = e->pos();
	if (e->modifiers() == Qt::ControlModifier) {
		if (e->button() == Qt::LeftButton) {
			m_mode = kMode_Distance;
		} else if (e->button() == Qt::RightButton) {
			m_mode = kMode_Radius;
		}
		return;
	} else if (e->button() == Qt::RightButton) {
		m_mode = kMode_Orbit;
		return;
	}

	m_mode = kMode_None;
	GLNavWidget::mousePressEvent(e);
}

void ModelEditorNavWidget::mouseReleaseEvent(QMouseEvent *e) {
	if (m_mode == kMode_None) {
		GLNavWidget::mouseReleaseEvent(e);
		return;
	}

	m_mode = kMode_None;
}

///////////////////////////////////////////////////////////////////////////////

ModelEditorWidget::ModelEditorWidget(
	const pkg::Asset::Ref &asset,
	bool editable,
	QWidget *parent
) : QWidget(parent), m_asset(asset), m_tree(0), m_glw(0), m_bundle(0) {
	m_skVerts[0] = 0;
	m_skVerts[1] = 0;
}

ModelEditorWidget::~ModelEditorWidget() {
	for (int i = 0; i < 2; ++i) {
		if (m_skVerts[i]) {
			if (m_skModel) {
				for (int k = 0; k < m_skModel->numMeshes; ++k) {
					zone_free(m_skVerts[i][k]);
				}
			} else {
				RAD_ASSERT(m_vtModel);
				for (int k = 0; k < m_vtModel->numMeshes; ++k) {
					zone_free(m_skVerts[i][k]);
				}
			}

			delete [] m_skVerts[i];
		}
	}
}


void ModelEditorWidget::resizeEvent(QResizeEvent *event) {

	if (m_tree)
		return;

	int left, right;
	right = (int)(this->width()*0.85f);
	left = this->width()-right;

	QSplitter *s = new (ZEditor) QSplitter(this);
	s->setOpaqueResize(false);
	s->setOrientation(Qt::Horizontal);

	QWidget *w = new (ZEditor) QWidget();

	QVBoxLayout *vblOuter = new (ZEditor) QVBoxLayout(w);

	QGroupBox *group = new (ZEditor) QGroupBox("Options");
	
	QVBoxLayout *vbl = new (ZEditor) QVBoxLayout(group);
	m_wireframe = new (ZEditor) QCheckBox("Wireframe");
	m_normals = new (ZEditor) QCheckBox("Normals");
	m_tangents = new (ZEditor) QCheckBox("Tangents");

	vbl->addWidget(m_wireframe);
	vbl->addWidget(m_normals);
	vbl->addWidget(m_tangents);

	QGroupBox *lightGroup = new (ZEditor) QGroupBox("Lighting");
	QFormLayout *llayout = new (ZEditor) QFormLayout(lightGroup);

	m_lighting = new (ZEditor) QCheckBox("Enable Lighting");
	llayout->addRow(m_lighting);
	m_showLightRadius = new (ZEditor) QCheckBox("Show Light Radius");
	llayout->addRow(m_showLightRadius);
	m_brightness = new (ZEditor) QLineEdit();
	m_brightness->setValidator(new (ZEditor) QDoubleValidator(m_brightness));
	llayout->addRow("Brightness", m_brightness);
	RAD_VERIFY(connect(m_brightness, SIGNAL(textEdited(const QString&)), SLOT(OnLightBrightnessChanged(const QString&))));
	ColorPicker *colorPicker = new (ZEditor) ColorPicker();
	llayout->addRow("Diffuse Color", colorPicker);
	RAD_VERIFY(connect(colorPicker, SIGNAL(OnColorChanged(const Vec4&)), SLOT(OnLightDiffuseColorChanged(const Vec4&))));
	colorPicker = new (ZEditor) ColorPicker();
	llayout->addRow("Specular Color", colorPicker);
	RAD_VERIFY(connect(colorPicker, SIGNAL(OnColorChanged(const Vec4&)), SLOT(OnLightSpecularColorChanged(const Vec4&))));
	m_specularExponent = new (ZEditor) QLineEdit();
	m_specularExponent->setValidator(new (ZEditor) QDoubleValidator(m_specularExponent));
	llayout->addRow("Specular Exponent", m_specularExponent);
	RAD_VERIFY(connect(m_specularExponent, SIGNAL(textEdited(const QString&)), SLOT(OnLightSpecularExponentChanged(const QString&))));
	vbl->addWidget(lightGroup);
	vblOuter->addWidget(group);
	
	m_tree = new (ZEditor) QTreeWidget();
	m_tree->resize(left, m_tree->height());
	m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
	RAD_VERIFY(connect(m_tree, SIGNAL(itemSelectionChanged()), SLOT(OnItemSelectionChanged())));
	vblOuter->addWidget(m_tree);

	s->addWidget(w);
	
	m_glw = new (ZEditor) ModelEditorNavWidget();
	RAD_VERIFY(connect(m_glw, SIGNAL(OnRenderGL(GLWidget&)), SLOT(OnRenderGL(GLWidget&))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnAdjustLightDistance(float)), SLOT(OnAdjustLightDistance(float))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnAdjustLightRadius(float)), SLOT(OnAdjustLightRadius(float))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnOrbitLight(float, float)), SLOT(OnOrbitLight(float, float))));

	m_glw->resize(right, m_glw->height());
	s->addWidget(m_glw);

	QGridLayout *l = new (ZEditor) QGridLayout(this);
	l->addWidget(s, 0, 0);
}

bool ModelEditorWidget::Load() {
	
	if ((m_asset->type != asset::AT_Mesh) &&
		(m_asset->type != asset::AT_SkModel) &&
		(m_asset->type != asset::AT_VtModel))
		return false;

	m_glw->bindGL(true);

	if (m_asset) {
		
		int r = m_asset->Process(
			xtime::TimeSlice::Infinite,
			pkg::P_Load|pkg::P_FastPath
		);

		if (r != pkg::SR_Success) {
			QMessageBox::critical(
				this,
				"Error",
				QString("Failed to load ") + m_asset->path.get()
			);
			m_asset.reset();
			return false;
		}

		if (m_asset->type == asset::AT_SkModel) {
			m_skModel = r::SkMesh::New(m_asset);
		} else if (m_asset->type == asset::AT_VtModel) {
			m_vtModel = r::VtMesh::New(m_asset);
		} else {
			m_bundle = asset::MeshVBLoader::Cast(m_asset);
		}
	}

	// Load tree view with animstates
	if (m_skModel) {
		QList<QTreeWidgetItem*> items;

		const ska::AnimState::Map &states = *m_skModel->states.get();
		for (ska::AnimState::Map::const_iterator it = states.begin(); it != states.end(); ++it) {
			QTreeWidgetItem *root = new QTreeWidgetItem(QStringList(QString(it->first.c_str.get())));
		
			const ska::AnimState &state = it->second;
			for (ska::Variant::Vec::const_iterator it = state.variants.begin(); it != state.variants.end(); ++it) {
				new QTreeWidgetItem(root, QStringList(QString((*it).name.c_str.get())));
			}

			items.push_back(root);
		}

		m_tree->addTopLevelItems(items);
		m_tree->setHeaderLabel("AnimStates");

		m_skVerts[0] = new float*[m_skModel->numMeshes];

#if defined(VALIDATE_SIMD_SKIN)
		m_skVerts[1] = new float*[m_skModel->numMeshes];
#endif

		for (int i = 0; i < m_skModel->numMeshes; ++i) {
			const ska::DSkMesh *m = m_skModel->DMesh(i);
			m_skVerts[0][i] = (float*)safe_zone_malloc(ZTools, ska::DSkMesh::kNumVertexFloats * m->totalVerts * sizeof(float), 0, SIMDDriver::kAlignment);
#if defined(VALIDATE_SIMD_SKIN)
			m_skVerts[1][i] = (float*)safe_zone_malloc(ZTools, ska::DSkMesh::kNumVertexFloats * m->totalVerts * sizeof(float), 0, SIMDDriver::kAlignment);
#endif
		}

	} else if (m_vtModel) {
		QList<QTreeWidgetItem*> items;

		const ska::AnimState::Map &states = *m_vtModel->states.get();
		for (ska::AnimState::Map::const_iterator it = states.begin(); it != states.end(); ++it) {
			QTreeWidgetItem *root = new QTreeWidgetItem(QStringList(QString(it->first.c_str.get())));
		
			const ska::AnimState &state = it->second;
			for (ska::Variant::Vec::const_iterator it = state.variants.begin(); it != state.variants.end(); ++it) {
				new QTreeWidgetItem(root, QStringList(QString((*it).name.c_str.get())));
			}

			items.push_back(root);
		}

		m_tree->addTopLevelItems(items);
		m_tree->setHeaderLabel("AnimStates");

		m_skVerts[0] = new float*[m_vtModel->numMeshes];

#if defined(VALIDATE_SIMD_SKIN)
		m_skVerts[1] = new float*[m_vtModel->numMeshes];
#endif

		for (int i = 0; i < m_vtModel->numMeshes; ++i) {
			const ska::DVtMesh *m = m_vtModel->DMesh(i);
			m_skVerts[0][i] = (float*)safe_zone_malloc(ZTools, ska::DVtm::kNumVertexFloats * m->numVerts * sizeof(float), 0, SIMDDriver::kAlignment);
#if defined(VALIDATE_SIMD_SKIN)
			m_skVerts[1][i] = (float*)safe_zone_malloc(ZTools, ska::DVtm::kNumVertexFloats * m->numVerts * sizeof(float), 0, SIMDDriver::kAlignment);
#endif
		}

	} else {
		m_tree->setHeaderLabel("N/A");
	}

	m_wireframeMat = LoadMaterial("Sys/DebugWireframe_M");
	if (!m_wireframeMat)
		return false;
	m_lightSphereMat = LoadMaterial("Sys/EditorLightSphere_M");
	if (!m_lightSphereMat)
		return false;
	m_lightRadiusMat = LoadMaterial("Sys/EditorLightRadius_M");
	if (!m_lightRadiusMat)
		return false;

	m_glw->camera->pos = Vec3(300.f, 0.f, 0.f);
	m_glw->camera->LookAt(Vec3::Zero);
	m_glw->camera->fov = 70.f;
	m_glw->SetFreeMode();
	m_glw->kbSpeed = 800.f;
	m_glw->mouseSpeed = 0.3f;
	m_glw->camera->fov = 90.f;

	m_lightSphere = r::Mesh::MakeSphere(ZEditor, false);
	m_lightDfColor = Vec3(1.f,1.f,1.f);
	m_lightSpColor = Vec3(1.f,1.f,1.f);
	m_lightPos = Vec3(65.f, 0.f, 120.f);
	m_lightRadius = 400;
	m_lightBrightness = 2.f;
	m_lightSpecularExp = 32.f;

	m_brightness->setText(QString("%1").arg(m_lightBrightness));
	m_specularExponent->setText(QString("%1").arg(m_lightSpecularExp));

	m_glw->unbindGL();

	return true;
}

pkg::Asset::Ref ModelEditorWidget::LoadMaterial(const char *m) {
	pkg::Asset::Ref asset = Packages()->Resolve(m, pkg::Z_ContentBrowser);
	
	if (!asset) {
		QMessageBox::critical(this, "Error", QString("Failed to load '") + m + QString("'"));
		return asset;
	}
	
	if (asset->type != asset::AT_Material) {
		QMessageBox::critical(this, "Error", QString("Failed to load '") + m + QString("'"));
		return pkg::Asset::Ref();
	}

	int r = asset->Process(xtime::TimeSlice::Infinite, pkg::P_Load);
	RAD_ASSERT(r != pkg::SR_Pending);
	if (r != pkg::SR_Success) {
		QMessageBox::critical(this, "Error", QString("Failed to load '") + m + QString("'"));
		return pkg::Asset::Ref();
	}

	asset->Process(xtime::TimeSlice::Infinite, pkg::P_Trim);

	return asset;
}

void ModelEditorWidget::OnRenderGL(GLWidget &src) {
	if (!(m_skModel || m_vtModel || m_bundle))
		return;

	glClearColor(0.3f, 0.3f, 0.3f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	float vpw = src.width();
	float vph = src.height();

	gl.RotateForCamera(
		m_glw->camera->pos,
		Mat4::Rotation(m_glw->camera->rot.get()),
		1.f,
		16384.f,
		m_glw->camera->fov,
		vph/vpw
	);

	r::Shader::Uniforms u(r::Shader::Uniforms::kDefault);

	if (m_lighting->isChecked()) {
		u.eyePos = m_glw->camera->pos;
		u.lights.numLights = 1;
		u.lights.lights[0].radius = m_lightRadius;
		u.lights.lights[0].brightness = m_lightBrightness;
		u.lights.lights[0].pos = m_lightPos;
		u.lights.lights[0].diffuse = m_lightDfColor;
		u.lights.lights[0].specular = Vec4(m_lightSpColor, m_lightSpecularExp);
		u.lights.lights[0].flags = r::LightDef::kFlag_Diffuse|r::LightDef::kFlag_Specular;
		for (int i = 0; i < r::Material::kNumSorts; ++i) {
			Draw((r::Material::Sort)i, u);
		}
	} else {
		for (int i = 0; i < r::Material::kNumSorts; ++i) {
			Draw((r::Material::Sort)i, u);
		}
	}

	if (m_wireframe->isChecked())
		DrawWireframe();
	if (m_normals->isChecked() || m_tangents->isChecked())
		DrawNormals(m_normals->isChecked(), m_tangents->isChecked());

	if (m_lighting->isChecked())
		DrawLightSphere();
	
	gls.Set(kDepthWriteMask_Enable, -1); // for glClear()
	gls.Commit();
}

void ModelEditorWidget::Draw(
	Material::Sort sort,
	const r::Shader::Uniforms &u
) {

	if (m_skModel) {
		asset::SkMaterialLoader *skMaterials = asset::SkMaterialLoader::Cast(m_asset);
		RAD_ASSERT(skMaterials);
	
		for (int i = 0; i < m_skModel->numMeshes; ++i) {
			asset::MaterialLoader *loader = asset::MaterialLoader::Cast(
				skMaterials->MaterialAsset(i)
			);

			if (!loader)
				continue;

			asset::MaterialParser *parser = asset::MaterialParser::Cast(
				skMaterials->MaterialAsset(i)
			);

			RAD_ASSERT(parser);
			r::Material *mat = parser->material;

			if (mat->sort != sort)
				continue;

			m_skModel->Skin(i);

			mat->BindTextures(loader);
			mat->BindStates();

			if (u.lights.numLights > 0) {
				if (mat->shader->HasPass(Shader::kPass_DiffuseSpecular1)) {
					mat->shader->Begin(Shader::kPass_DiffuseSpecular1, *mat);
				} else if (mat->shader->HasPass(Shader::kPass_Diffuse1)) {
					mat->shader->Begin(Shader::kPass_Diffuse1, *mat);
				} else if (mat->shader->HasPass(Shader::kPass_Specular1)) {
					mat->shader->Begin(Shader::kPass_Specular1, *mat);
				} else {
					mat->shader->Begin(Shader::kPass_Preview, *mat);
				}
			} else {
				mat->shader->Begin(Shader::kPass_Preview, *mat);
			}

			Mesh &m = m_skModel->Mesh(i);
			m.BindAll(0);
			mat->shader->BindStates(u);
			gls.Commit();
			m.Draw();
			mat->shader->End();
		}
	} else if (m_vtModel) {
		asset::VtMaterialLoader *vtMaterials = asset::VtMaterialLoader::Cast(m_asset);
		RAD_ASSERT(vtMaterials);
	
		for (int i = 0; i < m_vtModel->numMeshes; ++i) {
			asset::MaterialLoader *loader = asset::MaterialLoader::Cast(
				vtMaterials->MaterialAsset(i)
			);

			if (!loader)
				continue;

			asset::MaterialParser *parser = asset::MaterialParser::Cast(
				vtMaterials->MaterialAsset(i)
			);

			RAD_ASSERT(parser);
			r::Material *mat = parser->material;

			if (mat->sort != sort)
				continue;

			m_vtModel->Skin(i);

			mat->BindTextures(loader);
			mat->BindStates();
			mat->shader->Begin(Shader::kPass_Preview, *mat);
			Mesh &m = m_vtModel->Mesh(i);
			m.BindAll(0);
			mat->shader->BindStates(u);
			gls.Commit();
			m.Draw();
			mat->shader->End();
		}
	} else if (m_bundle) {
		for (int i = 0; i < m_bundle->numMeshes; ++i) {
			asset::MeshMaterialLoader *mbLoader = asset::MeshMaterialLoader::Cast(m_asset);
			asset::MaterialLoader *loader = asset::MaterialLoader::Cast(
				mbLoader->MaterialAsset(i)
			);

			if (!loader)
				continue;

			asset::MaterialParser *parser = asset::MaterialParser::Cast(
				mbLoader->MaterialAsset(i)
			);

			RAD_ASSERT(parser);
			r::Material *mat = parser->material;

			if (mat->sort != sort)
				continue;

			mat->BindTextures(loader);
			mat->BindStates();
			mat->shader->Begin(Shader::kPass_Preview, *mat);
			Mesh &m = *m_bundle->Mesh(i);
			m.BindAll(0);
			mat->shader->BindStates(u);
			gls.Commit();
			m.Draw();
			mat->shader->End();
		}
	}
}

void ModelEditorWidget::DrawWireframe() {

	r::gl.wireframe = true;

	asset::MaterialLoader *loader = asset::MaterialLoader::Cast(m_wireframeMat);
	asset::MaterialParser *parser = asset::MaterialParser::Cast(m_wireframeMat);

	r::Material *mat = parser->material;

	if (m_skModel) {
		asset::SkMaterialLoader *skMaterials = asset::SkMaterialLoader::Cast(m_asset);
		RAD_ASSERT(skMaterials);
	
		for (int i = 0; i < m_skModel->numMeshes; ++i) {
			m_skModel->Skin(i);

			mat->BindTextures(loader);
			mat->BindStates();
			mat->shader->Begin(Shader::kPass_Preview, *mat);
			Mesh &m = m_skModel->Mesh(i);
			m.BindAll(0);
			mat->shader->BindStates();
			gls.Commit();
			m.Draw();
			mat->shader->End();
		}
	} else if (m_vtModel) {
		asset::VtMaterialLoader *vtMaterials = asset::VtMaterialLoader::Cast(m_asset);
		RAD_ASSERT(vtMaterials);
	
		for (int i = 0; i < m_vtModel->numMeshes; ++i) {
			m_vtModel->Skin(i);

			mat->BindTextures(loader);
			mat->BindStates();
			mat->shader->Begin(Shader::kPass_Preview, *mat);
			Mesh &m = m_vtModel->Mesh(i);
			m.BindAll(0);
			mat->shader->BindStates();
			gls.Commit();
			m.Draw();
			mat->shader->End();
		}
	} else {
		for (int i = 0; i < m_bundle->numMeshes; ++i) {
			mat->BindTextures(loader);
			mat->BindStates();
			mat->shader->Begin(Shader::kPass_Preview, *mat);
			Mesh &m = *m_bundle->Mesh(i);
			m.BindAll(0);
			mat->shader->BindStates();
			gls.Commit();
			m.Draw();
			mat->shader->End();
		}
	}

	r::gl.wireframe = false;
}

void ModelEditorWidget::DrawNormals(bool normals, bool tangents) {
	if (m_skModel) {
		DrawSkaNormals(normals, tangents);
	} else if (m_vtModel) {
		DrawVtmNormals(normals, tangents);
	} else {
		DrawMeshNormals(normals, tangents);
	}
}

void ModelEditorWidget::DrawSkaNormals(bool normals, bool tangents) {
	
	gls.DisableAllMGSources();
	gls.DisableAllMTSources();
	gls.DisableTextures();
	gls.DisableVertexAttribArrays();
	gls.UseProgram(0);
	gls.Set(r::kDepthTest_Less|r::kDepthWriteMask_Disable, r::kBlendMode_Off);
	gls.Commit();

	for (int i = 0; i < m_skModel->numMeshes; ++i) {
		const ska::DSkMesh *m = m_skModel->DMesh(i);

		m_skModel->SkinToBuffer(*SIMD, i, m_skVerts[0][i]);
#if defined(VALIDATE_SIMD_SKIN)
		// Compare the C reference implementation with optimized path:
		static const SIMDDriver *SIMD_ref = SIMD_ref_bind();
		m_skModel->SkinToBuffer(*SIMD_ref, i, m_skVerts[1][i]);
		const float *src = m_skVerts[0][i];
		const float *cmp = m_skVerts[1][i];
		for (int z = 0; z < (int)m->totalVerts; ++z) {
			float d[4];

			for (int j = 0; j < 3; ++j)
				d[j] = math::Abs(src[j]-cmp[j]);

			if (d[0] > 0.1f ||
				d[1] > 0.1f ||
				d[2] > 0.1f) {
					int b = 0; // bad vertex
			}

			src += 4;
			cmp += 4;

			for (int j = 0; j < 3; ++j)
				d[j] = math::Abs(src[j]-cmp[j]);

			if (d[0] > 0.1f ||
				d[1] > 0.1f ||
				d[2] > 0.1f) {
					int b = 0; // bad normal
			}

			src += 4;
			cmp += 4;

			for (int j = 0; j < 4; ++j) {
				d[j] = math::Abs(src[j]-cmp[j]);
			}

			if (d[0] > 0.1f ||
				d[1] > 0.1f ||
				d[2] > 0.1f ||
				d[3] != 0.f) {
					int b = 0; // bad tangent
			}

			src += 4;
			cmp += 4;
		}
#endif

		const float *verts = (const float*)m_skVerts[0][i];

		for (int i = 0; i < (int)m->totalVerts; ++i) {
			Vec3 v(verts[0], verts[1], verts[2]);
			Vec3 n(verts[4], verts[5], verts[6]);
			
			if (tangents) {
				const float *f = verts + 8;

				for (U16 k = 0; k < m->numChannels; ++k) {
					Vec3 t(f[0], f[1], f[2]);
					Vec3 b = f[3] * n.Cross(t);
					
					t = v + (t*s_kNormalLen);
					b = v + (b*s_kNormalLen);

					glBegin(GL_LINES);
					glColor4f(1.f, 0.f, 0.f, 1.f);
					glVertex3f(v[0], v[1], v[2]);
					glVertex3f(t[0], t[1],t[2]);
					glColor4f(0.f, 0.f, 1.f, 1.f);
					glVertex3f(v[0], v[1], v[2]);
					glVertex3f(b[0], b[1],b[2]);
					glEnd();

					f += 4;
				}
			}

			if (normals) {
				glColor4f(0.f, 1.f, 0.f, 1.f);
				n = v + (n*s_kNormalLen);
				glBegin(GL_LINES);
				glVertex3f(v[0], v[1], v[2]);
				glVertex3f(n[0], n[1], n[2]);
				glEnd();
			}

			verts += ska::DSkMesh::kNumVertexFloats;
		}
	}
}

void ModelEditorWidget::DrawVtmNormals(bool normals, bool tangents) {
	
	gls.DisableAllMGSources();
	gls.DisableAllMTSources();
	gls.DisableTextures();
	gls.DisableVertexAttribArrays();
	gls.UseProgram(0);
	gls.Set(r::kDepthTest_Less|r::kDepthWriteMask_Disable, r::kBlendMode_Off);
	gls.Commit();

	for (int i = 0; i < m_vtModel->numMeshes; ++i) {
		const ska::DVtMesh *m = m_vtModel->DMesh(i);

		m_vtModel->SkinToBuffer(*SIMD, i, m_skVerts[0][i]);
#if defined(VALIDATE_SIMD_SKIN)
		// Compare the C reference implementation with optimized path:
		static const SIMDDriver *SIMD_ref = SIMD_ref_bind();
		m_vtModel->SkinToBuffer(*SIMD_ref, i, m_skVerts[1][i]);
		const float *src = m_skVerts[0][i];
		const float *cmp = m_skVerts[1][i];
		for (int z = 0; z < (int)m->numVerts; ++z) {
			float d[4];

			for (int j = 0; j < 3; ++j)
				d[j] = math::Abs(src[j]-cmp[j]);

			if (d[0] > 0.1f ||
				d[1] > 0.1f ||
				d[2] > 0.1f) {
					int b = 0; // bad vertex
			}

			src += 4;
			cmp += 4;

			for (int j = 0; j < 3; ++j)
				d[j] = math::Abs(src[j]-cmp[j]);

			if (d[0] > 0.1f ||
				d[1] > 0.1f ||
				d[2] > 0.1f) {
					int b = 0; // bad normal
			}

			src += 4;
			cmp += 4;

			for (int j = 0; j < 4; ++j) {
				d[j] = math::Abs(src[j]-cmp[j]);
			}

			if (d[0] > 0.1f ||
				d[1] > 0.1f ||
				d[2] > 0.1f ||
				d[3] != 0.f) {
					int b = 0; // bad tangent
			}

			src += 4;
			cmp += 4;
		}
#endif

		const float *verts = (const float*)m_skVerts[0][i];

		for (int i = 0; i < (int)m->numVerts; ++i) {
			Vec3 v(verts[0], verts[1], verts[2]);
			Vec3 n(verts[4], verts[5], verts[6]);
			
			if (tangents) {
				const float *f = verts + 8;

				for (U16 k = 0; k < m->numChannels; ++k) {
					Vec3 t(f[0], f[1], f[2]);
					Vec3 b = f[3] * n.Cross(t);
					
					t = v + (t*s_kNormalLen);
					b = v + (b*s_kNormalLen);

					glBegin(GL_LINES);
					glColor4f(1.f, 0.f, 0.f, 1.f);
					glVertex3f(v[0], v[1], v[2]);
					glVertex3f(t[0], t[1],t[2]);
					glColor4f(0.f, 0.f, 1.f, 1.f);
					glVertex3f(v[0], v[1], v[2]);
					glVertex3f(b[0], b[1],b[2]);
					glEnd();

					f += 4;
				}
			}

			if (normals) {
				glColor4f(0.f, 1.f, 0.f, 1.f);
				n = v + (n*s_kNormalLen);
				glBegin(GL_LINES);
				glVertex3f(v[0], v[1], v[2]);
				glVertex3f(n[0], n[1], n[2]);
				glEnd();
			}

			verts += ska::DVtm::kNumVertexFloats;
		}
	}
}

void ModelEditorWidget::DrawMeshNormals(bool normals, bool tangents) {
	asset::MeshParser *parser = asset::MeshParser::Cast(m_asset);
	if (!parser)
		return;

	const asset::DMeshBundle *bundle = parser->bundle;

	gls.DisableAllMGSources();
	gls.DisableAllMTSources();
	gls.DisableTextures();
	gls.DisableVertexAttribArrays();
	gls.UseProgram(0);
	gls.Set(r::kDepthTest_Less|r::kDepthWriteMask_Disable, r::kBlendMode_Off);
	gls.Commit();

	for (asset::DMesh::Vec::const_iterator it = bundle->meshes.begin(); it != bundle->meshes.end(); ++it) {
		const asset::DMesh &m = *it;

		const float *verts = (const float*)m.vertices;

		for (int i = 0; i < (int)m.numVerts; ++i) {
			Vec3 v(verts[0], verts[1], verts[2]);
			Vec3 n(verts[3], verts[4], verts[5]);
			
			if (tangents) {
				const float *f = verts + 6;

				for (U16 k = 0; k < m.numChannels; ++k) {
					Vec3 t(f[0], f[1], f[2]);
					Vec3 b = f[3] * n.Cross(t);
					
					t = v + (t*s_kNormalLen);
					b = v + (b*s_kNormalLen);

					glBegin(GL_LINES);
					glColor4f(1.f, 0.f, 0.f, 1.f);
					glVertex3f(v[0], v[1], v[2]);
					glVertex3f(t[0], t[1],t[2]);
					glColor4f(0.f, 0.f, 1.f, 1.f);
					glVertex3f(v[0], v[1], v[2]);
					glVertex3f(b[0], b[1],b[2]);
					glEnd();

					f += 4;
				}
			}

			if (normals) {
				glColor4f(0.f, 1.f, 0.f, 1.f);
				n = v + (n*s_kNormalLen);
				glBegin(GL_LINES);
				glVertex3f(v[0], v[1], v[2]);
				glVertex3f(n[0], n[1], n[2]);
				glEnd();
			}

			verts += asset::DMesh::kNumVertexFloats;
		}
	}
}

void ModelEditorWidget::DrawLightSphere() {

	asset::MaterialLoader *loader = asset::MaterialLoader::Cast(m_lightSphereMat);
	asset::MaterialParser *parser = asset::MaterialParser::Cast(m_lightSphereMat);
	r::Material *mat = parser->material;

	r::gl.MatrixMode(GL_MODELVIEW);
	r::gl.PushMatrix();
	r::gl.Translatef(m_lightPos[0], m_lightPos[1], m_lightPos[2]);
	r::gl.Scalef(8.f, 8.f, 8.f);

	mat->BindTextures(loader);
	mat->BindStates();
	mat->shader->Begin(Shader::kPass_Preview, *mat);
	m_lightSphere->BindAll(0);
	mat->shader->BindStates(r::Shader::Uniforms(Vec4(m_lightDfColor, 1.f)));
	gls.Commit();
	m_lightSphere->Draw();
	mat->shader->End();

	r::gl.PopMatrix();

	if (m_showLightRadius->isChecked()) {
		loader = asset::MaterialLoader::Cast(m_lightRadiusMat);
		parser = asset::MaterialParser::Cast(m_lightRadiusMat);
		mat = parser->material;

		r::gl.MatrixMode(GL_MODELVIEW);
		r::gl.PushMatrix();
		r::gl.Translatef(m_lightPos[0], m_lightPos[1], m_lightPos[2]);
		r::gl.Scalef(m_lightRadius, m_lightRadius, m_lightRadius);

		mat->BindTextures(loader);
		mat->BindStates();
		mat->shader->Begin(Shader::kPass_Preview, *mat);
		m_lightSphere->BindAll(0);
		mat->shader->BindStates();
		gls.Commit();
		m_lightSphere->Draw();
		mat->shader->End();

		r::gl.PopMatrix();
	}
}

void ModelEditorWidget::Tick(float dt) {
	
	if (m_skModel) {
		m_skModel->ska->Tick(dt, -1.f, true, true, Mat4::Identity);

		asset::SkMaterialLoader *skMaterials = asset::SkMaterialLoader::Cast(m_asset);
		RAD_ASSERT(skMaterials);

		for (int i = 0; i < m_skModel->numMeshes; ++i) {
			asset::MaterialParser *parser = asset::MaterialParser::Cast(
				skMaterials->MaterialAsset(i)
			);

			RAD_ASSERT(parser);
			parser->material->Sample(App::Get()->time, dt);
		}
	} else if (m_vtModel) {
		m_vtModel->vtm->Tick(dt, true, false);
		
		asset::VtMaterialLoader *vtMaterials = asset::VtMaterialLoader::Cast(m_asset);
		RAD_ASSERT(vtMaterials);

		for (int i = 0; i < m_vtModel->numMeshes; ++i) {
			asset::MaterialParser *parser = asset::MaterialParser::Cast(
				vtMaterials->MaterialAsset(i)
			);

			RAD_ASSERT(parser);
			parser->material->Sample(App::Get()->time, dt);
		}
	} else if (m_bundle) {
		asset::MeshMaterialLoader * mbMaterials = asset::MeshMaterialLoader::Cast(m_asset);
		for (int i = 0; i < mbMaterials->numUniqueMaterials; ++i) {
			asset::MaterialParser *parser = asset::MaterialParser::Cast(
				mbMaterials->UniqueMaterialAsset(i)
			);

			RAD_ASSERT(parser);
			parser->material->Sample(App::Get()->time, dt);
		}
	}

	m_glw->TickCamera(dt);
	m_glw->updateGL();
}

void ModelEditorWidget::OnItemSelectionChanged() {
	QList<QTreeWidgetItem*> s = m_tree->selectedItems();

	if (m_skModel) {
		if (s.empty()) {
			m_skModel->ska->root = ska::Controller::Ref();
			return;
		}

		QTreeWidgetItem *i = s.first();

		ska::Controller::Ref target;

		if (i->parent()) { 
			// animation
			ska::Animation::Map::const_iterator it = m_skModel->ska->anims->find(
				String(i->text(0).toAscii().constData())
			);
		
			if (it != m_skModel->ska->anims->end())
			{
				ska::AnimationSource::Ref animSource = ska::AnimationSource::New(
					*it->second,
					1.f,
					1.f,
					1.f,
					0,
					ska::AnimState::kMoveType_None,
					*m_skModel->ska.get(),
					ska::Notify::Ref()
				);

				target = boost::static_pointer_cast<ska::Controller>(animSource);
			}
		} else { 
			// state
			ska::AnimState::Map::const_iterator it = m_skModel->states->find(
				String(i->text(0).toAscii().constData())
			);

			if (it != m_skModel->states->end()) {
				ska::AnimationVariantsSource::Ref animSource = ska::AnimationVariantsSource::New(
					it->second,
					*m_skModel->ska.get(),
					ska::Notify::Ref()
				);

				target = boost::static_pointer_cast<ska::Controller>(animSource);
			}
		}

		if (target) {
			if (!m_skModel->ska->root.get()) {
				// attach blend controller.
				ska::BlendToController::Ref blendTo = ska::BlendToController::New(*m_skModel->ska.get(), ska::Notify::Ref());
				m_skModel->ska->root = boost::static_pointer_cast<ska::Controller>(blendTo);
				blendTo->Activate();
			}

			boost::static_pointer_cast<ska::BlendToController>(m_skModel->ska->root.get())->BlendTo(target);
		} else {
			m_skModel->ska->root = ska::Controller::Ref();
		}
	} else if (m_vtModel) {
		if (s.empty()) {
			m_vtModel->vtm->root = ska::Controller::Ref();
			return;
		}

		QTreeWidgetItem *i = s.first();

		ska::Controller::Ref target;

		if (i->parent()) { 
			// animation
			ska::Animation::Map::const_iterator it = m_vtModel->vtm->anims->find(
				String(i->text(0).toAscii().constData())
			);
		
			if (it != m_vtModel->vtm->anims->end())
			{
				ska::AnimationSource::Ref animSource = ska::AnimationSource::New(
					*it->second,
					1.f,
					1.f,
					1.f,
					0,
					*m_vtModel->vtm.get(),
					ska::Notify::Ref()
				);

				target = boost::static_pointer_cast<ska::Controller>(animSource);
			}
		} else { 
			// state
			ska::AnimState::Map::const_iterator it = m_vtModel->states->find(
				String(i->text(0).toAscii().constData())
			);

			if (it != m_vtModel->states->end()) {
				ska::AnimationVariantsSource::Ref animSource = ska::AnimationVariantsSource::New(
					it->second,
					*m_vtModel->vtm.get(),
					ska::Notify::Ref()
				);

				target = boost::static_pointer_cast<ska::Controller>(animSource);
			}
		}

		if (target) {
			if (!m_vtModel->vtm->root.get()) {
				// attach blend controller.
				ska::BlendToController::Ref blendTo = ska::BlendToController::New(*m_vtModel->vtm.get(), ska::Notify::Ref());
				m_vtModel->vtm->root = boost::static_pointer_cast<ska::Controller>(blendTo);
				blendTo->Activate();
			}

			boost::static_pointer_cast<ska::BlendToController>(m_vtModel->vtm->root.get())->BlendTo(target);
		} else {
			m_vtModel->vtm->root = ska::Controller::Ref();
		}
	}
}

void ModelEditorWidget::OnAdjustLightDistance(float distance) {
	distance *= 3.f;
	float d = m_lightPos.Magnitude();
	d += distance;
	if (d < 10.f)
		d = 10.f;
	m_lightPos.Normalize();
	m_lightPos *= d;
}

void ModelEditorWidget::OnAdjustLightRadius(float adjust) {
	m_lightRadius += adjust * 3.f;
	if (m_lightRadius < 64.f)
		m_lightRadius = 64.f;
}

void ModelEditorWidget::OnOrbitLight(float x, float y) {
	x *= math::Constants<float>::PI() / 1000.f;
	y *= math::Constants<float>::PI() / 1000.f;

	Quat a(Vec3(0,0,1), x);

	Vec3 localX;
	Vec3 up(0,0,1);

	Vec3 dir(m_lightPos);
	dir.Normalize();

	if (up.Dot(dir) > 0.999f) {
		up = Vec3(0,1,0);
	}

	localX = up.Cross(dir);

	Quat b(localX, y);

	Mat4 m = Mat4::Rotation(b * a);
	m_lightPos = m * m_lightPos;
}

void ModelEditorWidget::OnLightBrightnessChanged(const QString &text) {
	m_lightBrightness = text.toFloat();
}

void ModelEditorWidget::OnLightSpecularExponentChanged(const QString &text) {
	m_lightSpecularExp = text.toFloat();
}

void ModelEditorWidget::OnLightDiffuseColorChanged(const Vec4 &rgba) {
	m_lightDfColor = rgba;
}

void ModelEditorWidget::OnLightSpecularColorChanged(const Vec4 &rgba) {
	m_lightSpColor = rgba;
}

} // editor
} // tools

#include "moc_EditorModelEditorWidget.cc"

