// SoundLoader.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "SoundLoader.h"
#include <OpenAL/al.h>
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS SoundLoader : public pkg::Sink<SoundLoader> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Process,
		AssetType = AT_Sound
	};

	typedef boost::shared_ptr<SoundLoader> Ref;

	SoundLoader();
	virtual ~SoundLoader();

	RAD_DECLARE_READONLY_PROPERTY(SoundLoader, id, ALuint);

protected:

	RAD_DECLARE_GET(id, ALuint) { 
		return m_id; 
	}

	int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

	int m_size;
	ALuint m_id;
};

} // asset

#include <Runtime/PopPack.h>
