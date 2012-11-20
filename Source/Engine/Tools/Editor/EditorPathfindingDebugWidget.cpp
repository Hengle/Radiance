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
#include "../../MathUtils.h"
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

	if (m_move)
		DrawMove(m_move, Vec4(0.f, 1.f, 0.f, 1.f));
}

void PathfindingDebugWidget::mousePressEvent(QMouseEvent *e) {

	if (!m_loaded)
		return;

	if (e->modifiers() & Qt::ControlModifier) {
		int idx = (e->button() == Qt::LeftButton) ? 0 : 1;

		Vec3 start;
		Vec3 end;

		Project(e->pos().x(), e->pos().y(), start, end);

		world::FloorPosition pos;
		if (m_floors.ClipToFloor(start, end, pos)) {
			m_validPos[idx] = true;
			m_pos[idx] = pos;

			if (m_validPos[0] && m_validPos[1])
				m_move = m_floors.CreateMove(m_pos[0], m_pos[1]);
		}
	} else {
		GLNavWidget::mousePressEvent(e);
	}
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
			m_bsp = m_map->bspFile;
			m_floors.Load(m_bsp);
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

	for (U32 i = 0; i < m_bsp->numEntities; ++i) {
		const char *sz = StringForKey(i, "classname");
		if (sz && (CStr(sz) == kClass))
			return (int)i;
	}

	return -1;
}

const char *PathfindingDebugWidget::StringForKey(int entityNum, const char *key) {
	const world::bsp_file::BSPEntity *entity = m_bsp->Entities() + entityNum;

	const String kKey(CStr(key));

	for (U32 i = 0; i < entity->numStrings; ++i) {
		const char *name = m_bsp->String(entity->firstString + (i*2));
		const char *value = m_bsp->String(entity->firstString + (i*2) + 1);
		
		const String kName(CStr(name));
		if (kName == kKey)
			return value;
	}

	return 0;
}

void PathfindingDebugWidget::DrawFloors() {

	const Vec4 normal(1, 0, 0, 1);
	const Vec4 highlight(1, 1, 0, 1);

	for (U32 i = 0; i < m_bsp->numFloors; ++i)
		DrawFloor(i, normal);

	if (m_validPos[0]) {
		const world::bsp_file::BSPFloor *floor = m_bsp->Floors() + m_pos[0].floor;
		DrawFloorTri(m_pos[0].tri, highlight);
	}

	if (m_validPos[1]) {
		const world::bsp_file::BSPFloor *floor = m_bsp->Floors() + m_pos[1].floor;
		DrawFloorTri(m_pos[1].tri, highlight);
	}
}

void PathfindingDebugWidget::DrawFloor(int floorNum, const Vec4 &color) {
	const world::bsp_file::BSPFloor *floor = m_bsp->Floors() + floorNum;

	for (U32 i = 0; i < floor->numTris; ++i)
		DrawFloorTri((int)(floor->firstTri) + i, color);
}

void PathfindingDebugWidget::DrawFloorTri(int triNum, const Vec4 &color) {
	const world::bsp_file::BSPFloorTri *tri = m_bsp->FloorTris() + triNum;

	glBegin(GL_LINES);
	for (int i = 0; i < 3; ++i) {
		int n = (i+1) % 3;
		const world::bsp_file::BSPVertex *v0 = m_bsp->Vertices() + tri->verts[i];
		const world::bsp_file::BSPVertex *v1 = m_bsp->Vertices() + tri->verts[n];
		const world::bsp_file::BSPFloorEdge *edge = m_bsp->FloorEdges() + tri->edges[i];

		if ((edge->tris[0] == -1 || edge->tris[1] == -1)) {
			glColor4f(0.f, 0.f, 1.f, 1.f);
		} else {
			glColor4f(color[0], color[1], color[2], color[3]);
		}

		glVertex3f(v0->v[0], v0->v[1], v0->v[2]);
		glVertex3f(v1->v[0], v1->v[1], v1->v[2]);
	}
	glEnd();
}

void PathfindingDebugWidget::DrawMove(const ::world::FloorMove::Ref &move, const Vec4 &color) {
	glColor4f(color[0], color[1], color[2], color[3]);

	const world::FloorMove::Route *route = move->route;
	for (world::FloorMove::Step::Vec::const_iterator it = route->steps->begin(); it != route->steps->end(); ++it) {
		DrawSpline((*it).path);
	}
}

void PathfindingDebugWidget::DrawSpline(const ::world::FloorMove::Spline &spline) {
	const world::FloorMove::Spline::Point *points = spline.points;

	glBegin(GL_LINES);
	for (int i = 0; i < world::FloorMove::Spline::kNumPts-1; ++i) {
		glVertex3f(points[i].pos[0], points[i].pos[1], points[i].pos[2]);
		glVertex3f(points[i+1].pos[0], points[i+1].pos[1], points[i+1].pos[2]);
	}
	glEnd();
}

void PathfindingDebugWidget::Project(int x, int y, Vec3 &start, Vec3 &end) {
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

	int viewport[4] = {0, 0, (int)vpw, (int)vph};

	start = ::Unproject(
		gl.GetModelViewProjectionMatrix(),
		viewport,
		Vec3((float)x, (float)y, 0.f)
	);

	end = ::Unproject(
		gl.GetModelViewProjectionMatrix(),
		viewport,
		Vec3((float)x, (float)y, 1.f)
	);
}

} // editor
} // tools

#include "moc_EditorPathfindingDebugWidget.cc"
