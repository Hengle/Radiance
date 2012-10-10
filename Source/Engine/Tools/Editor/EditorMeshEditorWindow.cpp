// EditorMeshEditorWindow.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorMeshEditorWindow.h"
#include "EditorMainWindow.h"
#include "EditorUtils.h"
#include "EditorGLNavWidget.h"
#include "../../Renderer/GL/GLState.h"
#include "../../Renderer/GL/GLTexture.h"
#include "../../Renderer/Shader.h"
#include "../../Renderer/Material.h"
#include "../../Assets/MeshMaterialLoader.h"
#include "../../Assets/MaterialParser.h"
#include <QtGui/QGridLayout>
#include <QtGui/QWidget>
#include <QtGui/QMessageBox>

using namespace r;

namespace tools {
namespace editor {

void MeshEditorWindow::LaunchEditor(int assetId)
{
	(new (ZEditor) MeshEditorWindow())->Load(assetId);
}

MeshEditorWindow::MeshEditorWindow()
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
	
	m_glw = new (ZEditor) GLNavWidget(this);
	RAD_VERIFY(connect(m_glw, SIGNAL(OnRenderGL(GLWidget&)), SLOT(OnRenderGL(GLWidget&))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnRenderGL(GLWidget&)), SLOT(OnRenderGL(GLWidget&))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnInitializeGL(GLWidget&)), SLOT(OnInitializeGL(GLWidget&))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnResizeGL(GLWidget&, int, int)), SLOT(OnResizeGL(GLWidget&, int, int))));

	QGridLayout *l = new (ZEditor) QGridLayout(this);
	l->addWidget(m_glw, 0, 0);
}

MeshEditorWindow::~MeshEditorWindow()
{
}

void MeshEditorWindow::Load(int id)
{
	show();

	m_glw->bindGL(true);

	pkg::Asset::Ref asset = Packages()->Asset(id, pkg::Z_Unique);
	if (asset)
	{
		setWindowTitle(QString("Viewing ") + asset->path.get());
		
		int r = asset->Process(
			xtime::TimeSlice::Infinite,
			pkg::P_Load|pkg::P_FastPath
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

		m_bundle = r::MeshBundle::New(asset);
	}

	m_glw->unbindGL();
	m_glw->camera->pos = Vec3(300.f, 0.f, 0.f);
	m_glw->camera->LookAt(Vec3::Zero);
	m_glw->camera->fov = 70.f;
	m_glw->SetOrbitMode(Vec3::Zero, GLNavWidget::kOrbitMode_LeftButton);
	m_glw->orbitSpeed = 1.f;
	m_glw->lookSpeed = 0.5f;
	m_glw->wheelSpeed = 4.f;
}

void MeshEditorWindow::OnRenderGL(GLWidget &src)
{
	if (!m_bundle)
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

void MeshEditorWindow::Draw(Material::Sort sort)
{
	for (int i = 0; i < m_bundle->numMeshes; ++i)
	{
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

void MeshEditorWindow::OnResizeGL(GLWidget &src, int width, int height)
{
}

void MeshEditorWindow::OnInitializeGL(GLWidget &src)
{
}

void MeshEditorWindow::OnTick(float dt)
{
	if (!m_bundle)
		return;

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

	m_glw->updateGL();
}


void MeshEditorWindow::closeEvent(QCloseEvent *e)
{
	e->accept(); // assume we can close.
	emit OnClose(e);
	if (e->isAccepted())
	{
		emit Closing();
		DoClose();
	}
}

void MeshEditorWindow::MainWinClose(QCloseEvent *e)
{
	e->setAccepted(close());
}

void MeshEditorWindow::DoClose()
{
}

} // editor
} // tools

#include "moc_EditorMeshEditorWindow.cc"

