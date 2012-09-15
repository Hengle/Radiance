// BoxBSP.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#include "BoxBSP.h"
#include "../../../SkAnim/SkBuilder.h"
#include "../../../COut.h"
#include <Runtime/Base/Utils.h>
#include <algorithm>
#undef min
#undef max

namespace {

enum
{
	MaxBatchElements = Kilo*64,
	SmallTriBatch = Kilo // any leaf holding less than this is a candidate for merging.
};

const float SplitAxisBounds[3] = { 1024.f, 1024.f, 4096.f };
const float MaxAxisBounds[3] = { 16384.f, 16384.f, 4096.f };

}

namespace tools {
namespace box_bsp {

BSPBuilder::BSPBuilder()
{
}

BSPBuilder::~BSPBuilder()
{
}

bool BSPBuilder::Build(const SceneFile &map)
{
	m_skaSize = 0;

	if (!map.worldspawn)
		return false;
	m_bspFile.reset(new world::bsp_file::BSPFileBuilder());
	EmitMaterials(map);

	BuildBSP(map);
	EmitEntities(map);
	
	if (!EmitCinematics(map))
		return false;

	// Stats:
	String s;

	COut(C_Info) << "MapStats:" << std::endl;
	s.Printf("\t%8d Strings", m_bspFile->numStrings.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Entities", m_bspFile->numEntities.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Nodes", m_bspFile->numNodes.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Leafs", m_bspFile->numLeafs.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Planes", m_bspFile->numPlanes.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Models", m_bspFile->numModels.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Verts", m_bspFile->numVerts.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Indices", m_bspFile->numIndices.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Camera TMs", m_bspFile->numCameraTMs.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Camera Tracks", m_bspFile->numCameraTracks.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Cinematic Triggers", m_bspFile->numCinematicTriggers.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Cinematics", m_bspFile->numCinematics.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Skas", m_bspFile->numSkas.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Actors", m_bspFile->numActors.get());
	COut(C_Info) << s << std::endl;
	s.Printf("\t%8d Actor Indices", m_bspFile->numActorIndices.get());
	COut(C_Info) << s << std::endl;

	SizeBuffer memSize;
	FormatSize(memSize, m_skaSize);
	s.Printf("\t%s Ska Data", memSize);
	COut(C_Info) << s << std::endl;

	return true;
}

void BSPBuilder::TriModel::AddVertex(const Vert &vert)
{
	VertMap::iterator it = vmap.find(vert);
	if (it != vmap.end())
	{
		indices.push_back(it->second);
		return;
	}
	int ofs = (int)verts.size();
	verts.push_back(vert);
	vmap.insert(VertMap::value_type(vert, ofs));
	indices.push_back(ofs);
}

void BSPBuilder::EmitMaterials(const SceneFile &map)
{
	m_bspFile->ReserveMaterials((int)map.mats.size());
	m_bspFile->ReserveStrings((int)map.mats.size());

	for (SceneFile::MatVec::const_iterator it = map.mats.begin(); it != map.mats.end(); ++it)
	{
		world::bsp_file::BSPMaterial *m = m_bspFile->AddMaterial();
		m->string = m_bspFile->numStrings;
		*m_bspFile->AddString() = (*it).name;
	}
}

void BSPBuilder::EmitEntities(const SceneFile &map)
{
	EmitEntity(map.worldspawn);
	for (SceneFile::Entity::Vec::const_iterator it = map.ents.begin(); it != map.ents.end(); ++it)
		EmitEntity(*it);
}

bool BSPBuilder::EmitCinematics(const SceneFile &map)
{
	// create actor list
	m_actors.reserve(64);

	const SceneFile::Entity::Ref &world = map.worldspawn;
	for (SceneFile::TriModel::Vec::const_iterator it = world->models.begin(); it != world->models.end(); ++it)
	{
		const SceneFile::TriModel::Ref &model = *it;
		if (!model->cinematic || model->skel < 0)
			continue;
		Actor a;
		a.emitId = -1;
		a.index = (int)(it-world->models.begin());
		a.flags = world::bsp_file::kCinematicObj;
		if (model->hideUntilRef)
			a.flags |= world::bsp_file::kHideUntilRef;
		if (model->hideWhenDone)
			a.flags |= world::bsp_file::kHideWhenDone;
		
		m_actors.push_back(a);
	}

	// pull names from cameras first...

	for (SceneFile::Camera::Vec::const_iterator it = map.cameras.begin(); it != map.cameras.end(); ++it)
	{
		const SceneFile::Camera::Ref &camera = *it;
		for (SceneFile::AnimMap::const_iterator it = camera->anims.begin(); it != camera->anims.end(); ++it)
		{
			if (!EmitCinematic(map, it->second->name))
				return false;
		}
	}

	// find cinematic actors.

	for (Actor::Vec::const_iterator it = m_actors.begin(); it != m_actors.end(); ++it)
	{
		const Actor &actor = *it;
		const SceneFile::TriModel::Ref &model = world->models[actor.index];
		for (SceneFile::AnimMap::const_iterator it = model->anims.begin(); it != model->anims.end(); ++it)
		{
			if (!EmitCinematic(map, it->second->name))
				return false;
		}
	}

	return true;
}

bool BSPBuilder::EmitCinematic(const SceneFile &map, const String &name)
{
	if (m_cinematics.find(name) != m_cinematics.end())
		return true;

	COut(C_Info) << "Compiling cinematic '" << name << "'..." << std::endl;
	m_cinematics.insert(name);

	int fps = 30;
	Trigger::Map triggers;

	const SceneFile::Entity::Ref &world = map.worldspawn;

	// gather camera motion first.
	for (SceneFile::Camera::Vec::const_iterator it = map.cameras.begin(); it != map.cameras.end(); ++it)
	{
		const SceneFile::Camera::Ref &camera = *it;
		SceneFile::AnimMap::const_iterator animIt = camera->anims.find(name);
		if (animIt == camera->anims.end())
			continue;

		const SceneFile::Anim::Ref &anim = animIt->second;
		fps = (int)anim->frameRate;
		Trigger::Map::iterator triggerIt = triggers.find(anim->firstFrame);

		if (triggerIt != triggers.end())
		{
			triggerIt->second.camera = (int)(it-map.cameras.begin());
		}
		else
		{
			Trigger t;
			t.frame = anim->firstFrame;
			t.camera = (int)(it-map.cameras.begin());
			triggers[t.frame] = t;
		}
	}

	// gather actor triggers
	for (Actor::Vec::iterator it = m_actors.begin(); it != m_actors.end(); ++it)
	{
		Actor &actor = *it;
		const SceneFile::TriModel::Ref &model = world->models[actor.index];
		SceneFile::AnimMap::const_iterator animIt = model->anims.find(name);
		if (animIt == model->anims.end())
			continue;
		
		if (!EmitActor(map, actor))
			return false;

		const SceneFile::Anim::Ref &anim = animIt->second;
		fps = (int)anim->frameRate;
		Trigger::Map::iterator triggerIt = triggers.find(anim->firstFrame);

		int actorIdx = (int)(it-m_actors.begin());
		if (anim->looping)
			actorIdx |= 0x10000000;

		if (triggerIt != triggers.end())
		{
			triggerIt->second.actors.push_back(actorIdx);
		}
		else
		{
			Trigger t;
			t.frame = anim->firstFrame;
			t.camera = -1;
			t.actors.push_back(actorIdx);
			triggers[t.frame] = t;
		}
	}

	if (triggers.empty())
		return true;

	int firstFrame = triggers.begin()->first;

	world::bsp_file::BSPCinematic *cinematic = m_bspFile->AddCinematic();
	cinematic->name = (int)m_bspFile->numStrings.get();
	*m_bspFile->AddString() = name.c_str;
	cinematic->firstTrigger = (int)m_bspFile->numCinematicTriggers.get();
	cinematic->fps = fps;

	for (Trigger::Map::const_iterator it = triggers.begin(); it != triggers.end(); ++it)
	{
		const Trigger &trigger = it->second;
		int frame = trigger.frame - firstFrame;

		world::bsp_file::BSPCinematicTrigger *bspTrigger = m_bspFile->AddCinematicTrigger();
		bspTrigger->frame = frame;
		bspTrigger->camera = -1;
		bspTrigger->firstActor = (int)m_bspFile->numActorIndices.get();
		bspTrigger->numActors = 0;

		if (trigger.camera > -1)
		{ // emit camera track
			const SceneFile::Camera::Ref &camera = map.cameras[trigger.camera];
			SceneFile::AnimMap::const_iterator animIt = camera->anims.find(name);
			RAD_VERIFY(animIt != camera->anims.end());
			const SceneFile::Anim::Ref &anim = animIt->second;

			bspTrigger->camera = (int)m_bspFile->numCameraTracks.get();
			world::bsp_file::BSPCameraTrack *track = m_bspFile->AddCameraTrack();

			track->firstTM = (int)m_bspFile->numCameraTMs.get();
			track->name = (int)m_bspFile->numStrings.get();
			*m_bspFile->AddString() = camera->name.c_str;

			m_bspFile->ReserveCameraTMs((int)anim->frames.size());

			for (SceneFile::BoneFrames::const_iterator it = anim->frames.begin(); it != anim->frames.end(); ++it)
			{
				const SceneFile::BonePoseVec &pose = *it;
				RAD_VERIFY(pose.size() == 1);
				world::bsp_file::BSPCameraTM *tm = m_bspFile->AddCameraTM();

				tm->r = pose[0].m.r;
				tm->t = pose[0].m.t;
				tm->fov = pose[0].fov;

				if (pose[0].tag.empty)
				{
					tm->tag = -1;
				}
				else
				{
					tm->tag = (int)m_bspFile->numStrings.get();
					*m_bspFile->AddString() = pose[0].tag.c_str;
				}

				++track->numTMs;
			}
		}

		for (IntVec::const_iterator it = trigger.actors.begin(); it != trigger.actors.end(); ++it)
		{
			int index = *it;
			bool loop = index&0x10000000 ? true : false;
			index &= 0x00ffffff;

			Actor &actor = m_actors[index];

			index = actor.emitId;
			if (loop)
				index |= 0x10000000;

			*m_bspFile->AddActorIndex() = (U32)index;
			++bspTrigger->numActors;
		}

		++cinematic->numTriggers;
	}

	return true;
}

bool BSPBuilder::EmitActor(const SceneFile &map, Actor &actor)
{
	if (actor.emitId > -1)
		return true;

	actor.emitId = (int)m_bspFile->numActors.get();
	world::bsp_file::BSPActor *bspActor = m_bspFile->AddActor();
	
	bspActor->flags = actor.flags;
	bspActor->initial = -1;

	// emit skas
	tools::SkaData::Ref ska = tools::CompileSkaData(
		"BSPBuilder",
		map,
		actor.index
	);

	if (!ska)
		return false;

	tools::SkmData::Ref skm = tools::CompileSkmData(
		"BSPBuilder",
		map,
		actor.index,
		ska::SkinCpu,
		ska->dska
	);

	if (!skm)
		return false;

	bspActor->ska = m_bspFile->AddSka(ska, skm);

	m_skaSize += ska->skaSize;
	m_skaSize += skm->skmSize[0] + skm->skmSize[1];

	return true;
}

void BSPBuilder::EmitEntity(const SceneFile::Entity::Ref &entity)
{
	world::bsp_file::BSPEntity *bspEntity = m_bspFile->AddEntity();
	bspEntity->firstString = m_bspFile->numStrings;
	bspEntity->numStrings = (int)entity->keys.pairs.size();

	for (world::Keys::Pairs::const_iterator it = entity->keys.pairs.begin(); it != entity->keys.pairs.end(); ++it)
	{
		*m_bspFile->AddString() = it->first;
		*m_bspFile->AddString() = it->second;
	}
}

void BSPBuilder::BuildBSP(const SceneFile &map)
{
	m_numNodes = 1;
	m_numLeafs = 0;

	Node::Ref root(new (world::bsp_file::ZBSPBuilder) Node());
	root->bounds = FindBounds(map);

	for (int i = 0; i < 3; ++i)
	{
		if (root->bounds.Mins()[i] < -MaxAxisBounds[i])
		{
			Vec3 v = root->bounds.Mins();
			v[i] = -MaxAxisBounds[i];
			root->bounds.SetMins(v);
		}

		if (root->bounds.Maxs()[i] > MaxAxisBounds[i])
		{
			Vec3 v = root->bounds.Maxs();
			v[i] = MaxAxisBounds[i];
			root->bounds.SetMaxs(v);
		}
	}

	// Hack for Crow:
	// Insert a Z plane at 1280 units, right underneath the sky plane
	if (root->bounds.Maxs()[2] > 1280)
	{
		Vec3 normal(Vec3::Zero);
		normal[2] = 1.f;
		Plane pl(normal, 1280.f);

		root->planenum = m_planeHash.FindPlaneNum(pl);

		Node::Ref front(new (world::bsp_file::ZBSPBuilder) Node());
		Node::Ref back(new (world::bsp_file::ZBSPBuilder) Node());

		front->parent = root;
		back->parent = root;

		int bits = SplitBounds(2, pl.D(), root->bounds, front->bounds, back->bounds);
		RAD_VERIFY(bits == 3); // should have split!

		root->children[0] = front;
		root->children[1] = back;

		// leaf node front
		front->planenum = -1;

		++m_numNodes;
		++m_numLeafs;

		BoxTree(back, 0);
	}
	else
	{
		BoxTree(root, 0);
	}

	InsertEntModels(root, map.worldspawn);
	OptimizeTree(root);
	BoundTree(root);

	EmitPlanes();

	m_bspFile->ReserveNodes(m_numNodes);
	m_bspFile->ReserveLeafs(m_numLeafs);

	EmitNodes(root, (int)map.mats.size());
}

void BSPBuilder::BoxTree(const Node::Ref &node, int planebits)
{
	Vec3 size = node->bounds.Size();

	int axis;

	// find best axial split...
	for (axis = 2; axis >= 0; --axis)
	{
		if (planebits & (1<<axis))
			continue;

		if (size[axis] > SplitAxisBounds[axis]+32.f)
			break;

		planebits |= (1<<axis);
	}

	if (axis < 0)
	{
		for (axis = 2; axis >= 0; --axis)
		{
			if (size[axis] > SplitAxisBounds[axis]+32.f)
				break;
		}
	}
	else
	{
		planebits |= (1<<axis);
	}

	if (planebits == 7)
		planebits = 0; // completed rotation.

	if (axis < 0) // leaf node
	{
		node->planenum = -1;
		++m_numLeafs;
		return;
	}

	Vec3 normal(Vec3::Zero);
	normal[axis] = 1.f;
	Plane pl(normal, node->bounds.Mins()[axis] + (size[axis]*0.5f));

	node->planenum = m_planeHash.FindPlaneNum(pl);

	Node::Ref front(new (world::bsp_file::ZBSPBuilder) Node());
	Node::Ref back(new (world::bsp_file::ZBSPBuilder) Node());

	front->parent = node;
	back->parent = node;

	RAD_DEBUG_ONLY(int bits = )SplitBounds(axis, pl.D(), node->bounds, front->bounds, back->bounds);
	RAD_ASSERT(bits == 3); // should have split!

	node->children[0] = front;
	node->children[1] = back;

	m_numNodes += 2;

	BoxTree(front, planebits);
	BoxTree(back, planebits);
}

BBox BSPBuilder::FindBounds(const SceneFile &map)
{
	BBox bounds;
	bounds.Initialize();

	for (SceneFile::TriModel::Vec::const_iterator it = map.worldspawn->models.begin(); it != map.worldspawn->models.end(); ++it)
	{
		const SceneFile::TriModel::Ref &src = *it;
		if (src->cinematic)
			continue; // not here

		for (SceneFile::TriFaceVec::const_iterator it = src->tris.begin(); it != src->tris.end(); ++it)
		{
			const SceneFile::TriFace &tri = *it;
			if (tri.mat < 0)
				continue;

			for (int i = 0; i < 3; ++i)
				bounds.Insert(src->verts[tri.v[i]].pos);
		}
	}

	return bounds;
}

int BSPBuilder::SplitBounds(int axis, float distance, const BBox &bounds, BBox &front, BBox &back)
{
	// range?
	if (bounds.Mins()[axis] >= distance)
	{
		front = bounds;
		return 1;
	}

	if (bounds.Maxs()[axis] <= distance)
	{
		back = bounds;
		return 2;
	}

	Vec3 v = bounds.Mins();
	v[axis] = distance;
	front.SetMins(v);
	front.SetMaxs(bounds.Maxs());

	v = bounds.Maxs();
	v[axis] = distance;
	back.SetMins(bounds.Mins());
	back.SetMaxs(v);

	return 3;
}

BSPBuilder::Node::Ref BSPBuilder::FindBoundingNode(const Node::Ref &node, const BBox &bounds)
{
	if (node->planenum == -1)
		return node;

	const Plane &pl = m_planeHash.Plane(node->planenum);

	int axis;
	for (axis = 0; axis < 3; ++axis)
	{
		if (pl.Normal()[axis] > 0.f)
			break;
	}

	if (bounds.Mins()[axis] >= pl.D())
		return FindBoundingNode(node->children[0], bounds); // on front;
	if (bounds.Maxs()[axis] <= pl.D())
		return FindBoundingNode(node->children[1], bounds); // on back;

	// this is the bounding node
	return node;
}

void BSPBuilder::InsertEntModels(const Node::Ref &root, const SceneFile::Entity::Ref &entity)
{
	NodeTri nodeTri;

	for (SceneFile::TriModel::Vec::const_iterator it = entity->models.begin(); it != entity->models.end(); ++it)
	{
		const SceneFile::TriModel::Ref &src = *it;
		if (src->cinematic)
			continue; // not here

		Node::Ref node = FindBoundingNode(root, src->bounds);
				
		for (SceneFile::TriFaceVec::const_iterator it = src->tris.begin(); it != src->tris.end(); ++it)
		{
			const SceneFile::TriFace &tri = *it;
			if (tri.mat < 0)
				continue;

			nodeTri.mat = tri.mat;
			nodeTri.channels = src->numChannels;

			for (int i = 0; i < 3; ++i)
				nodeTri.v[i] = Vert(src->verts[tri.v[i]]);

			InsertModelTri(node, nodeTri);
		}
	}
}

// Find the best containing leaf to add the triangle to.
// There will be overruns, but the leaf bounds will be expanded to
// contain the contents.

void BSPBuilder::InsertModelTri(const Node::Ref &node, const NodeTri &tri)
{
	if (node->planenum == -1)
	{
		node->tris.reserve(32);
		node->tris.push_back(tri);
		return;
	}

	const Plane &pl = m_planeHash.Plane(node->planenum);

	int axis;
	for (axis = 0; axis < 3; ++axis)
	{
		if (pl.Normal()[axis] > 0.f)
			break;
	}

	int side = 0;
	float best = 0.f;
	float dist;

	for (int i = 0; i < 3; ++i)
	{
		dist = tri.v[i].pos[axis] - pl.D();
		if (dist > 0.f && dist > best)
		{
			best = dist;
			side = 0;
		}
		else if (dist < 0.f && -dist > best)
		{
			best = -dist;
			side = 1;
		}
	}

	InsertModelTri(node->children[side], tri);
}

void BSPBuilder::BoundTree(const Node::Ref &node)
{
	if (node->planenum != -1)
	{
		BoundTree(node->children[0]);
		BoundTree(node->children[1]);

		node->bounds.Initialize();
		node->bounds.Insert(node->children[0]->bounds);
		node->bounds.Insert(node->children[1]->bounds);
		return;
	}

	if (node->tris.empty())
		return; // preserve bounds.

	node->bounds.Initialize();

	for (NodeTri::Vec::const_iterator it = node->tris.begin(); it != node->tris.end(); ++it)
	{
		const NodeTri &tri = *it;
		for (int i = 0; i < 3; ++i)
			node->bounds.Insert(tri.v[i].pos);
	}
}

void BSPBuilder::OptimizeTree(Node::Ref &node)
{
	if (node->planenum == -1)
		return;

	OptimizeTree(node->children[0]);
	OptimizeTree(node->children[1]);

	// can only optimize nodes spanning a leaf on at least one side.
	if (node->children[0]->planenum != -1 &&
		node->children[1]->planenum != -1)
	{
		return;
	}

	// tris on front side, can merge if other side is leaf
	if (!node->children[0]->tris.empty() && node->children[1]->planenum != -1)
		return;

	// tris on back side, can merge if other side is  leaf
	if (!node->children[1]->tris.empty() && node->children[0]->planenum != -1)
		return;

	if (node->children[0]->planenum == -1 &&
		node->children[1]->planenum == -1)
	{ // merge empty leaf

		// candidates for merging based on tri count?
		if ((node->children[0]->tris.size() > SmallTriBatch) && (node->children[1]->tris.size() > SmallTriBatch))
			return;
		if ((node->children[0]->tris.size() > SmallTriBatch*4) && !node->children[1]->tris.empty())
			return;
		if ((node->children[1]->tris.size() > SmallTriBatch*4) && !node->children[0]->tris.empty())
			return;

		node->bounds.Initialize();

		// merge
		if (node->children[0]->tris.empty() || node->children[1]->tris.empty())
		{
			int s = node->children[0]->tris.empty() == true;
			node->tris.swap(node->children[s]->tris);
		}
		else
		{
			node->tris.swap(node->children[0]->tris);
			node->tris.reserve(node->tris.size() + node->children[1]->tris.size());
			std::copy(node->children[1]->tris.begin(), node->children[1]->tris.end(), std::back_inserter(node->tris));
		}

		// preserve bounds.

		node->bounds.Insert(node->children[0]->bounds);
		node->bounds.Insert(node->children[1]->bounds);

		// empty nodes, merge.

		node->planenum = -1;

		node->children[0].reset();
		node->children[1].reset();
	}
	else
	{
		// remove seperating plane.

		int s = node->children[0]->planenum == -1;
		RAD_ASSERT(node->children[s]->planenum != -1);

		Node::WRef parent = node->parent;

		node = node->children[s];
		node->parent = parent;
	}

	--m_numLeafs;
	--m_numNodes;
}

void BSPBuilder::EmitPlanes()
{
	m_bspFile->ReservePlanes((int)m_planeHash.Planes().size());

	for (PlaneHash::PlaneVec::const_iterator it = m_planeHash.Planes().begin(); it != m_planeHash.Planes().end(); ++it)
	{
		const Plane &pl = *it;

		world::bsp_file::BSPPlane *bspPlane = m_bspFile->AddPlane();
		bspPlane->p[0] = pl.A();
		bspPlane->p[1] = pl.B();
		bspPlane->p[2] = pl.C();
		bspPlane->p[3] = pl.D();
	}
}

void BSPBuilder::EmitNodes(const Node::Ref &node, int numMaterials)
{
	if (node->planenum == -1)
	{
		node->emitId = (int)m_bspFile->numLeafs.get();
		world::bsp_file::BSPLeaf *leaf = m_bspFile->AddLeaf();
		
		leaf->parent = -1;

		if (Node::Ref parent = node->parent.lock())
			leaf->parent = parent->emitId;

		for (int i = 0; i < 3; ++i)
		{
			leaf->mins[i] = node->bounds.Mins()[i];
			leaf->maxs[i] = node->bounds.Maxs()[i];
		}

		EmitModels(node, leaf, numMaterials);
		return;
	}

	node->emitId = (int)m_bspFile->numNodes.get();
	world::bsp_file::BSPNode *bspNode = m_bspFile->AddNode();

	bspNode->parent = -1;
	if (node->parent.lock())
		bspNode->parent = (S32)node->parent.lock()->emitId;
	bspNode->planenum = (U32)node->planenum;

	for (int i = 0; i < 3; ++i)
	{
		bspNode->mins[i] = node->bounds.Mins()[i];
		bspNode->maxs[i] = node->bounds.Maxs()[i];
	}

	EmitNodes(node->children[0], numMaterials);
	EmitNodes(node->children[1], numMaterials);

	// grab pointer, it may have been invalidated.
	bspNode = const_cast<world::bsp_file::BSPNode*>(m_bspFile->Nodes()) + node->emitId;

	for (int i = 0; i < 2; ++i)
	{
		if (node->children[i]->planenum == -1)
		{
			bspNode->children[i] = (S32)-(node->children[i]->emitId+1);
		}
		else
		{
			bspNode->children[i] = (S32)node->children[i]->emitId;
		}
	}
}

void BSPBuilder::EmitModels(const Node::Ref &node, world::bsp_file::BSPLeaf *leaf, int numMaterials)
{
	leaf->firstModel = (U32)m_bspFile->numModels.get();
	leaf->numModels = 0;

	for (int c = 1; c <= world::bsp_file::kMaxUVChannels; ++c)
	{
		for (int mat = 0; mat < numMaterials; ++mat)
		{
			TriModel model;
			model.mat = mat;
			model.numChannels = c;

			for (NodeTri::Vec::const_iterator it = node->tris.begin(); it != node->tris.end(); ++it)
			{
				const NodeTri &tri = *it;
				if (tri.mat != mat)
					continue;
				if (tri.channels != c)
					continue;

				if (model.indices.size() >= MaxBatchElements-3 ||
					model.verts.size() >= MaxBatchElements-3)
				{ // flush
					EmitModel(model);
					model.Clear();
					++leaf->numModels;
				}

				for (int i = 0; i < 3; ++i)
					model.AddVertex(tri.v[i]);
			}

			if (!model.verts.empty())
			{
				EmitModel(model);
				++leaf->numModels;
			}
		}
	}
}

void BSPBuilder::EmitModel(const TriModel &model)
{
	world::bsp_file::BSPModel *bspModel = m_bspFile->AddModel();
	bspModel->firstVert = m_bspFile->numVerts;
	bspModel->numVerts = (U32)model.verts.size();
	bspModel->firstIndex = m_bspFile->numIndices;
	bspModel->numIndices = (U32)model.indices.size();
	bspModel->material = (U32)model.mat;
	bspModel->numChannels = (U32)model.numChannels;
	bspModel->flags = 0;

	m_bspFile->ReserveVertices((int)model.verts.size());
	m_bspFile->ReserveIndices((int)model.indices.size());

	for (VertVec::const_iterator it = model.verts.begin(); it != model.verts.end(); ++it)
	{
		const Vert &v = *it;
		world::bsp_file::BSPVertex *bspV = m_bspFile->AddVertex();

		int i;
		for (i = 0; i < 3; ++i)
			bspV->v[i] = (float)v.pos[i];

		for (i = 0; i < model.numChannels && i < world::bsp_file::kMaxUVChannels; ++i)
		{
			bspV->st[i*2+0] = (float)v.st[i][0];
			bspV->st[i*2+1] = (float)v.st[i][1];
		}

		for (; i < world::bsp_file::kMaxUVChannels; ++i)
			bspV->st[i*2+0] = bspV->st[i*2+1] = 0.f;
	}

	for (TriModel::Indices::const_iterator it = model.indices.begin(); it != model.indices.end(); ++it)
	{ // NOTE: IOS only supports GL_UNSIGNED_SHORT
		RAD_ASSERT(*it < std::numeric_limits<U16>::max());
		*m_bspFile->AddIndex() = (U16)*it;
	}
}

} // box_bsp
} // tools
