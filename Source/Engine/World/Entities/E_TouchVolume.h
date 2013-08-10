/*! \file E_TouchVolume.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "../Entity.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS E_TouchVolume : public Entity {
	E_DECL_BASE(Entity);
public:

	E_TouchVolume();
	virtual ~E_TouchVolume();

	virtual int Spawn(
		const Keys &keys,
		const xtime::TimeSlice &time,
		int flags
	);

	Entity::Vec GetTouching(int classbits) const;

protected:

	virtual void PushCallTable(lua_State *L);

private:

	static int lua_GetTouching(lua_State *L);

	int m_firstBrush;
	int m_numBrushes;
	bool m_bboxOnly;
};

} // world

#include <Runtime/PopPack.h>
