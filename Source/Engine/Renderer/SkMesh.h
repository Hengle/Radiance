// SkMesh.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Mesh.h"
#include "../SkAnim/SkAnim.h"
#include "../SkAnim/SkControllers.h"
#include "../Packages/PackagesDef.h"
#include "../Assets/SkModelParser.h"
#include <Runtime/Base/SIMD.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace r {

class RADENG_CLASS SkMesh {
public:
	typedef boost::shared_ptr<SkMesh> Ref;

	~SkMesh();

	static Ref New(const pkg::AssetRef &asset);
	static Ref New(const ska::DSka &dska, const ska::DSkm &dskm, ska::SkinType skinType);

	r::Mesh &Mesh(int idx) { 
		return m_meshes[idx].m; 
	}

	// skin verts, uploads. does nothing if gpu skin
	void Skin(int mesh);

	void SkinToBuffer(const SIMDDriver *driver, int mesh, void *buffer);

	RAD_DECLARE_READONLY_PROPERTY(SkMesh, numMeshes, int);
	RAD_DECLARE_READONLY_PROPERTY(SkMesh, ska, ska::Ska*);
	RAD_DECLARE_READONLY_PROPERTY(SkMesh, states, const ska::AnimState::Map*);
	RAD_DECLARE_READONLY_PROPERTY(SkMesh, asset, const pkg::AssetRef&);

#if !defined(RAD_OPT_SHIP)
	const ska::DMesh *DMesh(int idx) {
		return m_meshes[idx].dm;
	}
#endif


private:

	SkMesh();

	void Load(
		const ska::Ska::Ref &skanim,
		const ska::DSkm &dskm,
		ska::SkinType type
	);

	RAD_DECLARE_GET(numMeshes, int)  { 
		return (int)m_meshes.size(); 
	}

	RAD_DECLARE_GET(ska, ska::Ska*) { 
		return m_ska.get(); 
	}

	RAD_DECLARE_GET(states, const ska::AnimState::Map*) { 
		return m_parser ? m_parser->states.get() : 0; 
	}

	RAD_DECLARE_GET(asset, const pkg::AssetRef&) {
		return m_asset; 
	}

	enum {
		kSkinFrames = 2
	};

	struct DefMesh {
		typedef zone_vector<DefMesh, ZEngineT>::type Vec;
		r::Mesh m;
		int boneFrame;
		int vertStreamIdx;
		const ska::DMesh *dm;
	};

	DefMesh::Vec m_meshes;
	ska::Ska::Ref m_ska;
	pkg::AssetRef m_asset;
	asset::SkModelParser *m_parser;
	ska::SkinType m_type;
};

} // r

#include <Runtime/PopPack.h>
