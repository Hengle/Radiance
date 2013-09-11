/*! \file SolidBSPEmit.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#include RADPCH
#include "SolidBSP.h"
#include "../CinematicsBuilder.h"
#include "../../Floors.h"
#include "../../Light.h"
#include <algorithm>

using namespace world::bsp_file;

namespace tools {
namespace solid_bsp {

namespace {
enum {
	kMaxBatchElements = kKilo*64
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
	
	if (!EmitBSPAreas())
		return false;
	
	if (!EmitBSPFloors())
		return false;

	if (!EmitBSPEntities())
		return false;

	if (!EmitBSPWaypoints())
		return false;

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
	Log("\t%8d ClipModel(s)\n", m_bspFile->numClipModels.get());
	Log("\t%8d ClipSurface(s)\n", m_bspFile->numClipSurfaces.get());
	Log("\t%8d ClipEdgePlane(s)\n", m_bspFile->numClipEdgePlanes.get());
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

		if (!trim->cinematic && !(trim->contents & (kContentsFlag_DetailContents|kContentsFlag_Fog)))
			continue;

		for (SceneFile::TriFaceVec::const_iterator f = trim->tris.begin(); f != trim->tris.end(); ++f) {
			const SceneFile::TriFace &trif = *f;

			if (!trim->cinematic) {
				if (!(trif.contents & (kContentsFlag_DetailContents|kContentsFlag_Fog)))
					continue;
				if (trif.surface & kSurfaceFlag_NoDraw)
					continue;
			}

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

bool BSPBuilder::EmitBSPEntities() {
	
	if (!EmitBSPEntity(m_map->worldspawn))
		return false;
	
	if (!EmitSceneLights())
		return false;

	for (SceneFile::Entity::Vec::const_iterator it = m_map->ents.begin(); it != m_map->ents.end(); ++it) {
		if (!EmitBSPEntity(*it))
			return false;
	}

	return true;
}

bool BSPBuilder::PutEntityOnFloor(const SceneFile::Entity::Ref &entity) {
	const char *floor = entity->keys.StringForKey("floor");
	if (!floor)
		return true;

	int floorNum = FindBSPFloor(floor);
	if (floorNum < 0) {
		Log("ERROR: entity '%s' is looking for non-existent floor '%s'\n", entity->keys.StringForKey("classname", "<null>"), floor);
		SetResult(pkg::SR_CompilerError);
		return false;
	}

	Vec3 pos(ToBSPType(entity->origin));
	int triNum = PutPointOnFloor(pos, floorNum);
	if (triNum < 0) {
		Log("ERROR: entity '%s' is not inside floor '%s'\n", entity->keys.StringForKey("classname", "<null>"), floor);
		return false;
	}

	entity->origin = FromBSPType(pos);

	String s;
	s.PrintfASCII("%f %f %f", entity->origin[0], entity->origin[1], entity->origin[2]);
	entity->keys.pairs[String("origin")] = s;

	s.PrintfASCII("%d", floorNum);
	entity->keys.pairs[String("floorNum")] = s;

	s.PrintfASCII("%d", triNum);
	entity->keys.pairs[String("floorTri")] = s;

	return true;
}

bool BSPBuilder::EmitSceneLights() {
	return EmitSceneOmniLights();
}

bool BSPBuilder::EmitSceneOmniLights() {
	SceneFile::Entity::Ref e(new (ZBSPBuilder) SceneFile::Entity());

	e->keys.pairs[CStr("classname")] = CStr("info_dynlight");
	e->keys.pairs[CStr("type")] = CStr("omni");

	int baseUUID = kKilo*kKilo*16; // this is a huge f---ing hack.

	String s;
	for (SceneFile::OmniLight::Vec::const_iterator it = m_map->omniLights.begin(); it != m_map->omniLights.end(); ++it) {
		const SceneFile::OmniLight::Ref &light = *it;

		if (!(light->flags&(SceneFile::kLightFlag_AffectObjects|SceneFile::kLightFlag_AffectPlayer|SceneFile::kLightFlag_AffectWorld)))
			continue; // light doesn't do anything

		e->origin = light->pos;
		
		e->keys.pairs.erase(CStr("targetname"));
		
		if (!light->name.empty)
			e->keys.pairs[CStr("targetname")] = light->name;

		s.PrintfASCII("%f %f %f", light->color[0], light->color[1], light->color[2]);
		e->keys.pairs[CStr("diffuseColor")] = s;
		e->keys.pairs[CStr("specularColor")] = s;
		
		s.PrintfASCII("%f", light->radius);
		e->keys.pairs[CStr("radius")] = s;

		s.PrintfASCII("%f", light->intensity);
		e->keys.pairs[CStr("intensity")] = s;

		s.PrintfASCII("%d", light->flags);
		e->keys.pairs[CStr("flags")] = s;

		s.PrintfASCII("%d", baseUUID++);
		e->keys.pairs[CStr("uuid")] = s;

		s.PrintfASCII("%d %d %d", (int)light->pos[0], (int)light->pos[1], (int)light->pos[2]);
		e->keys.pairs[CStr("origin")] = s;

		if (!EmitBSPEntity(e))
			return false;
	}

	return true;
}

bool BSPBuilder::EmitBSPEntity(const SceneFile::Entity::Ref &entity) {

	if (!PutEntityOnFloor(entity))
		return false;

	BSPEntity *bspEntity = m_bspFile->AddEntity();
	bspEntity->firstString = m_bspFile->numStrings;
	bspEntity->numStrings = (int)entity->keys.pairs.size();

	m_bspFile->ReserveStrings((int)entity->keys.pairs.size() * 2);

	for (world::Keys::Pairs::const_iterator it = entity->keys.pairs.begin(); it != entity->keys.pairs.end(); ++it) {
		*m_bspFile->AddString() = it->first;
		*m_bspFile->AddString() = it->second;
	}

	U32 firstBrushStringOfs = m_bspFile->numStrings;

	if (!entity->brushes.empty()) {
		bspEntity->numStrings += 2;
		m_bspFile->ReserveStrings(4);

		*m_bspFile->AddString() = CStr("firstBrush");
		*m_bspFile->AddString() = CStr("0");
		*m_bspFile->AddString() = CStr("numBrushes");

		String x;
		x.PrintfASCII("%d", entity->brushes.size());
		*m_bspFile->AddString() = x;
	}

	for (SceneFile::Brush::Vec::const_iterator it = entity->brushes.begin(); it != entity->brushes.end(); ++it) {
		int idx = EmitBSPBrush(*it);
		if (it == entity->brushes.begin()) {
			String x;
			x.PrintfASCII("%d", idx);
			m_bspFile->SetString(firstBrushStringOfs+1, x);
		}
	}

	return true;
}

/*
==============================================================================
Brushes
==============================================================================
*/

int BSPBuilder::EmitBSPBrush(const SceneFile::Brush &brush) {
	int idx = (int)m_bspFile->numBrushes.get();
	BSPBrush *bspBrush = m_bspFile->AddBrush();
	bspBrush->firstPlane = 0;
	bspBrush->numPlanes = 0;

	for (int i = 0; i < 3; ++i) {
		bspBrush->mins[i] = brush.bounds->Mins()[i];
		bspBrush->maxs[i] = brush.bounds->Maxs()[i];
	}

	for (SceneFile::BrushWinding::Vec::const_iterator it = brush.windings->begin(); it != brush.windings->end(); ++it) {
		int planeNum = m_planes.FindPlaneNum(ToBSPType((*it).winding.Plane()));

		if (it == brush.windings->begin())
			bspBrush->firstPlane = (U32)planeNum;
		++bspBrush->numPlanes;
	}

	return idx;
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

		if (!EmitBSPAreaportals(m_root.get(), area->area, *bspArea))
			return false;

		bspArea->firstModel = m_bspFile->numModelIndices;
		bspArea->numModels = 0;
		bspArea->sky = area->sky ? 1 : 0;

		for (SceneFile::TriModel::Vec::const_iterator m = m_map->worldspawn->models.begin(); m != m_map->worldspawn->models.end(); ++m) {
			const SceneFile::TriModel::Ref &trim = *m;

			SceneFile::AreaModelIdVec::const_iterator it = trim->emitIds.find(area->area);

			if (it == trim->emitIds.end())
				continue; // not in this area.
			
			const SceneFile::IntVec &emitIds = it->second;
			if (emitIds.empty())
				continue; // not in this area
			
			bounds.Insert(trim->bounds);

			m_bspFile->ReserveModelIndices((int)emitIds.size());

			for (SceneFile::IntVec::const_iterator id = emitIds.begin(); id != emitIds.end(); ++id) {
				if (*id > std::numeric_limits<U16>::max()) {
					Log("ERROR: there are too many models in this map (limit is 32k)!\n");
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

		if (trim->portalAreas[0] == -1 ||
			trim->portalAreas[1] == -1) {
			Log("ERRROR: Areaportal '%s' does not seperate areas.\n", trim->name.c_str.get());
			SetResult(pkg::SR_CompilerError);
			return false;
		}
	}

	return true;
}

/*
==============================================================================
Portals
==============================================================================
*/

bool BSPBuilder::EmitBSPAreaportals(Node *leaf, int areaNum, BSPArea &area) {
	if (leaf->planenum != kPlaneNumLeaf) {
		return EmitBSPAreaportals(leaf->children[0].get(), areaNum, area) &&
			EmitBSPAreaportals(leaf->children[1].get(), areaNum, area);
	}

	if (!leaf->area || (leaf->area->area != areaNum))
		return true;

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
			Log("ERROR: portal fragment bounds area but is not an area portal.\n");
			SetResult(pkg::SR_CompilerError);
			return false;
		}

		if (original->model->portalAreas[0] == -1) {
			original->model->portalAreas[0] = areaNum;
		} else if (original->model->portalAreas[0] != areaNum) {
			if (original->model->portalAreas[1] == -1) {
				original->model->portalAreas[1] = areaNum;
			} else if (original->model->portalAreas[1] != areaNum) {
				Log("ERROR: Areaportal '%s' touches more than 2 areas (%d, %d, %d).\n", original->model->name.c_str.get(),areaNum, other->area->area, p->areas[side]);
				SetResult(pkg::SR_CompilerError);
				return false;
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
			Log("ERROR: Areaportal fragment touches more than 2 areas (%d, %d, %d).\n", areaNum, other->area->area, p->areas[side]);
			SetResult(pkg::SR_CompilerError);
			return false;
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

	return true;
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

		if (!(trim->contents & kContentsFlag_DetailContents))
			continue;

		EmitBSPModel(*trim, 0);
	}
}

void BSPBuilder::EmitBSPModel(SceneFile::TriModel &triModel, int contents) {

	IntSet mats;

	// gather materials.

	for (SceneFile::TriFaceVec::const_iterator f = triModel.tris.begin(); f != triModel.tris.end(); ++f) {
		const SceneFile::TriFace &trif = *f;
		if (!contents && trif.areas.empty())
			continue;
		if (trif.surface&kSurfaceFlag_NoDraw)
			continue;
		mats.insert(trif.mat);
	}

	if (contents) {
		EmitBSPModel(triModel, mats, 0);
	} else {
		for (SceneFile::TriFaceVec::const_iterator f = triModel.tris.begin(); f != triModel.tris.end(); ++f) {
			const SceneFile::TriFace &trif = *f;
			if (trif.emitId != -1)
				continue; // already done

			// get areas from tris
			EmitBSPModel(
				triModel,
				mats,
				&trif.areas
			);
		}
	}
}

void BSPBuilder::EmitBSPModel(
	SceneFile::TriModel &triModel,
	const IntSet &mats,
	const SceneFile::AreaNumSet *areas
) {
	for (int c = 1; c <= kMaxUVChannels; ++c){
		for (IntSet::const_iterator it = mats.begin(); it != mats.end(); ++it) {
			EmitTriModel m;
			m.mat = *it;
			m.numChannels = c;
			m.bounds.Initialize();

			for (SceneFile::TriFaceVec::iterator f = triModel.tris.begin(); f != triModel.tris.end(); ++f) {
				SceneFile::TriFace &trif = *f;

				if (trif.emitId != -1)
					continue;
				if (trif.surface&kSurfaceFlag_NoDraw)
					continue;
				if (trif.mat != m.mat)
					continue;
				if (trif.model->numChannels != c)
					continue;
				if (areas && (trif.areas != *areas))
					continue;

				trif.emitId = 0; // just flag as emitted

				if (((int)m.indices.size() >= kMaxBatchElements-3) ||
					((int)m.verts.size() >= kMaxBatchElements-3)) {
					EmitBSPModel(triModel, m, areas);
					m.Clear();
				}

				m.AddVertex(EmitTriModel::Vert(ToBSPType(triModel.verts[trif.v[0]])));
				m.AddVertex(EmitTriModel::Vert(ToBSPType(triModel.verts[trif.v[1]])));
				m.AddVertex(EmitTriModel::Vert(ToBSPType(triModel.verts[trif.v[2]])));
			}

			if (!m.verts.empty()) {
				EmitBSPModel(triModel, m, areas);
			}
		}
	}
}

int BSPBuilder::EmitBSPModel(
	SceneFile::TriModel &triModel,
	const EmitTriModel &model,
	const SceneFile::AreaNumSet *areas
) {
	int id = EmitBSPModel(model, triModel.contents, triModel.uvBumpChannel);
	if (areas) {
		for (SceneFile::AreaNumSet::const_iterator it = areas->begin(); it != areas->end(); ++it) {
			triModel.emitIds[*it].push_back(id);
		}
	} else {
		triModel.fogId = id;
	}
	return id;
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

int BSPBuilder::EmitBSPModel(const EmitTriModel &model, int contents, int uvBumpChannel) {
	RAD_ASSERT(uvBumpChannel < world::bsp_file::kMaxUVChannels);
	
	BSPModel *bspModel = m_bspFile->AddModel();
	bspModel->firstVert = m_bspFile->numVerts;
	bspModel->numVerts = (U32)model.verts.size();
	bspModel->firstIndex = m_bspFile->numIndices;
	bspModel->numIndices = (U32)model.indices.size();
	bspModel->material = FindBSPMaterial(m_map->mats[model.mat].name.c_str);
	bspModel->numChannels = (U32)model.numChannels;
	bspModel->contents = (U32)contents;

	for (int i = 0; i < 3; ++i) {
		bspModel->mins[i] = (float)model.bounds.Mins()[i];
		bspModel->maxs[i] = (float)model.bounds.Maxs()[i];
	}
	
	m_bspFile->ReserveVertices((int)model.verts.size());
	m_bspFile->ReserveIndices((int)model.indices.size());

	for (EmitTriModel::VertVec::const_iterator it = model.verts.begin(); it != model.verts.end(); ++it) {
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
		}

		for (int k = 0; k < 4; ++k)
			bspV->t[k] = (float)v.tangent[uvBumpChannel][k];

		for (; i < world::bsp_file::kMaxUVChannels; ++i) {
			for (int k = 0; k < 2; ++k)
				bspV->st[i*2+k] = 0.f;
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

		EmitBSPClipModels(node, leaf);

		if (leaf->numClipModels > 0)
			leaf->contents |= kContentsFlag_Clip;

		leaf->firstFog = m_bspFile->numModelIndices;
		leaf->numFogs = 0;

		if ((leaf->area != -1) && (leaf->contents&kContentsFlag_Fog)) {
			// emit fogs
			for (TriModelFragVec::const_iterator it = node->models.begin(); it != node->models.end(); ++it) {
				SceneFile::TriModel *original = (*it)->original;
				if (original->contents&kContentsFlag_Fog) {
					if (original->fogId == -1) {
						EmitBSPModel(*original, kContentsFlag_Fog);
					}
					RAD_ASSERT(original->fogId != -1);
					*m_bspFile->AddModelIndex() = (U16)original->fogId;
					++leaf->numFogs;
				}
			}
		}

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
Clip Models
==============================================================================
*/

void BSPBuilder::EmitBSPClipModels(const Node *node, world::bsp_file::BSPLeaf *leaf) {
	leaf->firstClipModel = m_bspFile->numClipModels;
	leaf->numClipModels = 0;

	m_bspFile->ReserveClipModels((int)node->clipModels.size());

	for (TriModelFragVec::const_iterator it = node->clipModels.begin(); it != node->clipModels.end(); ++it) {
		const TriModelFragRef &model = *it;
		EmitBSPClipModel(model);
		++leaf->numClipModels;
	}

}

void BSPBuilder::EmitBSPClipModel(const TriModelFragRef &model) {
	BSPClipModel *clipModel = m_bspFile->AddClipModel();

	for (int i = 0; i < 3; ++i) {
		clipModel->mins[i] = model->bounds.Mins()[i];
		clipModel->maxs[i] = model->bounds.Maxs()[i];
	}

	clipModel->firstClipSurface = m_bspFile->numClipSurfaces;
	clipModel->numClipSurfaces = 0;

	m_bspFile->ReserveClipSurfaces((int)model->polys.size());

	for (PolyVec::const_iterator it = model->polys.begin(); it != model->polys.end(); ++it) {
		const PolyRef &poly = *it;

		if (poly->winding->Vertices().empty())
			continue;

		BSPClipSurface *clipSurface = m_bspFile->AddClipSurface();
		clipSurface->flags = 0;
		clipSurface->contents = poly->original->contents;
		clipSurface->surface  = poly->original->surface;
		clipSurface->planenum = poly->planenum;

		clipSurface->firstEdgePlane = m_bspFile->numClipEdgePlanes;
		clipSurface->numEdgePlanes = 0;

		const Plane &plane = m_planes.Plane(poly->planenum);
		const WindingRef &winding = poly->winding;
		const Winding::VertexListType &vertices = winding->Vertices();

		m_bspFile->ReserveClipEdgePlanes((int)vertices.size());

		for (int i = 0; i < (int)vertices.size(); ++i) {
			int k = i+1;
			if (k >= (int)vertices.size())
				k = 0;
			
			Vec3 edge(vertices[i] - vertices[k]);
			edge.Normalize();
			edge = edge.Cross(plane.Normal());

			Plane edgePlane(edge, vertices[i], Plane::Unit);
			
			/*int j = k+1;
			if (j >= (int)vertices.size())
				j = 0;

			if (edgePlane.Side(vertices[j], 0.f) == Plane::Back)
				edgePlane = -edgePlane;*/

			BSPPlane *pplane = m_bspFile->AddClipEdgePlane();
			pplane->p[0] = edgePlane.A();
			pplane->p[1] = edgePlane.B();
			pplane->p[2] = edgePlane.C();
			pplane->p[3] = edgePlane.D();

			++clipSurface->numEdgePlanes;
		}

		++clipModel->numClipSurfaces;
	}
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

	if (!b.Compile(*m_map, m_caMap, m_bspFile)) {
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

int BSPBuilder::FloorBuilder::AddVert(const Vert &_v) {

	Vert v;
	for (int i = 0; i < 3; ++i)
		v[i] = math::Floor(_v[i] + 0.5f); // round.

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
			e.mid = (verts[v1] + verts[v0]) * 0.5f;
			e.vec = (verts[v1] - verts[v0]);
			e.vec.Normalize();
			e.dist[0] = e.vec.Dot(verts[v0]);
			e.dist[1] = e.vec.Dot(verts[v1]);

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

bool BSPBuilder::FloorBuilder::ValidateTopology() {
	TriBits visited;
	visited.resize(tris.size());

	// done without recursion so we can handle large floors

	struct Stack {
		typedef zone_vector<Stack, ZBSPBuilderT>::type Vec;
		int triNum;
		int edgeIdx;
	};

	Stack::Vec stack;

	Stack cur;
	cur.triNum = 0;
	cur.edgeIdx = 0;

	int numVisited = 1;
	visited.set(0);

	for(;;) {
		
		const Tri *tri = &tris[cur.triNum];

		for (;cur.edgeIdx < 3; ++cur.edgeIdx) {
			const Edge &edge = edges[tri->e[cur.edgeIdx]];
			int other = edge.t[0] == cur.triNum;
			if (edge.t[other] == -1)
				continue;

			if (!visited.test(edge.t[other])) {
				++numVisited;
				visited.set(edge.t[other]);
				++cur.edgeIdx; // done this edge.
				stack.push_back(cur);
				cur.edgeIdx = 0;
				cur.triNum = edge.t[other];
				tri = 0;
				break;
			}
		}

		if (!tri)
			continue; // stack changed.

		if (stack.empty())
			break;

		cur = stack.back();
		stack.pop_back();
	}

	return numVisited == (int)tris.size();
}

void BSPBuilder::FloorBuilder::Flood(int triNum, int &numVisited, TriBits &visited) {
	++numVisited;
	visited.set(triNum);

	const Tri &tri = tris[triNum];

	for (int i = 0; i < 3; ++i) {
		const Edge &edge = edges[tri.e[i]];
		int side = edge.t[1] == triNum;

		// cross?
		if (!visited.test(edge.t[!side]))
			Flood(edge.t[!side], numVisited, visited);
	}
}

bool BSPBuilder::EmitBSPFloors() {

	const SceneFile::Vec3 kZAxis(0, 0, 1);

	for (SceneFile::TriModel::Vec::const_iterator it = m_map->worldspawn->models.begin(); it != m_map->worldspawn->models.end(); ++it) {
		const SceneFile::TriModel::Ref &m = *it;

		if (!(m->contents&kContentsFlag_Floor))
			continue;
		if (m->name.empty) {
			Log("WARNING: floor mesh has no name (floor removed).\n");
			continue;
		}

		if (FindBSPFloor(m->name.c_str) >= 0) {
			Log("ERROR: there are multiple floor meshes for '%s', floors can only be a single mesh.\n", m->name.c_str.get());
			SetResult(pkg::SR_CompilerError);
			return false;
		}

		FloorBuilder builder(*m, this);

		for (SceneFile::TriFaceVec::const_iterator fIt = m->tris.begin(); fIt != m->tris.end(); ++fIt) {
			const SceneFile::TriFace &f = *fIt;

			if (!builder.AddTri(
				SnapVertex(m->verts[f.v[0]].pos),
				SnapVertex(m->verts[f.v[1]].pos),
				SnapVertex(m->verts[f.v[2]].pos)
			)) {
				return false;
			}
		}

		if (builder.tris.empty())
			continue;

		if (builder.tris.size() >= world::kMaxFloorTris) {
			Log("ERROR: Maximum of %d floor triangles (per-floor-limit) exceeded, contact a programmer to increase limit.\n", world::kMaxFloorTris);
			SetResult(pkg::SR_CompilerError);
			return false;
		}

		if (m_bspFile->numFloors >= world::kMaxFloors) {
			Log("ERROR: Maximum of %d floors exceeded, contact a programmer to increase limit.\n", world::kMaxFloors);
			SetResult(pkg::SR_CompilerError);
			return false;
		}

		if (!builder.ValidateTopology()) {
			Log("ERROR: Floor \"%s\" has an invalid configuration, all triangles must be connected and reachable.\n", m->name.c_str.get());
			SetResult(pkg::SR_CompilerError);
			return false;
		}

		int emitId = (int)m_bspFile->numFloors;
		U32 firstVert = m_bspFile->numVerts;
		U32 firstTri  = m_bspFile->numFloorTris;
		U32 firstEdge = m_bspFile->numFloorEdges;
		
		BSPFloor *bspFloor = m_bspFile->AddFloor();
		bspFloor->name = m_bspFile->numStrings;
		*m_bspFile->AddString() = m->name;
		bspFloor->firstTri = firstTri;
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
			
			for (int i = 0; i < 3; ++i) {
				e->vec[i] = edge.vec[i];
			}

			e->dist[0] = edge.dist[0];
			e->dist[1] = edge.dist[1];

			SceneFile::Vec3 vedge(v0 - v1);
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

			// validate: should put t[0] on front, t[1] on back:
			const FloorBuilder::Tri &tri = builder.tris[edge.t[0]];
			int vertNum;
			for (vertNum = 0; vertNum < 3; ++vertNum) {
				if (tri.v[vertNum] != edge.v[0] &&
					tri.v[vertNum] != edge.v[1]) {
						break;
				}
			}

			RAD_ASSERT(vertNum < 3);
			const SceneFile::Vec3 &vert = builder.verts[tri.v[vertNum]];
			Plane::SideType side = plane.Side(ToBSPType(vert));
			if (side == Plane::Back) {
				Log("WARNING: floor %s has a winding direction error!\n", m->name.c_str.get());
				SetResult(pkg::SR_CompilerError);
				return false;
			}

			e->planenum = (U32)m_planes.FindPlaneNum(plane);
		}

		for (FloorBuilder::Tri::Vec::const_iterator tIt = builder.tris.begin(); tIt != builder.tris.end(); ++tIt) {
			const FloorBuilder::Tri &tri = *tIt;

			BSPFloorTri *t = m_bspFile->AddFloorTri();

			for (int i = 0; i < 3; ++i) {
				t->edges[i] = firstEdge + (U32)tri.e[i];
				t->verts[i] = firstVert + (U32)tri.v[i];
			}

			const Plane kPlane(
				ToBSPType(builder.verts[tri.v[0]]),
				ToBSPType(builder.verts[tri.v[1]]),
				ToBSPType(builder.verts[tri.v[2]])
			);

			t->planenum = m_planes.FindPlaneNum(kPlane);
		}

		m->emitIds[0].push_back(emitId);
	}

	return true;
}

bool BSPBuilder::EmitBSPWaypoints() {

	// only emit connected waypoints

	// pass 1: emit waypoint structure, and connection

	for (SceneFile::WaypointConnection::Map::const_iterator it = m_map->waypointConnections.begin(); it != m_map->waypointConnections.end(); ++it) {
		const SceneFile::WaypointConnection::Ref &connection = it->second;
		RAD_ASSERT(connection->waypoints.head);

		if (!connection->waypoints.tail) {
			Log("ERROR: waypoint connection is missing tail (head = %d) (this indicates a bug, contact a programmer!).\n", connection->waypoints.head->uid);
			SetResult(pkg::SR_CompilerError);
			return false;
		}

		RAD_ASSERT(connection->waypoints.tail);

		// floor connections must seperate floors
		if (!connection->waypoints.head->floorName.empty && 
			(connection->waypoints.head->floorName ==
			connection->waypoints.tail->floorName)) {
			// does not connect floors
			Log(
				"WARNING: waypoints connect the same floor \"%s\" (connection removed).\n", 
				connection->waypoints.head->floorName.c_str.get()
			);
			continue;
		}

		if (m_bspFile->numWaypointConnections.get() > std::numeric_limits<U16>::max()) {
			Log("ERROR: too many waypoint connections (internal error, contact programmer to increase limit)!\n");
			SetResult(pkg::SR_CompilerError);
			return false;
		}

		if (!EmitBSPWaypoint(*connection->waypoints.head) ||
			!EmitBSPWaypoint(*connection->waypoints.tail)) {
			return false;
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

		for (int i = 0; i < 4; ++i) {
			if (!connection->cmds[i].empty) {
				c->cmds[i] = m_bspFile->numStrings;
				*m_bspFile->AddString() = connection->cmds[i];
			} else {
				c->cmds[i] = -1;
			}
		}

		for (int i = 0; i < 2; ++i) {
			if (!connection->anims[i].empty) {
				c->anims[i] = m_bspFile->numStrings;
				*m_bspFile->AddString() = connection->anims[i];
			} else {
				c->anims[i] = -1;
			}
		}
	}

	// pass 2: emit waypoint connection indices

	for (SceneFile::Waypoint::Map::const_iterator it = m_map->waypoints.begin(); it != m_map->waypoints.end(); ++it) {
		const SceneFile::Waypoint::Ref &waypoint = it->second;

		if (waypoint->emitId < 0) {
			// this waypoint was overlooked cause it has no connections but may be a teleport, emit it into the BSP
			if (waypoint->userId.empty && waypoint->targetName.empty) {
				Log("WARNING: waypoint has no connections and no targetname or userid (waypoint removed).");
				continue;
			}
			if (!EmitBSPWaypoint(*waypoint)) {
				return false;
			}
		}

		BSPWaypoint *w = const_cast<BSPWaypoint*>(m_bspFile->Waypoints() + waypoint->emitId);

		m_bspFile->ReserveWaypointIndices((int)waypoint->connections.size());

		struct WaypointCost {
			typedef zone_vector<WaypointCost, ZBSPBuilderT>::type Vec;
			float distance;
			int id;

			bool operator < (const WaypointCost &other) const {
				return distance < other.distance;
			}
		};

		// sort waypoints connections by distance (for pathfinding).

		WaypointCost::Vec sortedWaypoints;

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

			SceneFile::Waypoint *otherWaypoint = connection->waypoints.tail;
			if (connection->waypoints.tail == waypoint.get()) {
				otherWaypoint = connection->waypoints.head;
			}

			SceneFile::Vec3 v(otherWaypoint->pos - waypoint->pos);
			WaypointCost cost;
			cost.distance = v.MagnitudeSquared();
			cost.id = connection->emitId;
			sortedWaypoints.push_back(cost);
		}

		std::sort(sortedWaypoints.begin(), sortedWaypoints.end());

		for (WaypointCost::Vec::const_iterator it = sortedWaypoints.begin(); it != sortedWaypoints.end(); ++it) {
			const WaypointCost &connection = *it;

			if (w->firstConnection == std::numeric_limits<U32>::max())
				w->firstConnection = m_bspFile->numWaypointIndices;

			*m_bspFile->AddWaypointIndex() = (U16)connection.id;
			++w->numConnections;
		}
	}

	// pass 3: emit floor waypoint indexes
	for (U32 i = 0; i < m_bspFile->numFloors; ++i) {
		BSPFloor *floor = const_cast<BSPFloor*>(m_bspFile->Floors() + i);
		
		floor->firstWaypoint = std::numeric_limits<U32>::max();
		floor->numWaypoints = 0;

		for (U32 k = 0; k < m_bspFile->numWaypoints; ++k) {
			const BSPWaypoint *waypoint = m_bspFile->Waypoints() + k;

			if (waypoint->floorNum == i) {
				if (floor->firstWaypoint == std::numeric_limits<U32>::max())
					floor->firstWaypoint = m_bspFile->numWaypointIndices;
				*m_bspFile->AddWaypointIndex() = (U16)k;
				++floor->numWaypoints;
			}
		}
	}

	return true;
}

bool BSPBuilder::EmitBSPWaypoint(SceneFile::Waypoint &waypoint) {

	if (waypoint.emitId > -1)
		return true; // already emitted.

	int floorNum = -1;
	int floorTriNum = -1;

	if (!waypoint.floorName.empty) {
		floorNum = FindBSPFloor(waypoint.floorName.c_str);
		if (floorNum < 0) {
			Log("ERROR: Waypoint (targetname=\"%s\", userid=\"%s\") references a floor that does not exist: \"%s\".\n", waypoint.targetName.c_str.get(), waypoint.userId.c_str.get(), waypoint.floorName.c_str.get());
			return false;
		}

		Vec3 pos = ToBSPType(waypoint.pos);
		floorTriNum = PutPointOnFloor(pos, floorNum);

		if (floorTriNum < 0) {
			Log("ERROR: Waypoint (targetname=\"%s\", userid=\"%s\") is not on or above floor \"%s\".\n", waypoint.targetName.c_str.get(), waypoint.userId.c_str.get(), waypoint.floorName.c_str.get());
			SetResult(pkg::SR_CompilerError);
			return false;
		}

		waypoint.pos = FromBSPType(pos);
	}

	if (m_bspFile->numWaypoints >= world::kMaxWaypoints) {
		Log("ERROR: Maximum number of waypoints exceeded (%d), contact a programmer to increase limit!\n", world::kMaxWaypoints);
		SetResult(pkg::SR_CompilerError);
		return false;
	}

	waypoint.emitId = (int)m_bspFile->numWaypoints.get();

	BSPWaypoint *w = m_bspFile->AddWaypoint();
	w->floorNum = (S32)floorNum;
	w->triNum = (S32)floorTriNum;
	w->pos[0] = waypoint.pos[0];
	w->pos[1] = waypoint.pos[1];
	w->pos[2] = waypoint.pos[2];
	w->uid = (U32)waypoint.uid;

	w->firstConnection = std::numeric_limits<U32>::max();
	w->numConnections = 0;
	
	w->targetName = -1;
	w->userId = -1;

	if (!waypoint.targetName.empty) {
		w->targetName = (S32)m_bspFile->numStrings.get();
		*m_bspFile->AddString() = waypoint.targetName;
	}

	if (!waypoint.userId.empty) {
		w->userId = (S32)m_bspFile->numStrings.get();
		*m_bspFile->AddString() = waypoint.userId;
	}

	return true;
}

int BSPBuilder::FindBSPFloor(const char *name) {
	const String kName(CStr(name));

	for (SceneFile::TriModel::Vec::const_iterator it = m_map->worldspawn->models.begin(); it != m_map->worldspawn->models.end(); ++it) {
		const SceneFile::TriModel::Ref &m = *it;

		if (!(m->contents&kContentsFlag_Floor))
			continue;

		if (m->name == kName) {
			if (m->emitIds.empty() || m->emitIds[0].empty())
				return -1;
			return m->emitIds[0][0];
		}
	}

	return -1;
}

int BSPBuilder::PutPointOnFloor(Vec3 &pos, int floorNum) {
	const BSPFloor *floor = m_bspFile->Floors() + floorNum;

	RAD_ASSERT(floor->numTris > 0);

	int bestTri = -1;

	Vec3 end(pos - Vec3(0, 0, 512));
	float bestDistSq = std::numeric_limits<float>::max();

	for (U32 i = 0; i < floor->numTris; ++i) {
		U32 triNum = floor->firstTri + i;
		const BSPFloorTri *tri = m_bspFile->FloorTris() + triNum;

		const Plane &kTriPlane = m_planes.Plane(tri->planenum);
		
		Vec3 clip;

		if (!kTriPlane.IntersectLineSegment(clip, pos, end, 0.01f))
			continue;

		ValueType distSq = (clip-pos).MagnitudeSquared();
		if (distSq >= bestDistSq) // can't be better
			continue;

		int k;
		
		for (k = 0; k < 3; ++k) {
			const BSPFloorEdge *edge = m_bspFile->FloorEdges() + tri->edges[k];
			Plane plane = m_planes.Plane(edge->planenum);

			int side = edge->tris[1] == triNum;

			if (side)
				plane.Flip();

			if (plane.Side(clip, 0.01f) == Plane::Back) {
				break;
			}
		}

		if (k == 3) {
			// inside triangle hull
			bestTri = (int)i;
			bestDistSq = distSq;
			pos = clip;
		}
	}

	return bestTri;
}

} // solid_bsp
} // tools
