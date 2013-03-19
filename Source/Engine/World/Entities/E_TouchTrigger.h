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

	virtual bool HandleEvent(const Event &event);

	Entity::Vec GetTouching() const;

protected:

	virtual void PushCallTable(lua_State *L);
	virtual int PostSpawn();

private:

	ENT_DECL_GETSET(TouchClassBits);

	static int lua_GetTouching(lua_State *L);

	void CheckEnter();
	void CheckExit(const Entity &instigator);
	void DoExit();
	void DoEnter();
	void UpdateAttachment(
		int frame,
		float dt, 
		const xtime::TimeSlice &time
	);
	void NotifyAttachmentTouch();

	int SetupAttachment();

	String m_enter;
	String m_exit;
	String m_sAttachment;
	String m_sAttachmentBone;

	Entity::WRef m_instigator;
	Entity::WRef m_attachment;
	SkMeshDrawModel::WRef m_attachmentModel;
	Vec3 m_attachmentXform;
	int m_attachmentBone;
	int m_firstBrush;
	int m_numBrushes;
	int m_classbits;
	bool m_bboxOnly;
	bool m_enabled;
	bool m_occupied;
};

} // world

#include <Runtime/PopPack.h>
