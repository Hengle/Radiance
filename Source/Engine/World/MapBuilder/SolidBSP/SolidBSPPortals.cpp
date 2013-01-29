/*! \file SolidBSPPortals.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#include RADPCH
#include "SolidBsp.h"

using namespace world::bsp_file;

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
				last->next[ls] = test->next[pside];
			} else {
				node->portals = test->next[pside];
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
	m_numPortalSplits = 0;
	m_numPortalFaces = 0;

	m_outside.portals.reset();
	m_outside.children[0].reset();
	m_outside.children[1].reset();
	m_outside.models.clear();
	m_outside.occupied = false;
	m_outside.area = 0;
	m_outside.planenum = kPlaneNumLeaf;
	m_outside.bounds = m_root->bounds;
	m_outside.contents = 0;
	m_outside.bounds.Expand(ValueType(32), ValueType(32), ValueType(32)); // avoid null volumes.
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
	Log("\n%d portal faces(s)\n", m_numPortalFaces);
	//if (g_glDebug) { DisplayPortals(0, 0, 0, false, Map::VisibleContents); }
}

void BSPBuilder::MakeNodePortal(Node *node) {
	PortalRef p(new Portal());
	Winding f, b;
	p->plane.planenum = node->planenum;
	p->plane.winding.Initialize(m_planes.Plane(node->planenum), SceneFile::kMaxRange);

	// split portal by all bounding portals that look into the node.
	int side;
	for (PortalRef bounding = node->portals; bounding; bounding = bounding->next[side]) {
		RAD_ASSERT((bounding->plane.planenum&~1) != (node->planenum&~1));
		
		side = bounding->nodes[1] == node;

		Plane::SideType s = p->plane.winding.Side(
			m_planes.Plane(bounding->plane.planenum),
			kBSPSplitEpsilon
		);

		if (s == Plane::On) {
			// portal is too small to split
			continue;
		}
		
		if (s == Plane::Front && side) {
			Log("WARNING: portal clipped away (back)!\n");
			return;
		} else if (s == Plane::Back && !side) {
			Log("WARNING: portal clipped away (front)!\n");
			return;
		} else if (s == Plane::Cross) {
			p->plane.winding.Split(
				m_planes.Plane(bounding->plane.planenum),
				&f,
				&b,
				kBSPSplitEpsilon
			);

			if (f.Empty())
				Log("WARNING: portal clipped away (cross/front)!\n");
			if (b.Empty())
				Log("WARNING: portal clipped away (cross/back)!\n");

			if (side)
				std::swap(f, b);

			if (f.Empty())
				return;

			p->plane.winding = f;
		}
	}

	p->onNode = node;
	AddPortalToNodes(p, node->children[0].get(), node->children[1].get());
}

void BSPBuilder::SplitNodePortals(Node *node) {
	// split bounding portals by node, and assign them to
	// the nodes children.

	Winding f, b;
	const Plane &plane = m_planes.Plane(node->planenum);

	while (node->portals) {
		PortalRef p = node->portals;
		int side = p->nodes[1] == node;
		Node *other = p->nodes[side^1];

		RemovePortalFromNode(p, p->nodes[0]);
		RemovePortalFromNode(p, p->nodes[1]);

		p->plane.winding.Split(plane, &f, &b, kBSPSplitEpsilon);

		if (++m_numPortalSplits % 1000 == 0)
			EmitProgress();
		
		if (f.Empty() && b.Empty()) {
			// add to both sides
			PortalRef z(new Portal(*p));
			z->plane.winding = p->plane.winding;
			
			if (side == 0) {
				AddPortalToNodes(p, node->children[0].get(), other);
				AddPortalToNodes(z, node->children[1].get(), other);
			} else {
				AddPortalToNodes(p, other, node->children[0].get());
				AddPortalToNodes(z, other, node->children[1].get());
			}
		} else if (!f.Empty() && !b.Empty()) {
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
		
		if (!p->original.empty()) 
			continue;

		int contents = (p->nodes[0]->contents ^ p->nodes[1]->contents) & kContentsFlag_VisibleContents;
		for (int i = kContentsFlag_FirstVisibleContents; i <= kContentsFlag_LastVisibleContents; i <<= 1) {
			if (i & contents) {
				contents = i;
				break;
			}
		}

		if (!contents)
			continue;

		// matching always faces away from the leaf
		int planenum = p->plane.planenum ^ (side^1);

		for (TriModelFragVec::iterator it = node->models.begin(); it != node->models.end(); ++it) {

			if (!(contents & (*it)->original->contents))
				continue;

			for (PolyVec::iterator poly = (*it)->polys.begin(); poly != (*it)->polys.end(); ++poly) {
					
				if ((*poly)->planenum == planenum) {
					p->contents |= contents;
					p->original.push_back((*poly)->original);
					p->poly = (*poly).get();
					++m_numPortalFaces;
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
