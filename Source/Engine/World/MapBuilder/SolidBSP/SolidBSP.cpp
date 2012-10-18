/*! \file SolidBSP.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#include RADPCH

#include "SolidBSP.h"
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
	const ValueType kMaxBoxExtents = ValueType(1024);
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
m_numPortalFaces(0),
m_numPortalSplits(0),
m_progress(0),
m_numOutsideNodes(0),
m_numOutsideTris(0),
m_numOutsideModels(0),
m_numInsideNodes(0),
m_numInsideModels(0),
m_numInsideTris(0),
m_numAreaNodes(0),
m_numAreaLeafs(0),
m_flood(false),
m_abort(false) {
	m_result = SR_Success;
}

BSPBuilder::~BSPBuilder() {
	{
		Lock L(m_paintMutex);
		m_paint.reset();
	}

	m_abort = true;
	WaitForCompletion();
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

void BSPBuilder::DebugDraw(float time, float dt, const QRect &viewport) {
	Lock L(m_paintMutex);
	if (m_paint && m_debugUI) {
		if (!m_paint->Paint(time, dt, viewport, *m_debugUI, *this)) {
			m_debugUI->enabled = false;
			m_paint.reset();
			m_debugUI->SetDebugMenu(0);
		}
	}
}

void BSPBuilder::OnDebugMenu(const QVariant &data) {
	Lock L(m_paintMutex);
	if (m_paint && m_debugUI) {
		if (!m_paint->OnMenu(data, *m_debugUI, *this)) {
			m_debugUI->enabled = false;
			m_paint.reset();
			m_debugUI->SetDebugMenu(0);
		}
	}
}

void BSPBuilder::DisplayPaintHandler(PaintHandler *handler) {
	RAD_ASSERT(handler);
	RAD_ASSERT(m_debugUI);

	m_paintMutex.lock();
	m_paint.reset(handler);
	handler->Init(*m_debugUI, *this);
	m_debugUI->enabled = true;
	m_paintMutex.unlock();

	for (;;) {
		thread::Sleep(500);
		Lock L(m_paintMutex);
		if (!m_paint)
			break;
	}
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
	if (m_ui) {
		m_ui->title = "Loading Materials...";
		m_ui->total = 0.f;
		m_ui->totalProgress = 0;
		m_ui->Refresh();
	}

	if (!LoadMaterials())
		return;

	if (m_ui) {
		m_ui->title = "Building Hull...";
		m_ui->total = 0.f;
		m_ui->totalProgress = 0;
		m_ui->Refresh();
	}

	MarkDetail();
	CreateRootNode();
	Log("------------\n");
	Log("Building Hull (%d structural tri(s), %d detail tri(s), %d total)\n", m_numStructural, m_numDetail, m_numStructural+m_numDetail);

	if (m_numStructural == 0) {
		SetResult(SR_CompilerError);
		Log("ERROR: map has no structural hull!\n");
		return;
	}

	ResetProgress();
	EmitProgress();
	Split(m_root.get(), 0);
	Log("\n%d node(s), %d leaf(s)\n", m_numNodes, m_numLeafs);
	
	if (m_debugUI)
		DisplayPaintHandler(new (world::bsp_file::ZBSPBuilder) LeafFacesDraw());

	if (m_abort)
		return;

	Portalize();

	if (m_abort)
		return;

	if (FloodFill()) {
		m_flood = true;
		FillOutside();

		if (m_ui) {
			m_ui->title = "Building Optimized Hull...";
			m_ui->total = 0.f;
			m_ui->totalProgress = 0;
			m_ui->Refresh();
		}

		// make a better tree.
		m_root.reset();

		CreateRootNode();
		Log("------------\n");
		Log("Building Optimized Hull (%d structural tri(s), %d detail tri(s), %d total)\n", m_numStructural, m_numDetail, m_numStructural+m_numDetail);

		m_numNodes = 0;
		m_numLeafs = 0;

		ResetProgress();
		EmitProgress();
		Split(m_root.get(), 0);
		Log("\n%d node(s), %d leaf(s)\n", m_numNodes, m_numLeafs);
		Portalize();
		if (!FloodFill()) {
			Log("ERROR: map leaked after fill pass!\n");
			SetResult(SR_CompilerError);
			return;
		}
		FillOutside();
	}
	
	if (!AreaFlood())
		return;
	if (!CompileAreas())
		return;
	EmitBSPFile();

	if (m_debugUI)
		DisplayPaintHandler(new (world::bsp_file::ZBSPBuilder) AreaBSPDraw());
	SetResult(SR_Success);
}

void BSPBuilder::MarkDetail() {
	for (SceneFile::TriModel::Vec::iterator m = m_map->worldspawn->models.begin(); m != m_map->worldspawn->models.end(); ++m) {
		if ((*m)->ignore || (*m)->cinematic)
			continue;

		if ((*m)->contents & kContentsFlag_Detail) {
			// details area never outside (they aren't in the BSP).
			(*m)->outside = false;
			for (SceneFile::TriFaceVec::iterator f = (*m)->tris.begin(); f != (*m)->tris.end(); ++f) {
				(*f).outside = false;
			}
		}
	}
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

	// probably should turn bounds into a volume and split it.
	// but we don't actually use the bounds for anything so...
	if (node->children[0]->models.empty())
		node->children[0]->bounds = node->bounds;
	if (node->children[1]->models.empty())
		node->children[1]->bounds = node->bounds;

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

		Plane::SideType s;

		if (poly->planenum == planenum) {
			s = Plane::Back;
			RAD_ASSERT(poly->onNode);
		} else if (poly->planenum == (planenum^1)) {
			s = Plane::Front;
			RAD_ASSERT(poly->onNode);
		} else {
			s = poly->winding->Side(p, kBSPSplitEpsilon);
		}

		if (s == Plane::Back || s == Plane::On) {
			back->polys.push_back(poly);
			WindingBounds(*poly->winding.get(), back->bounds);
		}
		
		if (s == Plane::Front || s == Plane::On) {
			front->polys.push_back(poly);
			WindingBounds(*poly->winding.get(), front->bounds);
		}
		
		if (s == Plane::Cross) {

			PolyRef f(new Poly(*poly.get()));
			f->winding.reset(new Winding());
			PolyRef b(new Poly(*poly.get()));
			b->winding.reset(new Winding());

			poly->winding->Split(p, f->winding.get(), b->winding.get(), ValueType(0));

			if (f->winding->Empty()) {
				Log("WARNING: winding clipped away (front).\n");
			} else {
				front->polys.push_back(f);
				WindingBounds(*f->winding.get(), front->bounds);
			}

			if (b->winding->Empty()) {
				Log("WARNING: winding clipped away (back).\n");
			} else {
				back->polys.push_back(b);
				WindingBounds(*b->winding.get(), back->bounds);
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

		if (trim->cinematic)
			continue;

		// gather contents.
		SceneFile::TriFaceVec::iterator f;
		for (f = trim->tris.begin(); f != trim->tris.end(); ++f) {
			SceneFile::TriFace &trif = *f;
			if (trif.mat == -1)
				break;
			const SceneFile::Material &mat = m_map->mats[(*f).mat];
			pkg::Package::Entry::Ref material = App::Get()->engine->sys->packages->Resolve(mat.name.c_str, 0);

			// assume detail.
			int contents = kContentsFlag_Detail;
			int surface = 0;

			if (material) {
				const String *s = material->KeyValue<String>("BSP.Contents", 0);
				if (!s) {
					Log("WARNING: meta-data error on material '%s'", mat.name.c_str.get());
					break;
				}

				contents = ContentsForString(*s);

				s = material->KeyValue<String>("BSP.Surface", 0);
				if (s)
					surface = SurfaceForString(*s);
			}

			trif.contents = contents;
			if (!trim->contents)
				trim->contents = trif.contents;
			if (trim->contents != trif.contents) {
				Log("ERROR: mixed contents on '%s', material '%s', this has to be corrected.\n", trim->name.c_str.get(), mat.name.c_str.get());
				SetResult(SR_CompilerError);
				return false;
			}

			trif.surface = surface;
		}

		if (f != trim->tris.end()) {
			trim->ignore = true;
			Log("WARNING: '%s' has a face or faces without a material, it will be discarded from the BSP.\n", trim->name.c_str.get(), trim->id);
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
		if (trim->ignore || trim->cinematic)
			continue;

		if (trim->contents & kContentsFlag_Structural) {
			m_numStructural += (int)trim->tris.size();
		} else {
			m_numDetail += (int)trim->tris.size();
		}

		if (!(trim->contents & kContentsFlag_BSPContents)) 
			continue;
		//if (trim->contents & kContentsFlag_Areaportal)
		//	continue;
		
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

int BSPBuilder::BoxPlaneNum(Node *node, int &boxAxis) {
	Vec3 boundSize = node->bounds.Size();
	for (int i = 0; i < 3; ++i) {
		boxAxis = (boxAxis+1) % 3;
		if (boundSize[i] > kMaxBoxExtents) {
			Plane split;
			switch (i) {
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

	int num = kPlaneNumLeaf;
	if (num != kPlaneNumLeaf) 
		return num;

	int front = 0;
	int back  = 0;
	int split = 0;
	int bestNum = kPlaneNumLeaf;
	int bestVal = std::numeric_limits<int>::max();

	for (int outside = 0; outside <= 1; ++outside) {
		for (int contents = kContentsFlag_FirstVisibleContents; contents <= kContentsFlag_LastVisibleContents; contents <<= 1) {
			if (!(contents & kContentsFlag_BSPContents)) 
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
					{
						int front = 0;
						int back  = 0;
						int split = 0;
						int areaportal = 0;

						const Plane &p = m_planes.Plane(planenum);

						for (TriModelFragVec::const_iterator test = node->models.begin(); test != node->models.end(); ++test) {
							const TriModelFragRef &mdl = *test;
							Plane::SideType s = p.Side(mdl->bounds, ValueType(0));

							switch (s) {
							case Plane::Front:
								front += (int)mdl->polys.size();
								break;
							case Plane::Back:
								back += (int)mdl->polys.size();
								break;
							case Plane::Cross:
								for (PolyVec::iterator poly = mdl->polys.begin(); poly != mdl->polys.end(); ++poly) {
									if (((*poly)->planenum&~1) == planenum) 
										continue;
									s = (*poly)->winding->Side(p, kBSPSplitEpsilon);
									switch (s) {
									case Plane::Front:
										++front;
										break;
									case Plane::Back:
										++back;
										break;
									case Plane::Cross:
										++split;
										if (mdl->original->contents == kContentsFlag_Areaportal) {
											// areaportals don't get penalized for splitting eachother
											if (!((*polyIt)->contents & kContentsFlag_Areaportal)) 
												++areaportal;
										}
										break;
									case Plane::On:
										++front;
										++back;
										break;
									default:
										break;
									}
								}
								
								break;
							case Plane::On:
								break;
							default:
								SOLID_BSP_ICE();
							}
						}

						int val = math::Abs(front-back) + split*1000 + areaportal*10000;

						if (val < bestVal) {
							bestNum = planenum;
							bestVal = val;
						}
					}
				}
			}

			if (bestNum != -1) 
				break; // idealized splitter
		}

		if (bestNum != -1) 
			break; // don't split using outside unless we have no inside

		if (!m_flood) 
			break; // don't do 2 passes unless we filled outside.
	}

	return bestNum;
}

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
	struct Flags {
		const char *sz;
		int flag;
	};
	static Flags kFlags[] = {
		{ "No Draw", kSurfaceFlag_NoDraw },
		{ 0, }
	};
	int flags = 0;

	for (int i = 0; kFlags[i].sz; ++i) {
		if (s.StrStr(CStr(kFlags[i].sz)) != -1)
			flags |= kFlags[i].flag;
	}

	return flags;
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

Vec4 BSPBuilder::ToBSPType(const SceneFile::Vec4 &vec) {
	return Vec4(
		(ValueType)vec[0],
		(ValueType)vec[1],
		(ValueType)vec[2],
		(ValueType)vec[3]
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
		v.tangent[i] = ToBSPType(vec.tangent[i]);
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

Vec3 RandomColor(int index) {
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
	if (index == -1)
		index = rand();
	return s_colors[index & 0xff];
}

} // solid_bsp
} // tools
