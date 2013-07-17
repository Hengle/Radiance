/*! \file MeshVBLoader.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "MeshVBLoader.h"
#include "MeshParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

MeshVBLoader::MeshVBLoader() : m_loaded(false) {
}

MeshVBLoader::~MeshVBLoader() {
}

int MeshVBLoader::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Unload)))
		return SR_Success;

	if (m_loaded && (flags&P_Load))
		return SR_Success;

	if (flags&P_Unload) {
		m_meshes.clear();
		m_loaded = false;
		return SR_Success;
	}

	asset::MeshParser *parser = asset::MeshParser::Cast(asset);
	if (!parser || !parser->valid || parser->bundle->meshes.empty())
		return SR_MetaError;
	
	const int kVertSize = asset::DMesh::kNumVertexFloats*sizeof(float);

	for (size_t i = 0; i < parser->bundle->meshes.size(); ++i) {
		r::Mesh::Ref m(new (r::ZRender) r::Mesh());
		const asset::DMesh &dm = parser->bundle->meshes[i];
		
		// Mesh bundle data is interleaved

		int streamIdx = m->AllocateStream(r::kStreamUsage_Static, kVertSize, (int)dm.numVerts);
		r::Mesh::StreamPtr::Ref vb = m->Map(streamIdx);
		memcpy(vb->ptr, dm.vertices, vb->size.get());
		vb.reset();

		m->MapSource(
			streamIdx,
			r::kMaterialGeometrySource_Vertices,
			0,
			kVertSize,
			0,
			3
		);
		
		m->MapSource(
			streamIdx,
			r::kMaterialGeometrySource_Normals,
			0,
			kVertSize,
			sizeof(float)*3,
			3
		);

		if (dm.numChannels > 0) {
			m->MapSource(
				streamIdx,
				r::kMaterialGeometrySource_Tangents,
				0,
				kVertSize,
				sizeof(float)*6,
				3
			);
			m->MapSource(
				streamIdx,
				r::kMaterialGeometrySource_TexCoords,
				0,
				kVertSize,
				sizeof(float)*10,
				2
			);
		}

		if (dm.numChannels > 1) {
			m->MapSource(
				streamIdx,
				r::kMaterialGeometrySource_TexCoords,
				1,
				kVertSize,
				sizeof(float)*12,
				2
			);
		}
		
		vb = m->MapIndices(r::kStreamUsage_Static, sizeof(U16), (int)dm.numIndices);
		memcpy(vb->ptr, dm.indices, vb->size.get());
		vb.reset();

		m_meshes.push_back(m);
	}

	m_loaded = true;
	return SR_Success;
}

void MeshVBLoader::Register(Engine &engine) {
	static pkg::Binding::Ref r = engine.sys->packages->Bind<MeshVBLoader>();
}

} // asset
