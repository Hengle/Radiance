// Bsp.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Common/Map.h"
#include "../Common/MaxScene.h"
#include "../Common/Log.h"
#include "../Common/Asset.h"
#include "../Common/Files.h"
#include "../Common/RGL.h"
#include "../Common/GLCamera.h"
#include "../Common/GLNavWindow.h"
#include "../../Common/BSPFile.h"
#include <Runtime/Stream.h>
#include <vector>
#include <iostream>


extern bool g_glDebug;
extern bool g_fast;
extern bool g_le;
extern int  g_plats;
extern char g_mapname[1024];

typedef Map::ValueType ValueType;
typedef Map::Plane Plane;
typedef Map::Vec2  Vec2;
typedef Map::Vec3  Vec3;
typedef Map::Vec4  Vec4;
typedef Map::Mat4 Mat4;
typedef Map::Quat Quat;
typedef Map::Winding Winding;
typedef Map::BBox BBox;

typedef boost::shared_ptr<Winding> WindingRef;
typedef std::vector<WindingRef> WindingVec;
typedef Map::Vec3Vec Vec3Vec;

const ValueType SplitEpsilon = ValueType(0.00019999999);

#define INTERNAL_COMPILER_ERROR() Error("Internal Compiler Error: file: "__FILE__" function: "__FUNCTION__" line: " RAD_STRINGIZE(__LINE__))

#include "VecHash.h"
#include "PlaneHash.h"

Map::Vec3 RandomColor();

struct Poly
{
	Poly()
	{
		original = 0;
		color = RandomColor();
		++s_num;
	}

	Poly(const Poly &p)
	{
		original = p.original;
		planenum = p.planenum;
		contents = p.contents;
		onNode = p.onNode;
		color = p.color;
		++s_num;
	}

	~Poly()
	{
		--s_num;
	}

	Map::TriFace *original;
	WindingRef winding;
	Vec3 color;
	int planenum;
	int contents;
	bool onNode;
	
	static int s_num;
};

typedef boost::shared_ptr<Poly> PolyRef;
typedef std::vector<PolyRef> PolyVec;

struct TriModelFrag
{
	TriModelFrag()
	{
		original = 0;
		bounds.Initialize();
		++s_num;
	}

	~TriModelFrag()
	{
		--s_num;
	}

	PolyVec    polys;
	BBox       bounds;
	Map::TriModel *original;

	static int s_num;
};

typedef boost::shared_ptr<TriModelFrag> TriModelFragRef;
typedef std::vector<TriModelFragRef> TriModelFragVec;

struct WindingPlane
{
	WindingPlane()
	{
		++s_num;
	}

	~WindingPlane()
	{
		--s_num;
	}

	Winding winding;
	int planenum;

	static int s_num;
};

typedef boost::shared_ptr<WindingPlane> WindingPlaneRef;
typedef std::vector<WindingPlaneRef> WindingPlaneVec;

enum
{
	PlaneNumLeaf = -1
};

struct Node;
typedef boost::shared_ptr<Node> NodeRef;
struct Portal;
typedef boost::shared_ptr<Portal> PortalRef;

typedef std::vector<Map::TriFace*> TriFacePtrVec;

struct Portal
{
	Portal()
	{
		onNode = 0;
		nodes[0] = nodes[1] = 0;
		contents = 0;
		poly = 0;
		bounding = false;
		color = RandomColor();
		++s_num;
	}

	Portal (const Portal &p)
	{
		original = p.original;
		onNode = p.onNode;
		plane.planenum = p.plane.planenum;
		nodes[0] = nodes[1] = 0;
		contents = 0;
		poly = 0;
		bounding = p.bounding;
		color = p.color;
		++s_num;
	}

	~Portal()
	{
		--s_num;
	}

	WindingPlane plane;
	int contents; // bounded contents.
	TriFacePtrVec original;
	Poly *poly;
	Node *onNode; // null means outside node
	PortalRef next[2];
	Node   *nodes[2];
	Vec3 color;
	bool bounding;

	static int s_num;
};

typedef math::ConvexPolygon<Map::TriVert, Plane> SectorWinding;

struct AreaPoly
{
	AreaPoly() {}
	AreaPoly(const AreaPoly &p)
	{
		tri = p.tri;
	}

	Winding winding;
	Map::TriFace *tri;
};

struct SectorPoly
{
	SectorPoly() {}
	SectorPoly(const SectorPoly &p)
	{
		tri = p.tri;
	}

	SectorWinding  winding;
	Map::TriFace  *tri;
};

typedef boost::shared_ptr<SectorPoly> SectorPolyRef;
typedef std::vector<SectorPolyRef> SectorPolyVec;

struct Sector
{
	BBox bounds;
	SectorPolyVec polys;
};

struct SharedSector : public Sector
{
	std::vector<int> areas;
};

typedef boost::shared_ptr<Sector> SectorRef;
typedef std::vector<SectorRef> SectorVec;

typedef boost::shared_ptr<SharedSector> SharedSectorRef;
typedef std::vector<SharedSectorRef> SharedSectorVec;
typedef std::vector<Map::TriFace*> TriFacePtrVec;

struct Area
{
	Area() : area(-1)
	{

	}

	int area;
	
	BBox bounds;
	TriFacePtrVec tris;
	SectorVec  sectors;
	SharedSectorVec shared;
};

typedef boost::shared_ptr<Area> AreaRef;
typedef std::vector<AreaRef> AreaVec;

typedef container::hash_set<int>::type PlaneNumHash;

struct Node
{
	Node()
	{
		area = 0;
		parent = 0;
		contents = 0;
		planenum = 0;
		numPolys = 0;
		occupied = 0;
		bounds.Initialize();
		++s_num;
	}

	~Node()
	{
		--s_num;
	}

	TriModelFragVec models;
	BBox bounds;
	Node *parent;
	PortalRef portals;
	NodeRef children[2];
	int numPolys;
	int contents;
	int planenum;
	int occupied;
	Area *area;
	static int s_num;
};

struct BSP
{
	BSP(Map &map) : 
		m_map(map),
		m_numStructural(0),
		m_numDetail(0),
		m_numNodes(0),
		m_numLeafs(0),
		m_numPortals(0),
		m_progress(0),
		m_numOutsideNodes(0),
		m_numOutsideTris(0),
		m_numOutsideModels(0),
		m_numInsideNodes(0),
		m_numInsideModels(0),
		m_numInsideTris(0),
		m_validContents(0),
		m_numSectors(0),
		m_numSharedSectors(0),
		m_flood(false)
	{
	}

	void Build();
	void Write(const char *filename);

private:

	Map &m_map;
	Map::EntityRef m_leakEnt;
	PlaneHash m_planes;
	NodeRef m_root;
	Node m_outside;
	Vec3Vec m_leakpts;
	AreaVec m_areas;
	SharedSectorVec m_sharedSectors;
	int  m_numStructural;
	int  m_numDetail;
	int  m_numNodes;
	int  m_numLeafs;
	int  m_numPortals;
	int  m_progress;
	int  m_numOutsideNodes;
	int  m_numOutsideTris;
	int  m_numOutsideModels;
	int  m_numInsideTris;
	int  m_numInsideNodes;
	int  m_numInsideModels;
	int  m_validContents;
	int  m_numSectors;
	int  m_numSharedSectors;
	int  m_work;
	bool m_flood;

	void CreateRootNode();

	void DisplayTree(
		Node *node, 
		bool outside=true,
		const Node *target = 0
	);
	void DisplayPortals(const Node *node, Portal *a=0, Portal *b=0, bool leak=false, int contents=0);

	void LeafNode(Node *node);
	void Split(Node *node, int boxAxis);
	void Split(const TriModelFragRef &model, const Plane &p, int planenum, TriModelFragRef &front, TriModelFragRef &back);
	bool MarkNodePolys(int planenum, const TriModelFragRef &m);
	void Portalize();
	void SplitNodePortals(Node *node);
	void MakeNodePortal(Node *node);
	void MakeTreePortals(Node *node);
	void AddPortalToNodes(const PortalRef &p, Node *front, Node *back);
	void RemovePortalFromNode(const PortalRef &p, Node *node);
	void FindPortalNodeFaces(Node *node);
	Node *LeafForPoint(const Vec3 &pos, Node *node=0);
	bool FloodFill();
	void PortalFlood(Node *leaf, int depth);
	void MarkLeakTrail();
	void DumpLeakFile();
	void FillOutside();
	void MarkOccupiedNodeFaces(Node *node);
	void AreaFlood();
	void AreaFlood(Node *leaf, Area *area);
	void FindAreas(Node *node);
	void BuildSectors();
	void DecomposeAreaModel(const Map::TriModel &model);
	void DecomposeAreaPoly(Node *node, AreaPoly *poly);
	void BuildAreaSectors();
	void BuildAreaSectors(Area &area);
	void SubdivideSector(Area &area, Sector *sector);
	void SplitSector(const Plane &p, Sector &sector, Sector &front, Sector &back);
	void BuildSharedSectors();
	int FindSplitPlane(Node *node, int &boxAxis);
	int BoxPlaneNum(Node *node, int &boxAxis);

	void ResetProgress();
	void EmitProgress();

	class DrawTreeHandler : public GLNavWindow::PaintHandler
	{
	public:
		DrawTreeHandler (
			const Node *root, 
			bool outside=true,
			const Node *target = 0
			) : m_root(root), m_outside(outside),
				m_target(target) {}
		virtual void OnPaint(GLNavWindow &w);

	private:
		void DrawNode(const Node *node, GLState &s);
		const Node *m_root, *m_target;
		bool m_outside, m_wireframeOnly;
	};

	class DrawPortalsHandler : public GLNavWindow::PaintHandler
	{
	public:
		DrawPortalsHandler (const Node *root, Portal *a, Portal *b, Vec3Vec *leak, int contents=0) 
			: m_root(root), m_contents(contents), m_leak(leak)
		{
			m_p[0] = a;
			m_p[1] = b;
		}
		virtual void OnPaint(GLNavWindow &w);

	private:
		void DrawNode(const Node *node, GLState &s);
		const Node *m_root;
		Portal *m_p[2];
		Vec3Vec *m_leak;
		int   m_numDrawn;
		int   m_contents;
	};
};

// functions

void BBoxPlanes(const BBox &bounds, Plane *planes);
void BBoxWindings(const BBox &bounds, WindingVec &out);
void WindingBounds(const Winding &winding, BBox &bounds);
Vec3 WindingCenter(const Winding &winding);

// debug

void DisplayMap(const Map &m);
void RandomizeVertColors(Map &m);

