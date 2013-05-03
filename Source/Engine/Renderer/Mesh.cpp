/*! \file Mesh.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup renderer
*/

#include RADPCH
#include "Mesh.h"
#include <Runtime/Math.h>

namespace r {

Mesh::Ref Mesh::MakeSphere(Zone &zone, bool uvs, float scale, int tessLat, int tessLog) {
	if (tessLat < 4)
		tessLat = 4;
	if (tessLog < 3)
		tessLog = 3;

	const int kNumVerts = tessLog * tessLat;
	const int kNumVertFloats = 3 + (uvs ? 2 : 0);
	const int kVertSize = sizeof(float) * kNumVertFloats;
	const int kNumTris = (tessLat - 1) * (tessLog - 1) * 2;

	Mesh::Ref m(new (zone) Mesh());

	const int kVertStream = m->AllocateStream(
		kStreamUsage_Static, 
		kVertSize,
		kNumVerts
	);

	Mesh::StreamPtr::Ref vb = m->Map(kVertStream);
	float *verts = (float*)vb->ptr.get();

	// sweep sphere
	const float kLatStep = 1.f / (tessLat - 1);
	const float kLogStep = 1.f / (tessLog - 1);

	for (float lat = 0.f; lat <= 1.f; lat += kLatStep) {
		const float kLatRad = lat * math::Constants<float>::_2_PI();

		float x, y;
		math::SinAndCos(&y, &x, kLatRad);

		x = x * scale;
		y = y * scale;

		for (float log = 0.f; log <= 1.f; log += kLogStep) {
			const float kLogRad = log * math::Constants<float>::PI();

			float s;
			float z;
			math::SinAndCos(&s, &z, kLogRad);

			verts[0] = x*s;
			verts[1] = y*s;
			verts[2] = z*scale;

			if (uvs) {
				verts[3] = lat;
				verts[4] = log;
			}

			verts += kNumVertFloats;
		}
	}

	vb = m->MapIndices(kStreamUsage_Static, sizeof(U16), kNumTris*3);

	U16 *indices = (U16*)vb->ptr.get();

	for (int lat = 0; lat < (tessLat-1); ++lat) {

		const int kBaseVert = lat * tessLog;
		const int kBaseVert2 = (lat+1) * tessLog;

		for (int log = 0; log < (tessLog-1); ++log) {
			indices[0] = (U16)(kBaseVert + log);
			indices[1] = (U16)(kBaseVert + log + 1);
			indices[2] = (U16)(kBaseVert2 + log);
			indices[3] = (U16)(kBaseVert2 + log);
			indices[4] = (U16)(kBaseVert + log + 1);
			indices[5] = (U16)(kBaseVert2 + log + 1);
			indices += 6;
		}
	}

	vb.reset();

	m->MapSource(
		kVertStream,
		kMaterialGeometrySource_Vertices,
		0,
		kVertSize,
		0
	);

	if (uvs) {
		m->MapSource(
			kVertStream,
			kMaterialGeometrySource_TexCoords,
			0,
			kVertSize,
			sizeof(float)*3
		);
	}

	return m;
}

}
