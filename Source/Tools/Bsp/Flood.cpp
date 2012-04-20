// Flood.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Bsp.h"
#include <deque>

bool BSP::FloodFill()
{
	bool didFlood = false;

	ResetProgress();
	m_numInsideNodes = 0;
	Log(LogNormal, "------------\n");
	Log(LogNormal, "Flood Fill...\n");
	
	for (Map::EntityVec::iterator it = m_map.ents.begin(); it != m_map.ents.end(); ++it)
	{
		const Map::EntityRef &ent = *it;
		if (ent == m_map.worldspawn) continue;
		Node *leaf = LeafForPoint(ent->origin);
		if (!leaf)
		{
			INTERNAL_COMPILER_ERROR();
		}
		if (leaf->contents & Map::ContentsSolid)
		{
			Log(LogWarning, "\nWARNING: entity named '%s' id %d is in solid space.\n", ent->name.c_str(), ent->id);
			continue;
		}

		didFlood = true;
		EmitProgress();
		PortalFlood(leaf, 1);

		if (m_outside.occupied)
		{
			m_leakEnt = ent;
			MarkLeakTrail();
			Log(LogWarning, "\n!!!!!!!!!!!\nMAP LEAKED, level will not be optimized (entity named '%s' id %d can see outside)!\n!!!!!!!!!!!\n", ent->name.c_str(), ent->id);
			if (g_glDebug) { DisplayPortals(0, 0, 0, true, Map::VisibleContents); }
			DumpLeakFile();
			return false;
		}
	}

	if (!didFlood)
	{
		Log(LogWarning, "\n!!!!!!!!!!!\nWARNING: no valid entities found for flood, level will not be optimized.\n!!!!!!!!!!!\n");
	}
	else
	{
		Log(LogNormal, "\n");
	}

	return didFlood;
}

void BSP::FillOutside()
{
	Log(LogNormal, "------------\nFillOutside (Structural Only)\n");

	m_numOutsideNodes = m_numInsideNodes = m_numOutsideModels = m_numInsideModels = 0;
	m_numInsideTris = m_numOutsideTris = 0;

	MarkOccupiedNodeFaces(m_root.get());

	for (Map::TriModelVec::iterator m = m_map.worldspawn->models.begin(); m != m_map.worldspawn->models.end(); ++m)
	{
		if (!((*m)->contents & Map::ContentsSolid)) continue; // skip non solids for outside

		bool outside = true;
		for (Map::TriFaceVec::iterator f = (*m)->tris.begin(); f != (*m)->tris.end(); ++f)
		{
			if ((*f).outside)
			{
				++m_numOutsideTris;
			}
			else
			{
				++m_numInsideTris;
				outside = false;
			}
		}

		(*m)->outside = outside;

		if (outside)
		{
			++m_numOutsideModels;
		}
		else
		{
			++m_numInsideModels;			
		}
	}

	Log(LogNormal, "Inside : %d node(s), %d model(s), %d tri(s)\n", m_numInsideNodes, m_numInsideModels, m_numInsideTris);
	Log(LogNormal, "Outside: %d node(s), %d model(s), %d tri(s)\n", m_numOutsideNodes, m_numOutsideModels, m_numOutsideTris);
	Log(LogNormal, "Total  : %d node(s), %d model(s), %d tri(s)\n", m_numInsideNodes+m_numOutsideNodes, m_numInsideModels+m_numOutsideModels, m_numInsideTris+m_numOutsideTris);
}

void BSP::MarkOccupiedNodeFaces(Node *node)
{
	if (node->planenum != PlaneNumLeaf)
	{
		MarkOccupiedNodeFaces(node->children[0].get());
		MarkOccupiedNodeFaces(node->children[1].get());
		return;
	}

	// mark all original faces that have fragments in occupied leaves as "inside".
	if (node->occupied)
	{
		++m_numInsideNodes;

		int side;
		for (PortalRef p = node->portals; p; p = p->next[side])
		{
			side = p->nodes[1] == node;

			// suppress areaportals
			int c = p->nodes[0]->contents ^ p->nodes[1]->contents;
			if (c & Map::ContentsAreaportal) continue;

			//if (p->nodes[0]->area != -1 && p->nodes[1]->area != -1 &&
			//	p->nodes[0]->area != p->nodes[1]->area) continue;

			
			if (!p->original.empty())
			{
				for (TriFacePtrVec::iterator it = p->original.begin(); it != p->original.end(); ++it)
				{
					(*it)->outside = false;
				}
			}
		}
	}
	else
	{
		++m_numOutsideNodes;
	}
}

void BSP::PortalFlood(Node *leaf, int depth)
{
	RAD_ASSERT(leaf->planenum == PlaneNumLeaf);

	int side;
	PortalRef p;

	if (++m_numInsideNodes % 1000 == 0) { EmitProgress(); }

	p = leaf->portals;
	leaf->occupied = depth;

	while (p)
	{
		side = p->nodes[1] == leaf;
		Node *other = p->nodes[side^1];
		if (!other->occupied)
		{
			if (!(other->contents&Map::SolidContents))
			{
				//RAD_ASSERT(!p->original.empty() || !p->contents);
				PortalFlood(other, depth+1);
			}
		}
		p = p->next[side];
	}
}

void BSP::MarkLeakTrail()
{
	Node *node = &m_outside;
	int count = m_outside.occupied;

	while (node->occupied > 1)
	{
		Node *next = 0;
		Portal *mark = 0;
		int side;

		for (PortalRef p = node->portals; p; p = p->next[side])
		{
			side = p->nodes[1] == node;
			int other = side^1;

			// find shortest path.
			if (p->nodes[other]->occupied && p->nodes[other]->occupied < count)
			{
				mark = p.get();
				next = p->nodes[other]; // cross.
				count = p->nodes[other]->occupied;
			}
		}

		if (!mark || !next) break;
		node = next;
		m_leakpts.push_back(WindingCenter(mark->plane.winding));
	}

	m_leakpts.push_back(m_leakEnt->origin);
}

void BSP::DumpLeakFile()
{
	char name[1024];
	strcpy(name, g_mapname);
	strcat(name, ".leak");
	FILE *fp = fopen(name, "wt");
	if (!fp)
	{
		Log(LogWarning, "WARNING: unable to save leak file to '%s'\n", name);
		return;
	}
	for (Vec3Vec::iterator it = m_leakpts.begin(); it != m_leakpts.end(); ++it)
	{
		float v[3];
		v[0] = float((*it).X());
		v[1] = float((*it).Y());
		v[2] = float((*it).Z());
		fprintf(fp, "( %f %f %f )\n", v[0], v[1], v[2]);
	}

	fclose(fp);
}

void BSP::AreaFlood()
{
	ResetProgress();
	Log(LogNormal, "------------\n");
	Log(LogNormal, "Area Flood...\n");
	FindAreas(m_root.get());	
	Log(LogNormal, "Set %d area(s).\n", m_areas.size());
}

void BSP::FindAreas(Node *node)
{
	if (node->planenum != PlaneNumLeaf)
	{
		FindAreas(node->children[0].get());
		FindAreas(node->children[1].get());
		return;
	}

	if (node->area) return;
	if (m_flood && !node->occupied)  return;
	if (node->contents&Map::ContentsSolid) return;
	if (node->contents&Map::ContentsAreaportal) return;

	AreaRef area(new Area());
	area->area = (int)m_areas.size();
	m_areas.push_back(area);
	AreaFlood(node, area.get());
}

void BSP::AreaFlood(Node *leaf, Area *area)
{
	RAD_ASSERT(leaf->planenum == PlaneNumLeaf);
	RAD_ASSERT(!(leaf->contents&Map::ContentsSolid));
	
	if (leaf->contents & Map::ContentsAreaportal)
	{
		if (!leaf->area) leaf->area = area;
		return;
	}

	if (leaf->area) return;
	leaf->area = area;

	int side;
	for (PortalRef p = leaf->portals; p; p = p->next[side])
	{
		side = p->nodes[1] == leaf;
		Node *other = p->nodes[side^1];
		if (other->contents & Map::ContentsSolid) continue;
		AreaFlood(other, area);
	}
}