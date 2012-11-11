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

bool BSPBuilder::EmitBSPFile() {

	if (m_ui) {
		m_ui->title = "Writing BSP File...";
		m_ui->total = 0;
		m_ui->totalProgress = 0;
		m_ui->Refresh();
	}

	m_bspFile.reset(new world::bsp_file::BSPFileBuilder());

	Log("------------\n");
	Log("EmitBSPFile...\n");

	EmitBSPMaterials();
	EmitBSPNodes(m_root.get(), -1);
	EmitBSPModels();
	EmitBSPAreas();
	EmitBSPEntities();

	if (!EmitBSPFloors())
		return false;

	EmitBSPWaypoints();
	EmitBSPPlanes();

	int skaSize = EmitBSPCinematics();
	if (m_result < pkg::SR_Success)
		return false;

	// Stats:
	Log("------------\n");
	Log("\t%8d String(s)\n", m_bspFile->numStrings.get());
	Log("\t%8d Entities\n", m_bspFile->numEntities.get());
	Log("\t%8d Node(s)\n", m_bspFile->numNodes.get());
	Log("\t%8d Leaf(s)\n", m_bspFile->numLeafs.get());
	Log("\t%8d Area(s)\n", m_bspFile->numAreas.get());
	Log("\t%8d Plane(s)\n", m_bspFile->numPlanes.get());
	Log("\t%8d Areaportal(s)\n", m_bspFile->numAreaportals.get());
	Log("\t%8d AreaportalIndice(s)\n", m_bspFile->numAreaportalIndices.get());
	Log("\t%8d Model(s)\n", m_bspFile->numModels.get());
	Log("\t%8d ClipSurface(s)\n", m_bspFile->numClipSurfaces.get());
	Log("\t%8d Vert(s)\n", m_bspFile->numVerts.get());
	Log("\t%8d Waypoint(s)\n", m_bspFile->numWaypoints.get());
	Log("\t%8d WaypointConnection(s)\n", m_bspFile->numWaypointConnections.get());
	Log("\t%8d Floor(s)\n", m_bspFile->numFloors.get());
	Log("\t%8d FloorTri(s)\n", m_bspFile->numFloorTris.get());
	Log("\t%8d FloorEdge(s)\n", m_bspFile->numFloorEdges.get());
	Log("\t%8d Indices\n", m_bspFile->numIndices.get());
	Log("\t%8d Camera TM(s)\n", m_bspFile->numCameraTMs.get());
	Log("\t%8d Camera Track(s)\n", m_bspFile->numCameraTracks.get());
	Log("\t%8d Cinematic Trigger(s)\n", m_bspFile->numCinematicTriggers.get());
	Log("\t%8d Cinematic(s)\n", m_bspFile->numCinematics.get());
	Log("\t%8d Ska(s)\n", m_bspFile->numSkas.get());
	Log("\t%8d Actor(s)\n", m_bspFile->numActors.get());
	Log("\t%8d Actor Indices\n", m_bspFile->numActorIndices.get());
	
	SizeBuffer memSize;
	FormatSize(memSize, skaSize);
	Log("\t%s Ska Data\n", memSize);

	return true;
}

/*
==============================================================================
Materials
==============================================================================
*/

void BSPBuilder::EmitBSPMaterials() {

	typedef zone_set<String, world::bsp_file::ZBSPBuilderT>::type StringSet;

	StringSet mats;

	for (SceneFile::TriModel::Vec::const_iterator m = m_map->worldspawn->models.begin(); m != m_map->worldspawn->models.end(); ++m) {
		const SceneFile::TriModel::Ref &trim = *m;

		if (trim->ignore)
			continue;

		if (!(trim->contents & kContentsFlag_EmitContents))
			continue;

		for (SceneFile::TriFaceVec::const_iterator f = trim->tris.begin(); f != trim->tris.end(); ++f) {
			const SceneFile::TriFace &trif = *f;

			if (!(trif.contents & kContentsFlag_EmitContents))
				continue;

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

/*
==============================================================================
Entities
==============================================================================
*/

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

/*
==============================================================================
Areas
==============================================================================
*/

bool BSPBuilder::EmitBSPAreas() {
	m_bspFile->ReserveAreas((int)m_areas.size());

	for (AreaVec::const_iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
		const AreaRef &area = *it;

		BSPArea *bspArea = m_bspFile->AddArea();

		SceneFile::BBox bounds;
		bounds.Initialize();

		bspArea->firstPortal = std::numeric_limits<U32>::max();
		bspArea->numPortals = 0;

		EmitBSPAreaportals(m_root.get(), area->area, *bspArea);

		bspArea->firstModel = m_bspFile->numModelIndices;
		bspArea->numModels = 0;

		for (SceneFile::TriModel::Vec::const_iterator m = m_map->worldspawn->models.begin(); m != m_map->worldspawn->models.end(); ++m) {
			const SceneFile::TriModel::Ref &trim = *m;

			if (trim->areas.find(area->area) == trim->areas.end())
				continue; // not in this area.
			if (trim->emitIds.empty())
				continue;
			
			bounds.Insert(trim->bounds);

			// NOTE this indexes all model fragments, even if they are isolated in other areas they still
			// belong to a model that touches this area.

			m_bspFile->ReserveModelIndices((int)trim->emitIds.size());

			for (SceneFile::IntVec::const_iterator id = trim->emitIds.begin(); id != trim->emitIds.end(); ++id) {
				if (*id > std::numeric_limits<U16>::max()) {
					Log("ERROR: there are too many models in this map (exceeds 32k)!\n");
					SetResult(pkg::SR_CompilerError);
					return false;
				}

				*(m_bspFile->AddModelIndex()) = (U16)*id;
				++bspArea->numModels;
			}
		}

		for (int i = 0; i < 3; ++i) {
			bspArea->mins[i] = (float)bounds.Mins()[i];
			bspArea->maxs[i] = (float)bounds.Maxs()[i];
		}
	}

	for (SceneFile::TriModel::Vec::const_iterator m = m_map->worldspawn->models.begin(); m != m_map->worldspawn->models.end(); ++m) {
		const SceneFile::TriModel::Ref &trim = *m;

		// check portal seperation

		if (!(trim->contents & kContentsFlag_Areaportal))
			continue;

		if (trim->areas.empty())
			continue;

		if (trim->portalAreas[0] == -1 ||
			trim->portalAreas[1] == -1) {
			Log("WARNING: Areaportal '%s' does not seperate areas!\n", trim->name.c_str.get());
		}
	}

	return true;
}

/*
==============================================================================
Portals
==============================================================================
*/

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

		if (p->original.empty()) {
			//Log("WARNING: portal fragment with no original!");
			continue;
		}

		SceneFile::TriFace *original = 0;
		for (TriFacePtrVec::const_iterator it = p->original.begin(); it != p->original.end(); ++it) {
			if ((*it)->contents & kContentsFlag_Areaportal) {
				original = *it;
				break;
			}
		}

		if (!original) {
			Log("WARNING: portal fragment bounds area but is not an area portal!\n");
			continue;
		}

		if (original->model->portalAreas[0] == -1) {
			original->model->portalAreas[0] = areaNum;
		} else if (original->model->portalAreas[0] != areaNum) {
			if (original->model->portalAreas[1] == -1) {
				original->model->portalAreas[1] = areaNum;
			} else if (original->model->portalAreas[1] != areaNum) {
				Log("WARNING: Areaportal '%s' touches more than 2 areas (%d, %d, %d), map will not render correctly.\n", original->model->name.c_str.get(),areaNum, other->area->area, p->areas[side]);
				continue;
			}
		}

		if (p->emitId == -1) {
			p->emitId = (int)m_bspFile->numAreaportals.get();
			original->model->portalIds.push_back(p->emitId);

			BSPAreaportal *areaportal = m_bspFile->AddAreaportal();
			areaportal->firstVert = m_bspFile->numVerts;
			areaportal->numVerts = 0;
			areaportal->planenum = p->plane.planenum;

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
			// This should never-ever happen.
			Log("WARNING: Areaportal fragment touches more than 2 areas (%d, %d, %d), map will not render correctly.\n", areaNum, other->area->area, p->areas[side]);
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

/*
==============================================================================
Models
==============================================================================
*/

void BSPBuilder::EmitBSPModels() {

	for (SceneFile::TriModel::Vec::const_iterator m = m_map->worldspawn->models.begin(); m != m_map->worldspawn->models.end(); ++m) {
		const SceneFile::TriModel::Ref &trim = *m;

		if (trim->ignore)
			continue;

		if (trim->areas.empty())
			continue;

		if (!(trim->contents & kContentsFlag_EmitContents))
			continue;

		EmitBSPModel(trim);
	}
}

void BSPBuilder::EmitBSPModel(const SceneFile::TriModel::Ref &triModel) {

	typedef zone_set<int, ZBSPBuilderT>::type IntSet;
	IntSet mats;

	// gather materials.

	for (SceneFile::TriFaceVec::const_iterator f = triModel->tris.begin(); f != triModel->tris.end(); ++f) {
		const SceneFile::TriFace &trif = *f;
		if (trif.areas.empty())
			continue;
		if (trif.surface&kSurfaceFlag_NoDraw)
			continue;
		mats.insert(trif.mat);
	}

	for (int c = 1; c <= kMaxUVChannels; ++c){
		for (IntSet::const_iterator it = mats.begin(); it != mats.end(); ++it) {
			EmitTriModel m;
			m.mat = *it;
			m.numChannels = c;
			m.bounds.Initialize();

			for (SceneFile::TriFaceVec::const_iterator f = triModel->tris.begin(); f != triModel->tris.end(); ++f) {
				const SceneFile::TriFace &trif = *f;

				if (trif.mat != m.mat)
					continue;
				if (trif.model->numChannels != c)
					continue;
				if (trif.areas.empty())
					continue;
				if (trif.surface&kSurfaceFlag_NoDraw)
					continue;

				if (((int)m.indices.size() >= kMaxBatchElements-3) ||
					((int)m.verts.size() >= kMaxBatchElements-3)) {
					triModel->emitIds.push_back(EmitBSPModel(m));
					m.Clear();
				}

				m.AddVertex(EmitTriModel::Vert(ToBSPType(triModel->verts[trif.v[0]])));
				m.AddVertex(EmitTriModel::Vert(ToBSPType(triModel->verts[trif.v[1]])));
				m.AddVertex(EmitTriModel::Vert(ToBSPType(triModel->verts[trif.v[2]])));
			}

			if (!m.verts.empty()) {
				triModel->emitIds.push_back(EmitBSPModel(m));
			}
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
	bounds.Insert(vert.pos);
}

int BSPBuilder::EmitBSPModel(const EmitTriModel &model) {
	
	BSPModel *bspModel = m_bspFile->AddModel();
	bspModel->firstVert = m_bspFile->numVerts;
	bspModel->numVerts = (U32)model.verts.size();
	bspModel->firstIndex = m_bspFile->numIndices;
	bspModel->numIndices = (U32)model.indices.size();
	bspModel->material = FindBSPMaterial(m_map->mats[model.mat].name.c_str);
	bspModel->numChannels = (U32)model.numChannels;

	for (int i = 0; i < 3; ++i) {
		bspModel->mins[i] = (float)model.bounds.Mins()[i];
		bspModel->maxs[i] = (float)model.bounds.Maxs()[i];
	}
	
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
			for (int k = 0; k < 2; ++k)
				bspV->st[i*2+k] = (float)v.st[i][k];
			for (int k = 0; k < 4; ++k)
				bspV->t[i*4+k] = (float)v.tangent[i][k];
		}

		for (; i < world::bsp_file::kMaxUVChannels; ++i) {
			for (int k = 0; k < 2; ++k)
				bspV->st[i*2+k] = 0.f;
			for (int k = 0; k < 4; ++k)
				bspV->t[i*4+k] = 0.f;
		}
	}

	for (EmitTriModel::Indices::const_iterator it = model.indices.begin(); it != model.indices.end(); ++it) { 
		// NOTE: IOS only supports GL_UNSIGNED_SHORT
		RAD_ASSERT(*it < std::numeric_limits<U16>::max());
		*m_bspFile->AddIndex() = (U16)*it;
	}

	return (int)(m_bspFile->numModels - 1);
}

/*
==============================================================================
Tree
==============================================================================
*/

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
		bspNode->mins[i] = (float)node->bounds.Mins()[i];
		bspNode->maxs[i] = (float)node->bounds.Maxs()[i];
	}

	U32 left = EmitBSPNodes(node->children[0].get(), index);
	U32 right = EmitBSPNodes(node->children[1].get(), index);

	// modified node vector.
	bspNode = const_cast<BSPNode*>(m_bspFile->Nodes() + index);
	bspNode->children[0] = left;
	bspNode->children[1] = right;
	return index;
}

/*
==============================================================================
Clip Hull (unused)
==============================================================================
*/

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

/*
==============================================================================
Planes
==============================================================================
*/

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

/*
==============================================================================
Utils
==============================================================================
*/

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

/*
==============================================================================
Floors
==============================================================================
*/

int BSPBuilder::FloorBuilder::Edge::Compare(const Edge &e) const {
	for (int i = 0; i < 2; ++i) {
		if (v[i] < e.v[i])
			return -1;
		if (v[i] > e.v[i])
			return 1;
	}

	return 0;
}

int BSPBuilder::FloorBuilder::AddVert(const Vert &v) {
	VertMap::const_iterator it = vmap.find(v);
	if (it != vmap.end())
		return it->second;

	int vertNum = (int)verts.size();
	vmap.insert(VertMap::value_type(v, vertNum));
	verts.push_back(v);

	return vertNum;
}

int BSPBuilder::FloorBuilder::AddEdge(int v0, int v1, int triNum) {
	Edge e;
	e.v[0] = v0;
	e.v[1] = v1;

	Edge::Map::iterator it = edgeMap.find(e);
	if (it == edgeMap.end()) {
		e.v[0] = v1;
		e.v[1] = v0;
		it = edgeMap.find(e);
		if (it == edgeMap.end()) {
			e.v[0] = v0;
			e.v[1] = v1;
			e.t[0] = triNum;
			int edgeNum = (int)(edges.size());
			edges.push_back(e);
			edgeMap.insert(Edge::Map::value_type(e, edgeNum));
			return edgeNum;
		}

		Edge &x = edges[it->second];
		if (x.t[1] != -1) {
			bspBuilder->Log(
				"ERROR: Floor \"%s\" has an edge that is connected to > 2 triangles!\n", 
				original.name.c_str.get()
			);
			bspBuilder->SetResult(pkg::SR_CompilerError);
			return -1;
		}

		x.t[1] = triNum;
		return it->second;
	}

	const Edge &x = edges[it->second];
	RAD_ASSERT(x.t[0] != -1);

	bspBuilder->Log(
		"ERROR: Floor \"%s\" has an edge that is connected to > 2 triangles!\n",
		original.name.c_str.get()
	);
	bspBuilder->SetResult(pkg::SR_CompilerError);
	return -1;
}

bool BSPBuilder::FloorBuilder::AddTri(const Vert &v0, const Vert &v1, const Vert &v2) {
	Tri tri;

	int triNum = (int)tris.size();

	tri.v[0] = AddVert(v0);
	tri.v[1] = AddVert(v1);
	tri.v[2] = AddVert(v2);

	tri.e[0] = AddEdge(tri.v[0], tri.v[1], triNum);
	if (tri.e[0] == -1)
		return false;

	tri.e[1] = AddEdge(tri.v[1], tri.v[2], triNum);
	if (tri.e[1] == -1)
		return false;

	tri.e[2] = AddEdge(tri.v[2], tri.v[0], triNum);
	if (tri.e[2] == -1)
		return false;

	tris.push_back(tri);
	return true;
}

bool BSPBuilder::EmitBSPFloors() {

	const SceneFile::Vec3 kZAxis(0, 0, 1);

	for (SceneFile::TriModel::Vec::const_iterator it = m_map->worldspawn->models.begin(); it != m_map->worldspawn->models.end(); ++it) {
		const SceneFile::TriModel::Ref &m = *it;

		if (!(m->contents&kContentsFlag_Floor))
			continue;

		FloorBuilder builder(*m, this);

		for (SceneFile::TriFaceVec::const_iterator fIt = m->tris.begin(); fIt != m->tris.end(); ++fIt) {
			const SceneFile::TriFace &f = *fIt;

			if (!builder.AddTri(
				m->verts[f.v[0]].pos,
				m->verts[f.v[1]].pos,
				m->verts[f.v[2]].pos
			)) {
				return false;
			}
		}

		if (builder.tris.empty())
			continue;

		int emitId = (int)m_bspFile->numFloors;
		U32 firstVert = m_bspFile->numVerts;
		U32 firstTri  = m_bspFile->numFloorTris;
		U32 firstEdge = m_bspFile->numFloorEdges;
		
		BSPFloor *bspFloor = m_bspFile->AddFloor();
		bspFloor->firstTri = std::numeric_limits<U32>::max();
		bspFloor->numTris = (U32)builder.tris.size();
		bspFloor->firstWaypoint = std::numeric_limits<U32>::max();
		bspFloor->numWaypoints = 0;

		for (FloorBuilder::VertVec::const_iterator vIt = builder.verts.begin(); vIt != builder.verts.end(); ++vIt) {
			BSPVertex *v = m_bspFile->AddVertex();
			memset(v, 0, sizeof(BSPVertex));

			v->v[0] = (*vIt)[0];
			v->v[1] = (*vIt)[1];
			v->v[2] = (*vIt)[2];
		}

		for (FloorBuilder::Edge::Vec::const_iterator eIt = builder.edges.begin(); eIt != builder.edges.end(); ++eIt) {
			const FloorBuilder::Edge &edge = *eIt;

			BSPFloorEdge *e = m_bspFile->AddFloorEdge();
			e->verts[0] = firstVert + (U32)edge.v[0];
			e->verts[1] = firstVert + (U32)edge.v[1];
			e->tris[0] = -1;
			e->tris[1] = -1;
			
			if (edge.t[0] != -1)
				e->tris[0] = firstTri + (U32)edge.t[0];
			if (edge.t[1] != -1)
				e->tris[1] = firstTri + (U32)edge.t[1];

			const FloorBuilder::Vert &v0 = builder.verts[edge.v[0]];
			const FloorBuilder::Vert &v1 = builder.verts[edge.v[1]];

			SceneFile::Vec3 vedge(v1 - v0);
			vedge.Normalize();

			if (math::Abs(vedge.Dot(kZAxis)) > 0.99) {
				Log("ERROR: Floor \"%s\" has vertical surfaces (unsupported)!\n", m->name.c_str.get());
				SetResult(pkg::SR_CompilerError);
				return false;
			}

			SceneFile::Vec3 normal(vedge.Cross(kZAxis));

			Plane plane(
				ToBSPType(normal),
				ToBSPType(v0)
			);

			e->planenum = (U32)m_planes.FindPlaneNum(plane);
		}

		for (FloorBuilder::Tri::Vec::const_iterator tIt = builder.tris.begin(); tIt != builder.tris.end(); ++tIt) {
			const FloorBuilder::Tri &tri = *tIt;

			BSPFloorTri *t = m_bspFile->AddFloorTri();
			t->edges[0] = firstEdge + (U32)tri.e[0];
			t->edges[1] = firstEdge + (U32)tri.e[1];
			t->edges[2] = firstEdge + (U32)tri.e[2];

			t->verts[0] = firstVert + (U32)tri.v[0];
			t->verts[1] = firstVert + (U32)tri.v[1];
			t->verts[2] = firstVert + (U32)tri.v[2];
		}

		m->emitIds.push_back(emitId);
	}

	return true;
}

void BSPBuilder::EmitBSPWaypoints() {

	// only emit connected waypoints

	// pass 1: emit waypoint structure, and connection

	for (SceneFile::WaypointConnection::Map::const_iterator it = m_map->waypointConnections.begin(); it != m_map->waypointConnections.end(); ++it) {
		const SceneFile::WaypointConnection::Ref &connection = it->second;
		RAD_ASSERT(connection->waypoints.head);

		if (!connection->waypoints.tail) {
			Log("WARNING: waypoint connection is missing tail (head = %d) (connection removed).\n", connection->waypoints.head->id);
			continue;
		}

		RAD_ASSERT(connection->waypoints.tail);

		if (connection->waypoints.head->floorName.empty) {
			Log("WARNING: waypoint %d has no floor name (connection removed).\n", connection->waypoints.head->id);
			continue;
		}

		if (connection->waypoints.tail->floorName.empty) {
			Log("WARNING: waypoint %d has no floor name (connection removed).\n", connection->waypoints.tail->id);
			continue;
		}

		// valid connection? must connect floors
		if (connection->waypoints.head->floorName ==
			connection->waypoints.tail->floorName) {
			// does not connect floors
			Log(
				"WARNING: waypoints connect the same floor \"%s\" (connection removed).\n", 
				connection->waypoints.head->floorName.c_str.get()
			);
			continue;
		}

		if (m_bspFile->numWaypointConnections.get() > std::numeric_limits<U16>::max()) {
			Log("WARNING: too many waypoint connections (internal error, contact programmer to increase limit)!\n");
			continue;
		}

		if (!EmitBSPWaypoint(*connection->waypoints.head) ||
			!EmitBSPWaypoint(*connection->waypoints.tail)) {
			continue;
		}

		connection->emitId = (int)m_bspFile->numWaypointConnections.get();
		BSPWaypointConnection *c = m_bspFile->AddWaypointConnection();

		c->flags = (S32)connection->flags;
		c->waypoints[0] = (U32)connection->waypoints.head->emitId;
		c->waypoints[1] = (U32)connection->waypoints.tail->emitId;

		for (int i = 0; i < 2; ++i) {
			for (int k = 0; k < 3; ++k) {
				c->ctrls[i][k] = connection->ctrls[i][k];
			}
		}
	}

	// pass 2: emit waypoint connection indices

	for (SceneFile::Waypoint::Map::const_iterator it = m_map->waypoints.begin(); it != m_map->waypoints.end(); ++it) {
		const SceneFile::Waypoint::Ref &waypoint = it->second;

		if (waypoint->emitId < 0)
			continue;

		BSPWaypoint *w = const_cast<BSPWaypoint*>(m_bspFile->Waypoints() + waypoint->emitId);

		m_bspFile->ReserveWaypointConnectionIndices((int)waypoint->connections.size());

		for (SceneFile::WaypointConnection::Map::const_iterator it = waypoint->connections.begin(); it != waypoint->connections.end(); ++it) {
			const SceneFile::WaypointConnection::Ref &connection = it->second;
			if (!connection->waypoints.tail)
				continue; // bad connection (skipped in pass 1 as well).

			if (connection->waypoints.head->emitId == -1 ||
				connection->waypoints.tail->emitId == -1) {
				continue; // bad data (skipped in pass 1 as well).
			}

			if (connection->emitId < 0)
				continue;

			if (w->firstConnection == std::numeric_limits<U32>::max())
				w->firstConnection = m_bspFile->numWaypointConnectionIndices;

			*m_bspFile->AddWaypointConnectionIndex() = (U16)connection->emitId;
			++w->numConnections;
		}
	}

}

bool BSPBuilder::EmitBSPWaypoint(SceneFile::Waypoint &waypoint) {

	if (waypoint.emitId > -1)
		return true; // already emitted.

	if (waypoint.floorName.empty) {
		Log("WARNING: waypoint id %d does not have a floor name set (waypoint removed).\n", waypoint.id);
		return false;
	}

	int floorNum = FindBSPFloor(waypoint.floorName.c_str);
	if (floorNum < 0) {
		Log("WARNING: Floor \"%s\" does not exist (waypoint removed).\n", waypoint.floorName.c_str.get());
		return false;
	}

	waypoint.emitId = (int)m_bspFile->numWaypoints.get();

	BSPWaypoint *w = m_bspFile->AddWaypoint();
	w->floorNum = (U32)floorNum;
	w->pos[0] = waypoint.pos[0];
	w->pos[1] = waypoint.pos[1];
	w->pos[2] = waypoint.pos[2];

	w->firstConnection = std::numeric_limits<U32>::max();
	w->numConnections = 0;
	w->transitionAnimation = -1;

	if (!waypoint.transitionAnimation.empty) {
		w->transitionAnimation = (S32)m_bspFile->numStrings.get();
		*m_bspFile->AddString() = waypoint.transitionAnimation;
	}

	return true;
}

int BSPBuilder::FindBSPFloor(const char *name) {
	const String kName(CStr(name));

	for (SceneFile::TriModel::Vec::const_iterator it = m_map->worldspawn->models.begin(); it != m_map->worldspawn->models.end(); ++it) {
		const SceneFile::TriModel::Ref &m = *it;

		if (!(m->contents&kContentsFlag_Floor))
			continue;

		if (m->name == kName)
			return m->emitIds[0];
	}

	return -1;
}

} // solid_bsp
} // tools
