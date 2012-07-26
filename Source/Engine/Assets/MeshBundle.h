// MeshBundle.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Renderer/Sources.h"
#include <Runtime/Container/ZoneVector.h>

#if defined(RAD_OPT_TOOLS)
#include "../Tools/MaxScene.h"
#endif

#include <Runtime/PushPack.h>

namespace asset {

RAD_ZONE_DEC(RADENG_API, ZMesh);

struct RADENG_CLASS DMesh {
	typedef zone_vector<DMesh, ZMeshT>::type Vec;

	enum {
		RAD_FLAG(Vertices),
		RAD_FLAG(Texture1),
		RAD_FLAG(Texture2)
	};

	U16 flags;
	U16 vertSize;
	U32 numVerts;
	U32 numIndices;

	const char *material;
	const void *vertices;
	const void *indices;
};

struct RADENG_CLASS DMeshBundle {
	DMesh::Vec meshes;

	// NOTE: bundle will reference data in place, do not free
	int Parse(const void *data, AddrSize len);
};

} // asset

#if defined(RAD_OPT_TOOLS)

namespace tools {

struct DMeshBundleData { 
	// cleans itself up when destructed
	typedef boost::shared_ptr<DMeshBundleData> Ref;

	DMeshBundleData();
	~DMeshBundleData();

	// references data in place
	asset::DMeshBundle bundle;

	void *data;
	AddrSize size;
};

RADENG_API DMeshBundleData::Ref RADENG_CALL CompileMeshBundle(
	const char *name,
	const MapVec &maps
);

} // tools

#endif

#include <Runtime/PopPack.h>
