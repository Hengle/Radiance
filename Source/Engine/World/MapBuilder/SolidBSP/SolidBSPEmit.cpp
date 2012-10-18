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
	EmitBSPPlanes();
	EmitBSPModels();
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
	Log("\t%8d Areaportals\n", m_bspFile->numAreaportals.get());
	Log("\t%8d AreaportalIndices\n", m_bspFile->numAreaportalIndices.get());
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
