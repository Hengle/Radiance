// SkMesh.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SkMesh.h"
#include "Sources.h"
#include "../Assets/SkModelParser.h"
#include <Runtime/Base/SIMD.h>
#include <limits>
#undef max

using namespace pkg;
using namespace asset;

namespace r {

SkMesh::SkMesh() {
}

SkMesh::~SkMesh() {
}

SkMesh::Ref SkMesh::New(const pkg::AssetRef &asset) {
	SkModelParser::Ref parser = SkModelParser::Cast(asset);
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

	for (size_t i = 0; i < m_meshes.size(); ++i)
	{
		DefMesh &m = m_meshes[i];
		const ska::DMesh &dm = dskm.meshes[i];

		m.boneFrame = -1;
		m.dm = &dm;

		// this must be modified to support the SkinGpu path!

		if (kSkinFrames > 1)
			m.m.AllocateSwapChains(kSkinFrames);

		const int kVertSize = 8+(4*dm.numChannels)*sizeof(float);

		m.vertStreamIdx = m.m.AllocateStream(SU_Stream, kVertSize, dm.totalVerts, true);
		
		m.m.MapSource(
			m.vertStreamIdx,
			MGS_Vertices,
			0,
			kVertSize,
			0
		);

		m.m.MapSource(
			m.vertStreamIdx,
			MGS_Normals,
			0,
			kVertSize,
			sizeof(float) * 4
		);

		if (dm.numChannels > 0) {
			m.m.MapSource(
				m.vertStreamIdx,
				MGS_Tangents,
				0,
				kVertSize,
				sizeof(float) * 8
			);
		}

		if (dm.numChannels > 1) {
			m.m.MapSource(
				m.vertStreamIdx,
				MGS_Tangents,
				1,
				kVertSize,
				sizeof(float) * 12
			);
		}

		int uvStreamIndex = -1;

		if (dm.numChannels > 1) {
			int streamIdx = m.m.AllocateStream(SU_Static, sizeof(float)*4, dm.totalVerts);
		
			m.m.MapSource(
				streamIdx,
				MGS_TexCoords,
				0,
				sizeof(float) * 4,
				0
			);

			m.m.MapSource(
				streamIdx,
				MGS_TexCoords,
				1,
				sizeof(float) * 4,
				sizeof(float) * 2
			);

		} else if (dm.numChannels > 0) {

			int streamIdx = m.m.AllocateStream(SU_Static, sizeof(float)*2, dm.totalVerts);
		
			m.m.MapSource(
				streamIdx,
				MGS_TexCoords,
				0,
				0,
				0
			);
		}

		if (dm.numChannels > 0) {
			Mesh::StreamPtr::Ref vb = m.m.Map(uvStreamIndex);
			memcpy(vb->ptr, dm.texCoords, vb->size.get());
			vb.reset();
		}

		Mesh::StreamPtr::Ref vb = m.m.MapIndices(SU_Static, sizeof(U16), (int)dm.numTris * 3);
		memcpy(vb->ptr, dm.indices, vb->size.get());
		vb.reset();
	}
}

void SkMesh::Skin(int mesh) {
	DefMesh &m = m_meshes[mesh];
	
	if (m.boneFrame == m_ska->boneFrame)
		return;
	m.boneFrame = m_ska->boneFrame;

	const float *srcVerts = m.dm->verts;
	const float *bones = m_ska->BoneTMs(ska::RowMajorTag());

	m.m.SwapChain();
	Mesh::StreamPtr::Ref vb = m.m.Map(m.vertStreamIdx);

	float *outVerts = reinterpret_cast<float*>(vb->ptr.get());
	RAD_ASSERT(IsAligned(outVerts, SIMDDriver::kAlignment));

	for (int i = 0; i < ska::kBonesPerVert; ++i) {
		int numVerts = (int)m.dm->numVerts[i];
		const U16 *boneIndices = m.dm->bones[i];

		if ((numVerts > 0) && (m.dm->numChannels < 2)) {
			SIMD->SkinVerts[i][m.dm->numChannels](
				outVerts,
				bones,
				srcVerts,
				boneIndices,
				numVerts
			);
		}

		outVerts += numVerts*4;
		srcVerts += numVerts*4*(i+1);
	}

	vb.reset();
}

} // r
