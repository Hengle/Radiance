// MeshBundle.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "MeshBundle.h"
#include "../COut.h"
#include "../SkAnim/SkAnimDef.h"
#include "../Packages/PackagesDef.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Stream.h>
#include <Runtime/Stream/MemoryStream.h>
#include <Runtime/Endian/EndianStream.h>

//#define MESH_NORMALS

enum {
	kMeshTag = RAD_FOURCC_LE('M', 'E', 'S', 'H'),
	kMeshVersion = 2,
	kMaxBatchElements = Kilo*64,
	kMaxUVChannels = ska::kMaxUVChannels
};

namespace asset {

RAD_ZONE_DEF(RADENG_API, ZMesh, "Mesh", ZEngine);

#define CHECK_SIZE(_size) if (((bytes+(_size))-reinterpret_cast<const U8*>(data)) > (int)len) return pkg::SR_CorruptFile;

int DMeshBundle::Parse(const void *data, AddrSize len) {
	meshes.clear();

	if (len < 12)
		return pkg::SR_InvalidFormat;

	const U8 *bytes = reinterpret_cast<const U8*>(data);
	const U32 *header = reinterpret_cast<const U32*>(data);

	if (header[0] != kMeshTag || header[1] != kMeshVersion)
		return pkg::SR_InvalidFormat;

	U32 numModels = header[2];
	bytes += 12;

	meshes.resize(numModels);

	for (U32 i = 0; i < numModels; ++i) {
		DMesh &m = meshes[i];

		CHECK_SIZE(ska::kDNameLen+1);
		m.material = reinterpret_cast<const char*>(bytes);
		bytes += ska::kDNameLen+1;

		CHECK_SIZE(sizeof(U16)*4);
		m.numVerts = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);
		m.numIndices = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);
		m.numChannels = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);

		// padd bytes
		bytes += sizeof(U16);

		CHECK_SIZE(m.numVerts * DMesh::kNumVertexFloats * sizeof(float));
		m.vertices = reinterpret_cast<const void*>(bytes);
		bytes += m.numVerts * DMesh::kNumVertexFloats * sizeof(float);
		
		CHECK_SIZE(sizeof(U16)*m.numIndices);
		m.indices = bytes;
		bytes += sizeof(U16)*m.numIndices;

		if (m.numIndices&1) { // padd
			CHECK_SIZE(sizeof(U16));
			bytes += sizeof(U16);
		}
	}

	return pkg::SR_Success;
}

} // asset

#if defined(RAD_OPT_TOOLS)

namespace tools {
namespace {

typedef SceneFile::NormalTriVert TriVert;
typedef zone_vector<TriVert, ZToolsT>::type TriVertVec;
typedef zone_map<TriVert, int, ZToolsT>::type TriVertMap;
typedef zone_vector<String, ZToolsT>::type StringVec;
typedef zone_map<String, int, ZToolsT>::type StringMap;

struct TriModel {
	typedef boost::shared_ptr<TriModel> Ref;
	typedef zone_vector<Ref, ZToolsT>::type Vec;
	typedef zone_vector<int, ZToolsT>::type IntVec;
	
	int mat;
	int numChannels;
	TriVertVec verts;
	IntVec indices;
	TriVertMap vmap;

	void AddVertex(const TriVert &v) {
		TriVertMap::iterator it = vmap.find(v);
		if (it != vmap.end()) {
#if defined(RAD_OPT_DEBUG)
			RAD_ASSERT(v.pos == it->first.pos);
			RAD_ASSERT(v.st[0] == it->first.st[0]);
			for (int i = 0; i < SceneFile::kMaxUVChannels; ++i) {
				RAD_ASSERT(v.st[i] == it->first.st[i]);
				RAD_ASSERT(v.tangent[i] == it->first.tangent[i]);
			}
#endif
			indices.push_back(it->second);
			return;
		}
		int ofs = (int)verts.size();
		verts.push_back(v);
		vmap.insert(TriVertMap::value_type(v, ofs));
		indices.push_back(ofs);
	}
};

bool DoCompileMeshBundle(
	const char *name,
	const SceneFileVec &maps,
	DMeshBundleData::Ref &bundle
) {
	TriModel::Vec models;
	TriModel::Ref m;
	StringMap matMap;
	StringVec mats;

	// gather materials
	for (SceneFileVec::const_iterator it = maps.begin(); it != maps.end(); ++it) {
		const SceneFile &map = (*(*it));

		for (int i = 0; i < (int)map.mats.size(); ++i) {
			String s(map.mats[i].name);
			StringMap::iterator it = matMap.find(s);
			if (it == matMap.end()) {
				matMap[s] = (int)mats.size();
				mats.push_back(s);
			}
		}
	}

	for (int c = 1; c <= kMaxUVChannels; ++c) {
		for (int i = 0; i < (int)mats.size(); ++i) {
			m.reset(new (ZTools) TriModel());
			m->mat = i;
			m->numChannels = c;
			
			for (SceneFileVec::const_iterator it = maps.begin(); it != maps.end(); ++it) {
				const SceneFile &map = (*(*it));
				const SceneFile::Entity::Ref &e = map.worldspawn;

				for (SceneFile::TriModel::Vec::const_iterator it = e->models.begin(); it != e->models.end(); ++it) {
					const SceneFile::TriModel::Ref &src = *it;
					if (src->numChannels != c)
						continue;
					
					for (SceneFile::TriFaceVec::const_iterator it = src->tris.begin(); it != src->tris.end(); ++it) {
						const SceneFile::TriFace &tri = *it;
						if (tri.mat < 0)
							continue;
						int mat = matMap[String(map.mats[tri.mat].name)];
						if (mat != m->mat)
							continue;

						if (m->indices.size() >= kMaxBatchElements-3 ||
							m->verts.size() >= kMaxBatchElements-3) { 
							// flush
							models.push_back(m);
							m.reset(new (ZTools) TriModel());
							m->mat = i;
							m->numChannels = c;
						}

						for (int i = 0; i < 3; ++i)
							m->AddVertex(TriVert(src->verts[tri.v[i]]));
					}
				}
			}

			if (m && !m->indices.empty())
				models.push_back(m);
		}
	}

	if (models.empty())
		return false;

	stream::DynamicMemOutputBuffer ob(asset::ZMesh);
	stream::LittleOutputStream os(ob);

	if (!os.Write((U32)kMeshTag) || !os.Write((U32)kMeshVersion))
		return false;
	if (!os.Write((U32)models.size()))
		return false;

	for (TriModel::Vec::const_iterator it = models.begin(); it != models.end(); ++it) {
		const TriModel::Ref &m = *it;

		if (mats[m->mat].length > ska::kDNameLen) {
			COut(C_ErrMsgBox) << "ska::kDNameLen exceeded, contact a programmer to increase." << std::endl;
			return false;
		}

		char name[ska::kDNameLen+1];
		string::ncpy(name, mats[m->mat].c_str.get(), ska::kDNameLen+1);
		if (!os.Write(name, ska::kDNameLen+1, 0))
			return false;

		
		if (!os.Write((U16)m->verts.size()))
			return false;
		if (!os.Write((U16)m->indices.size()))
			return false;
		if (!os.Write((U16)m->numChannels))
			return false;
		if (!os.Write((U16)0))
			return false;

		for (TriVertVec::const_iterator it = m->verts.begin(); it != m->verts.end(); ++it) {
			const TriVert &v = *it;

			for (int i = 0; i < 3; ++i) {
				if (!os.Write(v.pos[i]))
					return false;
			}

			for (int i = 0; i < 3; ++i) {
				if (!os.Write(v.normal[i]))
					return false;
			}

			for (int i = 0; i < 4; ++i) {
				if (!os.Write(v.tangent[0][i]))
					return false;
			}

			RAD_STATIC_ASSERT(SceneFile::kMaxUVChannels == 2);

			for (int i = 0; i < SceneFile::kMaxUVChannels; ++i) {
				for (int k = 0; k < 2; ++k) {
					if (!os.Write(v.st[i][k]))
						return false;
				}
			}
		}

		// write indices

		for (size_t i = 0; i < m->indices.size(); ++i) {
			if (!os.Write((U16)m->indices[i]))
				return false;
		}

		if (m->indices.size()&1) { 
			// pad
			if (!os.Write((U16)0))
				return false;
		}
	}

	bundle->data = ob.OutputBuffer().Ptr();
	bundle->size = (AddrSize)ob.OutPos();
	ob.OutputBuffer().Set(0, 0);
	bundle->data = zone_realloc(asset::ZMesh, bundle->data, bundle->size);
	RAD_VERIFY(bundle->bundle.Parse(bundle->data, bundle->size) == pkg::SR_Success);
	return true;
}

} // namespace

DMeshBundleData::DMeshBundleData() :
data(0),
size(0) {
}

DMeshBundleData::~DMeshBundleData() {
	if (data)
		zone_free(data);
}

RADENG_API DMeshBundleData::Ref RADENG_CALL CompileMeshBundle(
	const char *name,
	const tools::SceneFileVec &maps
) {
	DMeshBundleData::Ref bundle(new (ZTools) DMeshBundleData());
	if (!DoCompileMeshBundle(name, maps, bundle))
		bundle.reset();
	return bundle;
}

} // tools

#endif

