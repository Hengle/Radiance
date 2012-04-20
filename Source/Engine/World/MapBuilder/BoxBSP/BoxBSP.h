// BSPBuilder.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../../Types.h"
#include "../../../Tools/Map.h"
#include "../../BSPFile.h"
#include "MapTypes.h"
#include "VecHash.h"
#include "PlaneHash.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneHashSet.h>
#include <Runtime/Container/ZoneHashMap.h>
#include <vector>

#include <Runtime/PushPack.h>

namespace tools {
namespace box_bsp {

///////////////////////////////////////////////////////////////////////////////

Vec3 RandomColor();

///////////////////////////////////////////////////////////////////////////////

//#define BOXBSP_NORMALS

class RADENG_CLASS BSPBuilder
{
public:
	typedef boost::shared_ptr<BSPBuilder> Ref;

	BSPBuilder();
	~BSPBuilder();

	bool Build(const Map &map);

	RAD_DECLARE_READONLY_PROPERTY(BSPBuilder, bspFile, world::bsp_file::BSPFile::Ref);
	RAD_DECLARE_READONLY_PROPERTY(BSPBuilder, bspFileBuilder, world::bsp_file::BSPFileBuilder::Ref);

private:

	RAD_DECLARE_GET(bspFile, world::bsp_file::BSPFile::Ref) { return boost::static_pointer_cast<world::bsp_file::BSPFile>(m_bspFile); }
	RAD_DECLARE_GET(bspFileBuilder, world::bsp_file::BSPFileBuilder::Ref) { return m_bspFile; }

#if defined(BOXBSP_NORMALS)
	typedef Map::NormalTriVert Vert;
#else
	typedef Map::TriVert Vert;
#endif

	typedef Map::TriVertVec VertVec;

	typedef zone_vector<int, world::bsp_file::ZBSPBuilderT>::type IntVec;
	typedef zone_map<Vert, int, world::bsp_file::ZBSPBuilderT>::type VertMap;
	typedef zone_set<String, world::bsp_file::ZBSPBuilderT>::type StringSet;

	struct TriModel
	{
		typedef boost::shared_ptr<TriModel> Ref;
		typedef zone_vector<Ref, world::bsp_file::ZBSPBuilderT>::type Vec;
		typedef zone_vector<int, world::bsp_file::ZBSPBuilderT>::type Indices;
		VertVec verts;
		Indices indices;
		VertMap vmap;
		int mat;
		int numChannels;

		void AddVertex(const Vert &vert);
		void Clear()
		{
			verts.clear();
			indices.clear();
			vmap.clear();
		}
	};

	struct Actor
	{
		typedef zone_vector<Actor, world::bsp_file::ZBSPBuilderT>::type Vec;
		Actor() : emitId(-1), index(-1) {}

		int flags;
		int emitId;
		int index;
	};

	struct Trigger
	{
		typedef zone_map<int, Trigger, world::bsp_file::ZBSPBuilderT>::type Map;
		int frame;
		int camera;
		IntVec actors;
	};

	struct NodeTri
	{
		typedef zone_vector<NodeTri, world::bsp_file::ZBSPBuilderT>::type Vec;
		Vert v[3];
		int mat;
		int channels;
	};

	struct Node
	{
		typedef boost::shared_ptr<Node> Ref;
		typedef boost::weak_ptr<Node> WRef;

		Node() : emitId(-1), planenum(-1) {}

		Node::WRef parent;
		Node::Ref children[2];
		NodeTri::Vec tris;
		BBox bounds;

		int planenum; // -1 == leafnode
		int emitId;
	};

	void EmitMaterials(const Map &map);
	void EmitEntities(const Map &map);
	bool EmitCinematics(const Map &map);
	bool EmitCinematic(const Map &map, const String &name);
	bool EmitActor(const Map &map, Actor &actor);
	void EmitEntity(const Map::Entity::Ref &entity);

	void BuildBSP(const Map &map);
	BBox FindBounds(const Map &map);
	int SplitBounds(int axis, float distance, const BBox &bounds, BBox &front, BBox &back);
	Node::Ref FindBoundingNode(const Node::Ref &node, const BBox &bounds);
	void BoxTree(const Node::Ref &node, int planebits);
	void InsertEntModels(const Node::Ref &root, const Map::Entity::Ref &entity);
	void InsertModelTri(const Node::Ref &node, const NodeTri &tri);
	void BoundTree(const Node::Ref &node);
	void OptimizeTree(Node::Ref &node);

	void EmitPlanes();
	void EmitNodes(const Node::Ref &node, int numMaterials);
	void EmitModels(const Node::Ref &node, world::bsp_file::BSPLeaf *leaf, int numMaterials);
	void EmitModel(const TriModel &model);

	AddrSize m_skaSize;
	StringSet m_cinematics;
	Actor::Vec m_actors;
	TriModel::Vec m_models;
	PlaneHash m_planeHash;
	int m_numNodes;
	int m_numLeafs;

	world::bsp_file::BSPFileBuilder::Ref m_bspFile;
};

} // box_bsp
} // tools

#include <Runtime/PopPack.h>
