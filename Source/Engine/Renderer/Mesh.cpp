// Mesh.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Mesh.h"
#include "../Assets/MeshParser.h"

namespace r {

MeshBundle::Ref MeshBundle::New(const pkg::AssetRef &asset)
{
	asset::MeshParser::Ref parser = asset::MeshParser::Cast(asset);
	if (!parser || !parser->valid || parser->bundle->meshes.empty())
		return Ref();

	Ref r(new (ZRender) MeshBundle());

	for (size_t i = 0; i < parser->bundle->meshes.size(); ++i)
	{
		r::Mesh::Ref m(new (ZRender) r::Mesh());
		const asset::DMesh &dm = parser->bundle->meshes[i];

		// Mesh bundle data is interleaved

		int streamIdx = m->AllocateStream(SU_Static, (int)dm.vertSize, (int)dm.numVerts);
		r::Mesh::StreamPtr::Ref vb = m->Map(streamIdx);
		memcpy(vb->ptr, dm.vertices, vb->size.get());
		vb.reset();

		if (dm.flags & asset::DMesh::Vertices)
		{
			m->MapSource(
				streamIdx,
				MGS_Vertices,
				0,
				(int)dm.vertSize,
				0
			);
		}

		if (dm.flags & asset::DMesh::Texture1)
		{
			m->MapSource(
				streamIdx,
				MGS_TexCoords,
				0,
				(int)dm.vertSize,
				sizeof(float)*3
			);
		}

		if (dm.flags & asset::DMesh::Texture2)
		{
			m->MapSource(
				streamIdx,
				MGS_TexCoords,
				1,
				(int)dm.vertSize,
				sizeof(float)*5
			);
		}
		
		vb = m->MapIndices(SU_Static, sizeof(U16), (int)dm.numIndices);
		memcpy(vb->ptr, dm.indices, vb->size.get());
		vb.reset();

		r->m_meshes.push_back(m);
	}

	r->m_asset = asset;
	r->m_loader = asset::MeshMaterialLoader::Cast(asset);
	return r;
}

} // r
