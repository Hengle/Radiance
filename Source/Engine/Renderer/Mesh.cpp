// Mesh.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Mesh.h"
#include "../Assets/MeshParser.h"

namespace r {

MeshBundle::Ref MeshBundle::New(const pkg::AssetRef &asset) {
	asset::MeshParser *parser = asset::MeshParser::Cast(asset);
	if (!parser || !parser->valid || parser->bundle->meshes.empty())
		return Ref();

	Ref r(new (ZRender) MeshBundle());

	const int kVertSize = asset::DMesh::kNumVertexFloats*sizeof(float);

	for (size_t i = 0; i < parser->bundle->meshes.size(); ++i) {
		r::Mesh::Ref m(new (ZRender) r::Mesh());
		const asset::DMesh &dm = parser->bundle->meshes[i];
		
		// Mesh bundle data is interleaved

		int streamIdx = m->AllocateStream(kStreamUsage_Static, kVertSize, (int)dm.numVerts);
		r::Mesh::StreamPtr::Ref vb = m->Map(streamIdx);
		memcpy(vb->ptr, dm.vertices, vb->size.get());
		vb.reset();

		m->MapSource(
			streamIdx,
			kMaterialGeometrySource_Vertices,
			0,
			kVertSize,
			0
		);
		
		m->MapSource(
			streamIdx,
			kMaterialGeometrySource_Normals,
			0,
			kVertSize,
			sizeof(float)*3
		);

		if (dm.numChannels > 0) {
			m->MapSource(
				streamIdx,
				kMaterialGeometrySource_Tangents,
				0,
				kVertSize,
				sizeof(float)*6
			);
			m->MapSource(
				streamIdx,
				kMaterialGeometrySource_TexCoords,
				0,
				kVertSize,
				sizeof(float)*10
			);
		}

		if (dm.numChannels > 1) {
			m->MapSource(
				streamIdx,
				kMaterialGeometrySource_TexCoords,
				1,
				kVertSize,
				sizeof(float)*12
			);
		}
		
		vb = m->MapIndices(kStreamUsage_Static, sizeof(U16), (int)dm.numIndices);
		memcpy(vb->ptr, dm.indices, vb->size.get());
		vb.reset();

		r->m_meshes.push_back(m);
	}

	r->m_asset = asset;
	r->m_loader = asset::MeshMaterialLoader::Cast(asset);
	return r;
}

} // r
