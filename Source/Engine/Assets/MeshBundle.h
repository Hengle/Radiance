// MeshBundle.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Renderer/Common.h"
#include <Runtime/Container/ZoneVector.h>

#if defined(RAD_OPT_TOOLS)
#include "../Tools/SceneFile.h"
#endif

#include <Runtime/PushPack.h>

namespace asset {

RAD_ZONE_DEC(RADENG_API, ZMesh);

struct RADENG_CLASS DMesh {
	typedef zone_vector<DMesh, ZMeshT>::type Vec;

	enum {
		kNumVertexFloats = 12
	};

	U16 numVerts;
	U16 numIndices;
	U16 numChannels;

	const char *material;
	const void *vertices; // vertex packing not the same as ska's
						  // floats:
						  // 0-2   -> vertex (3 floats)
						  // 3-5   -> normal (3 floats)
						  // 6-9   -> tangent (4 floats)
	                      // 10-13 -> st (always has 2 channels) (4 floats)

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
	const SceneFileVec &maps
);

} // tools

#endif

#include <Runtime/PopPack.h>
