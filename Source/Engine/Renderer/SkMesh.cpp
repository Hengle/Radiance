// SkMesh.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SkMesh.h"
#include "Common.h"
#include <limits>
#undef max

using namespace pkg;
using namespace asset;

namespace r {

SkMesh::SkMesh() : m_parser(0) {
}

SkMesh::~SkMesh() {
}

SkMesh::Ref SkMesh::New(const pkg::AssetRef &asset) {
	SkModelParser *parser = SkModelParser::Cast(asset);
	if (!parser)
		return Ref();

	SkMesh::Ref sk = New(*parser->dska.get(), *parser->dskm.get(), parser->skinType);
	sk->m_asset = asset;
	sk->m_parser = parser;
	
	return sk;
}

SkMesh::Ref SkMesh::New(const ska::DSka &dska, const ska::DSkm &dskm, ska::SkinType skinType) {
	if (dskm.meshes.size() == 0)
		return Ref();

	if (skinType != ska::kSkinType_CPU)
		return Ref(); // for now.

	ska::Ska::Ref ska(new (ska::ZSka) ska::Ska(dska));
	r::SkMesh::Ref sk(new (ska::ZSka) r::SkMesh());

	sk->Load(ska, dskm, skinType);

	return sk;
}

void SkMesh::Load(
	const ska::Ska::Ref &skanim,
	const ska::DSkm &dskm,
	ska::SkinType type
) {
	BOOST_STATIC_ASSERT(sizeof(Vec3)==12);

	m_type = type;
	m_ska = skanim;
	m_meshes.clear();
	m_meshes.resize(dskm.meshes.size());

	const int kVertSize = ska::DSkMesh::kNumVertexFloats*sizeof(float);

	for (size_t i = 0; i < m_meshes.size(); ++i) {
		DefMesh &m = m_meshes[i];
		const ska::DSkMesh &dm = dskm.meshes[i];

		m.boneFrame = -1;
		m.dm = &dm;

		// this must be modified to support the SkinGpu path!

		if (kSkinFrames > 1)
			m.m.AllocateSwapChains(kSkinFrames);

		m.vertStreamIdx = m.m.AllocateStream(kStreamUsage_Stream, kVertSize, dm.totalVerts, true);
		
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
				4
			);
		}

		int uvStreamIndex = -1;

		if (dm.numChannels > 1) {
			uvStreamIndex = m.m.AllocateStream(kStreamUsage_Static, sizeof(float)*4, dm.totalVerts);
		
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

			uvStreamIndex = m.m.AllocateStream(kStreamUsage_Static, sizeof(float)*2, dm.totalVerts);
		
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

void SkMesh::Skin(int mesh) {
	DefMesh &m = m_meshes[mesh];
	
	if (m.boneFrame == m_ska->boneFrame)
		return;
	m.boneFrame = m_ska->boneFrame;

	m.m.SwapChain();
	Mesh::StreamPtr::Ref vb = m.m.Map(m.vertStreamIdx);

	SkinToBuffer(*SIMD, mesh, vb->ptr);

	vb.reset();
}

void SkMesh::SkinToBuffer(const SIMDDriver &driver, int mesh, void *buffer) {
	DefMesh &m = m_meshes[mesh];
	
	const float *srcVerts = m.dm->verts;
	const float *bones = m_ska->BoneTMs(ska::RowMajorTag());

	float *outVerts = (float*)buffer;

	for (int i = 0; i < ska::kBonesPerVert; ++i) {
		int numVerts = (int)m.dm->numVerts[i];
		const U16 *boneIndices = m.dm->bones[i];

		if (numVerts > 0) {
			driver.SkinVerts[i](
				outVerts,
				bones,
				srcVerts,
				boneIndices,
				numVerts
			);
		}

		outVerts += numVerts*ska::DSkMesh::kNumVertexFloats;
		srcVerts += numVerts*(i+1)*ska::DSkMesh::kNumVertexFloats;
	}
}


} // r
