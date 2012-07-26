// SkMaterialLoader.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

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

	typedef boost::shared_ptr<SkMaterialLoader> Ref;

	SkMaterialLoader();
	virtual ~SkMaterialLoader();

	const pkg::Asset::Ref &MaterialAsset(int mesh) const {
		return m_matRefs[mesh];
	}
	
protected:

	int Process(
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

	pkg::Asset::Vec m_matRefs;
	int m_state;
	int m_matIdx;
};

} // asset

#include <Runtime/PopPack.h>
