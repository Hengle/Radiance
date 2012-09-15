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
	kMaxUVChannels = 2
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
	U32 firstClipSurface;
	U32 numClipSurfaces;
	float mins[3];
	float maxs[3];
};

struct BSPClipSurface {
	enum {
		RAD_FLAG(kFlag_Bevel)
	};
	U32 flags;
	U32 contents;
	U32 surface;
	U32 planenum; // faces out of the leaf
};

struct BSPSector {
	U32 firstModel;
	U32 numModels;
	float mins[3];
	float maxs[3];
};

struct BSPArea {
	U32 firstPortal;
	U32 numPortals;
	U32 firstSector;
	U32 numSectors;
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

struct BSPVertex {
	float v[3];
	float n[3];
	float st[kMaxUVChannels*2];
};

struct BSPActor {
	int flags;
	int initial;
	int ska;
};

struct BSPCameraTM {
	Quat r;
	Vec3 t;
	float fov;
	int tag;
};

struct BSPCameraTrack {
	int name;
	int firstTM;
	int numTMs;
};

struct BSPCinematicTrigger {
	int frame;
	int camera;
	int firstActor;
	int numActors;
};

struct BSPCinematic {
	int name;
	int fps;
	int firstTrigger;
	int numTriggers;
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
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numSectors, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numModels, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numClipSurfaces, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numVerts, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numAreaportalIndices, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numSectorIndices, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numIndices, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numMaterials, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numPlanes, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numActors, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numActorIndices, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numCameraTMs, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numCameraTracks, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numCinematicTriggers, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numCinematics, U32);
	RAD_DECLARE_READONLY_PROPERTY(BSPFile, numSkas, U32);

	virtual const char *String(U32 idx) const = 0;
	virtual const BSPEntity *Entities() const = 0;
	virtual const BSPNode *Nodes() const = 0;
	virtual const BSPLeaf *Leafs() const = 0;
	virtual const BSPArea *Areas() const = 0;
	virtual const BSPAreaportal *Areaportals() const = 0;
	virtual const BSPSector *Sectors() const = 0;
	virtual const BSPModel *Models() const = 0;
	virtual const BSPClipSurface *ClipSurfaces() const = 0;
	virtual const BSPPlane *Planes() const = 0;
	virtual const BSPVertex *Vertices() const = 0;
	virtual const U16 *AreaportalIndices() const = 0;
	virtual const U16 *SectorIndices() const = 0;
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

protected:

	virtual RAD_DECLARE_GET(numEntities, U32) = 0;
	virtual RAD_DECLARE_GET(numStrings, U32) = 0;
	virtual RAD_DECLARE_GET(numNodes, U32) = 0;
	virtual RAD_DECLARE_GET(numLeafs, U32) = 0;
	virtual RAD_DECLARE_GET(numAreas, U32) = 0;
	virtual RAD_DECLARE_GET(numAreaportals, U32) = 0;
	virtual RAD_DECLARE_GET(numSectors, U32) = 0;
	virtual RAD_DECLARE_GET(numClipSurfaces, U32) = 0;
	virtual RAD_DECLARE_GET(numModels, U32) = 0;
	virtual RAD_DECLARE_GET(numVerts, U32) = 0;
	virtual RAD_DECLARE_GET(numAreaportalIndices, U32) = 0;
	virtual RAD_DECLARE_GET(numSectorIndices, U32) = 0;
	virtual RAD_DECLARE_GET(numIndices, U32) = 0;
	virtual RAD_DECLARE_GET(numMaterials, U32) = 0;
	virtual RAD_DECLARE_GET(numPlanes, U32) = 0;
	virtual RAD_DECLARE_GET(numActors, U32) = 0;
	virtual RAD_DECLARE_GET(numActorIndices, U32) = 0;
	virtual RAD_DECLARE_GET(numCameraTMs, U32) = 0;
	virtual RAD_DECLARE_GET(numCameraTracks, U32) = 0;
	virtual RAD_DECLARE_GET(numCinematicTriggers, U32) = 0;
	virtual RAD_DECLARE_GET(numCinematics, U32) = 0;
	virtual RAD_DECLARE_GET(numSkas, U32) = 0;
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
	virtual const BSPSector *Sectors() const;
	virtual const BSPModel *Models() const;
	virtual const BSPClipSurface *ClipSurfaces() const;
	virtual const BSPPlane *Planes() const;
	virtual const BSPVertex *Vertices() const;
	virtual const BSPActor *Actors() const;
	virtual const U16 *AreaportalIndices() const;
	virtual const U16 *SectorIndices() const;
	virtual const U16 *Indices() const;
	virtual const U32 *ActorIndices() const;
	virtual const BSPCameraTM *CameraTMs() const;
	virtual const BSPCameraTrack *CameraTracks() const;
	virtual const BSPCinematicTrigger *CinematicTriggers() const;
	virtual const BSPCinematic *Cinematics() const;
	virtual const ska::DSka &DSka(int idx) const;
	virtual const ska::DSkm &DSkm(int idx) const;

	virtual U32 NumTexCoords(int channel) const;

private:

	virtual RAD_DECLARE_GET(numEntities, U32);
	virtual RAD_DECLARE_GET(numStrings, U32);
	virtual RAD_DECLARE_GET(numMaterials, U32);
	virtual RAD_DECLARE_GET(numNodes, U32);
	virtual RAD_DECLARE_GET(numLeafs, U32);
	virtual RAD_DECLARE_GET(numAreas, U32);
	virtual RAD_DECLARE_GET(numAreaportals, U32);
	virtual RAD_DECLARE_GET(numSectors, U32);
	virtual RAD_DECLARE_GET(numClipSurfaces, U32);
	virtual RAD_DECLARE_GET(numModels, U32);
	virtual RAD_DECLARE_GET(numVerts, U32);
	virtual RAD_DECLARE_GET(numAreaportalIndices, U32);
	virtual RAD_DECLARE_GET(numSectorIndices, U32);
	virtual RAD_DECLARE_GET(numIndices, U32);
	virtual RAD_DECLARE_GET(numActorIndices, U32);
	virtual RAD_DECLARE_GET(numActors, U32);
	virtual RAD_DECLARE_GET(numPlanes, U32);
	virtual RAD_DECLARE_GET(numCameraTMs, U32);
	virtual RAD_DECLARE_GET(numCameraTracks, U32);
	virtual RAD_DECLARE_GET(numCinematicTriggers, U32);
	virtual RAD_DECLARE_GET(numCinematics, U32);
	virtual RAD_DECLARE_GET(numSkas, U32);

	typedef zone_vector<int, ZBSPFileT>::type IntVec;
	
	IntVec m_stringOfs;
	const char *m_strings;
	const BSPEntity *m_ents;
	const BSPMaterial *m_mats;
	const BSPNode *m_nodes;
	const BSPLeaf *m_leafs;
	const BSPArea *m_areas;
	const BSPAreaportal *m_areaportals;
	const BSPSector *m_sectors;
	const BSPModel *m_models;
	const BSPClipSurface *m_clipSurfaces;
	const BSPPlane *m_planes;
	const BSPVertex *m_verts;
	const BSPVertex *m_normals;
	const U16 *m_areaportalIndices;
	const U16 *m_sectorIndices;
	const U16 *m_indices;
	const U32 *m_actorIndices;
	const BSPActor *m_actors;
	const BSPCameraTM *m_cameraTMs;
	const BSPCameraTrack *m_cameraTracks;
	const BSPCinematicTrigger *m_cinematicTriggers;
	const BSPCinematic *m_cinematics;
	ska::DSka *m_skas;
	ska::DSkm *m_skms;
	U32 m_numStrings;
	U32 m_numEnts;
	U32 m_numMats;
	U32 m_numNodes;
	U32 m_numLeafs;
	U32 m_numAreas;
	U32 m_numAreaportals;
	U32 m_numSectors;
	U32 m_numClipSurfaces;
	U32 m_numModels;
	U32 m_numPlanes;
	U32 m_numVerts;
	U32 m_numTexCoords[kMaxUVChannels];
	U32 m_numAreaportalIndices;
	U32 m_numSectorIndices;
	U32 m_numIndices;
	U32 m_numActorIndices;
	U32 m_numActors;
	U32 m_numSkas;
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
	virtual const BSPSector *Sectors() const;
	virtual const BSPModel *Models() const;
	virtual const BSPClipSurface *ClipSurfaces() const;
	virtual const BSPPlane *Planes() const;
	virtual const BSPVertex *Vertices() const;
	virtual const U16 *AreaportalIndices() const;
	virtual const U16 *SectorIndices() const;
	virtual const U16 *Indices() const;
	virtual const U32 *ActorIndices() const;
	virtual const BSPCameraTM *CameraTMs() const;
	virtual const BSPCameraTrack *CameraTracks() const;
	virtual const BSPCinematicTrigger *CinematicTriggers() const;
	virtual const BSPCinematic *Cinematics() const;
	virtual const ska::DSka &DSka(int idx) const;
	virtual const ska::DSkm &DSkm(int idx) const;
	virtual const BSPActor *Actors() const;

	void Clear();

	void ReserveStrings(int num);
	void ReserveEntities(int num);
	void ReserveMaterials(int num);
	void ReserveNodes(int num);
	void ReserveLeafs(int num);
	void ReserveAreas(int num);
	void ReserveAreaportals(int num);
	void ReserveSectors(int num);
	void ReserveClipSurfaces(int num);
	void ReserveModels(int num);
	void ReservePlanes(int num);
	void ReserveVertices(int num);
	void ReserveAreaportalIndices(int num);
	void ReserveSectorIndices(int num);
	void ReserveIndices(int num);
	void ReserveActorIndices(int num);
	void ReserveCameraTMs(int num);
	void ReserveCameraTracks(int num);
	void ReserveCinematicTriggers(int num);
	void ReserveCinematics(int num);
	void ReserveSkas(int num);
	void ReserveActors(int num);
	
	::String *AddString();
	BSPEntity *AddEntity();
	BSPMaterial *AddMaterial();
	BSPNode *AddNode();
	BSPLeaf *AddLeaf();
	BSPArea *AddArea();
	BSPAreaportal *AddAreaportal();
	BSPSector *AddSector();
	BSPClipSurface *AddClipSurface();
	BSPModel *AddModel();
	BSPPlane *AddPlane();
	BSPVertex *AddVertex();
	BSPActor *AddActor();
	U16 *AddAreaportalIndex();
	U16 *AddSectorIndex();
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

	int Write(stream::OutputStream &os);

protected:
	
	virtual RAD_DECLARE_GET(numEntities, U32);
	virtual RAD_DECLARE_GET(numStrings, U32);
	virtual RAD_DECLARE_GET(numMaterials, U32);
	virtual RAD_DECLARE_GET(numNodes, U32);
	virtual RAD_DECLARE_GET(numLeafs, U32);
	virtual RAD_DECLARE_GET(numAreas, U32);
	virtual RAD_DECLARE_GET(numAreaportals, U32);
	virtual RAD_DECLARE_GET(numSectors, U32);
	virtual RAD_DECLARE_GET(numModels, U32);
	virtual RAD_DECLARE_GET(numClipSurfaces, U32);
	virtual RAD_DECLARE_GET(numVerts, U32);
	virtual RAD_DECLARE_GET(numAreaportalIndices, U32);
	virtual RAD_DECLARE_GET(numSectorIndices, U32);
	virtual RAD_DECLARE_GET(numIndices, U32);
	virtual RAD_DECLARE_GET(numPlanes, U32);
	virtual RAD_DECLARE_GET(numCameraTMs, U32);
	virtual RAD_DECLARE_GET(numCameraTracks, U32);
	virtual RAD_DECLARE_GET(numCinematicTriggers, U32);
	virtual RAD_DECLARE_GET(numCinematics, U32);
	virtual RAD_DECLARE_GET(numSkas, U32);
	virtual RAD_DECLARE_GET(numActors, U32);
	virtual RAD_DECLARE_GET(numActorIndices, U32);

	typedef zone_vector< ::String , ZBSPBuilderT>::type StringVec;
	typedef zone_vector<BSPEntity, ZBSPBuilderT>::type BSPEntityVec;
	typedef zone_vector<BSPMaterial, ZBSPBuilderT>::type BSPMaterialVec;
	typedef zone_vector<BSPNode, ZBSPBuilderT>::type BSPNodeVec;
	typedef zone_vector<BSPLeaf, ZBSPBuilderT>::type BSPLeafVec;
	typedef zone_vector<BSPArea, ZBSPBuilderT>::type BSPAreaVec;
	typedef zone_vector<BSPAreaportal, ZBSPBuilderT>::type BSPAreaportalVec;
	typedef zone_vector<BSPSector, ZBSPBuilderT>::type BSPSectorVec;
	typedef zone_vector<BSPModel, ZBSPBuilderT>::type BSPModelVec;
	typedef zone_vector<BSPClipSurface, ZBSPBuilderT>::type BSPClipSurfaceVec;
	typedef zone_vector<BSPPlane, ZBSPBuilderT>::type BSPPlaneVec;
	typedef zone_vector<BSPVertex, ZBSPBuilderT>::type BSPVertexVec;
	typedef zone_vector<U16, ZBSPBuilderT>::type BSPIndexVec;
	typedef zone_vector<U32, ZBSPBuilderT>::type BSPActorIndexVec;
	typedef zone_vector<BSPCameraTM, ZBSPBuilderT>::type BSPCameraTMVec;
	typedef zone_vector<BSPCameraTrack, ZBSPBuilderT>::type BSPCameraTrackVec;
	typedef zone_vector<BSPCinematicTrigger, ZBSPBuilderT>::type BSPCinematicTriggerVec;
	typedef zone_vector<BSPCinematic, ZBSPBuilderT>::type BSPCinematicVec;
	typedef zone_vector<BSPActor, ZBSPBuilderT>::type BSPActorVec;
	typedef zone_vector<tools::SkaData::Ref, ZBSPBuilderT>::type SkaVec;
	typedef zone_vector<tools::SkmData::Ref, ZBSPBuilderT>::type SkmVec;

	StringVec m_strings;
	BSPMaterialVec m_mats;
	BSPEntityVec m_ents;
	BSPNodeVec m_nodes;
	BSPLeafVec m_leafs;
	BSPAreaVec m_areas;
	BSPAreaportalVec m_areaportals;
	BSPSectorVec m_sectors;
	BSPModelVec m_models;
	BSPClipSurfaceVec m_clipSurfaces;
	BSPPlaneVec m_planes;
	BSPVertexVec m_vertices;
	BSPIndexVec m_areaportalIndices;
	BSPIndexVec m_sectorIndices;
	BSPIndexVec m_indices;
	BSPActorIndexVec m_actorIndices;
	BSPCameraTMVec m_cameraTMs;
	BSPCameraTrackVec m_cameraTracks;
	BSPCinematicTriggerVec m_cinematicTriggers;
	BSPCinematicVec m_cinematics;
	BSPActorVec m_actors;
	SkaVec m_skas;
	SkmVec m_skms;
};

#endif

} // bsp_file
} // world

#include <Runtime/PopPack.h>

#include "BSPFile.inl"
