/*! \file EditorPathfindingDebugWidget.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup editor
*/

#include RADPCH
#include "EditorMainWindow.h"
#include "EditorPathfindingDebugWidget.h"
#include "EditorUtils.h"
#include "../../Packages/Packages.h"
#include "../../App.h"
#include "../../Engine.h"
#include "../../Renderer/GL/GLState.h"
#include <QtCore/QPoint>
#include <QtGui/QMessageBox>

using namespace r;

namespace tools {
namespace editor {

PathfindingDebugWidget::PathfindingDebugWidget(QWidget *parent, Qt::WindowFlags f) : 
GLNavWidget(parent, f), m_loaded(false), m_progress(0) {
	RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(Closing()), SLOT(close())));
	RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(OnTick(float)), SLOT(OnTick(float))));

	SetFreeMode();
	GLNavWidget::kbSpeed = 15.f;
	GLNavWidget::mouseSpeed = 0.5f;
	GLNavWidget::camera->fov = 90.f;

	m_validPos[0] = m_validPos[1] = false;
}

PathfindingDebugWidget::~PathfindingDebugWidget() {
}

void PathfindingDebugWidget::DebugMap(int id) {
	pkg::Asset::Ref asset = Packages()->Asset(id, pkg::Z_Unique);
	if (!asset) {
		QMessageBox::critical(this, "Error", "Unable to find map asset!");
		close();
		return;
	}

	m_progress = new (ZEditor) ProgressDialog(
		"Compiling",
		QString(),
		QString(),
		0,
		0,
		this
	);
	m_progress->setMinimumDuration(500);

	m_map = asset::MapAsset::Cast(asset);
	m_map->SetProgressIndicator(m_progress);
	
	int r = asset->Process(
		xtime::TimeSlice(100),
		pkg::P_Parse
	);

	if (r < pkg::SR_Success) {
		QMessageBox::critical(this, "Error", QString("Error loading map %1").arg(asset->path.get()));
		close();
		return;
	}

	m_asset = asset;
}

void PathfindingDebugWidget::renderGL() {
	glClearColor(0.3f, 0.3f, 0.3f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if (!m_loaded)
		return;

	float vpw = width();
	float vph = height();

	gl.RotateForCamera(
		camera->pos,
		Mat4::Rotation(camera->rot.get()),
		1.f,
		16384.f,
		camera->fov,
		vph/vpw
	);

	gls.Set(DT_Disable, BM_Off);
	gls.Commit();

	DrawFloors();
}

void PathfindingDebugWidget::mousePressEvent(QMouseEvent *e) {
	GLNavWidget::mousePressEvent(e);
}

void PathfindingDebugWidget::OnTick(float dt) {
	if (m_loaded) {
		GLNavWidget::TickCamera(dt);
		updateGL();
		return;
	}

	if (m_asset) {
		if (m_progress && !m_progress->isVisible())
			m_progress->show();

		int r = m_asset->Process(
			xtime::TimeSlice(100),
			pkg::P_Parse
		);

		if (r == pkg::SR_Success) {
			LoadPlayerStart();
			m_loaded = true;
			m_progress->close();
			m_progress = 0;
		} else if (r < pkg::SR_Success) {
			m_asset.reset();
			m_map.reset();
			QMessageBox::critical(this, "Error", "Error loading map.");
			m_progress->close();
			m_progress = 0;
			close();
		}
	}
}

void PathfindingDebugWidget::LoadPlayerStart() {
	int entityNum = FindEntityByClass("info_player_start");
	if (entityNum == -1)
		return;

	const char *sz = StringForKey(entityNum, "origin");
	if (!sz)
		return;

	float origin[3];
	sscanf(sz, "%f %f %f", &origin[0], &origin[1], &origin[2]);

	camera->pos = Vec3(origin[0], origin[1], origin[2]);
}

int PathfindingDebugWidget::FindEntityByClass(const char *classname) {
	const String kClass(CStr(classname));

	for (U32 i = 0; i < m_map->bspFile->numEntities; ++i) {
		const char *sz = StringForKey(i, "classname");
		if (sz && (CStr(sz) == kClass))
			return (int)i;
	}

	return -1;
}

const char *PathfindingDebugWidget::StringForKey(int entityNum, const char *key) {
	const world::bsp_file::BSPEntity *entity = m_map->bspFile->Entities() + entityNum;

	const String kKey(CStr(key));

	for (U32 i = 0; i < entity->numStrings; ++i) {
		const char *name = m_map->bspFile->String(entity->firstString + (i*2));
		const char *value = m_map->bspFile->String(entity->firstString + (i*2) + 1);
		
		const String kName(CStr(name));
		if (kName == kKey)
			return value;
	}

	return 0;
}

void PathfindingDebugWidget::DrawFloors() {

	const Vec4 normal(1, 0, 0, 1);
	const Vec4 highlight(1, 1, 0, 1);

	for (U32 i = 0; i < m_map->bspFile->numFloors; ++i)
		DrawFloor(i, normal);

	if (m_validPos[0]) {
		const world::bsp_file::BSPFloor *floor = m_map->bspFile->Floors() + m_pos[0].floor;
		DrawFloorTri((int)(m_pos[0].tri + floor->firstTri), highlight);
	}

	if (m_validPos[1]) {
		const world::bsp_file::BSPFloor *floor = m_map->bspFile->Floors() + m_pos[1].floor;
		DrawFloorTri((int)(m_pos[1].tri + floor->firstTri), highlight);
	}
}

void PathfindingDebugWidget::DrawFloor(int floorNum, const Vec4 &color) {
	const world::bsp_file::BSPFloor *floor = m_map->bspFile->Floors() + floorNum;

	for (U32 i = 0; i < floor->numTris; ++i)
		DrawFloorTri((int)(floor->firstTri) + i, color);
}

void PathfindingDebugWidget::DrawFloorTri(int triNum, const Vec4 &color) {
	const world::bsp_file::BSPFloorTri *tri = m_map->bspFile->FloorTris() + triNum;

	glColor4f(color[0], color[1], color[2], color[3]);

	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < 3; ++i) {
		const world::bsp_file::BSPVertex *v = m_map->bspFile->Vertices() + tri->verts[i];
		glVertex3f(v->v[0], v->v[1], v->v[2]);
	}
	glEnd();
}

void PathfindingDebugWidget::DrawSpline(const ::world::FloorMove::Spline &spline) {
}

} // editor
} // tools

#include "moc_EditorPathfindingDebugWidget.cc"
