// BSPFile.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "../SkAnim/SkAnim.h"
#if defined(RAD_OPT_TOOLS)
#include "../SkAnim/SkBuilder.h"
#endif
#include <Runtime/StreamDef.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace world {
namespace bsp_file {

///////////////////////////////////////////////////////////////////////////////

enum {
	RAD_FLAG(kCinematicObj),
	RAD_FLAG(kHideUntilRef),
	RAD_FLAG(kHideWhenDone),
	RAD_FLAG(kAffectedByObjectLights),
	RAD_FLAG(kAffectedByWorldLights),
	RAD_FLAG(kCastShadows),
	RAD_FLAG(kSkyActor),
	kMaxUVChannels = 2
};

/*
==============================================================================
Contents & Surface Flags
==============================================================================
*/

enum ContentsFlags {
	RAD_FLAG(kContentsFlag_Areaportal),
	RAD_FLAG(kContentsFlag_Solid),
	RAD_FLAG(kContentsFlag_Fog),

	// Not in BSP
	RAD_FLAG(kContentsFlag_Clip),
	RAD_FLAG(kContentsFlag_Detail), // detail contents are removed if outside valid BSP
	RAD_FLAG(kContentsFlag_Floor),

	kContentsFlag_VisibleContents = kContentsFlag_Areaportal|kContentsFlag_Solid|kContentsFlag_Fog,
	kContentsFlag_FirstVisibleContents = kContentsFlag_Areaportal,
	kContentsFlag_LastVisibleContents = kContentsFlag_Fog,
	kContentsFlag_Structural = kContentsFlag_Solid|kContentsFlag_Areaportal, // just used for classification
	kContentsFlag_SolidContents = kContentsFlag_Solid, // blocks portal flood
	kContentsFlag_BSPContents = kContentsFlag_Areaportal|kContentsFlag_Solid|kContentsFlag_Fog,
	kContentsFlag_DetailContents = kContentsFlag_Detail|kContentsFlag_Clip
};

enum SurfaceFlags {
	RAD_FLAG(kSurfaceFlag_NoDraw),
	RAD_FLAG(kSurfaceFlag_SkyPortal)
};

enum WaypointConnectionFlags {
	RAD_FLAG(kWaypointConnectionFlag_AtoB),
	RAD_FLAG(kWaypointConnectionFlag_BtoA),
	RAD_FLAG(kWaypointConnectionFlag_BtoAUseAtoBScript),
	RAD_FLAG(kWaypointConnectionFlag_AutoFace),
	RAD_FLAG(kWaypointConnectionFlag_Interruptable)
};

struct BSPNode {
	S32 parent;
	S32 children[2];
	U32 planenum;
	float mins[3];
	float maxs[3];
};

struct BSPLeaf {
	S32 parent;
	S32 area;
	U32 contents;
	U32 firstClipModel;
	U32 numClipModels;
	U32 firstFog;
	U32 numFogs;
	float mins[3];
	float maxs[3];
};

struct BSPClipModel {
	U32 firstClipSurface;
	U32 numClipSurfaces;
	float mins[3];
	float maxs[3];
};

struct BSPClipSurface {
	U32 flags;
	U32 contents;
	U32 surface;
	U32 planenum;
	U32 firstEdgePlane;
	U32 numEdgePlanes;
};

struct BSPArea { // sky area is always 0
	U32 firstPortal;
	U32 numPortals;
	U32 firstModel;
	U32 numModels;
	U32 sky; // can see sky
	float mins[3];
	float maxs[3];
};

struct BSPAreaportal {
	U32 firstVert;
	U32 numVerts;
	U32 planenum; // facing areas[0]
	U32 areas[2];
};

struct BSPModel {
	U32 firstVert;
	U32 numVerts;
	U32 firstIndex;
	U32 numIndices;
	U32 material;
	U32 numChannels;
	U32 contents;
	float mins[3];
	float maxs[3];
};

struct BSPMaterial {
	U32 string;
};

struct BSPEntity {
	U32 firstString;
	U32 numStrings;
};

struct BSPPlane {
	float p[4];
};

struct BSPBrush {
	U32 firstPlane;
	U32 numPlanes;
	float mins[3];
	float maxs[3];
};

struct BSPVertex {
	float v[3];
	float n[3];
	float t[4];
	float st[kMaxUVChannels*2];
};

struct BSPActor {
	int flags;
	int initial;
	int ska; // negative number is vtm
	float pos[3];
	float mins[3];
	float maxs[3];
};

struct BSPCameraTM {
	Quat r;
	Vec3 t;
	float fov;
	int tag;
};

struct BSPCameraTrack {
	S32 name;
	S32 firstTM;
	S32 numTMs;
};

struct BSPCinematicTrigger {
	S32 frame;
	S32 camera;
	S32 firstActor;
	S32 numActors;
};

struct BSPCinematic {
	S32 name;
	S32 fps;
	S32 firstTrigger;
	S32 numTriggers;
};

struct BSPWaypoint {
	float pos[3];
	U32 uid;
	U32 firstConnection;
	U32 numConnections;
	S32 floorNum;
	S32 triNum;
	S32 targetName;
	S32 userId;
	S32 flags;
};

struct BSPWaypointConnection {
	S32 flags;
	S32 cmds[4];
	S32 anims[2];
	U32 waypoints[2];
	float ctrls[2][3]; // curve
};

struct BSPFloor { // walkable surface
	U32 name;
	U32 firstTri;
	U32 numTris;
	S32 firstWaypoint;
	S32 numWaypoints;
};

struct BSPFloorTri {
	U32 verts[3];
	U32 edges[3];
	U32 planenum;
};

struct BSPFloorEdge {
	U32 verts[2];
	S32 tris[2];
	float vec[3];
	float dist[2];
	U32 planenum;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS BSPFile {
public:

	typedef boost::shared_ptr<BSPFile> Ref;

	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numEntities, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numStrings, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numNodes, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numLeafs, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numAreas, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numAreaportals, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numAreaportalIndices, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numModels, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numModelIndices, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numBrushes, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numClipModels, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numClipSurfaces, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numClipEdgePlanes, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numVerts, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numIndices, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numMaterials, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numWaypoints, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numWaypointIndices, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numWaypointConnections, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numFloors, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numFloorTris, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numFloorEdges, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numPlanes, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numActors, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numActorIndices, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numCameraTMs, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numCameraTracks, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numCinematicTriggers, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numCinematics, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numSkas, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numVtms, U32);

	virtual const char *String(U32 idx) const = 0;
	virtual const BSPEntity *Entities() const = 0;
	virtual const BSPNode *Nodes() const = 0;
	virtual const BSPLeaf *Leafs() const = 0;
	virtual const BSPArea *Areas() const = 0;
	virtual const BSPAreaportal *Areaportals() const = 0;
	virtual const U16 *AreaportalIndices() const = 0;
	virtual const BSPModel *Models() const = 0;
	virtual const U16 *ModelIndices() const = 0;
	virtual const BSPBrush *Brushes() const = 0;
	virtual const BSPClipModel *ClipModels() const = 0;
	virtual const BSPClipSurface *ClipSurfaces() const = 0;
	virtual const BSPPlane *ClipEdgePlanes() const = 0;
	virtual const BSPWaypoint *Waypoints() const = 0;
	virtual const U16 *WaypointIndices() const = 0;
	virtual const BSPWaypointConnection *WaypointConnections() const = 0;
	virtual const BSPFloor *Floors() const = 0;
	virtual const BSPFloorTri *FloorTris() const = 0;
	virtual const BSPFloorEdge *FloorEdges() const = 0;
	virtual const BSPPlane *Planes() const = 0;
	virtual const BSPVertex *Vertices() const = 0;
	virtual const U16 *Indices() const = 0;
	virtual const BSPActor *Actors() const = 0;
	virtual const U32 *ActorIndices() const = 0;
	virtual const BSPMaterial *Materials() const = 0;
	virtual const BSPCameraTM *CameraTMs() const = 0;
	virtual const BSPCameraTrack *CameraTracks() const = 0;
	virtual const BSPCinematicTrigger *CinematicTriggers() const = 0;
	virtual const BSPCinematic *Cinematics() const = 0;
	virtual const ska::DSka &DSka(int idx) const = 0;
	virtual const ska::DSkm &DSkm(int idx) const = 0;
	virtual const ska::DVtm &DVtm(int idx) const = 0;

protected:

	virtual RAD_DECLARE_GET(numEntities, U32) = 0;
	virtual RAD_DECLARE_GET(numStrings, U32) = 0;
	virtual RAD_DECLARE_GET(numNodes, U32) = 0;
	virtual RAD_DECLARE_GET(numLeafs, U32) = 0;
	virtual RAD_DECLARE_GET(numAreas, U32) = 0;
	virtual RAD_DECLARE_GET(numAreaportals, U32) = 0;
	virtual RAD_DECLARE_GET(numAreaportalIndices, U32) = 0;
	virtual RAD_DECLARE_GET(numClipModels, U32) = 0;
	virtual RAD_DECLARE_GET(numClipSurfaces, U32) = 0;
	virtual RAD_DECLARE_GET(numClipEdgePlanes, U32) = 0;
	virtual RAD_DECLARE_GET(numModels, U32) = 0;
	virtual RAD_DECLARE_GET(numModelIndices, U32) = 0;
	virtual RAD_DECLARE_GET(numBrushes, U32) = 0;
	virtual RAD_DECLARE_GET(numVerts, U32) = 0;
	virtual RAD_DECLARE_GET(numIndices, U32) = 0;
	virtual RAD_DECLARE_GET(numMaterials, U32) = 0;
	virtual RAD_DECLARE_GET(numWaypoints, U32) = 0;
	virtual RAD_DECLARE_GET(numWaypointIndices, U32) = 0;
	virtual RAD_DECLARE_GET(numWaypointConnections, U32) = 0;
	virtual RAD_DECLARE_GET(numFloors, U32) = 0;
	virtual RAD_DECLARE_GET(numFloorTris, U32) = 0;
	virtual RAD_DECLARE_GET(numFloorEdges, U32) = 0;
	virtual RAD_DECLARE_GET(numPlanes, U32) = 0;
	virtual RAD_DECLARE_GET(numActors, U32) = 0;
	virtual RAD_DECLARE_GET(numActorIndices, U32) = 0;
	virtual RAD_DECLARE_GET(numCameraTMs, U32) = 0;
	virtual RAD_DECLARE_GET(numCameraTracks, U32) = 0;
	virtual RAD_DECLARE_GET(numCinematicTriggers, U32) = 0;
	virtual RAD_DECLARE_GET(numCinematics, U32) = 0;
	virtual RAD_DECLARE_GET(numSkas, U32) = 0;
	virtual RAD_DECLARE_GET(numVtms, U32) = 0;
};

///////////////////////////////////////////////////////////////////////////////

RAD_ZONE_DEC(RADENG_API, ZBSPFile);

class RADENG_CLASS BSPFileParser : public BSPFile {
public:

	typedef boost::shared_ptr<BSPFileParser> Ref;

	BSPFileParser();
	virtual ~BSPFileParser();

	int Parse(const void *data, AddrSize len);

	virtual const char *String(U32 idx) const;
	virtual const BSPEntity *Entities() const;
	virtual const BSPMaterial *Materials() const;
	virtual const BSPNode *Nodes() const;
	virtual const BSPLeaf *Leafs() const;
	virtual const BSPArea *Areas() const;
	virtual const BSPAreaportal *Areaportals() const;
	virtual const U16 *AreaportalIndices() const;
	virtual const BSPModel *Models() const;
	virtual const U16 *ModelIndices() const;
	virtual const BSPBrush *Brushes() const;
	virtual const BSPClipModel *ClipModels() const;
	virtual const BSPClipSurface *ClipSurfaces() const;
	virtual const BSPPlane *ClipEdgePlanes() const;
	virtual const BSPWaypoint *Waypoints() const;
	virtual const U16 *WaypointIndices() const;
	virtual const BSPWaypointConnection *WaypointConnections() const;
	virtual const BSPFloor *Floors() const;
	virtual const BSPFloorTri *FloorTris() const;
	virtual const BSPFloorEdge *FloorEdges() const;
	virtual const BSPPlane *Planes() const;
	virtual const BSPVertex *Vertices() const;
	virtual const BSPActor *Actors() const;
	virtual const U16 *Indices() const;
	virtual const U32 *ActorIndices() const;
	virtual const BSPCameraTM *CameraTMs() const;
	virtual const BSPCameraTrack *CameraTracks() const;
	virtual const BSPCinematicTrigger *CinematicTriggers() const;
	virtual const BSPCinematic *Cinematics() const;
	virtual const ska::DSka &DSka(int idx) const;
	virtual const ska::DSkm &DSkm(int idx) const;
	virtual const ska::DVtm &DVtm(int idx) const;

	virtual U32 NumTexCoords(int channel) const;

private:

	virtual RAD_DECLARE_GET(numEntities, U32);
	virtual RAD_DECLARE_GET(numStrings, U32);
	virtual RAD_DECLARE_GET(numMaterials, U32);
	virtual RAD_DECLARE_GET(numNodes, U32);
	virtual RAD_DECLARE_GET(numLeafs, U32);
	virtual RAD_DECLARE_GET(numAreas, U32);
	virtual RAD_DECLARE_GET(numAreaportals, U32);
	virtual RAD_DECLARE_GET(numAreaportalIndices, U32);
	virtual RAD_DECLARE_GET(numClipModels, U32);
	virtual RAD_DECLARE_GET(numClipSurfaces, U32);
	virtual RAD_DECLARE_GET(numClipEdgePlanes, U32);
	virtual RAD_DECLARE_GET(numModels, U32);
	virtual RAD_DECLARE_GET(numModelIndices, U32);
	virtual RAD_DECLARE_GET(numBrushes, U32);
	virtual RAD_DECLARE_GET(numVerts, U32);
	virtual RAD_DECLARE_GET(numIndices, U32);
	virtual RAD_DECLARE_GET(numWaypoints, U32);
	virtual RAD_DECLARE_GET(numWaypointIndices, U32);
	virtual RAD_DECLARE_GET(numWaypointConnections, U32);
	virtual RAD_DECLARE_GET(numFloors, U32);
	virtual RAD_DECLARE_GET(numFloorTris, U32);
	virtual RAD_DECLARE_GET(numFloorEdges, U32);
	virtual RAD_DECLARE_GET(numActorIndices, U32);
	virtual RAD_DECLARE_GET(numActors, U32);
	virtual RAD_DECLARE_GET(numPlanes, U32);
	virtual RAD_DECLARE_GET(numCameraTMs, U32);
	virtual RAD_DECLARE_GET(numCameraTracks, U32);
	virtual RAD_DECLARE_GET(numCinematicTriggers, U32);
	virtual RAD_DECLARE_GET(numCinematics, U32);
	virtual RAD_DECLARE_GET(numSkas, U32);
	virtual RAD_DECLARE_GET(numVtms, U32);

	typedef zone_vector<int, ZBSPFileT>::type IntVec;
	
	IntVec m_stringOfs;
	const char *m_strings;
	const BSPEntity *m_ents;
	const BSPMaterial *m_mats;
	const BSPNode *m_nodes;
	const BSPLeaf *m_leafs;
	const BSPArea *m_areas;
	const BSPAreaportal *m_areaportals;
	const U16 *m_areaportalIndices;
	const BSPModel *m_models;
	const U16 *m_modelIndices;
	const BSPBrush *m_brushes;
	const BSPClipModel *m_clipModels;
	const BSPClipSurface *m_clipSurfaces;
	const BSPPlane *m_clipEdgePlanes;
	const BSPPlane *m_planes;
	const BSPVertex *m_verts;
	const BSPVertex *m_normals;
	const BSPWaypoint *m_waypoints;
	const U16 *m_waypointIndices;
	const BSPWaypointConnection *m_waypointConnections;
	const BSPFloor *m_floors;
	const BSPFloorTri *m_floorTris;
	const BSPFloorEdge *m_floorEdges;
	const U16 *m_indices;
	const U32 *m_actorIndices;
	const BSPActor *m_actors;
	const BSPCameraTM *m_cameraTMs;
	const BSPCameraTrack *m_cameraTracks;
	const BSPCinematicTrigger *m_cinematicTriggers;
	const BSPCinematic *m_cinematics;
	ska::DSka *m_skas;
	ska::DSkm *m_skms;
	ska::DVtm *m_vtms;
	U32 m_numStrings;
	U32 m_numEnts;
	U32 m_numMats;
	U32 m_numNodes;
	U32 m_numLeafs;
	U32 m_numAreas;
	U32 m_numAreaportals;
	U32 m_numAreaportalIndices;
	U32 m_numClipModels;
	U32 m_numClipSurfaces;
	U32 m_numModels;
	U32 m_numModelIndices;
	U32 m_numBrushes;
	U32 m_numPlanes;
	U32 m_numClipEdgePlanes;
	U32 m_numVerts;
	U32 m_numTexCoords[kMaxUVChannels];
	U32 m_numIndices;
	U32 m_numWaypoints;
	U32 m_numWaypointIndices;
	U32 m_numWaypointConnections;
	U32 m_numFloors;
	U32 m_numFloorTris;
	U32 m_numFloorEdges;
	U32 m_numActorIndices;
	U32 m_numActors;
	U32 m_numSkas;
	U32 m_numVtms;
	U32 m_numCameraTMs;
	U32 m_numCameraTracks;
	U32 m_numCinematicTriggers;
	U32 m_numCinematics;
};

///////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_TOOLS)

RAD_ZONE_DEC(RADENG_API, ZBSPBuilder);

class RADENG_CLASS BSPFileBuilder : public BSPFile {
public:

	typedef boost::shared_ptr<BSPFileBuilder> Ref;

	virtual const char *String(U32 idx) const;
	virtual const BSPEntity *Entities() const;
	virtual const BSPMaterial *Materials() const;
	virtual const BSPNode *Nodes() const;
	virtual const BSPLeaf *Leafs() const;
	virtual const BSPArea *Areas() const;
	virtual const BSPAreaportal *Areaportals() const;
	virtual const U16 *AreaportalIndices() const;
	virtual const BSPModel *Models() const;
	virtual const U16 *ModelIndices() const;
	virtual const BSPBrush *Brushes() const;
	virtual const BSPClipModel *ClipModels() const;
	virtual const BSPClipSurface *ClipSurfaces() const;
	virtual const BSPPlane *ClipEdgePlanes() const;
	virtual const BSPWaypoint *Waypoints() const;
	virtual const U16 *WaypointIndices() const;
	virtual const BSPWaypointConnection *WaypointConnections() const;
	virtual const BSPFloor *Floors() const;
	virtual const BSPFloorTri *FloorTris() const;
	virtual const BSPFloorEdge *FloorEdges() const;
	virtual const BSPPlane *Planes() const;
	virtual const BSPVertex *Vertices() const;
	virtual const U16 *Indices() const;
	virtual const U32 *ActorIndices() const;
	virtual const BSPCameraTM *CameraTMs() const;
	virtual const BSPCameraTrack *CameraTracks() const;
	virtual const BSPCinematicTrigger *CinematicTriggers() const;
	virtual const BSPCinematic *Cinematics() const;
	virtual const ska::DSka &DSka(int idx) const;
	virtual const ska::DSkm &DSkm(int idx) const;
	virtual const ska::DVtm &DVtm(int idx) const;
	virtual const BSPActor *Actors() const;
		
	void Clear();

	void ReserveStrings(int num);
	void ReserveEntities(int num);
	void ReserveMaterials(int num);
	void ReserveNodes(int num);
	void ReserveLeafs(int num);
	void ReserveAreas(int num);
	void ReserveAreaportals(int num);
	void ReserveAreaportalIndices(int num);
	void ReserveClipModels(int num);
	void ReserveClipSurfaces(int num);
	void ReserveClipEdgePlanes(int num);
	void ReserveModels(int num);
	void ReserveModelIndices(int num);
	void ReserveBrushes(int num);
	void ReservePlanes(int num);
	void ReserveVertices(int num);
	void ReserveIndices(int num);
	void ReserveWaypoints(int num);
	void ReserveWaypointIndices(int num);
	void ReserveWaypointConnections(int num);
	void ReserveFloors(int num);
	void ReserveFloorTris(int num);
	void ReserveFloorEdges(int num);
	void ReserveActorIndices(int num);
	void ReserveCameraTMs(int num);
	void ReserveCameraTracks(int num);
	void ReserveCinematicTriggers(int num);
	void ReserveCinematics(int num);
	void ReserveSkas(int num);
	void ReserveVtms(int num);
	void ReserveActors(int num);
	
	::String *AddString();
	void SetString(U32 idx, const ::String &value);
	BSPEntity *AddEntity();
	BSPMaterial *AddMaterial();
	BSPNode *AddNode();
	BSPLeaf *AddLeaf();
	BSPArea *AddArea();
	BSPAreaportal *AddAreaportal();
	U16 *AddAreaportalIndex();
	BSPClipModel *AddClipModel();
	BSPClipSurface *AddClipSurface();
	BSPPlane *AddClipEdgePlane();
	BSPModel *AddModel();
	U16 *AddModelIndex();
	BSPBrush *AddBrush();
	BSPPlane *AddPlane();
	BSPVertex *AddVertex();
	BSPActor *AddActor();
	BSPWaypoint *AddWaypoint();
	U16 *AddWaypointIndex();
	BSPWaypointConnection *AddWaypointConnection();
	BSPFloor *AddFloor();
	BSPFloorTri *AddFloorTri();
	BSPFloorEdge *AddFloorEdge();
	U16 *AddIndex();
	U32 *AddActorIndex();
	BSPCameraTM *AddCameraTM();
	BSPCameraTrack *AddCameraTrack();
	BSPCinematicTrigger *AddCinematicTrigger();
	BSPCinematic *AddCinematic();
	
	int AddSka(
		const tools::SkaData::Ref &skaRef,
		const tools::SkmData::Ref &skmRef
	);

	int AddVtm(const tools::VtmData::Ref &vtmRef);

	int Write(stream::OutputStream &os);

protected:
	
	virtual RAD_DECLARE_GET(numEntities, U32);
	virtual RAD_DECLARE_GET(numStrings, U32);
	virtual RAD_DECLARE_GET(numMaterials, U32);
	virtual RAD_DECLARE_GET(numNodes, U32);
	virtual RAD_DECLARE_GET(numLeafs, U32);
	virtual RAD_DECLARE_GET(numAreas, U32);
	virtual RAD_DECLARE_GET(numAreaportals, U32);
	virtual RAD_DECLARE_GET(numModels, U32);
	virtual RAD_DECLARE_GET(numBrushes, U32);
	virtual RAD_DECLARE_GET(numClipModels, U32);
	virtual RAD_DECLARE_GET(numClipSurfaces, U32);
	virtual RAD_DECLARE_GET(numClipEdgePlanes, U32);
	virtual RAD_DECLARE_GET(numVerts, U32);
	virtual RAD_DECLARE_GET(numAreaportalIndices, U32);
	virtual RAD_DECLARE_GET(numModelIndices, U32);
	virtual RAD_DECLARE_GET(numIndices, U32);
	virtual RAD_DECLARE_GET(numWaypoints, U32);
	virtual RAD_DECLARE_GET(numWaypointIndices, U32);
	virtual RAD_DECLARE_GET(numWaypointConnections, U32);
	virtual RAD_DECLARE_GET(numFloors, U32);
	virtual RAD_DECLARE_GET(numFloorTris, U32);
	virtual RAD_DECLARE_GET(numFloorEdges, U32);
	virtual RAD_DECLARE_GET(numPlanes, U32);
	virtual RAD_DECLARE_GET(numCameraTMs, U32);
	virtual RAD_DECLARE_GET(numCameraTracks, U32);
	virtual RAD_DECLARE_GET(numCinematicTriggers, U32);
	virtual RAD_DECLARE_GET(numCinematics, U32);
	virtual RAD_DECLARE_GET(numSkas, U32);
	virtual RAD_DECLARE_GET(numVtms, U32);
	virtual RAD_DECLARE_GET(numActors, U32);
	virtual RAD_DECLARE_GET(numActorIndices, U32);

	typedef zone_vector<BSPEntity, ZBSPBuilderT>::type BSPEntityVec;
	typedef zone_vector<BSPMaterial, ZBSPBuilderT>::type BSPMaterialVec;
	typedef zone_vector<BSPNode, ZBSPBuilderT>::type BSPNodeVec;
	typedef zone_vector<BSPLeaf, ZBSPBuilderT>::type BSPLeafVec;
	typedef zone_vector<BSPArea, ZBSPBuilderT>::type BSPAreaVec;
	typedef zone_vector<BSPAreaportal, ZBSPBuilderT>::type BSPAreaportalVec;
	typedef zone_vector<BSPModel, ZBSPBuilderT>::type BSPModelVec;
	typedef zone_vector<BSPBrush, ZBSPBuilderT>::type BSPBrushVec;
	typedef zone_vector<BSPClipModel, ZBSPBuilderT>::type BSPClipModelVec;
	typedef zone_vector<BSPClipSurface, ZBSPBuilderT>::type BSPClipSurfaceVec;
	typedef zone_vector<BSPPlane, ZBSPBuilderT>::type BSPPlaneVec;
	typedef zone_vector<BSPVertex, ZBSPBuilderT>::type BSPVertexVec;
	typedef zone_vector<BSPWaypoint, ZBSPBuilderT>::type BSPWaypointVec;
	typedef zone_vector<BSPWaypointConnection, ZBSPBuilderT>::type BSPWaypointConnectionVec;
	typedef zone_vector<BSPFloor, ZBSPBuilderT>::type BSPFloorVec;
	typedef zone_vector<BSPFloorTri, ZBSPBuilderT>::type BSPFloorTriVec;
	typedef zone_vector<BSPFloorEdge, ZBSPBuilderT>::type BSPFloorEdgeVec;
	typedef zone_vector<U16, ZBSPBuilderT>::type BSPIndexVec;
	typedef zone_vector<U32, ZBSPBuilderT>::type BSPActorIndexVec;
	typedef zone_vector<BSPCameraTM, ZBSPBuilderT>::type BSPCameraTMVec;
	typedef zone_vector<BSPCameraTrack, ZBSPBuilderT>::type BSPCameraTrackVec;
	typedef zone_vector<BSPCinematicTrigger, ZBSPBuilderT>::type BSPCinematicTriggerVec;
	typedef zone_vector<BSPCinematic, ZBSPBuilderT>::type BSPCinematicVec;
	typedef zone_vector<BSPActor, ZBSPBuilderT>::type BSPActorVec;
	typedef zone_vector<tools::SkaData::Ref, ZBSPBuilderT>::type SkaVec;
	typedef zone_vector<tools::SkmData::Ref, ZBSPBuilderT>::type SkmVec;
	typedef zone_vector<tools::VtmData::Ref, ZBSPBuilderT>::type VtmVec;

	StringVec m_strings;
	BSPMaterialVec m_mats;
	BSPEntityVec m_ents;
	BSPNodeVec m_nodes;
	BSPLeafVec m_leafs;
	BSPAreaVec m_areas;
	BSPAreaportalVec m_areaportals;
	BSPIndexVec m_areaportalIndices;
	BSPModelVec m_models;
	BSPIndexVec m_modelIndices;
	BSPBrushVec m_brushes;
	BSPClipModelVec m_clipModels;
	BSPClipSurfaceVec m_clipSurfaces;
	BSPPlaneVec m_clipEdgePlanes;
	BSPPlaneVec m_planes;
	BSPVertexVec m_vertices;
	BSPWaypointVec m_waypoints;
	BSPIndexVec m_waypointIndices;
	BSPWaypointConnectionVec m_waypointConnections;
	BSPFloorVec m_floors;
	BSPFloorTriVec m_floorTris;
	BSPFloorEdgeVec m_floorEdges;
	BSPIndexVec m_indices;
	BSPActorIndexVec m_actorIndices;
	BSPCameraTMVec m_cameraTMs;
	BSPCameraTrackVec m_cameraTracks;
	BSPCinematicTriggerVec m_cinematicTriggers;
	BSPCinematicVec m_cinematics;
	BSPActorVec m_actors;
	SkaVec m_skas;
	SkmVec m_skms;
	VtmVec m_vtms;
};

#endif

} // bsp_file
} // world

#include <Runtime/PopPack.h>

#include "BSPFile.inl"
