/*! \file SolidBSPEmit.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#include RADPCH
#include "SolidBSP.h"
#include "../CinematicsBuilder.h"

using namespace world::bsp_file;

namespace tools {
namespace solid_bsp {

namespace {
enum {
	kMaxBatchElements = Kilo*64
};
}

void BSPBuilder::EmitBSPFile() {
	m_bspFile.reset(new world::bsp_file::BSPFileBuilder());

	Log("------------\n");
	Log("EmitBSPFile...\n");

	EmitBSPMaterials();
	EmitBSPNodes(m_root.get(), -1);
	EmitBSPPlanes();
	EmitBSPAreas();
	EmitBSPEntities();

	int skaSize = EmitBSPCinematics();
	if (m_result < pkg::SR_Success)
		return;

	// Stats:
	Log("------------\n");
	Log("\t%8d Strings\n", m_bspFile->numStrings.get());
	Log("\t%8d Entities\n", m_bspFile->numEntities.get());
	Log("\t%8d Nodes\n", m_bspFile->numNodes.get());
	Log("\t%8d Leafs\n", m_bspFile->numLeafs.get());
	Log("\t%8d Areas\n", m_bspFile->numAreas.get());
	Log("\t%8d Planes\n", m_bspFile->numPlanes.get());
	Log("\t%8d Models\n", m_bspFile->numModels.get());
	Log("\t%8d ClipSurfaces\n", m_bspFile->numClipSurfaces.get());
	Log("\t%8d Verts\n", m_bspFile->numVerts.get());
	Log("\t%8d Indices\n", m_bspFile->numIndices.get());
	Log("\t%8d Camera TMs\n", m_bspFile->numCameraTMs.get());
	Log("\t%8d Camera Tracks\n", m_bspFile->numCameraTracks.get());
	Log("\t%8d Cinematic Triggers\n", m_bspFile->numCinematicTriggers.get());
	Log("\t%8d Cinematics\n", m_bspFile->numCinematics.get());
	Log("\t%8d Skas\n", m_bspFile->numSkas.get());
	Log("\t%8d Actors\n", m_bspFile->numActors.get());
	Log("\t%8d Actor Indices\n", m_bspFile->numActorIndices.get());
	
	SizeBuffer memSize;
	FormatSize(memSize, skaSize);
	Log("\t%s Ska Data\n", memSize);
}

void BSPBuilder::EmitBSPMaterials() {

	typedef zone_set<String, world::bsp_file::ZBSPBuilderT>::type StringSet;

	StringSet mats;

	for (SceneFile::TriModel::Vec::const_iterator m = m_map->worldspawn->models.begin(); m != m_map->worldspawn->models.end(); ++m) {
		const SceneFile::TriModel::Ref &trim = *m;

		if (trim->ignore)
			continue;

		for (SceneFile::TriFaceVec::const_iterator f = trim->tris.begin(); f != trim->tris.end(); ++f) {
			const SceneFile::TriFace &trif = *f;

			const SceneFile::Material &mat = m_map->mats[(*f).mat];
			mats.insert(mat.name);
		}
	}

	m_bspFile->ReserveMaterials((int)mats.size());
	m_bspFile->ReserveStrings((int)mats.size());

	for (StringSet::const_iterator it = mats.begin(); it != mats.end(); ++it) {
		BSPMaterial *bspMaterial = m_bspFile->AddMaterial();
		bspMaterial->string = m_bspFile->numStrings;
		*m_bspFile->AddString() = *it;
	}
}

void BSPBuilder::EmitBSPEntities() {
	EmitBSPEntity(m_map->worldspawn);
	for (SceneFile::Entity::Vec::const_iterator it = m_map->ents.begin(); it != m_map->ents.end(); ++it)
		EmitBSPEntity(*it);
}

void BSPBuilder::EmitBSPEntity(const SceneFile::Entity::Ref &entity)
{
	BSPEntity *bspEntity = m_bspFile->AddEntity();
	bspEntity->firstString = m_bspFile->numStrings;
	bspEntity->numStrings = (int)entity->keys.pairs.size();

	for (world::Keys::Pairs::const_iterator it = entity->keys.pairs.begin(); it != entity->keys.pairs.end(); ++it) {
		*m_bspFile->AddString() = it->first;
		*m_bspFile->AddString() = it->second;
	}
}

void BSPBuilder::EmitBSPAreas() {
	m_bspFile->ReserveAreas((int)m_areas.size());

	for (AreaVec::const_iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
		const AreaRef &area = *it;

		BSPArea *bspArea = m_bspFile->AddArea();

		for (int i = 0; i < 3; ++i) {
			bspArea->mins[i] = (float)area->bounds.Mins()[i];
			bspArea->maxs[i] = (float)area->bounds.Maxs()[i];
		}

		bspArea->firstPortal = std::numeric_limits<U32>::max();
		bspArea->numPortals = 0;

		EmitBSPAreaportals(m_root.get(), area->area, *bspArea);

		bspArea->rootNode = -1;

		if (area->root)
			bspArea->rootNode = EmitBSPAreaNode(area->root.get(), *bspArea);
	}
}

S32 BSPBuilder::EmitBSPAreaNode(AreaNode *node, world::bsp_file::BSPArea &area) {

	if (node->planenum == kPlaneNumLeaf)
		return EmitBSPAreaLeaf(node, area);

	node->emitId = (int)m_bspFile->numAreaNodes.get();
	BSPAreaNode *bspNode = m_bspFile->AddAreaNode();

	for (int i = 0; i < 3; ++i) {
		bspNode->mins[i] = (float)node->bounds.Mins()[i];
		bspNode->maxs[i] = (float)node->bounds.Maxs()[i];
	}

	bspNode->parent = (S32)(node->parent ? node->parent->emitId : -1);
	bspNode->planenum = node->planenum;

	S32 front = EmitBSPAreaNode(node->children[0].get(), area);
	S32 back = EmitBSPAreaNode(node->children[1].get(), area);

	bspNode = const_cast<BSPAreaNode*>(m_bspFile->AreaNodes() + node->emitId);
	bspNode->children[0] = front;
	bspNode->children[1] = back;

	return (S32)node->emitId;
}

S32 BSPBuilder::EmitBSPAreaLeaf(AreaNode *leaf, world::bsp_file::BSPArea &area) {

	leaf->emitId = (int)m_bspFile->numAreaLeafs.get();

	BSPAreaLeaf *bspLeaf = m_bspFile->AddAreaLeaf();

	for (int i = 0; i < 3; ++i) {
		bspLeaf->mins[i] = (float)leaf->bounds.Mins()[i];
		bspLeaf->maxs[i] = (float)leaf->bounds.Maxs()[i];
	}
	
	typedef zone_set<int, ZBSPBuilderT>::type IntSet;
	IntSet mats;

	// gather materials.

	for (AreaNodePolyVec::const_iterator it = leaf->tris.begin(); it != leaf->tris.end(); ++it) {
		const AreaNodePolyRef &poly = *it;
		RAD_ASSERT(poly->tri);
		mats.insert(poly->tri->mat);
	}

	bspLeaf->firstModel = m_bspFile->numModels;
	bspLeaf->numModels = 0;

	for (int c = 1; c <= kMaxUVChannels; ++c){
		for (IntSet::const_iterator it = mats.begin(); it != mats.end(); ++it) {
			EmitTriModel m;
			m.mat = *it;
			m.numChannels = c;

			for (AreaNodePolyVec::const_iterator it = leaf->tris.begin(); it != leaf->tris.end(); ++it) {
				const AreaNodePolyRef &poly = *it;

				RAD_ASSERT(poly->tri);

				if (poly->tri->mat != m.mat)
					continue;
				if (poly->tri->model->numChannels != c)
					continue;

				int numVerts = (int)poly->winding.Vertices().size();

				if (((int)m.indices.size() >= kMaxBatchElements-numVerts) ||
					((int)m.verts.size() >= kMaxBatchElements-numVerts)) {
					EmitBSPModel(m);
					m.Clear();
					++bspLeaf->numModels;
				}

				for (AreaNodeWinding::VertexListType::const_iterator it = poly->winding.Vertices().begin(); it != poly->winding.Vertices().end(); ++it) {
					m.AddVertex(EmitTriModel::Vert(*it));
				}
			}

			if (!m.verts.empty()) {
				EmitBSPModel(m);
				++bspLeaf->numModels;
			}
		}
	}

	return -(leaf->emitId + 1);
}

void BSPBuilder::EmitBSPAreaportals(Node *leaf, int areaNum, BSPArea &area) {
	if (leaf->planenum != kPlaneNumLeaf) {
		EmitBSPAreaportals(leaf->children[0].get(), areaNum, area);
		EmitBSPAreaportals(leaf->children[1].get(), areaNum, area);
		return;
	}

	if (!leaf->area || (leaf->area->area != areaNum))
		return;

	int side;
	for (PortalRef p = leaf->portals; p; p = p->next[side]) {
		side = p->nodes[1] == leaf;
		Node *other = p->nodes[side^1];

		if (!other->area || (other->area->area == areaNum))
			continue;

		RAD_ASSERT(!p->plane.winding.Vertices().empty());

		// NOTE: areaportals are somewhat a casualty of war here.
		// Our original map skin is made of triangle mesh, not quads or other ngons.
		// This means that referecing the original model triangles that contributed 
		// to an area portal ends up producing triangle shaped portals, instead of
		// quads and ngons which cleanly seperate areas. 
		
		// By using the leaf portals as area portals directly we can sum contributing
		// areas easily, but the downside is we may end up with many portal "fragments"
		// being turned into area portals instead of one clean ngon, however in my
		// estimation by penalizing for areaportal splits, we may get better coverage
		// from portals at the leaf level (since they are at least probably not triangles).

		// crosses area.

		if (p->emitId == -1) {
			p->emitId = (int)m_bspFile->numAreaportals.get();

			BSPAreaportal *areaportal = m_bspFile->AddAreaportal();
			areaportal->firstVert = m_bspFile->numVerts;
			areaportal->numVerts = 0;

			for (Winding::VertexListType::const_iterator it = p->plane.winding.Vertices().begin(); it != p->plane.winding.Vertices().end(); ++it) {
				BSPVertex *v = m_bspFile->AddVertex();
				for (int i = 0; i < world::bsp_file::kMaxUVChannels; ++i) {
					v->st[i*2+0] = 0.f;
					v->st[i*2+1] = 0.f;
				}
				v->n[0] = 0.f;
				v->n[1] = 0.f;
				v->n[2] = 0.f;
				v->v[0] = (*it)[0];
				v->v[1] = (*it)[1];
				v->v[2] = (*it)[2];
				++areaportal->numVerts;
			}
		}

		if (p->areas[side] != -1) {
			Log("WARNING: Areaportal touches more than 2 areas (%d, %d, %d), map will not render correctly.\n", areaNum, other->area->area, p->areas[side]);
		} else {
			p->areas[side] = areaNum;
			if (area.firstPortal == std::numeric_limits<U32>::max()) {
				area.firstPortal = m_bspFile->numAreaportalIndices;
			}
			*m_bspFile->AddAreaportalIndex() = (U16)p->emitId;
			++area.numPortals;

			BSPAreaportal *areaportal = const_cast<BSPAreaportal*>(m_bspFile->Areaportals() + p->emitId);
			areaportal->areas[side] = (U32)areaNum;
		}
	}
}

void BSPBuilder::EmitTriModel::AddVertex(const Vert &vert) {
	VertMap::iterator it = vmap.find(vert);
	if (it != vmap.end()) {
		indices.push_back(it->second);
		return;
	}
	int ofs = (int)verts.size();
	verts.push_back(vert);
	vmap.insert(VertMap::value_type(vert, ofs));
	indices.push_back(ofs);
}

void BSPBuilder::EmitBSPModel(const EmitTriModel &model) {
	
	BSPModel *bspModel = m_bspFile->AddModel();
	bspModel->firstVert = m_bspFile->numVerts;
	bspModel->numVerts = (U32)model.verts.size();
	bspModel->firstIndex = m_bspFile->numIndices;
	bspModel->numIndices = (U32)model.indices.size();
	bspModel->material = (U32)model.mat;
	bspModel->numChannels = (U32)model.numChannels;
	
	m_bspFile->ReserveVertices((int)model.verts.size());
	m_bspFile->ReserveIndices((int)model.indices.size());

	for (EmitTriModel::VertVec::const_iterator it = model.verts.begin(); it != model.verts.end(); ++it)
	{
		const EmitTriModel::Vert &v = *it;
		BSPVertex *bspV = m_bspFile->AddVertex();

		int i;
		for (i = 0; i < 3; ++i) {
			bspV->v[i] = (float)v.pos[i];
			bspV->n[i] = (float)v.normal[i];
		}

		for (i = 0; i < model.numChannels && i < world::bsp_file::kMaxUVChannels; ++i) {
			bspV->st[i*2+0] = (float)v.st[i][0];
			bspV->st[i*2+1] = (float)v.st[i][1];
		}

		for (; i < world::bsp_file::kMaxUVChannels; ++i)
			bspV->st[i*2+0] = bspV->st[i*2+1] = 0.f;
	}

	for (EmitTriModel::Indices::const_iterator it = model.indices.begin(); it != model.indices.end(); ++it) { 
		// NOTE: IOS only supports GL_UNSIGNED_SHORT
		RAD_ASSERT(*it < std::numeric_limits<U16>::max());
		*m_bspFile->AddIndex() = (U16)*it;
	}
}

S32 BSPBuilder::EmitBSPNodes(const Node *node, S32 parent) {
	if (node->planenum == kPlaneNumLeaf) {
		S32 index = m_bspFile->numLeafs;

		BSPLeaf *leaf = m_bspFile->AddLeaf();

		leaf->area = node->area ? node->area->area : -1;
		leaf->contents = node->contents;
		leaf->parent = parent;

		for (int i = 0; i < 3; ++i) {
			leaf->mins[i] = node->bounds.Mins()[i];
			leaf->maxs[i] = node->bounds.Maxs()[i];
		}

		leaf->firstClipSurface = m_bspFile->numClipSurfaces;
		leaf->numClipSurfaces = 0;

		EmitBSPClipSurfaces(node, leaf);
		return -(index + 1);
	}

	S32 index = (S32)m_bspFile->numNodes;
	BSPNode *bspNode = m_bspFile->AddNode();

	bspNode->parent = parent;
	bspNode->planenum = (U32)node->planenum;

	for (int i = 0; i < 3; ++i) {
		bspNode->mins[i] = node->bounds.Mins()[i];
		bspNode->maxs[i] = node->bounds.Maxs()[i];
	}

	U32 left = EmitBSPNodes(node->children[0].get(), index);
	U32 right = EmitBSPNodes(node->children[1].get(), index);

	// modified node vector.
	bspNode = const_cast<BSPNode*>(m_bspFile->Nodes() + index);
	bspNode->children[0] = left;
	bspNode->children[1] = right;
	return index;
}

void BSPBuilder::EmitBSPClipSurfaces(const Node *node, world::bsp_file::BSPLeaf *leaf) {
	return;

	if (leaf->contents == 0)
		return; // this leaf has nothing to clip against.

	// walk the bounding portals and generate clip surfaces.

	int side;
	for (PortalRef p = node->portals; p; p = p->next[side]) {
		side = p->nodes[1] == node;

		int planenum = p->plane.planenum ^ (side^1);

		U32 i;
		for (i = 0; i < leaf->numClipSurfaces; ++i) {
			const BSPClipSurface *clip = m_bspFile->ClipSurfaces() + i + leaf->firstClipSurface;
			if (clip->planenum == planenum) {
				Log("WARNING: duplicate planenums present in clip surfaces, bounding portal ignored.\n");
				break; // already have this plane.
			}
		}

		if (i != leaf->numClipSurfaces)
			continue;

		BSPClipSurface *clip = m_bspFile->AddClipSurface();
		clip->contents = p->poly->original->contents;
		clip->surface = p->poly->original->surface;
		clip->planenum = p->plane.planenum ^ (side^1); // always faces away from the leaf.
		clip->flags = 0;
		++leaf->numClipSurfaces;
	}

	EmitBSPClipBevels(leaf);
}

void BSPBuilder::EmitBSPClipBevels(world::bsp_file::BSPLeaf *leaf) {
	
	Vec3 v;
	int i, j;

	for(i = 0; i < 2; ++i) {
		for(j = 0; j < 3; ++j) {
			v = Vec3::Zero;
			v[j] = i ? ValueType(-1) : ValueType(1);

			U32 k;
			for (k = 0; k < leaf->numClipSurfaces; ++k) {
				const BSPClipSurface *clip = m_bspFile->ClipSurfaces() + k + leaf->firstClipSurface;
				const Plane &p = m_planes.Plane((int)clip->planenum);

				if (p.Normal().Dot(v) > ValueType(0.999))
					break; // found.
			}

			if (k == leaf->numClipSurfaces)
				break; // need bevel.
		}

		if (j != 3)
			break; // need bevel.
	}

	if (i == 2)
		return; // don't need bevel planes.

	// Create mesh.

}

void BSPBuilder::EmitBSPPlanes() {
	m_bspFile->ReservePlanes((int)m_planes.Planes().size());

	for (PlaneHash::PlaneVec::const_iterator it = m_planes.Planes().begin(); it != m_planes.Planes().end(); ++it) {
		const Plane &pl = *it;

		world::bsp_file::BSPPlane *bspPlane = m_bspFile->AddPlane();
		bspPlane->p[0] = pl.A();
		bspPlane->p[1] = pl.B();
		bspPlane->p[2] = pl.C();
		bspPlane->p[3] = pl.D();
	}
}

U32 BSPBuilder::FindBSPMaterial(const char *name) {

	RAD_ASSERT(name);
	String reqName(CStr(name));

	const U32 numMaterials = m_bspFile->numMaterials;
	for (U32 i = 0; i < numMaterials; ++i) {
		const BSPMaterial *m = m_bspFile->Materials() + i;
		String matName = CStr(m_bspFile->String(m->string));

		if (matName == reqName)
			return i;
	}

	Log("WARNING: FindBSPMaterial(%s) failed.\n", name);
	return 0;
}

int BSPBuilder::EmitBSPCinematics() {
	CinematicsBuilder b;

	if (!b.Compile(*m_map, m_bspFile)) {
		SetResult(pkg::SR_CompilerError);
		return 0;
	}

	return b.skaSize;
}

} // solid_bsp
} // tools
