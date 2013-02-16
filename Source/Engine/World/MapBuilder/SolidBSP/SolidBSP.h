/*! \file SolidBSP.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#pragma once

#include "../../../Types.h"
#include "../../../COut.h"
#include "../../../Tools/SceneFile.h"
#include "../MapBuilderDebugUI.h"
#include "../../BSPFile.h"
#include "../../../Packages/Packages.h"
#include "../../../SkAnim/SkAnimDef.h"
#include "MapTypes.h"
#include "VecHash.h"
#include "PlaneHash.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Thread.h>
#include <Runtime/Thread/Locks.h>
#include <QtCore/QVariant>
#include <vector>
#include <boost/dynamic_bitset.hpp>
#include <Runtime/PushPack.h>

namespace tools {
namespace solid_bsp {

///////////////////////////////////////////////////////////////////////////////

Vec3 RandomColor(int index = -1);

///////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_LLVM)
#define SOLID_BSP_ICE() RAD_FAIL("SolidBSP Internal Compiler Error")
#else
#define SOLID_BSP_ICE() RAD_FAIL("SolidBSP Internal Compiler Error: file: "__FILE__" function: "__FUNCTION__" line: " RAD_STRINGIZE(__LINE__))
#endif

///////////////////////////////////////////////////////////////////////////////

//! Builds a solid area-based BSP.
/*! This is pretty similiar in function to Quake style BSP, a solid skin mesh
	is created by the artist, although any arbitrary enclosed shape will work.
	Typically this is just a big triangle skin (Quake brushes would work but 
	aren't necessary).

	The primary goal here is to automatically divide the map into areas seperated
	by portals for rendering. Detail models are the inserted into the BSP and indexed
	by area (no splitting of detail surfaces occurs).

	The BSP iteself is used for collision, but not really for rendering. At rendering time
	the BSP is used to locate the camera's area and the model list is rendered with
	frustum clipping. The model list is generated from visible areas.
 */
class RADENG_CLASS BSPBuilder : protected thread::Thread {
public:
	typedef boost::shared_ptr<BSPBuilder> Ref;

	BSPBuilder();
	~BSPBuilder();

	bool SpawnCompile(
		SceneFile &map, 
		CinematicActorCompressionMap &caMap,
		tools::UIProgress *ui = 0, 
		tools::map_builder::DebugUI *debugUI = 0, // if non-null debugging is enabled.
		std::ostream *cout = 0
	);

	RAD_DECLARE_READONLY_PROPERTY(BSPBuilder, bspFile, world::bsp_file::BSPFile::Ref);
	RAD_DECLARE_READONLY_PROPERTY(BSPBuilder, bspFileBuilder, world::bsp_file::BSPFileBuilder::Ref);
	RAD_DECLARE_READONLY_PROPERTY(BSPBuilder, result, int); // SR_ result

	void DebugDraw(float time, float dt, const QRect &viewport);
	void OnDebugMenu(const QVariant &data);

	void WaitForCompletion() const;

private:

	RAD_DECLARE_GET(bspFile, world::bsp_file::BSPFile::Ref) { 
		return boost::static_pointer_cast<world::bsp_file::BSPFile>(m_bspFile); 
	}

	RAD_DECLARE_GET(bspFileBuilder, world::bsp_file::BSPFileBuilder::Ref) { 
		return m_bspFile; 
	}

	enum {
		kPlaneNumLeaf = -1
	};

	/*
	==============================================================================
	BSP Polygon
	==============================================================================
	*/

	struct Poly {
		Poly() {
			original = 0;
			color = RandomColor();
			++s_num;
		}

		Poly(const Poly &p) {
			original = p.original;
			planenum = p.planenum;
			contents = p.contents;
			onNode = p.onNode;
			color = p.color;
			++s_num;
		}

		~Poly() {
			--s_num;
		}

		SceneFile::TriFace *original;
		WindingRef winding;
		Vec3 color;
		int planenum;
		int contents;
		bool onNode;
	
		static int s_num;
	};

	typedef boost::shared_ptr<Poly> PolyRef;
	typedef zone_vector<PolyRef, world::bsp_file::ZBSPBuilderT>::type PolyVec;

	/*
	==============================================================================
	BSP Original TriModel Polygon Fragment
	==============================================================================
	*/

	struct TriModelFrag {
		TriModelFrag() {
			original = 0;
			bounds.Initialize();
			++s_num;
		}

		~TriModelFrag() {
			--s_num;
		}

		PolyVec    polys;
		BBox       bounds;
		SceneFile::TriModel *original;

		static int s_num;
	};

	typedef boost::shared_ptr<TriModelFrag> TriModelFragRef;
	typedef zone_vector<TriModelFragRef, world::bsp_file::ZBSPBuilderT>::type TriModelFragVec;

	/*
	==============================================================================
	Winding Plane
	==============================================================================
	*/

	struct WindingPlane {
		WindingPlane() {
			++s_num;
		}

		~WindingPlane() {
			--s_num;
		}

		Winding winding;
		int planenum;

		static int s_num;
	};

	typedef boost::shared_ptr<WindingPlane> WindingPlaneRef;
	typedef zone_vector<WindingPlaneRef, world::bsp_file::ZBSPBuilderT>::type WindingPlaneVec;

	/*
	==============================================================================
	Forwards
	==============================================================================
	*/

	struct Node;
	typedef boost::shared_ptr<Node> NodeRef;
	struct Portal;
	typedef boost::shared_ptr<Portal> PortalRef;

	typedef zone_vector<SceneFile::TriFace*, world::bsp_file::ZBSPBuilderT>::type TriFacePtrVec;

	/*
	==============================================================================
	Portals
	==============================================================================
	*/

	struct Portal {
		Portal() {
			emitId = -1;
			onNode = 0;
			areas[0] = areas[1] = -1;
			nodes[0] = nodes[1] = 0;
			contents = 0;
			poly = 0;
			color = RandomColor();
			++s_num;
		}

		Portal (const Portal &p) {
			original = p.original;
			onNode = p.onNode;
			plane.planenum = p.plane.planenum;
			emitId = -1;
			areas[0] = areas[1] = -1;
			nodes[0] = nodes[1] = 0;
			contents = 0;
			poly = 0;
			color = p.color;
			++s_num;
		}

		~Portal() {
			--s_num;
		}

		WindingPlane plane;
		int contents; // bounded contents.
		int areas[2];
		int emitId;
		TriFacePtrVec original;
		Poly *poly;
		Node *onNode; // null means outside node
		PortalRef next[2];
		Node   *nodes[2];
		Vec3 color;

		static int s_num;
	};

	typedef math::Winding<SceneFileD::TriVert, Plane> AreaNodeWinding;

	/*
	==============================================================================
	Areas
	==============================================================================
	*/

	struct AreaPoly { // used in decompose
		AreaPoly() {}
		AreaPoly(const AreaPoly &p) {
			tri = p.tri;
			plane = p.plane;
		}

		Plane plane;
		Winding winding;
		SceneFile::TriFace *tri;
	};

	typedef zone_vector<int, world::bsp_file::ZBSPBuilderT>::type AreaNumVec;
	typedef zone_set<int, world::bsp_file::ZBSPBuilderT>::type AreaNumSet;

	///////////////////////////////////////////////////////////////////////////////

	struct Area {
		Area() : area(-1), numModels(0) {
		}

		int area;
		int numModels;
	};

	typedef boost::shared_ptr<Area> AreaRef;
	typedef zone_vector<AreaRef, world::bsp_file::ZBSPBuilderT>::type AreaVec;

	/*
	==============================================================================
	Nodes
	==============================================================================
	*/

	struct Node {
		Node() {
			area = 0;
			parent = 0;
			contents = 0;
			planenum = 0;
			numPolys = 0;
			occupied = 0;
			areaWarned = false;
			portalAreas[0] = -1;
			portalAreas[1] = -1;
			contentsOwner = 0;
			bounds.Initialize();
			++s_num;
		}

		~Node() {
			--s_num;
		}

		TriModelFragVec models;
		TriModelFragVec clipModels;
		BBox bounds;
		WindingVec windingBounds;
		Node *parent;
		PortalRef portals;
		NodeRef children[2];
		int numPolys;
		int contents;
		int planenum;
		int occupied;
		Area *area;
		SceneFile::TriModel *contentsOwner;
		bool areaWarned;
		int portalAreas[2];
		static int s_num;
	};

	/*
	==============================================================================
	TriModel emit helper
	==============================================================================
	*/

	struct EmitTriModel {
		typedef boost::shared_ptr<EmitTriModel> Ref;
		typedef SceneFileD::NormalTriVert Vert;
		typedef SceneFileD::NormalTriVertVec VertVec;
		typedef zone_map<Vert, int, world::bsp_file::ZBSPBuilderT>::type VertMap;
		typedef zone_vector<Ref, world::bsp_file::ZBSPBuilderT>::type Vec;
		typedef zone_vector<int, world::bsp_file::ZBSPBuilderT>::type Indices;
		VertVec verts;
		Indices indices;
		VertMap vmap;
		BBox bounds;
		int mat;
		int numChannels;
		int emitId;

		void AddVertex(const Vert &vert);
		void Clear() {
			verts.clear();
			indices.clear();
			vmap.clear();
			bounds.Initialize();
		}
	};

	/*
	==============================================================================
	Floors
	==============================================================================
	*/

	struct FloorBuilder {
		typedef SceneFile::Vec3 Vert;
		typedef SceneFile::Vec3Vec VertVec;
		typedef zone_map<Vert, int, world::bsp_file::ZBSPBuilderT>::type VertMap;

		FloorBuilder(
			const SceneFile::TriModel &_original,
			BSPBuilder *_bspBuilder
		) : original(_original), bspBuilder(_bspBuilder) {
		}
		
		struct Edge {
			typedef zone_vector<Edge, world::bsp_file::ZBSPBuilderT>::type Vec;
			typedef zone_map<Edge, int, world::bsp_file::ZBSPBuilderT>::type Map;

			int v[2];
			int t[2];
			Vert mid;
			Vert vec;
			Vert::ValueType dist[2];

			Edge() { 
				t[0] = -1;
				t[1] = -1;
			}

			int Compare(const Edge &e) const;

			bool operator == (const Edge &e) const {
				return Compare(e) == 0;
			}

			bool operator != (const Edge &e) const {
				return Compare(e) != 0;
			}

			bool operator > (const Edge &e) const {
				return Compare(e) > 0;
			}

			bool operator >= (const Edge &e) const {
				return Compare(e) >= 0;
			}

			bool operator < (const Edge &e) const {
				return Compare(e) < 0;
			}

			bool operator <= (const Edge &e) const {
				return Compare(e) <= 0;
			}
		};

		struct Tri {
			typedef zone_vector<Tri, world::bsp_file::ZBSPBuilderT>::type Vec;
			
			int v[3];
			int e[3];
		};

		VertVec verts;
		VertMap vmap;
		Edge::Vec edges;
		Edge::Map edgeMap;
		Tri::Vec tris;
		const SceneFile::TriModel &original;
		BSPBuilder *bspBuilder;
		
		int AddVert(const Vert &v);
		int AddEdge(int v0, int v1, int triNum);
		bool AddTri(const Vert &v0, const Vert &v1, const Vert &v2);
		bool ValidateTopology();

	private:

		typedef boost::dynamic_bitset<> TriBits;

		void Flood(int triNum, int &numVisited, TriBits &visited);
	};

	friend struct FloorBuilder;

	/*
	==============================================================================
	Debugging
	==============================================================================
	*/

	class PaintHandler {
	public:
		typedef boost::shared_ptr<PaintHandler> Ref;

		virtual ~PaintHandler() {}

		// return false from either of these to exit the paint handler.

		virtual void Init(tools::map_builder::DebugUI &ui, BSPBuilder &bsp);
		virtual bool Paint(float time, float dt, const QRect &viewport, tools::map_builder::DebugUI &ui, BSPBuilder &bsp) = 0;
		virtual bool OnMenu(const QVariant &data, tools::map_builder::DebugUI &ui, BSPBuilder &bsp) { return true; }

	protected:

		void EnableSmoothShading();
		void DisableSmoothShading();
		void BeginPaint(
			const QRect &viewport, 
			tools::map_builder::DebugUI &ui, 
			int state = 0,
			int blend = 0,
			bool cullBackfaces = true
		);
		void EndPaint();
		void BeginWireframe(bool backfaces = false);
		void EndWireframe();
		void SetMaterialColor(int id);
	};

	///////////////////////////////////////////////////////////////////////////////

	class AreaDraw;
	friend class AreaBSPDraw;

	class AreaBSPDraw : public PaintHandler {
	public:

		AreaBSPDraw();
		~AreaBSPDraw();

		virtual bool Paint(float time, float dt, const QRect &viewport, tools::map_builder::DebugUI &ui, BSPBuilder &bsp);
		virtual bool OnMenu(const QVariant &data, tools::map_builder::DebugUI &ui, BSPBuilder &bsp);
		

	private:

		void FindCameraArea(tools::map_builder::DebugUI &ui, BSPBuilder &bsp);
		void DrawModel(BSPBuilder &bsp, U32 model);
		void DrawAreaportals(BSPBuilder &bsp, int area);
		void DrawAreaportal(BSPBuilder &bsp, int portal);

		int m_area;
		Node *m_leaf;
	};

	///////////////////////////////////////////////////////////////////////////////

	class LeafFacesDraw;
	friend class LeafFacesDraw;

	class LeafFacesDraw : public PaintHandler {
	public:

		LeafFacesDraw();
		~LeafFacesDraw();

		virtual bool Paint(float time, float dt, const QRect &viewport, tools::map_builder::DebugUI &ui, BSPBuilder &bsp);
		virtual bool OnMenu(const QVariant &data, tools::map_builder::DebugUI &ui, BSPBuilder &bsp);

	private:

		void FindCameraLeaf(tools::map_builder::DebugUI &ui, BSPBuilder &bsp);
		void DrawNodes(Node *node, bool wireframe);
		
		Node *m_leaf;
		bool m_isolate;
		bool m_lock;
		tools::editor::PopupMenu *m_menu;
		QAction *m_isolateAction;
		QAction *m_lockAction;
	};

	/*
	==============================================================================
	COut/Log
	==============================================================================
	*/

	std::ostream &COut() {
		if (m_cout)
			return *m_cout;
		return ::COut(C_Debug);
	}

	void Log(const char *fmt, ...);

	void DisplayPaintHandler(PaintHandler *handler);

	RAD_DECLARE_GET(result, int);

	/*
	==============================================================================
	Members
	==============================================================================
	*/

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	SceneFile *m_map;
	SceneFile::Entity::Ref m_leakEnt;
	PlaneHash m_planes;
	world::bsp_file::BSPFileBuilder::Ref m_bspFile;
	NodeRef m_root;
	Node m_outside;
	Vec3Vec m_leakpts;
	AreaVec m_areas;
	Mutex m_paintMutex;
	PaintHandler::Ref m_paint;
	std::ostream *m_cout;
	tools::UIProgress *m_ui;
	tools::map_builder::DebugUI *m_debugUI;
	CinematicActorCompressionMap m_caMap;
	int m_numStructural;
	int m_numDetail;
	int m_numNodes;
	int m_numLeafs;
	int m_numPortalFaces;
	int m_numPortalSplits;
	int m_progress;
	int m_numOutsideNodes;
	int m_numOutsideTris;
	int m_numOutsideModels;
	int m_numInsideTris;
	int m_numInsideNodes;
	int m_numInsideModels;
	int m_numAreaNodes;
	int m_numAreaLeafs;
	int m_work;
	int m_result;
	bool m_flood;
	bool m_abort;

	/*
	==============================================================================
	Methods
	==============================================================================
	*/

	void Build();
	bool LoadMaterials();
	void CreateRootNode();

	void DisplayTree(
		Node *node, 
		bool outside=true,
		const Node *target = 0
	);

	void DisplayPortals(const Node *node, Portal *a=0, Portal *b=0, bool leak=false, int contents=0);

	void LeafNode(Node *node);
	void Split(Node *node);

	void SplitNodeBounds(
		Node *node, 
		const Plane &p, 
		BBox &front, 
		WindingVec &frontVec, 
		BBox &back, 
		WindingVec &backVec
	);
	
	void Split(
		const TriModelFragRef &model, 
		const Plane &p, 
		int planenum, 
		TriModelFragRef &front, 
		TriModelFragRef &back,
		ValueType epsilon
	);

	void InsertClipModel(
		const TriModelFragRef &model,
		Node *node
	);
	
	bool MarkNodePolys(int planenum, const TriModelFragRef &m);
	void MarkDetail();
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
	void FillOutsideNodes(Node *node);
	void MarkOccupiedNodeFaces(Node *node);
	bool AreaFlood();
	bool AreaFlood(Node *leaf, Area *area);
	bool CheckAreas(Node *node);
	bool FindAreas(Node *node);
	bool CompileAreas();
	void CompileClipModels();
	void DecomposeAreaModel(SceneFile::TriModel &model);
	void DecomposeAreaPoly(Node *node, AreaPoly *poly);
	int FindSplitPlane(Node *node);
	int BoxPlaneNum(Node *node);

	/*
	==============================================================================
	Emit
	==============================================================================
	*/

	bool EmitBSPFile();
	void EmitBSPMaterials();
	bool EmitBSPEntities();
	bool EmitBSPEntity(const SceneFile::Entity::Ref &entity);
	bool PutEntityOnFloor(const SceneFile::Entity::Ref &entity);
	bool EmitBSPAreas();
	void EmitBSPAreaportals(Node *leaf, int areaNum, world::bsp_file::BSPArea &area);
	void EmitBSPModels();
	void EmitBSPModel(const SceneFile::TriModel::Ref &triModel);
	int EmitBSPBrush(const SceneFile::Brush &brush);
	int EmitBSPModel(const EmitTriModel &model);
	S32 EmitBSPNodes(const Node *node, S32 parent);
	void EmitBSPClipModels(const Node *node, world::bsp_file::BSPLeaf *leaf);
	void EmitBSPClipModel(const TriModelFragRef &model);
	void EmitBSPPlanes();
	bool EmitBSPFloors();
	void EmitBSPWaypoints();
	bool EmitBSPWaypoint(SceneFile::Waypoint &waypoint);
	U32 FindBSPMaterial(const char *name);
	int FindBSPFloor(const char *name);
	int PutPointOnFloor(Vec3 &pos, int floorNum);
	int EmitBSPCinematics();

	void ResetProgress();
	void EmitProgress();
	void SetResult(int result);

	virtual int ThreadProc();

	/*
	==============================================================================
	Helpers
	==============================================================================
	*/

	static void BBoxPlanes(const BBox &bounds, Plane *planes);
	static void BBoxWindings(const BBox &bounds, WindingVec &out);
	static void WindingBounds(const Winding &winding, BBox &bounds);
	static Vec3 WindingCenter(const Winding &winding);
	static int ContentsForString(const String &s);
	static int SurfaceForString(const String &s);

	static Vec3 SnapVertex(const Vec3 &v);

	// Converts to BSP precision types.
	static Vec2 ToBSPType(const SceneFile::Vec2 &vec);
	static Vec3 ToBSPType(const SceneFile::Vec3 &vec);
	static Vec4 ToBSPType(const SceneFile::Vec4 &vec);
	static SceneFileD::TriVert ToBSPType(const SceneFile::TriVert &vec);
	static BBox ToBSPType(const SceneFile::BBox &bbox);
	static Plane ToBSPType(const SceneFile::Plane &plane);
	static SceneFile::Vec3 FromBSPType(const Vec3 &vec);
	static SceneFile::Plane FromBSPType(const Plane &plane);
};

} // solid_bsp
} // tools

#include <Runtime/PopPack.h>
