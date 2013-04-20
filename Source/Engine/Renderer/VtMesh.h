/*! \file VtMesh.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#pragma once

#include "Mesh.h"
#include "../SkAnim/SkAnim.h"
#include "../SkAnim/SkControllers.h"
#include "../Packages/PackagesDef.h"
#include "../Assets/VtModelParser.h"
#include <Runtime/Base/SIMD.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace r {

class RADENG_CLASS VtMesh {
public:
	typedef boost::shared_ptr<VtMesh> Ref;

	~VtMesh();

	static Ref New(const pkg::AssetRef &asset);
	static Ref New(const ska::DVtm &dvtm);

	r::Mesh &Mesh(int idx) { 
		return m_meshes[idx].m; 
	}

	// skin verts, uploads. does nothing if gpu skin
	void Skin(int mesh);

	void SkinToBuffer(const SIMDDriver &driver, int mesh, void *buffer);

	RAD_DECLARE_READONLY_PROPERTY(VtMesh, numMeshes, int);
	RAD_DECLARE_READONLY_PROPERTY(VtMesh, vtm, ska::Vtm*);
	RAD_DECLARE_READONLY_PROPERTY(VtMesh, states, const ska::AnimState::Map*);
	RAD_DECLARE_READONLY_PROPERTY(VtMesh, asset, const pkg::AssetRef&);

#if !defined(RAD_OPT_SHIP)
	const ska::DVtMesh *DMesh(int idx) {
		return m_meshes[idx].dm;
	}
#endif

private:

	VtMesh();

	void Load(
		const ska::Vtm::Ref &skanim,
		const ska::DVtm &dvtm
	);

	RAD_DECLARE_GET(numMeshes, int)  { 
		return (int)m_meshes.size(); 
	}

	RAD_DECLARE_GET(vtm, ska::Vtm*) { 
		return m_vtm.get(); 
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
		int vertexFrame;
		int vertStreamIdx;
		const ska::DVtMesh *dm;
	};

	DefMesh::Vec m_meshes;
	ska::Vtm::Ref m_vtm;
	pkg::AssetRef m_asset;
	asset::VtModelParser *m_parser;
};

} // r

#include <Runtime/PopPack.h>
