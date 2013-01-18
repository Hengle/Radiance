/*! \file GameCVars.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once
#include "../Types.h"
#include "../CVars.h"
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/PushPack.h>

class Game;
class GameCVars {
public:
	CVarBool r_showtris;
	CVarBool r_showportals;

	void AddLuaVar(const CVar::Ref &cvar) {
		m_vec.push_back(cvar);
		m_set.insert(cvar.get());
	}

	bool IsLuaVar(CVar *cvar) {
		return m_set.find(cvar) != m_set.end();
	}

private:
	typedef zone_vector<CVar::Ref, ZEngineT>::type CVarVec;
	typedef zone_set<CVar*, ZEngineT>::type CVarSet;
	friend class Game;

	GameCVars(Game &game, CVarZone &zone);

	CVarVec m_vec;
	CVarSet m_set;
};

#include <Runtime/PopPack.h>
