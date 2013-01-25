/*! \file EditorModelEditorWidget.h
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
#include "../../Assets/MeshParser.h"
#include "../../Assets/SkModelParser.h"
#include "../../Renderer/GL/GLState.h"
#include "../../Renderer/GL/GLTexture.h"
#include "../../Renderer/Shader.h"
#include "../../Renderer/Material.h"
#include "../../Renderer/SkMesh.h"
#include "../../Renderer/Mesh.h"
#include "../../Assets/SkMaterialLoader.h"
#include "../../Assets/MaterialParser.h"
#include "../../SkAnim/SkAnim.h"
#include "../../SkAnim/SkControllers.h"
#include <QtGui/QGridLayout>
#include <QtGui/QSplitter>
#include <QtGui/QWidget>
#include <QtGui/QBoxLayout.h>
#include <QtGui/QGroupBox.h>
#include <QtGui/QCheckBox.h>
#include <QtGui/QTreeWidget>
#include <QtGui/QMessageBox>
#include <Runtime/Base/SIMD.h>

#if defined(RAD_OPT_DEBUG)
//#define VALIDATE_SIMD_SKIN
#endif

#if defined(VALIDATE_SIMD_SKIN)
const SIMDDriver *SIMD_ref_bind();
#endif

using namespace r;

namespace tools {
namespace editor {

static const float s_kNormalLen = 10.f;

ModelEditorWidget::ModelEditorWidget(
	const pkg::Asset::Ref &asset,
	bool editable,
	QWidget *parent
) : QWidget(parent), m_asset(asset), m_tree(0), m_glw(0) {
	m_skVerts[0] = 0;
	m_skVerts[1] = 0;
}

ModelEditorWidget::~ModelEditorWidget() {
	for (int i = 0; i < 2; ++i) {
		if (m_skVerts[i]) {
			RAD_ASSERT(m_skModel);
			for (int k = 0; k < m_skModel->numMeshes; ++k) {
				zone_free(m_skVerts[i][k]);
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

	QSplitter *s = new QSplitter(this);
	s->setOpaqueResize(false);
	s->setOrientation(Qt::Horizontal);

	QWidget *w = new QWidget();

	QVBoxLayout *vblOuter = new QVBoxLayout(w);

	QGroupBox *group = new QGroupBox("Options");
	
	QVBoxLayout *vbl = new QVBoxLayout(group);
	m_wireframe = new QCheckBox("Wireframe");
	m_normals = new QCheckBox("Normals");
	m_tangents = new QCheckBox("Tangents");

	vbl->addWidget(m_wireframe);
	vbl->addWidget(m_normals);
	vbl->addWidget(m_tangents);

	vblOuter->addWidget(group);
	
	m_tree = new QTreeWidget();
	m_tree->resize(left, m_tree->height());
	m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
	RAD_VERIFY(connect(m_tree, SIGNAL(itemSelectionChanged()), SLOT(ItemSelectionChanged())));
	vblOuter->addWidget(m_tree);

	s->addWidget(w);
	
	m_glw = new (ZEditor) GLNavWidget();
	RAD_VERIFY(connect(m_glw, SIGNAL(OnRenderGL(GLWidget&)), SLOT(OnRenderGL(GLWidget&))));
	
	m_glw->resize(right, m_glw->height());
	s->addWidget(m_glw);

	QGridLayout *l = new (ZEditor) QGridLayout(this);
	l->addWidget(s, 0, 0);
}

bool ModelEditorWidget::Load() {
	
	if ((m_asset->type != asset::AT_Mesh) &&
		(m_asset->type != asset::AT_SkModel))
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
		} else {
			m_bundle = r::MeshBundle::New(m_asset);
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
			const ska::DMesh *m = m_skModel->DMesh(i);
			m_skVerts[0][i] = (float*)safe_zone_malloc(ZTools, ska::DMesh::kNumVertexFloats * m->totalVerts * sizeof(float), 0, SIMDDriver::kAlignment);
#if defined(VALIDATE_SIMD_SKIN)
			m_skVerts[1][i] = (float*)safe_zone_malloc(ZTools, ska::DMesh::kNumVertexFloats * m->totalVerts * sizeof(float), 0, SIMDDriver::kAlignment);
#endif
		}

	} else {
		m_tree->setHeaderLabel("N/A");
	}

	m_wireframeMat = LoadMaterial("Sys/DebugWireframe_M");
	if (!m_wireframeMat)
		return false;
	m_normalMat = LoadMaterial("Sys/DebugNormals_M");
	if (!m_normalMat)
		return false;
	m_tangentMat = LoadMaterial("Sys/DebugTangents_M");
	if (!m_tangentMat)
		return false;

	m_glw->unbindGL();
	m_glw->camera->pos = Vec3(300.f, 0.f, 0.f);
	m_glw->camera->LookAt(Vec3::Zero);
	m_glw->camera->fov = 70.f;
	m_glw->SetFreeMode();
	m_glw->kbSpeed = 8.f;
	m_glw->mouseSpeed = 0.3f;
	m_glw->camera->fov = 90.f;

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
	if (!(m_skModel || m_bundle))
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

	for (int i = 0; i < r::Material::kNumSorts; ++i) {
		Draw((r::Material::Sort)i);
	}

	if (m_wireframe->isChecked())
		DrawWireframe();
	if (m_normals->isChecked() || m_tangents->isChecked())
		DrawNormals(m_normals->isChecked(), m_tangents->isChecked());
	
	gls.Set(kDepthWriteMask_Enable, -1); // for glClear()
	gls.Commit();
}

void ModelEditorWidget::Draw(Material::Sort sort) {

	if (m_skModel) {
		asset::SkMaterialLoader::Ref skMaterials = asset::SkMaterialLoader::Cast(m_skModel->asset);
		RAD_ASSERT(skMaterials);
	
		for (int i = 0; i < m_skModel->numMeshes; ++i) {
			asset::MaterialLoader::Ref loader = asset::MaterialLoader::Cast(
				skMaterials->MaterialAsset(i)
			);

			if (!loader)
				continue;

			asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(
				skMaterials->MaterialAsset(i)
			);

			RAD_ASSERT(parser);
			r::Material *mat = parser->material;

			if (mat->sort != sort)
				continue;

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
	} else {
		for (int i = 0; i < m_bundle->numMeshes; ++i) {
			asset::MaterialLoader::Ref loader = asset::MaterialLoader::Cast(
				m_bundle->MaterialAsset(i)
			);

			if (!loader)
				continue;

			asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(
				m_bundle->MaterialAsset(i)
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
			mat->shader->BindStates();
			gls.Commit();
			m.Draw();
			mat->shader->End();
		}
	}
}

void ModelEditorWidget::DrawWireframe() {

	r::gl.wireframe = true;

	asset::MaterialLoader::Ref loader = asset::MaterialLoader::Cast(m_wireframeMat);
	asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(m_wireframeMat);

	r::Material *mat = parser->material;

	if (m_skModel) {
		asset::SkMaterialLoader::Ref skMaterials = asset::SkMaterialLoader::Cast(m_skModel->asset);
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
		const ska::DMesh *m = m_skModel->DMesh(i);

		m_skModel->SkinToBuffer(SIMD, i, m_skVerts[0][i]);
#if defined(VALIDATE_SIMD_SKIN)
		// Compare the C reference implementation with optimized path:
		static const SIMDDriver *SIMD_ref = SIMD_ref_bind();
		m_skModel->SkinToBuffer(SIMD_ref, i, m_skVerts[1][i]);
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

			verts += ska::DMesh::kNumVertexFloats;
		}
	}
}

void ModelEditorWidget::DrawMeshNormals(bool normals, bool tangents) {
	asset::MeshParser::Ref parser = asset::MeshParser::Cast(m_asset);
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
				const float *f = verts + 10;

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

			verts += ska::DMesh::kNumVertexFloats;
		}
	}
}

void ModelEditorWidget::Tick(float dt) {
	
	if (m_skModel) {
		ska::BoneTM unused;
		m_skModel->ska->Tick(dt, -1.f, true, true, Mat4::Identity);

		asset::SkMaterialLoader::Ref skMaterials = asset::SkMaterialLoader::Cast(m_skModel->asset);
		RAD_ASSERT(skMaterials);

		for (int i = 0; i < m_skModel->numMeshes; ++i) {
			asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(
				skMaterials->MaterialAsset(i)
			);

			RAD_ASSERT(parser);
			parser->material->Sample(App::Get()->time, dt);
		}
	} else {
		asset::MeshMaterialLoader::Ref meshMaterials = asset::MeshMaterialLoader::Cast(m_bundle->asset);
		RAD_ASSERT(meshMaterials);

		for (int i = 0; i < meshMaterials->numUniqueMaterials; ++i) {
			asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(
				meshMaterials->UniqueMaterialAsset(i)
			);

			RAD_ASSERT(parser);
			parser->material->Sample(App::Get()->time, dt);
		}
	}

	m_glw->TickCamera(dt);
	m_glw->updateGL();
}

void ModelEditorWidget::ItemSelectionChanged() {
	if (!m_skModel)
		return;

	QList<QTreeWidgetItem*> s = m_tree->selectedItems();

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
}

} // editor
} // tools

#include "moc_EditorModelEditorWidget.cc"

