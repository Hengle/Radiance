// MeshMaterialLoader.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS MeshMaterialLoader : public pkg::Sink<MeshMaterialLoader>
{
public:

	static void Register(Engine &engine);

	enum
	{
		SinkStage = pkg::SS_Process,
		AssetType = AT_Mesh
	};

	typedef boost::shared_ptr<MeshMaterialLoader> Ref;

	MeshMaterialLoader();
	virtual ~MeshMaterialLoader();

	const pkg::Asset::Ref &MaterialAsset(int mesh) const
	{
		return m_matRefs[mesh];
	}

	const pkg::Asset::Ref &UniqueMaterialAsset(int mesh) const
	{
		return m_umatRefs[mesh];
	}

	RAD_DECLARE_READONLY_PROPERTY(MeshMaterialLoader, numUniqueMaterials, int);
	
protected:

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

	int Load(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

	enum
	{
		S_None,
		S_LoadMaterials,
		S_Done,
		S_Error
	};

	void AddUMatRef(const pkg::Asset::Ref &m);

	RAD_DECLARE_GET(numUniqueMaterials, int) { return (int)m_umatRefs.size(); }

	pkg::Asset::Vec m_matRefs;
	pkg::Asset::Vec m_umatRefs;
	int m_state;
	int m_matIdx;
};

} // asset

#include <Runtime/PopPack.h>
