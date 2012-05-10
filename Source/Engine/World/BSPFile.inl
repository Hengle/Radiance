// BSPFile.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace world {
namespace bsp_file {

inline const char *BSPFileParser::String(U32 idx) const
{
	RAD_ASSERT(idx < m_numStrings);
	return m_strings + m_stringOfs[idx];
}

inline const BSPEntity *BSPFileParser::Entities() const
{
	return m_ents;
}

inline const BSPMaterial *BSPFileParser::Materials() const
{
	return m_mats;
}

inline const BSPNode *BSPFileParser::Nodes() const
{
	return m_nodes;
}

inline const BSPLeaf *BSPFileParser::Leafs() const
{
	return m_leafs;
}

inline const BSPModel *BSPFileParser::Models() const
{
	return m_models;
}

inline const BSPPlane *BSPFileParser::Planes() const
{
	return m_planes;
}

inline const BSPVertex *BSPFileParser::Vertices() const
{
	return m_verts;
}

inline const BSPActor *BSPFileParser::Actors() const
{
	return m_actors;
}

inline const U16 *BSPFileParser::Indices() const
{
	return m_indices;
}

inline const U32 *BSPFileParser::ActorIndices() const
{
	return m_actorIndices;
}

inline const BSPCameraTM *BSPFileParser::CameraTMs() const
{
	return m_cameraTMs;
}

inline const BSPCameraTrack *BSPFileParser::CameraTracks() const
{
	return m_cameraTracks;
}

inline const BSPCinematicTrigger *BSPFileParser::CinematicTriggers() const
{
	return m_cinematicTriggers;
}

inline const BSPCinematic *BSPFileParser::Cinematics() const
{
	return m_cinematics;
}

inline const ska::DSka &BSPFileParser::DSka(int idx) const
{
	RAD_ASSERT(idx < (int)m_numSkas);
	return m_skas[idx];
}

inline const ska::DSkm &BSPFileParser::DSkm(int idx) const
{
	RAD_ASSERT(idx < (int)m_numSkas);
	return m_skms[idx];
}

inline U32 BSPFileParser::NumTexCoords(int channel) const
{
	RAD_ASSERT(channel < MaxUVChannels);
	return m_numTexCoords[channel];
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numEntities)
{
	return m_numEnts;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numStrings)
{
	return m_numStrings;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numMaterials)
{
	return m_numMats;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numNodes)
{
	return m_numNodes;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numLeafs)
{
	return m_numLeafs;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numModels)
{
	return m_numModels;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numVerts)
{
	return m_numVerts;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numIndices)
{
	return m_numIndices;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numActorIndices)
{
	return m_numActorIndices;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numActors)
{
	return m_numActors;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numPlanes)
{
	return m_numPlanes;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numCameraTMs)
{
	return m_numCameraTMs;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numCameraTracks)
{
	return m_numCameraTracks;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numCinematicTriggers)
{
	return m_numCinematicTriggers;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numCinematics)
{
	return m_numCinematics;
}

inline U32 BSPFileParser::RAD_IMPLEMENT_GET(numSkas)
{
	return m_numSkas;
}

#if defined(RAD_OPT_TOOLS)

inline const char *BSPFileBuilder::String(U32 idx) const
{
	return m_strings[idx].c_str();
}

inline const BSPEntity *BSPFileBuilder::Entities() const
{
	return &m_ents[0];
}

inline const BSPMaterial *BSPFileBuilder::Materials() const
{
	return &m_mats[0];
}

inline const BSPNode *BSPFileBuilder::Nodes() const
{
	return &m_nodes[0];
}

inline const BSPLeaf *BSPFileBuilder::Leafs() const
{
	return &m_leafs[0];
}

inline const BSPModel *BSPFileBuilder::Models() const
{
	return &m_models[0];
}

inline const BSPPlane *BSPFileBuilder::Planes() const
{
	return &m_planes[0];
}

inline const BSPVertex *BSPFileBuilder::Vertices() const
{
	return &m_vertices[0];
}

inline const U16 *BSPFileBuilder::Indices() const
{
	return &m_indices[0];
}

inline const BSPCameraTM *BSPFileBuilder::CameraTMs() const
{
	return &m_cameraTMs[0];
}

inline const BSPCameraTrack *BSPFileBuilder::CameraTracks() const
{
	return &m_cameraTracks[0];
}

inline const BSPCinematicTrigger *BSPFileBuilder::CinematicTriggers() const
{
	return &m_cinematicTriggers[0];
}

inline const BSPCinematic *BSPFileBuilder::Cinematics() const
{
	return &m_cinematics[0];
}

inline const ska::DSka &BSPFileBuilder::DSka(int idx) const
{
	return m_skas[idx]->dska;
}

inline const ska::DSkm &BSPFileBuilder::DSkm(int idx) const
{
	return m_skms[idx]->dskm;
}

inline const BSPActor *BSPFileBuilder::Actors() const
{
	return &m_actors[0];
}

inline const U32 *BSPFileBuilder::ActorIndices() const
{
	return &m_actorIndices[0];
}

inline void BSPFileBuilder::Clear()
{
	m_strings.clear();
	m_mats.clear();
	m_ents.clear();
	m_nodes.clear();
	m_leafs.clear();
	m_models.clear();
	m_planes.clear();
	m_vertices.clear();
	m_indices.clear();
	m_actorIndices.clear();
	m_cameraTMs.clear();
	m_cameraTracks.clear();
	m_cinematicTriggers.clear();
	m_cinematics.clear();
	m_skas.clear();
	m_skms.clear();
	m_actors.clear();
}

inline void BSPFileBuilder::ReserveStrings(int num)
{
	m_strings.reserve(m_strings.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveEntities(int num)
{
	m_ents.reserve(m_ents.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveMaterials(int num)
{
	m_mats.reserve(m_mats.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveNodes(int num)
{
	m_nodes.reserve(m_nodes.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveLeafs(int num)
{
	m_leafs.reserve(m_leafs.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveModels(int num)
{
	m_models.reserve(m_models.size()+(size_t)num);
}

inline void BSPFileBuilder::ReservePlanes(int num)
{
	m_planes.reserve(m_planes.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveVertices(int num)
{
	m_vertices.reserve(m_vertices.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveIndices(int num)
{
	m_indices.reserve(m_indices.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveCameraTMs(int num)
{
	m_cameraTMs.reserve(m_cameraTMs.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveCameraTracks(int num)
{
	m_cameraTracks.reserve(m_cameraTracks.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveCinematicTriggers(int num)
{
	m_cinematicTriggers.reserve(m_cinematicTriggers.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveCinematics(int num)
{
	m_cinematics.reserve(m_cinematics.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveSkas(int num)
{
	m_skas.reserve(m_skas.size()+(size_t)num);
	m_skms.reserve(m_skms.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveActors(int num)
{
	m_actors.reserve(m_actors.size()+(size_t)num);
}

inline void BSPFileBuilder::ReserveActorIndices(int num)
{
	m_actorIndices.reserve(m_actorIndices.size()+(size_t)num);
}

inline String *BSPFileBuilder::AddString()
{
	m_strings.resize(m_strings.size()+1);
	return &m_strings.back();
}

inline BSPEntity *BSPFileBuilder::AddEntity()
{
	m_ents.resize(m_ents.size()+1);
	return &m_ents.back();
}

inline BSPMaterial *BSPFileBuilder::AddMaterial()
{
	m_mats.resize(m_mats.size()+1);
	return &m_mats.back();
}

inline BSPNode *BSPFileBuilder::AddNode()
{
	m_nodes.resize(m_nodes.size()+1);
	return &m_nodes.back();
}

inline BSPLeaf *BSPFileBuilder::AddLeaf()
{
	m_leafs.resize(m_leafs.size()+1);
	return &m_leafs.back();
}

inline BSPModel *BSPFileBuilder::AddModel()
{
	m_models.resize(m_models.size()+1);
	return &m_models.back();
}

inline BSPPlane *BSPFileBuilder::AddPlane()
{
	m_planes.resize(m_planes.size()+1);
	return &m_planes.back();
}

inline BSPVertex *BSPFileBuilder::AddVertex()
{
	m_vertices.resize(m_vertices.size()+1);
	return &m_vertices.back();
}

inline BSPActor *BSPFileBuilder::AddActor()
{
	m_actors.resize(m_actors.size()+1);
	return &m_actors.back();
}

inline U16 *BSPFileBuilder::AddIndex()
{
	m_indices.resize(m_indices.size()+1);
	return &m_indices.back();
}

inline BSPCameraTM *BSPFileBuilder::AddCameraTM()
{
	m_cameraTMs.resize(m_cameraTMs.size()+1);
	return &m_cameraTMs.back();
}

inline BSPCameraTrack *BSPFileBuilder::AddCameraTrack()
{
	m_cameraTracks.resize(m_cameraTracks.size()+1);
	return &m_cameraTracks.back();
}

inline BSPCinematicTrigger *BSPFileBuilder::AddCinematicTrigger()
{
	m_cinematicTriggers.resize(m_cinematicTriggers.size()+1);
	return &m_cinematicTriggers.back();
}

inline BSPCinematic *BSPFileBuilder::AddCinematic()
{
	m_cinematics.resize(m_cinematics.size()+1);
	return &m_cinematics.back();
}

inline int BSPFileBuilder::AddSka(
	const tools::SkaData::Ref &skaRef,
	const tools::SkmData::Ref &skmRef
)
{
	m_skas.push_back(skaRef);
	m_skms.push_back(skmRef);
	return (int)(m_skas.size()-1);
}

inline U32 *BSPFileBuilder::AddActorIndex()
{
	m_actorIndices.resize(m_actorIndices.size()+1);
	return &m_actorIndices.back();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numEntities)
{
	return (U32)m_ents.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numStrings)
{
	return (U32)m_strings.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numMaterials)
{
	return (U32)m_mats.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numNodes)
{
	return (U32)m_nodes.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numLeafs)
{
	return (U32)m_leafs.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numModels)
{
	return (U32)m_models.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numVerts)
{
	return (U32)m_vertices.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numIndices)
{
	return (U32)m_indices.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numPlanes)
{
	return (U32)m_planes.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numCameraTMs)
{
	return (U32)m_cameraTMs.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numCameraTracks)
{
	return (U32)m_cameraTracks.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numCinematicTriggers)
{
	return (U32)m_cinematicTriggers.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numCinematics)
{
	return (U32)m_cinematics.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numSkas)
{
	return (U32)m_skas.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numActors)
{
	return (U32)m_actors.size();
}

inline U32 BSPFileBuilder::RAD_IMPLEMENT_GET(numActorIndices)
{
	return (U32)m_actorIndices.size();
}

#endif

} // bsp_file
} // world