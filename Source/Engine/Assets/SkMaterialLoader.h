/*! \file SkMaterialLoader.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS SkMaterialLoader : public pkg::Sink<SkMaterialLoader> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Process,
		AssetType = AT_SkModel
	};

	SkMaterialLoader();
	virtual ~SkMaterialLoader();

	const pkg::Asset::Ref &MaterialAsset(int mesh) const {
		return m_matRefs[mesh];
	}

	const pkg::Asset::Ref &UniqueMaterialAsset(int idx) const {
		return m_umatRefs[idx];
	}

	RAD_DECLARE_READONLY_PROPERTY(SkMaterialLoader, numUniqueMaterials, int);
	
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

	enum {
		S_None,
		S_LoadMaterials,
		S_Done,
		S_Error
	};

	void AddUMatRef(const pkg::Asset::Ref &m);

	RAD_DECLARE_GET(numUniqueMaterials, int) { 
		return (int)m_umatRefs.size(); 
	}

	pkg::Asset::Vec m_matRefs;
	pkg::Asset::Vec m_umatRefs;
	int m_state;
	int m_matIdx;
};

} // asset

#include <Runtime/PopPack.h>
