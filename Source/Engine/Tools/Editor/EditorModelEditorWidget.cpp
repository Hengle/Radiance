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
#include <QtGui/QTreeWidget>
#include <QtGui/QMessageBox>

using namespace r;

namespace tools {
namespace editor {

ModelEditorWidget::ModelEditorWidget(
	const pkg::Asset::Ref &asset,
	bool editable,
	QWidget *parent
) : QWidget(parent), m_asset(asset), m_tree(0), m_glw(0) {
	
}

ModelEditorWidget::~ModelEditorWidget() {
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

	m_tree = new QTreeWidget(this);
	m_tree->resize(left, m_tree->height());
	m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
	RAD_VERIFY(connect(m_tree, SIGNAL(itemSelectionChanged()), SLOT(ItemSelectionChanged())));
	s->addWidget(m_tree);
	
	m_glw = new (ZEditor) GLNavWidget(this);
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
	} else {
		m_tree->setHeaderLabel("N/A");
	}

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

	for (int i = r::Material::S_Solid; i < r::Material::NumSorts; ++i) {
		Draw((r::Material::Sort)i);
	}

	gls.Set(DWM_Enable, -1); // for glClear()
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
			mat->shader->Begin(Shader::P_Default, *mat);
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
			mat->shader->Begin(Shader::P_Default, *mat);
			Mesh &m = *m_bundle->Mesh(i);
			m.BindAll(0);
			mat->shader->BindStates();
			gls.Commit();
			m.Draw();
			mat->shader->End();
		}
	}
}

void ModelEditorWidget::Tick(float dt) {
	
	if (m_skModel) {
		ska::BoneTM unused;
		m_skModel->ska->Tick(dt, true, true, Mat4::Identity, ska::Ska::MT_None, unused);

		asset::SkMaterialLoader::Ref skMaterials = asset::SkMaterialLoader::Cast(m_skModel->asset);
		RAD_ASSERT(skMaterials);

		for (int i = 0; i < m_skModel->numMeshes; ++i)
		{
			asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(
				skMaterials->MaterialAsset(i)
			);

			RAD_ASSERT(parser);
			parser->material->Sample(App::Get()->time, dt);
		}
	} else {
		asset::MeshMaterialLoader::Ref meshMaterials = asset::MeshMaterialLoader::Cast(m_bundle->asset);
		RAD_ASSERT(meshMaterials);

		for (int i = 0; i < meshMaterials->numUniqueMaterials; ++i)
		{
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
				true,
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

		if (it != m_skModel->states->end())
		{
			ska::AnimationVariantsSource::Ref animSource = ska::AnimationVariantsSource::New(
				it->second.variants,
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

