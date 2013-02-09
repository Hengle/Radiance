/*! \file MeshVBLoader.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../Renderer/Mesh.h"

class Engine;

namespace asset {

class RADENG_CLASS MeshVBLoader : public pkg::Sink<MeshVBLoader> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Process+1, // MeshMateriaLoader is SS_Process
		AssetType = AT_Mesh
	};

	MeshVBLoader();
	virtual ~MeshVBLoader();


	const r::Mesh::Ref &Mesh(int idx) { 
		return m_meshes[idx]; 
	}

	RAD_DECLARE_READONLY_PROPERTY(MeshVBLoader, numMeshes, int);

protected:

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

	RAD_DECLARE_GET(numMeshes, int) { 
		return (int)m_meshes.size(); 
	}

	r::Mesh::Vec m_meshes;
	bool m_loaded;
};

} // asset
