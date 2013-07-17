// VtMesh.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "VtMesh.h"
#include "Common.h"
#include <limits>
#undef max

using namespace pkg;
using namespace asset;

namespace r {

VtMesh::VtMesh() : m_parser(0) {
}

VtMesh::~VtMesh() {
}

VtMesh::Ref VtMesh::New(const pkg::AssetRef &asset) {
	VtModelParser *parser = VtModelParser::Cast(asset);
	if (!parser)
		return Ref();

	VtMesh::Ref vt = New(*parser->dvtm.get());
	vt->m_asset = asset;
	vt->m_parser = parser;
	
	return vt;
}

VtMesh::Ref VtMesh::New(const ska::DVtm &dvtm) {
	if (dvtm.meshes.size() == 0)
		return Ref();

	ska::Vtm::Ref vtm(new (ska::ZSka) ska::Vtm(dvtm));
	r::VtMesh::Ref vt(new (ska::ZSka) r::VtMesh());

	vt->Load(vtm, dvtm);

	return vt;
}

void VtMesh::Load(
	const ska::Vtm::Ref &vtm,
	const ska::DVtm &dvtm
) {
	BOOST_STATIC_ASSERT(sizeof(Vec3)==12);

	m_vtm = vtm;
	m_meshes.clear();
	m_meshes.resize(dvtm.meshes.size());

	const int kVertSize = ska::DVtm::kNumVertexFloats*sizeof(float);

	for (size_t i = 0; i < m_meshes.size(); ++i) {
		DefMesh &m = m_meshes[i];
		const ska::DVtMesh &dm = dvtm.meshes[i];

		m.vertexFrame = -1;
		m.dm = &dm;

		if (kSkinFrames > 1)
			m.m.AllocateSwapChains(kSkinFrames);

		m.vertStreamIdx = m.m.AllocateStream(kStreamUsage_Stream, kVertSize, dm.numVerts, true);
		
		m.m.MapSource(
			m.vertStreamIdx,
			kMaterialGeometrySource_Vertices,
			0,
			kVertSize,
			0,
			3
		);

		m.m.MapSource(
			m.vertStreamIdx,
			kMaterialGeometrySource_Normals,
			0,
			kVertSize,
			sizeof(float) * 4,
			3
		);

		if (dm.numChannels > 0) {
			m.m.MapSource(
				m.vertStreamIdx,
				kMaterialGeometrySource_Tangents,
				0,
				kVertSize,
				sizeof(float) * 8,
				3
			);
		}

		int uvStreamIndex = -1;

		if (dm.numChannels > 1) {
			uvStreamIndex = m.m.AllocateStream(kStreamUsage_Static, sizeof(float)*4, dm.numVerts);
		
			m.m.MapSource(
				uvStreamIndex,
				kMaterialGeometrySource_TexCoords,
				0,
				sizeof(float) * 4,
				0,
				2
			);

			m.m.MapSource(
				uvStreamIndex,
				kMaterialGeometrySource_TexCoords,
				1,
				sizeof(float) * 4,
				sizeof(float) * 2,
				2
			);

		} else if (dm.numChannels > 0) {

			uvStreamIndex = m.m.AllocateStream(kStreamUsage_Static, sizeof(float)*2, dm.numVerts);
		
			m.m.MapSource(
				uvStreamIndex,
				kMaterialGeometrySource_TexCoords,
				0,
				0,
				0,
				2
			);
		}

		if (dm.numChannels > 0) {
			Mesh::StreamPtr::Ref vb = m.m.Map(uvStreamIndex);
			memcpy(vb->ptr, dm.texCoords, vb->size.get());
			vb.reset();
		}

		Mesh::StreamPtr::Ref vb = m.m.MapIndices(kStreamUsage_Static, sizeof(U16), (int)dm.numTris * 3);
		memcpy(vb->ptr, dm.indices, vb->size.get());
		vb.reset();
	}
}

void VtMesh::Skin(int mesh) {
	DefMesh &m = m_meshes[mesh];
	
	if (m.vertexFrame == m_vtm->vertexFrame)
		return;
	m.vertexFrame = m_vtm->vertexFrame;

	m.m.SwapChain();
	Mesh::StreamPtr::Ref vb = m.m.Map(m.vertStreamIdx);

	SkinToBuffer(*SIMD, mesh, vb->ptr);

	vb.reset();
}

void VtMesh::SkinToBuffer(const SIMDDriver &driver, int mesh, void *buffer) {
	DefMesh &m = m_meshes[mesh];
	m_vtm->BlendVerts(driver, (float*)buffer, (int)m.dm->vertOfs, (int)m.dm->numVerts);
}


} // r
