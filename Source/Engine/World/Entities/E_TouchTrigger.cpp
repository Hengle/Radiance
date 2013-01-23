/*! \file E_TouchTrigger.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "E_TouchTrigger.h"
#include "../World.h"

namespace world {

E_TouchTrigger::E_TouchTrigger() : 
E_CONSTRUCT_BASE, 
m_firstBrush(0), 
m_numBrushes(0),
m_enabled(false),
m_classbits(kEntityClassBits_Any) {
}

E_TouchTrigger::~E_TouchTrigger() {
}

int E_TouchTrigger::Spawn(
	const Keys &keys,
	const xtime::TimeSlice &time,
	int flags
) {
	E_SPAWN_BASE();
	m_firstBrush = keys.IntForKey("firstBrush");
	m_numBrushes = keys.IntForKey("numBrushes");
	m_enabled = keys.BoolForKey("enabled");
	
	const char *sz = keys.StringForKey("enter_cmd");
	if (sz)
		m_enter = sz;

	sz = keys.StringForKey("exit_cmd");
	if (sz)
		m_exit = sz;

	if (m_enter.empty && m_exit.empty)
		m_enabled = false; // it can never be

	if (m_firstBrush < 0 ||
		m_numBrushes < 1) {
		COut(C_Error) << "ERROR: TouchTrigger: I have no brushes!" << std::endl;
		return pkg::SR_ParseError;
	}

	SetNextTick(1.f/4.f); // 4hz

	return pkg::SR_Success;
}

void E_TouchTrigger::Tick(
	int frame,
	float dt, 
	const xtime::TimeSlice &time
) {
	if (!m_enabled)
		return;

	Entity::Ref instigator = m_instigator.lock();
	if (instigator) {
		CheckExit(*instigator);
		return;
	}

	if (m_occupied) {
		// instigator died.
		m_instigator.reset();
		m_occupied = false;
		DoExit();
		return;
	}

	CheckEnter();
}

void E_TouchTrigger::CheckEnter() {
	
	Entity::Ref instigator;

	for (int i = 0; i < m_numBrushes; ++i) {
		instigator = world->FirstEntityTouchingBrush(m_classbits, m_firstBrush + i);
		if (instigator)
			break;
	}

	if (instigator) {
		m_instigator = instigator;
		DoEnter();
	}
}

void E_TouchTrigger::CheckExit(const Entity &instigator) {

	bool touching = false;

	for (int i = 0; i < m_numBrushes; ++i) {
		touching = world->EntityTouchesBrush(instigator, m_firstBrush + i);
		if (touching)
			break;
	}

	if (!touching)
		DoExit();
}

void E_TouchTrigger::DoExit() {
	m_occupied = false;
	m_instigator.reset();
	if (!m_exit.empty)
		world->PostEvent(m_exit.c_str);
}

void E_TouchTrigger::DoEnter() {
	m_occupied = true;
	if (!m_enter.empty)
		world->PostEvent(m_enter.c_str);
}

bool E_TouchTrigger::HandleEvent(const Event::Ref &event) {
	if (Entity::HandleEvent(event))
		return true;

	const String kEnable(CStr("enable"));
	const String kDisable(CStr("disable"));
	const String kCmd(CStr(event->cmd));

	if (kCmd == kEnable) {
		if (!m_enabled) {
			if (!m_enter.empty || !m_exit.empty)
				m_enabled = true;
		}

		return true;
	}

	if (kCmd == kDisable) {
		if (m_enabled) {
			m_enabled = false;
			if (m_occupied)
				DoExit();
		}
		return true;
	}

	return false;
}

void E_TouchTrigger::PushCallTable(lua_State *L) {
	Entity::PushCallTable(L);
	LUART_REGISTER_GETSET(L, TouchClassBits);
}

ENT_GETSET(E_TouchTrigger, TouchClassBits, int, m_classbits);

} // world
