// E_SoundEmitter.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Entity.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS E_SoundEmitter : public Entity
{
	E_DECL_BASE(Entity);
public:

	E_SoundEmitter();

protected:

	virtual void Tick(
		int frame,
		float dt,
		const xtime::TimeSlice &time
	);

	virtual int Spawn(
		const Keys &keys,
		const xtime::TimeSlice &time,
		int flags
	);

private:

	bool m_on;
	float m_maxDistance;
	bool m_positional;

};

} // world

E_DECL_SPAWN(RADENG_API, info_sound_emitter)

#include <Runtime/PopPack.h>
