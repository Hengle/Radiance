// EditorSkModelEditorWindow.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorSkModelEditorWindow.h"
#include "EditorMainWindow.h"
#include "EditorUtils.h"
#include "EditorGLNavWidget.h"
#include "../../Renderer/GL/GLState.h"
#include "../../Renderer/GL/GLTexture.h"
#include "../../Renderer/Shader.h"
#include "../../Renderer/Material.h"
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

void SkModelEditorWindow::LaunchEditor(int assetId)
{
	(new (ZEditor) SkModelEditorWindow())->Load(assetId);
}

SkModelEditorWindow::SkModelEditorWindow()
: QWidget(0, 
	Qt::Window|
	Qt::CustomizeWindowHint|
	Qt::WindowTitleHint|
	Qt::WindowSystemMenuHint|
	Qt::WindowMinMaxButtonsHint|
	Qt::WindowCloseButtonHint
)
{
	setAttribute(Qt::WA_DeleteOnClose);
	PercentSize(*this, *MainWindow::Get(), 0.85f, 0.85f);
	CenterWidget(*this, *MainWindow::Get());
	RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(OnClose(QCloseEvent*)), SLOT(MainWinClose(QCloseEvent*))));
	RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(OnTick(float)), SLOT(OnTick(float))));

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
	RAD_VERIFY(connect(m_glw, SIGNAL(OnRenderGL(GLWidget&)), SLOT(OnRenderGL(GLWidget&))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnInitializeGL(GLWidget&)), SLOT(OnInitializeGL(GLWidget&))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnResizeGL(GLWidget&, int, int)), SLOT(OnResizeGL(GLWidget&, int, int))));

	m_glw->resize(right, m_glw->height());
	s->addWidget(m_glw);

	QGridLayout *l = new (ZEditor) QGridLayout(this);
	l->addWidget(s, 0, 0);
}

SkModelEditorWindow::~SkModelEditorWindow()
{
}

void SkModelEditorWindow::Load(int id)
{
	show();

	m_glw->bindGL(true);

	pkg::Asset::Ref asset = Packages()->Asset(id, pkg::Z_Unique);
	if (asset)
	{
		setWindowTitle(QString("Viewing ") + asset->path.get());
		
		int r = asset->Process(
			xtime::TimeSlice::Infinite,
			pkg::P_Load
		);

		if (r != pkg::SR_Success)
		{
			QMessageBox::critical(
				this,
				"Error",
				QString("Failed to load ") + asset->path.get()
			);
			asset.reset();
			close();
			return;
		}

		m_mesh = r::SkMesh::New(asset);
	}

	// Load tree view with animstates
	QList<QTreeWidgetItem*> items;

	const ska::AnimState::Map &states = *m_mesh->states.get();
	for (ska::AnimState::Map::const_iterator it = states.begin(); it != states.end(); ++it)
	{
		QTreeWidgetItem *root = new QTreeWidgetItem(QStringList(QString(it->first.c_str.get())));
		
		const ska::AnimState &state = it->second;
		for (ska::Variant::Vec::const_iterator it = state.variants.begin(); it != state.variants.end(); ++it)
		{
			new QTreeWidgetItem(root, QStringList(QString((*it).name.c_str.get())));
		}

		items.push_back(root);
	}

	m_tree->addTopLevelItems(items);
	m_tree->setHeaderLabel("AnimStates");

	m_glw->unbindGL();
	m_glw->camera->pos = Vec3(300.f, 0.f, 0.f);
	m_glw->camera->LookAt(Vec3::Zero);
	m_glw->camera->fov = 70.f;
	m_glw->SetOrbitMode(Vec3::Zero, GLNavWidget::OM_LeftButton);
	m_glw->orbitSpeed = 1.f;
	m_glw->lookSpeed = 0.5f;
	m_glw->wheelSpeed = 4.f;
}

void SkModelEditorWindow::OnRenderGL(GLWidget &src)
{
	if (!m_mesh)
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

	for (int i = r::Material::S_Solid; i < r::Material::NumSorts; ++i)
	{
		Draw((r::Material::Sort)i);
	}

	gls.Set(DWM_Enable, -1); // for glClear()
	gls.Commit();
}

void SkModelEditorWindow::Draw(Material::Sort sort)
{
	asset::SkMaterialLoader::Ref skMaterials = asset::SkMaterialLoader::Cast(m_mesh->asset);
	RAD_ASSERT(skMaterials);
	
	for (int i = 0; i < m_mesh->numMeshes; ++i)
	{
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

		m_mesh->Skin(i);

		mat->BindTextures(loader);
		mat->BindStates();
		mat->shader->Begin(Shader::P_Default, *mat);
		Mesh &m = m_mesh->Mesh(i);
		m.BindAll(0);
		mat->shader->BindStates();
		gls.Commit();
		m.Draw();
		mat->shader->End();
	}
}

void SkModelEditorWindow::OnResizeGL(GLWidget &src, int width, int height)
{
}

void SkModelEditorWindow::OnInitializeGL(GLWidget &src)
{
}

void SkModelEditorWindow::OnTick(float dt)
{
	if (!m_mesh)
		return;

	ska::BoneTM unused;
	m_mesh->ska->Tick(dt, true, true, Mat4::Identity, ska::Ska::MT_None, unused);

	asset::SkMaterialLoader::Ref skMaterials = asset::SkMaterialLoader::Cast(m_mesh->asset);
	RAD_ASSERT(skMaterials);

	for (int i = 0; i < m_mesh->numMeshes; ++i)
	{
		asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(
			skMaterials->MaterialAsset(i)
		);

		RAD_ASSERT(parser);
		parser->material->Sample(App::Get()->time, dt);
	}

	m_glw->updateGL();
}

void SkModelEditorWindow::ItemSelectionChanged()
{
	QList<QTreeWidgetItem*> s = m_tree->selectedItems();

	if (s.empty())
	{
		m_mesh->ska->root = ska::Controller::Ref();
		return;
	}

	QTreeWidgetItem *i = s.first();

	ska::Controller::Ref target;

	if (i->parent())
	{ // animation
		ska::Animation::Map::const_iterator it = m_mesh->ska->anims->find(
			String(i->text(0).toAscii().constData())
		);
		
		if (it != m_mesh->ska->anims->end())
		{
			ska::AnimationSource::Ref animSource = ska::AnimationSource::New(
				*it->second,
				1.f,
				1.f,
				1.f,
				0,
				true,
				*m_mesh->ska.get(),
				ska::Notify::Ref()
			);

			target = boost::static_pointer_cast<ska::Controller>(animSource);
		}
	}
	else
	{ // state
		ska::AnimState::Map::const_iterator it = m_mesh->states->find(
			String(i->text(0).toAscii().constData())
		);

		if (it != m_mesh->states->end())
		{
			ska::AnimationVariantsSource::Ref animSource = ska::AnimationVariantsSource::New(
				it->second.variants,
				*m_mesh->ska.get(),
				ska::Notify::Ref()
			);

			target = boost::static_pointer_cast<ska::Controller>(animSource);
		}
	}

	if (target)
	{
		if (!m_mesh->ska->root.get())
		{
			// attach blend controller.
			ska::BlendToController::Ref blendTo = ska::BlendToController::New(*m_mesh->ska.get(), ska::Notify::Ref());
			m_mesh->ska->root = boost::static_pointer_cast<ska::Controller>(blendTo);
			blendTo->Activate();
		}

		boost::static_pointer_cast<ska::BlendToController>(m_mesh->ska->root.get())->BlendTo(target);
	}
	else
	{
		m_mesh->ska->root = ska::Controller::Ref();
	}
}

void SkModelEditorWindow::closeEvent(QCloseEvent *e)
{
	e->accept(); // assume we can close.
	emit OnClose(e);
	if (e->isAccepted())
	{
		emit Closing();
		DoClose();
	}
}

void SkModelEditorWindow::MainWinClose(QCloseEvent *e)
{
	e->setAccepted(close());
}

void SkModelEditorWindow::DoClose()
{
}

} // editor
} // tools

#include "moc_EditorSkModelEditorWindow.cc"

