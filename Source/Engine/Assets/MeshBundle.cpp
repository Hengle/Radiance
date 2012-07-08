// MeshBundle.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

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

enum
{
	MeshTag = RAD_FOURCC_LE('M', 'E', 'S', 'H'),
	MeshVersion = 1,
	MaxBatchElements = Kilo*64,
	MaxUVChannels = 2
};

namespace asset {

RAD_ZONE_DEF(RADENG_API, ZMesh, "Mesh", ZEngine);

#define CHECK_SIZE(_size) if (((bytes+(_size))-reinterpret_cast<const U8*>(data)) > (int)len) return pkg::SR_CorruptFile;

int DMeshBundle::Parse(const void *data, AddrSize len)
{
	meshes.clear();

	if (len < 12)
		return pkg::SR_InvalidFormat;

	const U8 *bytes = reinterpret_cast<const U8*>(data);
	const U32 *header = reinterpret_cast<const U32*>(data);

	if (header[0] != MeshTag || header[1] != MeshVersion)
		return pkg::SR_InvalidFormat;

	U32 numModels = header[2];
	bytes += 12;

	meshes.resize(numModels);

	for (U32 i = 0; i < numModels; ++i)
	{
		DMesh &m = meshes[i];

		CHECK_SIZE(ska::DNameLen+1);
		m.material = reinterpret_cast<const char*>(bytes);
		bytes += ska::DNameLen+1;

		CHECK_SIZE(sizeof(U16)*2);
		m.flags = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);
		m.vertSize = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);
		
		CHECK_SIZE(sizeof(U32)*2);
		m.numVerts = *reinterpret_cast<const U32*>(bytes);
		bytes += sizeof(U32);
		m.numIndices = *reinterpret_cast<const U32*>(bytes);
		bytes += sizeof(U32);

		CHECK_SIZE(((U32)m.vertSize)*m.numVerts);
		m.vertices = reinterpret_cast<const void*>(bytes);
		bytes += ((U32)m.vertSize)*m.numVerts;
		
		CHECK_SIZE(sizeof(U16)*m.numIndices);
		m.indices = bytes;
		bytes += sizeof(U16)*m.numIndices;

		if (m.numIndices&1)
		{ // padd
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

typedef Map::NormalTriVert TriVert;
typedef zone_vector<TriVert, ZToolsT>::type TriVertVec;
typedef zone_map<TriVert, int, ZToolsT>::type TriVertMap;
typedef zone_vector<String, ZToolsT>::type StringVec;
typedef zone_map<String, int, ZToolsT>::type StringMap;

struct TriModel
{
	typedef boost::shared_ptr<TriModel> Ref;
	typedef zone_vector<Ref, ZToolsT>::type Vec;
	typedef zone_vector<int, ZToolsT>::type IntVec;
	
	int mat;
	int numChannels;
	TriVertVec verts;
	IntVec indices;
	TriVertMap vmap;

	void AddVertex(const TriVert &v)
	{
		TriVertMap::iterator it = vmap.find(v);
		if (it != vmap.end())
		{
#if defined(RAD_OPT_DEBUG)
			RAD_ASSERT(v.pos == it->first.pos);
			RAD_ASSERT(v.st[0] == it->first.st[0]);
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
	const MapVec &maps,
	DMeshBundleData::Ref &bundle
)
{
	TriModel::Vec models;
	TriModel::Ref m;
	StringMap matMap;
	StringVec mats;

	// gather materials
	for (MapVec::const_iterator it = maps.begin(); it != maps.end(); ++it)
	{
		const Map &map = (*(*it));

		for (int i = 0; i < (int)map.mats.size(); ++i)
		{
			String s(map.mats[i].name);
			StringMap::iterator it = matMap.find(s);
			if (it == matMap.end())
			{
				matMap[s] = (int)mats.size();
				mats.push_back(s);
			}
		}
	}

	for (int c = 1; c <= MaxUVChannels; ++c)
	{
		for (int i = 0; i < (int)mats.size(); ++i)
		{
			m.reset(new (ZTools) TriModel());
			m->mat = i;
			m->numChannels = c;
			
			for (MapVec::const_iterator it = maps.begin(); it != maps.end(); ++it)
			{
				const Map &map = (*(*it));
				const Map::Entity::Ref &e = map.worldspawn;

				for (Map::TriModel::Vec::const_iterator it = e->models.begin(); it != e->models.end(); ++it)
				{
					const Map::TriModel::Ref &src = *it;
					if (src->numChannels != c)
						continue;
					
					for (Map::TriFaceVec::const_iterator it = src->tris.begin(); it != src->tris.end(); ++it)
					{
						const Map::TriFace &tri = *it;
						if (tri.mat < 0)
							continue;
						int mat = matMap[String(map.mats[tri.mat].name)];
						if (mat != m->mat)
							continue;

						if (m->indices.size() >= MaxBatchElements-3 ||
							m->verts.size() >= MaxBatchElements-3)
						{ // flush
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

	if (!os.Write((U32)MeshTag) || !os.Write((U32)MeshVersion))
		return false;
	if (!os.Write((U32)models.size()))
		return false;

	for (TriModel::Vec::const_iterator it = models.begin(); it != models.end(); ++it)
	{
		const TriModel::Ref &m = *it;

		if (mats[m->mat].length > ska::DNameLen)
		{
			COut(C_ErrMsgBox) << "ska::DNameLen exceeded, contact a programmer to increase." << std::endl;
			return false;
		}

		char name[ska::DNameLen+1];
		string::ncpy(name, mats[m->mat].c_str.get(), ska::DNameLen+1);
		if (!os.Write(name, ska::DNameLen+1, 0))
			return false;

		U16 flags = asset::DMesh::Vertices|asset::DMesh::Texture1;
		U16 vertSize = sizeof(float)*5;

		if (m->numChannels > 1)
		{
			flags |= asset::DMesh::Texture2;
			vertSize += sizeof(float)*2;
		}

		if (!os.Write(flags))
			return false;
		if (!os.Write(vertSize))
			return false;
		if (!os.Write((U32)m->verts.size()))
			return false;
		if (!os.Write((U32)m->indices.size()))
			return false;

		for (TriVertVec::const_iterator it = m->verts.begin(); it != m->verts.end(); ++it)
		{
			const TriVert &v = *it;
			float floats[7];

			int i;
			for (i = 0; i < 3; ++i)
				floats[i] = v.pos[i];
			for (i = 0; i < 2; ++i)
				floats[3+i] = v.st[0][i];

			if (flags&asset::DMesh::Texture2)
			{
				for (i = 0; i < 2; ++i)
					floats[5+i] = v.st[1][i];
				if (os.Write(floats, sizeof(float)*7, 0) != sizeof(float)*7)
					return false;
			}
			else
			{
				if (os.Write(floats, sizeof(float)*5, 0) != sizeof(float)*5)
					return false;
			}
		}

		// write indices

		for (size_t i = 0; i < m->indices.size(); ++i)
		{
			if (!os.Write((U16)m->indices[i]))
				return false;
		}

		if (m->indices.size()&1)
		{ // pad
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
size(0)
{
}

DMeshBundleData::~DMeshBundleData()
{
	if (data)
		zone_free(data);
}

RADENG_API DMeshBundleData::Ref RADENG_CALL CompileMeshBundle(
	const char *name,
	const tools::MapVec &maps
)
{
	DMeshBundleData::Ref bundle(new (ZTools) DMeshBundleData());
	if (!DoCompileMeshBundle(name, maps, bundle))
		bundle.reset();
	return bundle;
}

} // tools

#endif

