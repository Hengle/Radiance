// Portals.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if defined(RAD_OPT_TOOLS)

#include "SolidBsp.h"

namespace tools {
namespace solid_bsp {

void BSPBuilder::AddPortalToNodes(const PortalRef &p, Node *front, Node *back) {
	RAD_ASSERT(!p->nodes[0] && !p->nodes[1]);
	p->nodes[0] = front;
	p->next[0] = front->portals;
	front->portals = p;
	p->nodes[1] = back;
	p->next[1] = back->portals;
	back->portals = p;
}

void BSPBuilder::RemovePortalFromNode(const PortalRef &p, Node *node) {
	RAD_ASSERT(p->nodes[0] == node || p->nodes[1] == node);
	int pside = p->nodes[1] == node;
	int ls, ns;
	Portal *last = 0;

	for (PortalRef test = node->portals; test; test = test->next[ns]) {
		RAD_ASSERT(last != test.get());
		ns = test->nodes[1] == node;
		
		if (p == test) // found
		{
			// unlink using previous portal.
			if (last) {
				last->next[ls] = test->next[ns];
			} else {
				node->portals = test->next[ns];
			}

			p->nodes[pside] = 0;
			p->next[pside].reset();
			break;
		}

		last = test.get();
		ls = ns;
	}
}

void BSPBuilder::Portalize() {
	RAD_ASSERT(m_root);

	ResetProgress();
	Log("------------\n");
	Log("Portalize...\n");
	EmitProgress();

	m_outside.planenum = kPlaneNumLeaf;
	m_outside.bounds = m_root->bounds;
	m_outside.contents = 0;
	m_outside.bounds.Expand(ValueType(16), ValueType(16), ValueType(16)); // avoid null volumes.
	WindingVec windings;
	
	// make 6 bounding portals (axis aligned box that contains the world) that
	// face outward towards the "outside" node, with the root on the back.
	BBoxWindings(m_outside.bounds, windings);

	for (int i = 0; i < 6; ++i) {
		PortalRef p(new Portal());
		p->plane.winding = *windings[i];
		p->plane.planenum = m_planes.FindPlaneNum(windings[i]->Plane());
		AddPortalToNodes(p, &m_outside, m_root.get());
	}

	MakeTreePortals(m_root.get());
	FindPortalNodeFaces(m_root.get());
	Log("\n%d portal(s)\n", m_numPortals);
	//if (g_glDebug) { DisplayPortals(0, 0, 0, false, Map::VisibleContents); }
}

void BSPBuilder::MakeNodePortal(Node *node) {
	PortalRef p(new Portal());
	Winding f, b;
	p->plane.planenum = node->planenum;
	p->plane.winding.Initialize(m_planes.Plane(node->planenum), SceneFile::kMaxRange);

	// split portal by all bounding portals that look into the node.
	int bside;
	for (PortalRef bounding = node->portals; bounding; bounding = bounding->next[bside]) {
		RAD_ASSERT((bounding->plane.planenum&~1) != (node->planenum&~1));
		bside = bounding->nodes[1] == node;
		p->plane.winding.Split(
			m_planes.Plane(bounding->plane.planenum),
			&f,
			&b,
			kSplitEpsilon
		);

		if (bside == 1) {
			std::swap(f, b);
		}

		//if (f.Empty() && b.Empty()) // winding is *on* plane
		//{
		//	continue;
		////}

		if (f.Empty()) {
#if defined(RAD_OPT_DEBUG)
			ValueType d[4];
			const Plane &temp = m_planes.Plane(p->plane.planenum);
			const Plane &plane = m_planes.Plane(bounding->plane.planenum);
			d[0] = math::Abs(temp.A()-plane.A());
			d[1] = math::Abs(temp.B()-plane.B());
			d[2] = math::Abs(temp.C()-plane.C());
			d[3] = math::Abs(temp.D()-plane.D());
			if (0)
			{
//				DisplayPortals(0, bounding.get(), p.get());
			}
#endif
			return;
			//if (bounding->bounding) return; // clipped by bounding plane.

			// this portal no longer has a valid winding, but needs to be kept as a splitter.
			//p->bounding = true;
			//p->plane.winding.Initialize(m_planes.Plane(node->planenum), Map::MaxRange);
			break;
		}

		p->plane.winding = f;
	}

	p->onNode = node;
	AddPortalToNodes(p, node->children[0].get(), node->children[1].get());
}

void BSPBuilder::SplitNodePortals(Node *node) {
	// partition the nodes bounding portals by its plane, and assign them to
	// the nodes children.
	Winding f, b;
	const Plane &plane = m_planes.Plane(node->planenum);

	while (node->portals) {
		PortalRef p = node->portals;
		int side = p->nodes[1] == node;
		Node *other = p->nodes[side^1];

		RemovePortalFromNode(p, p->nodes[0]);
		RemovePortalFromNode(p, p->nodes[1]);

		//if (p->bounding) continue;

		p->plane.winding.Split(plane, &f, &b, kSplitEpsilon);

		if (!f.Empty() && !b.Empty()) {
			if (++m_numPortals % 1000 == 0)
				EmitProgress();
		}

		if (f.Empty() && b.Empty()) {
#if defined(RAD_OPT_DEBUG)
			Portal z;
			z.plane.winding.Initialize(plane, 1024.0f);
			ValueType d[4];
			const Plane &temp = m_planes.Plane(p->plane.planenum);
			d[0] = math::Abs(temp.A()-plane.A());
			d[1] = math::Abs(temp.B()-plane.B());
			d[2] = math::Abs(temp.C()-plane.C());
			d[3] = math::Abs(temp.D()-plane.D());
			if (0)
			{
//				DisplayPortals(0, &z, p.get());
			}
#endif
			continue;

			// put on both sides
	//		Plane::SideType s = p->plane.winding.MajorSide(plane, ValueType(0));
	////		p->bounding = true;
	//		switch (s)
	//		{
	//		case Plane::Front:
	//			if (side == 0)
	//				AddPortalToNodes(p, node->children[0].get(), other);
	//			else
	//				AddPortalToNodes(p, other, node->children[0].get());
	//			break;
	//		case Plane::Back:
	//			if (side == 0)
	//				AddPortalToNodes(p, node->children[1].get(), other);
	//			else
	//				AddPortalToNodes(p, other, node->children[1].get());
	//			break;
	//		case Plane::On:
	//			INTERNAL_COMPILER_ERROR();
	//			break;
	//		}

			/*PortalRef z(new Portal(*p));
			z->plane.winding = p->plane.winding;
			z->bounding = p->bounding = true;
			
			if (side == 0)
			{
				AddPortalToNodes(p, node->children[0].get(), other);
				AddPortalToNodes(z, node->children[1].get(), other);
			}
			else
			{
				AddPortalToNodes(p, other, node->children[0].get());
				AddPortalToNodes(z, other, node->children[1].get());
			}*/
		}
		else if (!f.Empty() && !b.Empty()) {
			PortalRef z(new Portal(*p));
			p->plane.winding = f;
			z->plane.winding = b;
			
			if (side == 0) {
				AddPortalToNodes(p, node->children[0].get(), other);
				AddPortalToNodes(z, node->children[1].get(), other);
			} else {
				AddPortalToNodes(p, other, node->children[0].get());
				AddPortalToNodes(z, other, node->children[1].get());
			}
		}
		else if (b.Empty()) { // front
			RAD_ASSERT(!f.Empty());
			if (side == 0)
				AddPortalToNodes(p, node->children[0].get(), other);
			else
				AddPortalToNodes(p, other, node->children[0].get());
		} else {
			if (side == 0)
				AddPortalToNodes(p, node->children[1].get(), other);
			else
				AddPortalToNodes(p, other, node->children[1].get());
		}

	}
}

void BSPBuilder::FindPortalNodeFaces(Node *node)
{
	if (node->planenum != kPlaneNumLeaf) {
		FindPortalNodeFaces(node->children[0].get());
		FindPortalNodeFaces(node->children[1].get());
	}

	if (!(node->contents&kContentsFlag_VisibleContents))
		return;

	int side;
	for (PortalRef p = node->portals; p; p = p->next[side]) {
		side = p->nodes[1] == node;
		//if (p->bounding)
		//{
		//	p->bounding = false;
		//	Vec3 org = WindingCenter(p->plane.winding);
		//	p->plane.winding.Initialize(m_planes.Plane(p->plane.planenum), 600.0);
		//	/*for (size_t i = 0; i < p->plane.winding.Vertices().size(); ++i)
		//	{
		//		p->plane.winding.Vertices()[i] += org;
		//	}*/
		//}
		if (p->bounding || !p->original.empty()) continue;
		int contents = p->nodes[0]->contents ^ p->nodes[1]->contents;
		if (contents&kContentsFlag_VisibleContents) {
			int planenum = p->plane.planenum;
			for (TriModelFragVec::iterator it = node->models.begin(); it != node->models.end(); ++it) {
				for (PolyVec::iterator poly = (*it)->polys.begin(); poly != (*it)->polys.end(); ++poly) {
					// because we have inward facing geometry, any matching planenum will work, we don't dictate
					// which direction the faces must point.
					if ((*poly)->planenum == planenum || (*poly)->planenum == (planenum^1)) {
						p->contents |= contents;
						p->original.push_back((*poly)->original);
						p->poly = (*poly).get();
					}
				}
			}
		}
	}
}

void BSPBuilder::MakeTreePortals(Node *node) {
	if (node->planenum == kPlaneNumLeaf)
		return;
	
	MakeNodePortal(node);
	SplitNodePortals(node);
//	DisplayPortals(node);
	MakeTreePortals(node->children[0].get());
	MakeTreePortals(node->children[1].get());
}

} // solid_bsp
} // tools

#endif // RAD_OPT_TOOLS
