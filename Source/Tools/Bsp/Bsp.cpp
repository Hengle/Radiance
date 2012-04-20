// Bsp.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Bsp.h"
#include <Runtime/Container/HashSet.h>
#include <Runtime/PushSystemMacros.h>

namespace
{
	const ValueType MaxBoxExtents = ValueType(1024); // subdivision helps fend off numeric imprecision
};

int Node::s_num = 0;
int Portal::s_num = 0;
int WindingPlane::s_num = 0;
int TriModelFrag::s_num = 0;
int Poly::s_num = 0;

void BBoxPlanes(const BBox &bounds, Plane *planes)
{
	planes[0] = Plane(-Plane::X.Normal(), -bounds.Mins().X());
	planes[1] = Plane(Plane::X.Normal(), bounds.Maxs().X());
	planes[2] = Plane(-Plane::Y.Normal(), -bounds.Mins().Y());
	planes[3] = Plane(Plane::Y.Normal(), bounds.Maxs().Y());
	planes[4] = Plane(-Plane::Z.Normal(), -bounds.Mins().Z());
	planes[5] = Plane(Plane::Z.Normal(), bounds.Maxs().Z());
}

void BBoxWindings(const BBox &bounds, WindingVec &out)
{
	Plane planes[6];
	BBoxPlanes(bounds, planes);

	Winding x, t;
	for (int i = 0; i < 6; ++i)
	{
		x.Initialize(planes[i], Map::MaxRange);
		for (int y = 0; y < 4; ++y)
		{
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

void WindingBounds(const Winding &winding, BBox &bounds)
{
	for (Winding::VertexListType::const_iterator it = winding.Vertices().begin(); it != winding.Vertices().end(); ++it)
	{
		bounds.Insert(*it);
	}
}

Vec3 WindingCenter(const Winding &winding)
{
	Vec3 p(Vec3::Zero);
	for (Winding::VertexListType::const_iterator it = winding.Vertices().begin(); it != winding.Vertices().end(); ++it)
	{
		p += *it;
	}
	p /= ValueType(winding.Vertices().size());
	return p;
}

void BSP::Build()
{
	m_validContents = Map::VisibleContents;
	CreateRootNode();
	Log(LogNormal, "------------\n");
	Log(LogNormal, "SolidBSP (%d structural tri(s), %d detail tri(s), %d total)\n", m_numStructural, m_numDetail, m_numStructural+m_numDetail);
	ResetProgress();
	EmitProgress();
	Split(m_root.get(), 0);
	Log(LogNormal, "\n%d node(s), %d leaf(s)\n", m_numNodes, m_numLeafs);
	if (g_glDebug) { DisplayTree(0); }
	Portalize();
	if (FloodFill())
	{
		m_flood = true;
		FillOutside();
	}
	AreaFlood();
	BuildSectors();
	if (g_glDebug) { DisplayTree(0, !m_flood); }
}

void BSP::ResetProgress()
{
	m_progress = 0;
}

void BSP::EmitProgress()
{
	if (++m_progress % 60 == 0) printf("\n");
	printf(".");
}

Node *BSP::LeafForPoint(const Vec3 &pos, Node *node)
{
	if (node == 0) { node = m_root.get(); }
	if (node->planenum == PlaneNumLeaf) { return node; }

	const Plane &plane = m_planes.Plane(node->planenum);
	Plane::SideType s = plane.Side(pos, ValueType(0));
	if (s == Plane::Front)
	{
		return LeafForPoint(pos, node->children[0].get());
	}
	return LeafForPoint(pos, node->children[1].get());
}

void BSP::Split(Node *node, int boxAxis)
{
	int planenum = FindSplitPlane(node, boxAxis);
	if (planenum == PlaneNumLeaf)
	{
		LeafNode(node);
		return;
	}

//	DisplayTree(m_root.get(), true, node);

#if defined(RAD_OPT_DEBUG)
	for (Node *parent = node->parent; parent; parent = parent->parent)
	{
		RAD_ASSERT(parent->planenum != planenum);
	}
#endif
	node->planenum = planenum;

	++m_numNodes;
	if (m_numNodes % 1000 == 0) 
	{ 
		EmitProgress(); 
	}

	const Plane &p = m_planes.Plane(planenum);

	node->children[0].reset(new Node());
	node->children[1].reset(new Node());

	node->children[0]->parent = node;
	node->children[1]->parent = node;

	node->children[0]->models.reserve(node->models.size());
	node->children[1]->models.reserve(node->models.size());

	while(!node->models.empty())
	{
		TriModelFragRef m = node->models.back();
		node->models.pop_back();

		Plane::SideType s = p.Side(m->bounds, ValueType(0));

		bool onNode = MarkNodePolys(planenum, m);

		if (onNode || s == Plane::Cross || s == Plane::On)
		{
			TriModelFragRef front, back;
			Split(m, p, planenum, front, back);
			if (front)
			{
				node->children[0]->models.push_back(front);
				node->children[0]->bounds.Insert(front->bounds);
			}
			if (back)
			{
				node->children[1]->models.push_back(back);
				node->children[1]->bounds.Insert(back->bounds);
			}
		}
		else
		{
			switch (s)
			{
			case Plane::Front:
				node->children[0]->models.push_back(m);
				node->children[0]->bounds.Insert(m->bounds);
				break;
			case Plane::Back:
				node->children[1]->models.push_back(m);
				node->children[1]->bounds.Insert(m->bounds);
				break;
			default:
				INTERNAL_COMPILER_ERROR();
			}
		}
	}

	Split(node->children[0].get(), boxAxis);
	Split(node->children[1].get(), boxAxis);
}

bool BSP::MarkNodePolys(int planenum, const TriModelFragRef &m)
{
	bool nodePolys = false;
	for (PolyVec::const_iterator f = m->polys.begin(); f != m->polys.end(); ++f)
	{
		const PolyRef &poly = *f;

		if (poly->planenum == planenum || (poly->planenum^1) == planenum)
		{
			poly->onNode = true;
			nodePolys = true;
		}
	}

	return nodePolys;
}

void BSP::Split(const TriModelFragRef &model, const Plane &p, int planenum, TriModelFragRef &front, TriModelFragRef &back)
{
	front.reset(new TriModelFrag());
	back.reset(new TriModelFrag());
	front->original = back->original = model->original;

	for (PolyVec::const_iterator f = model->polys.begin(); f != model->polys.end(); ++f)
	{
		const PolyRef &poly = *f;

		if (poly->planenum == planenum)
		{
			RAD_ASSERT(poly->onNode);
			back->polys.push_back(poly);
			WindingBounds(*poly->winding.get(), back->bounds);
		}
		else if ((poly->planenum^1) == planenum)
		{
			RAD_ASSERT(poly->onNode);
			front->polys.push_back(poly);
			WindingBounds(*poly->winding.get(), front->bounds);
		}
		else
		{
			PolyRef f(new Poly(*poly.get()));
			f->winding.reset(new Winding());
			PolyRef b(new Poly(*poly.get()));
			b->winding.reset(new Winding());

			poly->winding->Split(p, f->winding.get(), b->winding.get(), SplitEpsilon);

			if (f->winding->Empty() && b->winding->Empty()) // tiny winding, put on both sides
			{
				/*f->winding->Initialize(*poly->winding.get());
				front->polys.push_back(f);
				b->winding->Initialize(*poly->winding.get());
				back->polys.push_back(b);
				WindingBounds(*poly->winding.get(), front->bounds);
				WindingBounds(*poly->winding.get(), back->bounds);*/
			}
			else
			{
				if (!f->winding->Empty())
				{
					front->polys.push_back(f);
					WindingBounds(*f->winding.get(), front->bounds);
				}
				if (!b->winding->Empty())
				{
					back->polys.push_back(b);
					WindingBounds(*b->winding.get(), back->bounds);
				}
			}
		}
	}

	if (front->polys.empty())
	{
		front.reset();
	}
	if (back->polys.empty())
	{
		back.reset();
	}
}

void BSP::LeafNode(Node *node)
{
	++m_numLeafs;
	node->planenum = PlaneNumLeaf;
	node->contents = 0;

//	if (!node->models.empty()) { DisplayTree(m_root.get(), true, node); }

	for (TriModelFragVec::const_iterator it = node->models.begin(); it != node->models.end(); ++it)
	{
		const TriModelFragRef &m = *it;

		if (m->original->contents == Map::ContentsSolid)
		{
			// solid object, if all polys are on node it eats everything.
			PolyVec::const_iterator polyIt;

			for (polyIt = m->polys.begin(); polyIt != m->polys.end(); ++polyIt)
			{
				if (!(*polyIt)->onNode) break;
			}

			if (polyIt == m->polys.end())
			{
				node->contents = Map::ContentsSolid;
				break;
			}
		}
		else
		{
			node->contents |= m->original->contents;
		}
	}
}

void BSP::CreateRootNode()
{
	RAD_ASSERT(!m_root);

	m_numStructural = 0;
	m_numDetail = 0;

	Node *node = new Node();
	node->models.reserve(m_map.worldspawn->models.size());

	for (Map::TriModelVec::const_iterator m = m_map.worldspawn->models.begin(); m != m_map.worldspawn->models.end(); ++m)
	{
		const Map::TriModelRef &trim = *m;

		if (trim->contents & Map::StructuralContents)
		{
			m_numStructural += (int)trim->tris.size();
		}
		else
		{
			m_numDetail += (int)trim->tris.size();
		}

		if (!(trim->contents & m_validContents)) continue;
		
		TriModelFragRef frag(new TriModelFrag());
		frag->original = trim.get();
		frag->bounds = trim->bounds;
		node->bounds.Insert(frag->bounds);
		frag->polys.reserve(trim->tris.size());
		node->numPolys += (int)trim->tris.size();

		for (Map::TriFaceVec::const_iterator f = trim->tris.begin(); f != trim->tris.end(); ++f)
		{
			const Map::TriFace &trif = *f;
			PolyRef poly(new Poly());
			poly->original = (Map::TriFace*)&trif;
			poly->contents = trim->contents;
			poly->onNode = false;
			poly->planenum = m_planes.FindPlaneNum(trif.plane);
			poly->winding.reset(new Winding());
			poly->winding->Initialize(
				trim->verts[trif.v[0]].pos,
				trim->verts[trif.v[1]].pos,
				trim->verts[trif.v[2]].pos,
				m_planes.Plane(poly->planenum)
			);
			frag->polys.push_back(poly);
		}

		node->models.push_back(frag);
	}

	m_root.reset(node);
}

int BSP::BoxPlaneNum(Node *node, int &boxAxis)
{
	Vec3 boundSize = node->bounds.Size();
	for (int i = 0; i < 3; ++i)
	{
		int axis = boxAxis;
		boxAxis = (boxAxis+1) % 3;
		if (boundSize[i] > MaxBoxExtents)
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
	return PlaneNumLeaf;
}

int BSP::FindSplitPlane(Node *node, int &boxAxis)
{
	if (node->models.empty()) return PlaneNumLeaf;
	int num = PlaneNumLeaf;//BoxPlaneNum(node, boxAxis);
	if (num != PlaneNumLeaf) { return num; }

	int front = 0;
	int back  = 0;
	int split = 0;
	int bestNum = PlaneNumLeaf;
	int bestVal = std::numeric_limits<int>::max();

//	PlaneNumHash testedPlanes;

	for (int outside = 0; outside <= 1; ++outside)
	{
		for (int contents = Map::FirstVisibleContents; contents <= Map::LastVisibleContents; contents <<= 1)
		{
			if (!(contents & m_validContents)) continue;

			for (TriModelFragVec::const_iterator it = node->models.begin(); it != node->models.end(); ++it)
			{
				for (PolyVec::const_iterator polyIt = (*it)->polys.begin(); polyIt != (*it)->polys.end(); ++polyIt)
				{
					if ((*polyIt)->onNode) continue;
					if (!((*polyIt)->contents & contents)) continue;
					if (m_flood && ((*polyIt)->original->outside && !outside)) continue;
					int planenum = (*polyIt)->planenum&~1;
//					if (testedPlanes.find(planenum) == testedPlanes.end())
					{
						int front = 0;
						int back  = 0;
						int split = 0;

//						testedPlanes.insert(planenum);
						const Plane &p = m_planes.Plane(planenum);

						for (TriModelFragVec::const_iterator test = node->models.begin(); test != node->models.end(); ++test)
						{
							//if (test == it) continue;
							const TriModelFragRef &mdl = *test;
							Plane::SideType s = p.Side(mdl->bounds, ValueType(0));

							switch (s)
							{
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

						if (val < bestVal)
						{
							bestNum = planenum;
							bestVal = val;
						}
					}
				}
			}

			if (bestNum != -1) break; // don't split using outside unless we have no inside
		}

		if (!m_flood) break; // don't do 2 passes unless we filled outside.
	}

	return bestNum;
}

void BSP::DrawTreeHandler::OnPaint(GLNavWindow &w)
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

void BSP::DrawTreeHandler::DrawNode(const Node *node, GLState &s)
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

void BSP::DisplayTree(Node *node, 
		bool outside,
		const Node *target)
{
	GLNavWindow win;
	GLCamera &c = win.Camera();
	Map::EntityRef e = m_map.EntForName("sp_player_start");
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
	win.Open(L"BSP", -1, -1, 1280, 1024, true);
	win.WaitForClose();
}