/*! \file SolidBSPDraw.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#include RADPCH

#include "SolidBSP.h"
#include "../../../COut.h"
#include "../../../Tools/Editor/EditorUtils.h"
#include "../../../Tools/Editor/EditorPopupMenu.h"
#include "../../../Renderer/GL/GLState.h"
#include <QtGui/QAction>

using namespace r;

namespace tools {
namespace solid_bsp {

///////////////////////////////////////////////////////////////////////////////

void BSPBuilder::PaintHandler::Init(MapBuilderDebugUI &ui, BSPBuilder &bsp) {
	// find info_player_start
	const String k_info_player_start(CStr("info_player_start"));

	SceneFile::Entity::Ref playerStart;

	for (SceneFile::Entity::Vec::const_iterator it = bsp.m_map->ents.begin(); it != bsp.m_map->ents.end(); ++it) {
		const SceneFile::Entity::Ref &e = *it;

		const char *sz = e->keys.StringForKey("classname");
		if (sz && (CStr(sz) == k_info_player_start)) {
			playerStart = e;
			break;
		}
	}

	if (!playerStart)
		return;

	SceneFile::Vec3 origin = playerStart->keys.Vec3ForKey("origin");
	ui.camera->MakeIdentity();
	ui.camera->pos = origin;
}

void BSPBuilder::PaintHandler::EnableSmoothShading() {
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glShadeModel( GL_SMOOTH );

	float dir0[4]  = { 0.4f, 0.7f, 1.0f, 0.0f };
	float amb0[4]  = { 0.2f, 0.2f, 0.2f, 1.0f };
	float diff0[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	
	glLightfv( GL_LIGHT0, GL_POSITION, dir0 );
	glLightfv( GL_LIGHT0, GL_DIFFUSE, diff0 );
	glEnable(GL_LIGHT0);

	float dir1[4]  = { -0.4f, -0.7f, -1.0f, 0.0f };
	float diff1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	
	glLightfv( GL_LIGHT1, GL_POSITION, dir1 );
	glLightfv( GL_LIGHT1, GL_DIFFUSE, diff1 );
	glEnable(GL_LIGHT1);

	glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE );

	float c[4] = {1, 1, 1, 1};
	glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, c );
	glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, c );
}

void BSPBuilder::PaintHandler::DisableSmoothShading() {
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHT1);
}

void BSPBuilder::PaintHandler::BeginPaint(const QRect &viewport, MapBuilderDebugUI &ui, bool backfaces) {
	float vpw = viewport.width();
	float vph = viewport.height();

	gl.RotateForCamera(
		ui.camera->pos,
		::Mat4::Rotation(ui.camera->rot.get()),
		1.f,
		16384.f,
		ui.camera->fov,
		vph/vpw
	);

	glClearColor(0.3f, 0.3f, 0.3f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	gls.UseProgram(0);
	gls.DisableTextures();
	gls.DisableAllMGSources();
	gls.DisableAllMTSources();
	gls.DisableVertexAttribArrays();

	int cfm = backfaces ? CFM_Back : CFM_Front;
	gls.Set(DT_Less|cfm|CFM_CCW|CWM_RGBA, BM_Off);

	gls.Commit();

	EnableSmoothShading();
}

void BSPBuilder::PaintHandler::EndPaint() {
	DisableSmoothShading();
	
	gls.Set(DWM_Enable, -1); // for glClear()
	gls.Commit();
}

void BSPBuilder::PaintHandler::BeginWireframe(bool backfaces) {
	
	int cfm = backfaces ? CFM_Back : CFM_Front;

	gls.Set(DT_Disable|cfm|CFM_CCW|CWM_RGBA, BM_Off);
	gls.Commit();

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	gl.Color4f(1.f, 1.f, 1.f, 1.f);
}

void BSPBuilder::PaintHandler::EndWireframe() {
	gls.Set(DWM_Enable, -1); // for glClear()
	gls.Commit();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void BSPBuilder::PaintHandler::SetMaterialColor(int id) {
	Vec3 color = RandomColor(id);
	gl.Color4f((float)color[0], (float)color[1], (float)color[2], 0.f);
}

///////////////////////////////////////////////////////////////////////////////

BSPBuilder::AreaBSPDraw::AreaBSPDraw() : m_area(-1), m_leaf(0) {
}

bool BSPBuilder::AreaBSPDraw::Paint(float time, float dt, const QRect &viewport, MapBuilderDebugUI &ui, BSPBuilder &bsp) {

	FindCameraArea(ui, bsp);
	
	BeginPaint(viewport, ui);

	for (U32 i = 0; i < bsp.m_bspFile->numModels; ++i) {
		const world::bsp_file::BSPArea *area = bsp.m_bspFile->Areas() + i;
		DrawModel(bsp, i);
	}

	EndPaint();

	return true;
}

bool BSPBuilder::AreaBSPDraw::OnMenu(const QVariant &data, MapBuilderDebugUI &ui, BSPBuilder &bsp) {
	return true;
}

void BSPBuilder::AreaBSPDraw::DrawModel(BSPBuilder &bsp, U32 modelNum) {
	const world::bsp_file::BSPModel *model = bsp.m_bspFile->Models() + modelNum;

	const U16 *indices = bsp.m_bspFile->Indices() + model->firstIndex;
	const world::bsp_file::BSPVertex *vertices = bsp.m_bspFile->Vertices() + model->firstVert;

	SetMaterialColor((int)model->material);

	glBegin(GL_TRIANGLES);
	for (U32 i = 0; i < model->numIndices; ++i) {
		const world::bsp_file::BSPVertex *v = vertices + indices[i];
		glTexCoord2f(v->st[0], v->st[1]);
		glNormal3f(v->n[0], v->n[1], v->n[2]);
		glVertex3f(v->v[0], v->v[1], v->v[2]);
	}
	glEnd();
}

void BSPBuilder::AreaBSPDraw::FindCameraArea(MapBuilderDebugUI &ui, BSPBuilder &bsp) {
	int area = -1;

	BSPBuilder::Node *leaf = bsp.LeafForPoint(ToBSPType(ui.camera->pos.get()));
	if (leaf && leaf->area) {
		area = leaf->area->area;
	}

	if (m_area != area) {
		::COut(C_Debug) << "Camera Area: " << area << std::endl;
		m_area = area;
	}

	if (m_leaf != leaf) {
		if (leaf) {
			const char *contents = "Mixed";
			if (leaf->contents == kContentsFlag_Areaportal) {
				contents = "Areaportal";
			} else if (leaf->contents == kContentsFlag_Solid) {
				contents = "Solid";
			} else if (leaf->contents == 0) {
				contents = "Empty";
			}
			::COut(C_Debug) << "Leaf Changed, Contents = " << contents << " (" << leaf->contents << "), Occupied = " << leaf->occupied << std::endl;
		} else {
			::COut(C_Debug) << "Leaf is NULL" << std::endl;
		}
		m_leaf = leaf;
	}
}

///////////////////////////////////////////////////////////////////////////////

BSPBuilder::LeafFacesDraw::LeafFacesDraw() : m_leaf(0), m_isolate(false), m_lock(false), m_menu(0), m_isolateAction(0) {
}

bool BSPBuilder::LeafFacesDraw::Paint(float time, float dt, const QRect &viewport, MapBuilderDebugUI &ui, BSPBuilder &bsp) {

	if (!m_menu) {
		m_menu = new (world::bsp_file::ZBSPBuilder) tools::editor::PopupMenu();
		m_isolateAction = ui.AddDebugMenuAction(*m_menu, "Isolate");
		m_isolateAction->setData(0);
		m_isolateAction->setCheckable(true);
		m_isolateAction->setChecked(false);

		m_lockAction = ui.AddDebugMenuAction(*m_menu, "Lock");
		m_lockAction->setData(1);
		m_lockAction->setCheckable(true);
		m_lockAction->setChecked(false);

		m_menu->AddSep();

		QAction *a = ui.AddDebugMenuAction(*m_menu, "Continue...");
		a->setData(2);

		ui.SetDebugMenu(m_menu);
	}

	if (!m_lock)
		FindCameraLeaf(ui, bsp);
	
	BeginPaint(viewport, ui, m_isolate);

	if (m_isolate && m_leaf) {
		DrawNodes(m_leaf, false);
	} else {
		DrawNodes(bsp.m_root.get(), false);
	}

	EndPaint();
	BeginWireframe(m_isolate);

	if (m_isolate && m_leaf) {
		DrawNodes(m_leaf, true);
	} else {
		DrawNodes(bsp.m_root.get(), true);
	}

	EndWireframe();

	return true;
}

bool BSPBuilder::LeafFacesDraw::OnMenu(const QVariant &data, MapBuilderDebugUI &ui, BSPBuilder &bsp) {
	switch(data.toInt()) {
	case 0:
		{
			m_isolate = !m_isolate;
			m_isolateAction->setChecked(m_isolate);
		}
		break;
	case 1:
		{
			m_lock = !m_lock;
			m_lockAction->setChecked(m_isolate);
		}
		break;
	case 2: // continue
		return false;
	}
	return true;
}

void BSPBuilder::LeafFacesDraw::FindCameraLeaf(MapBuilderDebugUI &ui, BSPBuilder &bsp) {

	BSPBuilder::Node *leaf = bsp.LeafForPoint(ToBSPType(ui.camera->pos.get()));
	
	if (m_leaf != leaf) {
		if (leaf) {
			const char *contents = "Mixed";
			if (leaf->contents == kContentsFlag_Areaportal) {
				contents = "Areaportal";
			} else if (leaf->contents == kContentsFlag_Solid) {
				contents = "Solid";
			} else if (leaf->contents == 0) {
				contents = "Empty";
			}
			::COut(C_Debug) << "Leaf Changed, Contents = " << contents << " (" << leaf->contents << "), Occupied = " << leaf->occupied << std::endl;
		} else {
			::COut(C_Debug) << "Leaf is NULL" << std::endl;
		}
		m_leaf = leaf;
	}
}

void BSPBuilder::LeafFacesDraw::DrawNodes(Node *node, bool wireframe) {
	if (node->planenum != kPlaneNumLeaf) {
		DrawNodes(node->children[0].get(), wireframe);
		DrawNodes(node->children[1].get(), wireframe);
		return;
	}

	glNormal3f(0.7f, 0.7f, 0.7f);

	for (TriModelFragVec::const_iterator it = node->models.begin(); it != node->models.end(); ++it) {
		const TriModelFragRef &m = *it;
		for (PolyVec::const_iterator it2 = m->polys.begin(); it2 != m->polys.end(); ++it2) {
			const PolyRef &poly = *it2;

			RAD_ASSERT(poly->original);

			if (!wireframe)
				SetMaterialColor(poly->original->mat);

			glBegin(GL_POLYGON);
			for (Winding::VertexListType::const_iterator vIt = poly->winding->Vertices().begin(); vIt != poly->winding->Vertices().end(); ++vIt) {
				const Winding::VertexType &v = *vIt;
				glVertex3f((float)v[0], (float)v[1], (float)v[2]);
			}
			glEnd();
		}
	}
}

} // solid_bsp
} // tools
