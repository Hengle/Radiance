// BSPFile.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "BSPFile.h"
#include "../COut.h"
#include "../Packages/PackagesDef.h"
#include <Runtime/Base/SIMD.h>
#include <Runtime/Stream.h>

namespace world {
namespace bsp_file {

enum  { 
	kBspTag = RAD_FOURCC('b', 's', 'p', 't'),
	kBspVersion  = 0x3
};

RAD_ZONE_DEF(RADENG_API, ZBSPFile, "BSPFile", ZWorld);

BOOST_STATIC_ASSERT(sizeof(BSPVertex)==72);
BOOST_STATIC_ASSERT(sizeof(BSPCameraTM)==9*4);

BSPFileParser::BSPFileParser() :
m_strings(0),
m_ents(0),
m_mats(0),
m_nodes(0),
m_leafs(0),
m_areas(0),
m_areaportals(0),
m_models(0),
m_clipSurfaces(0),
m_planes(0),
m_verts(0),
m_waypoints(0),
m_waypointConnections(0),
m_waypointIndices(0),
m_floors(0),
m_floorTris(0),
m_floorEdges(0),
m_areaportalIndices(0),
m_indices(0),
m_actorIndices(0),
m_actors(0),
m_cameraTMs(0),
m_cameraTracks(0),
m_cinematicTriggers(0),
m_cinematics(0),
m_skas(0),
m_skms(0),
m_numStrings(0),
m_numEnts(0),
m_numMats(0),
m_numNodes(0),
m_numLeafs(0),
m_numModels(0),
m_numPlanes(0),
m_numVerts(0),
m_numWaypoints(0),
m_numWaypointConnections(0),
m_numFloors(0),
m_numFloorTris(0),
m_numFloorEdges(0),
m_numAreaportalIndices(0),
m_numWaypointIndices(0),
m_numModelIndices(0),
m_numIndices(0),
m_numActorIndices(0),
m_numActors(0),
m_numCameraTMs(0),
m_numCameraTracks(0),
m_numCinematicTriggers(0),
m_numCinematics(0) {
}

BSPFileParser::~BSPFileParser() {
	if (m_skas)
		delete [] m_skas;
	if (m_skms)
		delete [] m_skms;
}

#define CHECK_SIZE(_size) if (((bytes+(_size))-reinterpret_cast<const U8*>(data)) > (int)len) return pkg::SR_CorruptFile

int BSPFileParser::Parse(const void *data, AddrSize len) {
	// Read header
	const U8 *bytes = reinterpret_cast<const U8*>(data);
	CHECK_SIZE(sizeof(U32)*30);
	U32 tag = *reinterpret_cast<const U32*>(bytes);
	U32 version  = *reinterpret_cast<const U32*>(bytes+sizeof(U32));
	if (tag != kBspTag || version != kBspVersion)
		return pkg::SR_InvalidFormat;

	bytes += sizeof(U32)*2;

	U32 numChannels = *reinterpret_cast<const U32*>(bytes);
	if (numChannels > kMaxUVChannels)
		return pkg::SR_InvalidFormat;
	bytes += sizeof(U32);

	m_numStrings = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numMats = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numEnts = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numNodes = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numLeafs = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numAreas = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numAreaportals = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numModels = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numClipSurfaces = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);

	m_numWaypoints = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numWaypointConnections = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numFloors = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numFloorTris = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numFloorEdges = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numPlanes = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numVerts = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numAreaportalIndices = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numModelIndices = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);

	m_numWaypointIndices = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numIndices = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numActorIndices = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numActors  = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numCameraTMs = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numCameraTracks = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numCinematicTriggers = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numCinematics = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);
	
	m_numSkas = *reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32);

	// Setup pointers into data.
	CHECK_SIZE(sizeof(BSPMaterial)*m_numMats);
	m_mats = reinterpret_cast<const BSPMaterial*>(bytes);
	bytes += sizeof(BSPMaterial)*m_numMats;
	
	CHECK_SIZE(sizeof(BSPEntity)*m_numEnts);
	m_ents = reinterpret_cast<const BSPEntity*>(bytes);
	bytes += sizeof(BSPEntity)*m_numEnts;
	
	CHECK_SIZE(sizeof(BSPNode)*m_numNodes);
	m_nodes = reinterpret_cast<const BSPNode*>(bytes);
	bytes += sizeof(BSPNode)*m_numNodes;
	
	CHECK_SIZE(sizeof(BSPLeaf)*m_numLeafs);
	m_leafs = reinterpret_cast<const BSPLeaf*>(bytes);
	bytes += sizeof(BSPLeaf)*m_numLeafs;
	
	CHECK_SIZE(sizeof(BSPArea)*m_numAreas);
	m_areas = reinterpret_cast<const BSPArea*>(bytes);
	bytes += sizeof(BSPArea)*m_numAreas;
	
	CHECK_SIZE(sizeof(BSPAreaportal)*m_numAreaportals);
	m_areaportals = reinterpret_cast<const BSPAreaportal*>(bytes);
	bytes += sizeof(BSPAreaportal)*m_numAreaportals;
	
	CHECK_SIZE(sizeof(BSPModel)*m_numModels);
	m_models = reinterpret_cast<const BSPModel*>(bytes);
	bytes += sizeof(BSPModel)*m_numModels;
	
	CHECK_SIZE(sizeof(BSPClipSurface)*m_numClipSurfaces);
	m_clipSurfaces = reinterpret_cast<const BSPClipSurface*>(bytes);
	bytes += sizeof(BSPClipSurface)*m_numClipSurfaces;
	
	CHECK_SIZE(sizeof(BSPWaypoint)*m_numWaypoints);
	m_waypoints = reinterpret_cast<const BSPWaypoint*>(bytes);
	bytes += sizeof(BSPWaypoint)*m_numWaypoints;
	
	CHECK_SIZE(sizeof(BSPWaypointConnection)*m_numWaypointConnections);
	m_waypointConnections = reinterpret_cast<const BSPWaypointConnection*>(bytes);
	bytes += sizeof(BSPWaypointConnection)*m_numWaypointConnections;
	
	CHECK_SIZE(sizeof(BSPFloor)*m_numFloors);
	m_floors = reinterpret_cast<const BSPFloor*>(bytes);
	bytes += sizeof(BSPFloor)*m_numFloors;
	
	CHECK_SIZE(sizeof(BSPFloorTri)*m_numFloorTris);
	m_floorTris = reinterpret_cast<const BSPFloorTri*>(bytes);
	bytes += sizeof(BSPFloorTri)*m_numFloorTris;
	
	CHECK_SIZE(sizeof(BSPFloorEdge)*m_numFloorEdges);
	m_floorEdges = reinterpret_cast<const BSPFloorEdge*>(bytes);
	bytes += sizeof(BSPFloorEdge)*m_numFloorEdges;
	
	CHECK_SIZE(sizeof(BSPPlane)*m_numPlanes);
	m_planes = reinterpret_cast<const BSPPlane*>(bytes);
	bytes += sizeof(BSPPlane)*m_numPlanes;
	
	CHECK_SIZE(sizeof(BSPVertex)*m_numVerts);
	m_verts = reinterpret_cast<const BSPVertex*>(bytes);
	bytes += sizeof(BSPVertex)*m_numVerts;
	
	CHECK_SIZE(sizeof(U16)*m_numAreaportalIndices);
	m_areaportalIndices = reinterpret_cast<const U16*>(bytes);
	bytes += sizeof(U16)*m_numAreaportalIndices;
	
	CHECK_SIZE(sizeof(U16)*m_numModelIndices);
	m_modelIndices = reinterpret_cast<const U16*>(bytes);
	bytes += sizeof(U16)*m_numModelIndices;

	CHECK_SIZE(sizeof(U16)*m_numWaypointIndices);
	m_waypointIndices = reinterpret_cast<const U16*>(bytes);
	bytes += sizeof(U16)*m_numWaypointIndices;
	
	CHECK_SIZE(sizeof(U16)*m_numIndices);
	m_indices = reinterpret_cast<const U16*>(bytes);
	bytes += sizeof(U16)*m_numIndices;

	if ((m_numAreaportalIndices+m_numModelIndices+m_numWaypointIndices+m_numIndices)&1) // align?
		bytes += sizeof(U16);
	
	CHECK_SIZE(sizeof(U32)*m_numActorIndices);
	m_actorIndices = reinterpret_cast<const U32*>(bytes);
	bytes += sizeof(U32)*m_numActorIndices;

	CHECK_SIZE(sizeof(BSPActor)*m_numActors);
	m_actors = reinterpret_cast<const BSPActor*>(bytes);
	bytes += sizeof(BSPActor)*m_numActors;
	
	CHECK_SIZE(sizeof(BSPCameraTM)*m_numCameraTMs);
	m_cameraTMs = reinterpret_cast<const BSPCameraTM*>(bytes);
	bytes += sizeof(BSPCameraTM)*m_numCameraTMs;
	
	CHECK_SIZE(sizeof(BSPCameraTrack)*m_numCameraTracks);
	m_cameraTracks = reinterpret_cast<const BSPCameraTrack*>(bytes);
	bytes += sizeof(BSPCameraTrack)*m_numCameraTracks;
	
	CHECK_SIZE(sizeof(BSPCinematicTrigger)*m_numCinematicTriggers);
	m_cinematicTriggers = reinterpret_cast<const BSPCinematicTrigger*>(bytes);
	bytes += sizeof(BSPCinematicTrigger)*m_numCinematicTriggers;
	
	CHECK_SIZE(sizeof(BSPCinematic)*m_numCinematics);
	m_cinematics = reinterpret_cast<const BSPCinematic*>(bytes);
	bytes += sizeof(BSPCinematic)*m_numCinematics;

	m_skas = new (ZBSPFile) ska::DSka[m_numSkas];
	m_skms = new (ZBSPFile) ska::DSkm[m_numSkas];

	for (U32 i = 0; i < m_numSkas; ++i) {
		AddrSize sizes[3];
		const void *ptr[2];

		CHECK_SIZE(sizeof(U32)*3);
		sizes[0] = (AddrSize)*reinterpret_cast<const U32*>(bytes);
		bytes += sizeof(U32);
		sizes[1] = (AddrSize)*reinterpret_cast<const U32*>(bytes);
		bytes += sizeof(U32);
		sizes[2] = (AddrSize)*reinterpret_cast<const U32*>(bytes);
		bytes += sizeof(U32);

		CHECK_SIZE(sizes[0]);
		int r = m_skas[i].Parse(bytes, sizes[0]);
		if (r != pkg::SR_Success)
			return r;
		bytes += sizes[0];
		CHECK_SIZE(sizes[1]);
		ptr[0] = bytes;
		bytes += sizes[1];

		bytes = Align(bytes, SIMDDriver::Alignment);

		CHECK_SIZE(sizes[2]);
		ptr[1] = bytes;
		bytes += sizes[2];

		r = m_skms[i].Parse(ptr, &sizes[1], ska::SkinCpu);
		if (r != pkg::SR_Success)
			return r;
	}

	m_strings = reinterpret_cast<const char*>(bytes);

	// string table
	int ofs = 0;
	m_stringOfs.reserve(m_numStrings);

	for (U32 i = 0; i < m_numStrings; ++i) {
		CHECK_SIZE(sizeof(U16));
		U16 strLen = *reinterpret_cast<const U16*>(bytes);
		CHECK_SIZE(sizeof(U16)+strLen);
		m_stringOfs.push_back(ofs+sizeof(U16));
		ofs += strLen+(int)sizeof(U16);
		bytes += strLen+sizeof(U16);
	}

	return pkg::SR_Success;
}


#if defined(RAD_OPT_TOOLS)

RAD_ZONE_DEF(RADENG_API, ZBSPBuilder, "BSPBuilder", ZTools);

// NOTE: make support endianess someday

int BSPFileBuilder::Write(stream::OutputStream &os) {
	// header
	os << (U32)kBspTag << (U32) kBspVersion;
	os << (U32)kMaxUVChannels;
	os << (U32)m_strings.size();
	os << (U32)m_mats.size();
	os << (U32)m_ents.size();
	os << (U32)m_nodes.size();
	os << (U32)m_leafs.size();
	os << (U32)m_areas.size();
	os << (U32)m_areaportals.size();
	os << (U32)m_models.size();
	os << (U32)m_clipSurfaces.size();
	os << (U32)m_waypoints.size();
	os << (U32)m_waypointConnections.size();
	os << (U32)m_floors.size();
	os << (U32)m_floorTris.size();
	os << (U32)m_floorEdges.size();
	os << (U32)m_planes.size();
	os << (U32)m_vertices.size();
	os << (U32)m_areaportalIndices.size();
	os << (U32)m_modelIndices.size();
	os << (U32)m_waypointIndices.size();
	os << (U32)m_indices.size();
	os << (U32)m_actorIndices.size();
	os << (U32)m_actors.size();
	os << (U32)m_cameraTMs.size();
	os << (U32)m_cameraTracks.size();
	os << (U32)m_cinematicTriggers.size();
	os << (U32)m_cinematics.size();
	os << (U32)m_skas.size();

	// write!
	stream::SPos len = (stream::SPos)(sizeof(BSPMaterial)*m_mats.size());
	if (len && os.Write(&m_mats[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPEntity)*m_ents.size());
	if (len && os.Write(&m_ents[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPNode)*m_nodes.size());
	if (len && os.Write(&m_nodes[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPLeaf)*m_leafs.size());
	if (len && os.Write(&m_leafs[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPArea)*m_areas.size());
	if (len && os.Write(&m_areas[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPAreaportal)*m_areaportals.size());
	if (len && os.Write(&m_areaportals[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPModel)*m_models.size());
	if (len && os.Write(&m_models[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPClipSurface)*m_clipSurfaces.size());
	if (len && os.Write(&m_clipSurfaces[0], len, 0) != len)
		return pkg::SR_IOError;

	len = (stream::SPos)(sizeof(BSPWaypoint)*m_waypoints.size());
	if (len && os.Write(&m_waypoints[0], len, 0) != len)
		return pkg::SR_IOError;

	len = (stream::SPos)(sizeof(BSPWaypointConnection)*m_waypointConnections.size());
	if (len && os.Write(&m_waypointConnections[0], len, 0) != len)
		return pkg::SR_IOError;

	len = (stream::SPos)(sizeof(BSPFloor)*m_floors.size());
	if (len && os.Write(&m_floors[0], len, 0) != len)
		return pkg::SR_IOError;

	len = (stream::SPos)(sizeof(BSPFloorTri)*m_floorTris.size());
	if (len && os.Write(&m_floorTris[0], len, 0) != len)
		return pkg::SR_IOError;

	len = (stream::SPos)(sizeof(BSPFloorEdge)*m_floorEdges.size());
	if (len && os.Write(&m_floorEdges[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPPlane)*m_planes.size());
	if (len && os.Write(&m_planes[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPVertex)*m_vertices.size());
	if (len && os.Write(&m_vertices[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(U16)*m_areaportalIndices.size());
	if (len && os.Write(&m_areaportalIndices[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(U16)*m_modelIndices.size());
	if (len && os.Write(&m_modelIndices[0], len, 0) != len)
		return pkg::SR_IOError;

	len = (stream::SPos)(sizeof(U16)*m_waypointIndices.size());
	if (len && os.Write(&m_waypointIndices[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(U16)*m_indices.size());
	if (len && os.Write(&m_indices[0], len, 0) != len)
		return pkg::SR_IOError;

	if ((m_areaportalIndices.size()+m_modelIndices.size()+m_waypointIndices.size()+m_indices.size())&1) {
		if (!os.Write((U16)0)) // padd
			return pkg::SR_IOError;
	}
	
	len = (stream::SPos)(sizeof(U32)*m_actorIndices.size());
	if (len && os.Write(&m_actorIndices[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPActor)*m_actors.size());
	if (len && os.Write(&m_actors[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPCameraTM)*m_cameraTMs.size());
	if (len && os.Write(&m_cameraTMs[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPCameraTrack)*m_cameraTracks.size());
	if (len && os.Write(&m_cameraTracks[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPCinematicTrigger)*m_cinematicTriggers.size());
	if (len && os.Write(&m_cinematicTriggers[0], len, 0) != len)
		return pkg::SR_IOError;
	
	len = (stream::SPos)(sizeof(BSPCinematic)*m_cinematics.size());
	if (len && os.Write(&m_cinematics[0], len, 0) != len)
		return pkg::SR_IOError;

	// skas
	for (size_t i = 0; i < m_skas.size(); ++i) {
		const tools::SkaData::Ref &skad = m_skas[i];
		const tools::SkmData::Ref &skmd = m_skms[i];

		if (!os.Write((U32)skad->skaSize))
			return pkg::SR_IOError;
		if (!os.Write((U32)skmd->skmSize[0]))
			return pkg::SR_IOError;
		if (!os.Write((U32)skmd->skmSize[1]))
			return pkg::SR_IOError;

		if (os.Write(skad->skaData, (stream::SPos)skad->skaSize, 0) != (stream::SPos)skad->skaSize)
			return pkg::SR_IOError;
		if (os.Write(skmd->skmData[0], (stream::SPos)skmd->skmSize[0], 0) != (stream::SPos)skmd->skmSize[0])
			return pkg::SR_IOError;

		len = os.OutPos();
		if (!IsAligned(len, SIMDDriver::Alignment)) {
			U8 padd[SIMDDriver::Alignment-1];
			len = (SIMDDriver::Alignment) - (len & (SIMDDriver::Alignment-1));
			if (os.Write(padd, len, 0) != len)
				return false;
		}

		if (os.Write(skmd->skmData[1], (stream::SPos)skmd->skmSize[1], 0) != (stream::SPos)skmd->skmSize[1])
			return pkg::SR_IOError;
	}

	// string table.
	for (size_t i = 0; i < m_strings.size(); ++i) {
		if (!os.Write((U16)(m_strings[i].length+1)))
			return pkg::SR_IOError;
		if (os.Write(m_strings[i].c_str.get(), (stream::SPos)(m_strings[i].length+1), 0) != (stream::SPos)(m_strings[i].length+1))
			return pkg::SR_IOError;
	}

	SizeBuffer memSize;
	FormatSize(memSize, os.OutPos());

	COut(C_Info) << "BSPFileBuilder(" << memSize << ")" << std::endl;
	return pkg::SR_Success;
}

#endif

} // bsp_file
} // world
