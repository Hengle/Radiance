/*! \file E_TouchTrigger.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "../Entity.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS E_TouchTrigger : public Entity {
	E_DECL_BASE(Entity);
public:

	E_TouchTrigger();
	virtual ~E_TouchTrigger();

	virtual int Spawn(
		const Keys &keys,
		const xtime::TimeSlice &time,
		int flags
	);

	virtual void Tick(
		int frame,
		float dt, 
		const xtime::TimeSlice &time
	);

	virtual bool HandleEvent(const Event::Ref &event);

protected:

	virtual void PushCallTable(lua_State *L);

private:

	ENT_DECL_GETSET(TouchClassBits);

	void CheckEnter();
	void CheckExit(const Entity &instigator);
	void DoExit();
	void DoEnter();

	String m_enter;
	String m_exit;
	Entity::WRef m_instigator;
	int m_firstBrush;
	int m_numBrushes;
	int m_classbits;
	bool m_enabled;
	bool m_occupied;
};

} // world

#include <Runtime/PopPack.h>
