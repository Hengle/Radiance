/*! \file SolidBSP.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#include RADPCH

#if defined(RAD_OPT_TOOLS)

#include "SolidBSP.h"
#include "../../../SkAnim/SkBuilder.h"
#include "../../../Packages/Packages.h"
#include "../../../COut.h"
#include "../../../App.h"
#include "../../../Engine.h"
#include <Runtime/Base/Utils.h>
#include <algorithm>
#undef min
#undef max

using namespace pkg;

namespace tools {
namespace solid_bsp {

namespace {
	const ValueType kMaxBoxExtents = ValueType(1024); // subdivision helps fend off numeric imprecision
};

int BSPBuilder::Node::s_num = 0;
int BSPBuilder::Portal::s_num = 0;
int BSPBuilder::WindingPlane::s_num = 0;
int BSPBuilder::TriModelFrag::s_num = 0;
int BSPBuilder::Poly::s_num = 0;

BSPBuilder::BSPBuilder() : 
m_map(0), 
m_cout(0), 
m_ui(0), 
m_debugUI(0),
m_numStructural(0),
m_numDetail(0),
m_numNodes(0),
m_numLeafs(0),
m_numPortals(0),
m_progress(0),
m_numOutsideNodes(0),
m_numOutsideTris(0),
m_numOutsideModels(0),
m_numInsideNodes(0),
m_numInsideModels(0),
m_numInsideTris(0),
m_validContents(0),
m_numSectors(0),
m_numSharedSectors(0),
m_flood(false) {
	m_result = SR_Success;
}

BSPBuilder::~BSPBuilder() {
}

bool BSPBuilder::SpawnCompile(
	SceneFile &map, 
	tools::UIProgress *ui, 
	MapBuilderDebugUI *debugUI,
	std::ostream *cout
) {
	m_map = &map;
	m_result = SR_Pending;
	m_cout = cout;
	m_ui = ui;
	m_debugUI = debugUI;

	if (!m_ui)
		m_ui = &NullUIProgress;

	Run();

	return true;
}

void BSPBuilder::DebugDraw(float time, float dt) {
}

void BSPBuilder::OnDebugMenu(const QVariant &data) {
}

void BSPBuilder::WaitForCompletion() const {
	Join();
}

int BSPBuilder::ThreadProc() {
	Build();
	return 0;
}

void BSPBuilder::Build()
{
	m_validContents = 0xffffffff;
	if (!LoadMaterials())
		return;
	CreateRootNode();
	Log("------------\n");
	Log("BSPBuilder (%d structural tri(s), %d detail tri(s), %d total)\n", m_numStructural, m_numDetail, m_numStructural+m_numDetail);
	ResetProgress();
	EmitProgress();
	Split(m_root.get(), 0);
	Log("\n%d node(s), %d leaf(s)\n", m_numNodes, m_numLeafs);
	
	/*if (g_glDebug) { 
		DisplayTree(0); 
	}*/

	Portalize();

	if (FloodFill()) {
		m_flood = true;
		FillOutside();
	}

	AreaFlood();
	BuildSectors();

	//if (g_glDebug) { DisplayTree(0, !m_flood); }
}

void BSPBuilder::ResetProgress() {
	m_progress = 0;
}

void BSPBuilder::EmitProgress() {
	if (++m_progress % 60 == 0) 
		Log("\n");
	Log(".");
}

void BSPBuilder::Log(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	String s;
	s.PrintfASCII_valist(fmt, args);
	va_end(args);
	COut() << s << std::flush;
}

void BSPBuilder::SetResult(int result) {
	m_result = result;
}

int BSPBuilder::RAD_IMPLEMENT_GET(result) {
	if (Thread::exited) {
		Join();
		return m_result;
	}
	return SR_Pending;
}

BSPBuilder::Node *BSPBuilder::LeafForPoint(const Vec3 &pos, Node *node)
{
	if (!node)
		node = m_root.get();

	if (node->planenum == kPlaneNumLeaf)
		return node;

	const Plane &plane = m_planes.Plane(node->planenum);
	Plane::SideType s = plane.Side(pos, ValueType(0));
	if (s == Plane::Front)
		return LeafForPoint(pos, node->children[0].get());
	return LeafForPoint(pos, node->children[1].get());
}

void BSPBuilder::Split(Node *node, int boxAxis) {

	int planenum = FindSplitPlane(node, boxAxis);
	if (planenum == kPlaneNumLeaf) {
		LeafNode(node);
		return;
	}

//	DisplayTree(m_root.get(), true, node);

#if defined(RAD_OPT_DEBUG)
	for (Node *parent = node->parent; parent; parent = parent->parent) {
		RAD_ASSERT(parent->planenum != planenum);
	}
#endif
	node->planenum = planenum;

	++m_numNodes;
	if (m_numNodes % 1000 == 0) { 
		EmitProgress(); 
	}

	const Plane &p = m_planes.Plane(planenum);

	node->children[0].reset(new Node());
	node->children[1].reset(new Node());

	node->children[0]->parent = node;
	node->children[1]->parent = node;

	node->children[0]->models.reserve(node->models.size());
	node->children[1]->models.reserve(node->models.size());

	while(!node->models.empty()) {
		TriModelFragRef m = node->models.back();
		node->models.pop_back();

		Plane::SideType s = p.Side(m->bounds, ValueType(0));

		bool onNode = MarkNodePolys(planenum, m);

		if (onNode || s == Plane::Cross || s == Plane::On) {
			TriModelFragRef front, back;
			Split(m, p, planenum, front, back);
			if (front) {
				node->children[0]->models.push_back(front);
				node->children[0]->bounds.Insert(front->bounds);
			}
			if (back) {
				node->children[1]->models.push_back(back);
				node->children[1]->bounds.Insert(back->bounds);
			}
		} else {
			switch (s) {
			case Plane::Front:
				node->children[0]->models.push_back(m);
				node->children[0]->bounds.Insert(m->bounds);
				break;
			case Plane::Back:
				node->children[1]->models.push_back(m);
				node->children[1]->bounds.Insert(m->bounds);
				break;
			default:
				SOLID_BSP_ICE();
				break;
			}
		}
	}

	Split(node->children[0].get(), boxAxis);
	Split(node->children[1].get(), boxAxis);
}

bool BSPBuilder::MarkNodePolys(int planenum, const TriModelFragRef &m) {
	bool nodePolys = false;
	for (PolyVec::const_iterator f = m->polys.begin(); f != m->polys.end(); ++f) {
		const PolyRef &poly = *f;

		if (poly->planenum == planenum || (poly->planenum^1) == planenum) {
			poly->onNode = true;
			nodePolys = true;
		}
	}

	return nodePolys;
}

void BSPBuilder::Split(
	const TriModelFragRef &model, 
	const Plane &p, 
	int planenum, 
	TriModelFragRef &front, 
	TriModelFragRef &back
) {

	front.reset(new TriModelFrag());
	back.reset(new TriModelFrag());
	front->original = back->original = model->original;

	for (PolyVec::const_iterator f = model->polys.begin(); f != model->polys.end(); ++f) {
		const PolyRef &poly = *f;

		if (poly->planenum == planenum) {
			RAD_ASSERT(poly->onNode);
			back->polys.push_back(poly);
			WindingBounds(*poly->winding.get(), back->bounds);
		} else if ((poly->planenum^1) == planenum) {
			RAD_ASSERT(poly->onNode);
			front->polys.push_back(poly);
			WindingBounds(*poly->winding.get(), front->bounds);
		} else {
			PolyRef f(new Poly(*poly.get()));
			f->winding.reset(new Winding());
			PolyRef b(new Poly(*poly.get()));
			b->winding.reset(new Winding());

			poly->winding->Split(p, f->winding.get(), b->winding.get(), kSplitEpsilon);

			if (f->winding->Empty() && b->winding->Empty()) { // tiny winding, put on both sides
				/*f->winding->Initialize(*poly->winding.get());
				front->polys.push_back(f);
				b->winding->Initialize(*poly->winding.get());
				back->polys.push_back(b);
				WindingBounds(*poly->winding.get(), front->bounds);
				WindingBounds(*poly->winding.get(), back->bounds);*/
			} else {
				if (!f->winding->Empty()) {
					front->polys.push_back(f);
					WindingBounds(*f->winding.get(), front->bounds);
				}

				if (!b->winding->Empty()) {
					back->polys.push_back(b);
					WindingBounds(*b->winding.get(), back->bounds);
				}
			}
		}
	}

	if (front->polys.empty()) {
		front.reset();
	}

	if (back->polys.empty()) {
		back.reset();
	}
}

void BSPBuilder::LeafNode(Node *node) {
	++m_numLeafs;
	node->planenum = kPlaneNumLeaf;
	node->contents = 0;

//	if (!node->models.empty()) { DisplayTree(m_root.get(), true, node); }

	for (TriModelFragVec::const_iterator it = node->models.begin(); it != node->models.end(); ++it) {
		const TriModelFragRef &m = *it;

		if (m->original->contents == kContentsFlag_Solid) {
			// solid object, if all polys are on node it eats everything.
			PolyVec::const_iterator polyIt;

			for (polyIt = m->polys.begin(); polyIt != m->polys.end(); ++polyIt) {
				if (!(*polyIt)->onNode) 
					break;
			}

			if (polyIt == m->polys.end()) {
				node->contents = kContentsFlag_Solid;
				break;
			}
		} else {
			node->contents |= m->original->contents;
		}
	}
}

bool BSPBuilder::LoadMaterials() {
	for (SceneFile::TriModel::Vec::iterator m = m_map->worldspawn->models.begin(); m != m_map->worldspawn->models.end(); ++m) {
		SceneFile::TriModel::Ref &trim = *m;

		// gather contents.
		SceneFile::TriFaceVec::iterator f;
		for (f = trim->tris.begin(); f != trim->tris.end(); ++f) {
			SceneFile::TriFace &trif = *f;
			if (trif.mat == -1)
				break;
			const SceneFile::Material &mat = m_map->mats[(*f).mat];
			pkg::Package::Entry::Ref material = App::Get()->engine->sys->packages->Resolve(mat.name.c_str);
			if (!material) {
				Log("WARNING: no material named '%s'!\n", mat.name.c_str.get());
				break;
			}

			const String *s = material->KeyValue<String>("BSP.Contents", 0);
			if (!s) {
				Log("WARNING: meta-data error on material '%s'", mat.name.c_str.get());
				break;
			}

			trif.contents = ContentsForString(*s);
			if (!trim->contents)
				trim->contents = trif.contents;
			if (trim->contents != trif.contents) {
				Log("ERROR: mixed contents on model %d, material '%s', this needs to be corrected.", mat.name.c_str.get());
				trim->contents |= trif.contents;
			}

			s = material->KeyValue<String>("BSP.Surface", 0);
			if (s)
				trif.surface = SurfaceForString(*s);
		}

		if (f != trim->tris.end()) {
			trim->ignore = true;
			Log("ERROR: model %d has a face or faces without a material, this model will be discarded from the BSP.\n", trim->id);
		}
	}

	return true;
}

void BSPBuilder::CreateRootNode() {
	RAD_ASSERT(!m_root);

	m_numStructural = 0;
	m_numDetail = 0;

	Node *node = new Node();
	node->models.reserve(m_map->worldspawn->models.size());

	for (SceneFile::TriModel::Vec::const_iterator m = m_map->worldspawn->models.begin(); m != m_map->worldspawn->models.end(); ++m) {
		const SceneFile::TriModel::Ref &trim = *m;

		// gather contents.
		if (trim->ignore)
			continue;

		if (trim->contents & kContentsFlag_Structural) {
			m_numStructural += (int)trim->tris.size();
		} else {
			m_numDetail += (int)trim->tris.size();
		}

		if (!(trim->contents & m_validContents)) 
			continue;
		
		TriModelFragRef frag(new TriModelFrag());
		frag->original = trim.get();
		frag->bounds = ToBSPType(trim->bounds);
		node->bounds.Insert(frag->bounds);
		frag->polys.reserve(trim->tris.size());
		node->numPolys += (int)trim->tris.size();

		for (SceneFile::TriFaceVec::const_iterator f = trim->tris.begin(); f != trim->tris.end(); ++f) {
			const SceneFile::TriFace &trif = *f;
			if (trif.mat == -1)
				continue; // no material discard.
			PolyRef poly(new Poly());
			poly->original = (SceneFile::TriFace*)&trif;
			poly->contents = trim->contents;
			poly->onNode = false;
			poly->planenum = m_planes.FindPlaneNum(ToBSPType(trif.plane));
			poly->winding.reset(new Winding());
			poly->winding->Initialize(
				ToBSPType(trim->verts[trif.v[0]].pos),
				ToBSPType(trim->verts[trif.v[1]].pos),
				ToBSPType(trim->verts[trif.v[2]].pos),
				m_planes.Plane(poly->planenum)
			);
			frag->polys.push_back(poly);
		}

		node->models.push_back(frag);
	}

	m_root.reset(node);
}

int BSPBuilder::BoxPlaneNum(Node *node, int &boxAxis)
{
	Vec3 boundSize = node->bounds.Size();
	for (int i = 0; i < 3; ++i)
	{
		int axis = boxAxis;
		boxAxis = (boxAxis+1) % 3;
		if (boundSize[i] > kMaxBoxExtents)
		{
			Plane split;
			switch (i)
			{
			case 0:
				split = Plane::X;
				break;
			case 1:
				split = Plane::Y;
				break;
			case 2:
				split = Plane::Z;
				break;
			}
			split = Plane(split.Normal(), node->bounds.Origin()[i]);
			return m_planes.FindPlaneNum(split);
		}
	}
	return kPlaneNumLeaf;
}

int BSPBuilder::FindSplitPlane(Node *node, int &boxAxis)
{
	if (node->models.empty()) 
		return kPlaneNumLeaf;

	int num = kPlaneNumLeaf;//BoxPlaneNum(node, boxAxis);
	if (num != kPlaneNumLeaf) 
		return num;

	int front = 0;
	int back  = 0;
	int split = 0;
	int bestNum = kPlaneNumLeaf;
	int bestVal = std::numeric_limits<int>::max();

//	PlaneNumHash testedPlanes;

	for (int outside = 0; outside <= 1; ++outside) {
		for (int contents = kContentsFlag_FirstVisibleContents; contents <= kContentsFlag_LastVisibleContents; contents <<= 1) {
			if (!(contents & m_validContents)) 
				continue;

			for (TriModelFragVec::const_iterator it = node->models.begin(); it != node->models.end(); ++it) {
				for (PolyVec::const_iterator polyIt = (*it)->polys.begin(); polyIt != (*it)->polys.end(); ++polyIt) {
					if ((*polyIt)->onNode) 
						continue;

					if (!((*polyIt)->contents & contents)) 
						continue;

					if (m_flood && ((*polyIt)->original->outside && !outside)) 
						continue;

					int planenum = (*polyIt)->planenum&~1;
//					if (testedPlanes.find(planenum) == testedPlanes.end())
					{
						int front = 0;
						int back  = 0;
						int split = 0;

//						testedPlanes.insert(planenum);
						const Plane &p = m_planes.Plane(planenum);

						for (TriModelFragVec::const_iterator test = node->models.begin(); test != node->models.end(); ++test) {
							//if (test == it) continue;
							const TriModelFragRef &mdl = *test;
							Plane::SideType s = p.Side(mdl->bounds, ValueType(0));

							switch (s) {
							case Plane::Front:
								front += (int)mdl->polys.size();
								break;
							case Plane::Back:
								back += (int)mdl->polys.size();
								break;
#if 0
							case Plane::Cross:
								for (PolyVec::iterator poly = mdl->polys.begin(); poly != mdl->polys.end(); ++poly)
								{
									if (((*poly)->planenum&~1) == planenum) continue;
									s = (*poly)->winding->Side(p, SplitEpsilon);
									switch (s)
									{
									case Plane::Front:
										++front;
										break;
									case Plane::Back:
										++back;
										break;
									case Plane::Cross:
										++split;
										break;
									}
								}
								
								break;
							case Plane::On:
								break;
							default:
								INTERNAL_COMPILER_ERROR();
#endif
							}
						}

						int val = math::Abs(front-back) + split*1000;

						if (val < bestVal) {
							bestNum = planenum;
							bestVal = val;
						}
					}
				}
			}

			if (bestNum != -1) 
				break; // don't split using outside unless we have no inside
		}

		if (!m_flood) 
			break; // don't do 2 passes unless we filled outside.
	}

	return bestNum;
}

/*
void BSPBuilder::DrawTreeHandler::OnPaint(GLNavWindow &w)
{
	GLState &s = w.BeginFrame();
	w.Camera().Bind(s);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	s.SetState(DT_Less|CFM_Back|CFM_CCW|CWM_All|NoArrays, BM_Off);
	s.DisableAllTMUs();
	s.Commit();

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

	glColor3f(0.6f, 0.1f, 0.1f);

	DrawNode(m_root, s);

	if (!m_target)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHT1);
		glDisable(GL_COLOR_MATERIAL);

		s.SetState(DT_Disable, 0);
		s.Commit();
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glColor3f(1, 1, 1);
		DrawNode(m_root, s);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	w.EndFrame();
}

void BSPBuilder::DrawTreeHandler::DrawNode(const Node *node, GLState &s)
{
	if (!node) return; // partial tree

	if (m_target)
	{
		if (node == m_target)
		{
			s.SetState(DT_Less, 0);
			s.Commit();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else
		{
			s.SetState(DT_Disable, 0);
			s.Commit();
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
	}

	for (TriModelFragVec::const_iterator it = node->models.begin(); it != node->models.end(); ++it)
	{
		if (!m_outside && (*it)->original->outside) continue;

		for (PolyVec::const_iterator polyIt = (*it)->polys.begin(); polyIt != (*it)->polys.end(); ++polyIt)
		{
			if (!m_outside && (*polyIt)->original->outside) continue;

			glColor3d((*polyIt)->color[0], (*polyIt)->color[1], (*polyIt)->color[2]);
			glBegin(GL_POLYGON);

			for (Winding::VertexListType::const_iterator v = (*polyIt)->winding->Vertices().begin(); v != (*polyIt)->winding->Vertices().end(); ++v)
			{
				glVertex3d((*v).X(), (*v).Y(), (*v).Z());
			}

			glEnd();
		}
	}

	if (m_target) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }

	if (node->planenum != PlaneNumLeaf)
	{
		DrawNode(node->children[0].get(), s);
		DrawNode(node->children[1].get(), s);
		return;
	}
}

void BSPBuilder::DisplayTree(Node *node, 
		bool outside,
		const Node *target)
{
	GLNavWindow win;
	GLCamera &c = win.Camera();
	SceneFile::EntityRef e = m_map->EntForName("sp_player_start");
	if (e)
	{
		c.SetPos(
			GLVec3(
				GLVec3::ValueType(e->origin.X()), 
				GLVec3::ValueType(e->origin.Y()), 
				GLVec3::ValueType(e->origin.Z())
			)
		);
	}
	else
	{
		c.SetPos(GLVec3::Zero);
	}
	if (!node) { node = m_root.get(); }
	DrawTreeHandler p(node, outside, target);
	win.SetPaintHandler(&p);
	win.Open(L"BSPBuilder", -1, -1, 1280, 1024, true);
	win.WaitForClose();
}
*/

void BSPBuilder::BBoxPlanes(const BBox &bounds, Plane *planes) {
	planes[0] = Plane(-Plane::X.Normal(), -bounds.Mins().X());
	planes[1] = Plane(Plane::X.Normal(), bounds.Maxs().X());
	planes[2] = Plane(-Plane::Y.Normal(), -bounds.Mins().Y());
	planes[3] = Plane(Plane::Y.Normal(), bounds.Maxs().Y());
	planes[4] = Plane(-Plane::Z.Normal(), -bounds.Mins().Z());
	planes[5] = Plane(Plane::Z.Normal(), bounds.Maxs().Z());
}

void BSPBuilder::BBoxWindings(const BBox &bounds, WindingVec &out) {
	Plane planes[6];
	BBoxPlanes(bounds, planes);

	Winding x, t;
	for (int i = 0; i < 6; ++i) {
		x.Initialize(planes[i], SceneFile::kMaxRange);
		for (int y = 0; y < 4; ++y) {
			int z = ((i/2*2)+y+2) % 6;

			x.Chop(
				planes[z],
				Plane::Back,
				t,
				ValueType(0)
			);
			x = t;
			RAD_ASSERT(!x.Empty());
		}

		out.push_back(WindingRef(new Winding(x)));
	}
}

void BSPBuilder::WindingBounds(const Winding &winding, BBox &bounds) {
	for (Winding::VertexListType::const_iterator it = winding.Vertices().begin(); it != winding.Vertices().end(); ++it) {
		bounds.Insert(*it);
	}
}

Vec3 BSPBuilder::WindingCenter(const Winding &winding) {
	Vec3 p(Vec3::Zero);
	for (Winding::VertexListType::const_iterator it = winding.Vertices().begin(); it != winding.Vertices().end(); ++it) {
		p += *it;
	}
	p /= ValueType(winding.Vertices().size());
	return p;
}

int BSPBuilder::ContentsForString(const String &s) {
	if (s == "Clip")
		return kContentsFlag_Clip;
	if (s == "Detail")
		return kContentsFlag_Detail;
	if (s == "Fog")
		return kContentsFlag_Fog;
	if (s == "Water")
		return kContentsFlag_Water;
	if (s == "Areaportal")
		return kContentsFlag_Areaportal;
	return kContentsFlag_Solid;
}

int BSPBuilder::SurfaceForString(const String &s) {
	if (s == "No Draw")
		return kSurfaceFlag_NoDraw;
	return 0;
}

Vec2 BSPBuilder::ToBSPType(const SceneFile::Vec2 &vec) {
	return Vec2(
		(ValueType)vec[0],
		(ValueType)vec[1]
	);
}

Vec3 BSPBuilder::ToBSPType(const SceneFile::Vec3 &vec) {
	return Vec3(
		(ValueType)vec[0],
		(ValueType)vec[1],
		(ValueType)vec[2]
	);
}

SceneFileD::TriVert BSPBuilder::ToBSPType(const SceneFile::TriVert &vec) {
	SceneFileD::TriVert v;

	v.pos = ToBSPType(vec.pos);
	v.orgPos = ToBSPType(vec.orgPos);
	v.normal = ToBSPType(vec.normal);
	v.color = ToBSPType(vec.color);

	for (int i = 0; i < SceneFile::kMaxUVChannels; ++i) {
		v.st[i] = ToBSPType(vec.st[i]);
	}

	// NOTE: We don't transfer bone weights (not used in BSP).
	return v;
}

BBox BSPBuilder::ToBSPType(const SceneFile::BBox &bbox) {
	return BBox(
		ToBSPType(bbox.Mins()),
		ToBSPType(bbox.Maxs())
	);
}

Plane BSPBuilder::ToBSPType(const SceneFile::Plane &plane) {
	return Plane(
		ToBSPType(plane.Normal()),
		(ValueType)plane.D()
	);
}

SceneFile::Plane BSPBuilder::FromBSPType(const Plane &plane) {
	return SceneFile::Plane(
		(SceneFile::ValueType)plane.A(),
		(SceneFile::ValueType)plane.B(),
		(SceneFile::ValueType)plane.C(),
		(SceneFile::ValueType)plane.D()
	);
}

Vec3 RandomColor() {
	static Vec3Vec s_colors;
	if (s_colors.empty()) {
		for (int i = 0; i < 256; ++i) {
			s_colors.push_back(
				Vec3(
					Vec3::ValueType((((rand() & 0xf) + 1) & 0xf) / 16.0),
					Vec3::ValueType((((rand() & 0xf) + 1) & 0xf) / 16.0),
					Vec3::ValueType((((rand() & 0xf) + 1) & 0xf) / 16.0)
				)
			);
		}
	}
	return s_colors[rand() & 0xff];
}

} // box_bsp
} // tools

#endif // RAD_OPT_TOOLS
