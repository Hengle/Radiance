/*! \file Common.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#pragma once

#include "../Types.h"

namespace r {

// NOTE: these constants effect serialized shaders and materials.
// If you change them, make sure to increment cooker versions!

enum {
	kMaxTextures = 6,
	kMaxAttribArrays = 8,
	kInvalidMapping = 255,
	kMaxLights = 4
};

enum StreamUsage {
	kStreamUsage_Static,
	kStreamUsage_Dynamic,
	kStreamUsage_Stream
};

enum MaterialTextureSource {
	kMaterialTextureSource_Texture,
	kNumMaterialTextureSources,
	kMaterialTextureSource_MaxIndices = 6
};

enum MaterialGeometrySource {
	kMaterialGeometrySource_Vertices,
	kMaterialGeometrySource_Normals,
	kMaterialGeometrySource_Tangents,
	kMaterialGeometrySource_TexCoords,
	kMaterialGeometrySource_VertexColor,
	kMaterialGeometrySource_SpriteSkin,
	kNumMaterialGeometrySources,
	kMaterialGeometrySource_MaxIndices = 2
};

struct MaterialInputMappings {
	boost::array<U8, kMaterialTextureSource_MaxIndices> tcMods;
	boost::array<U8, kNumMaterialTextureSources> numMTSources;
	boost::array<U8, kNumMaterialGeometrySources> numMGSources;
	boost::array<boost::array<U8, 2>, kMaxTextures> textures;
	boost::array<boost::array<U8, 2>, kMaxAttribArrays> attributes;
};

struct LightDef {
	enum {
		RAD_FLAG(kFlag_Diffuse),
		RAD_FLAG(kFlag_Specular)
	};
	Vec3 specular;
	Vec3 diffuse;
	Vec3 pos;
	float radius;
	float brightness;
	int flags;

	bool operator == (const LightDef &d) {
		return (diffuse == d.diffuse) &&
			(specular == d.specular) &&
			(pos == d.pos);
	}

	bool operator != (const LightDef &d) {
		return !(*this == d);
	}
};

struct LightEnv {
	boost::array<LightDef, kMaxLights> lights;
	int numLights;

	bool operator == (const LightEnv &env) {
		if (numLights != env.numLights)
			return false;
		for (int i = 0; i < numLights; ++i) {
			if (lights[i] != env.lights[i])
				return false;
		}

		return true;
	}

	bool operator != (const LightEnv &env) {
		return !(*this == env);
	}
};

} // r
