// D_Sound.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../../Sound/SoundDef.h"
#include "D_Asset.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_Sound : public D_Asset
{
public:
	typedef boost::shared_ptr<D_Sound> Ref;

	static Ref New(const pkg::AssetRef &asset, const SoundRef &sound);

	RAD_DECLARE_READONLY_PROPERTY(D_Sound, sound, const SoundRef&);

protected:

	virtual void PushElements(lua_State *L);

private:

	RAD_DECLARE_GET(sound, const SoundRef&) { return m_sound; }

	D_Sound(const pkg::AssetRef &asset, const SoundRef &sound);

	static int lua_SetLoop(lua_State *L);
	static int lua_SetInnerAngle(lua_State *L);
	static int lua_SetOuterAngle(lua_State *L);
	static int lua_SetOuterVolume(lua_State *L);
	static int lua_SetMinMaxVolume(lua_State *L);
	static int lua_SetPitch(lua_State *L);
	static int lua_SetRolloff(lua_State *L);
	static int lua_SetRefDistance(lua_State *L);
	static int lua_SetMaxDistance(lua_State *L);
	static int lua_Volume(lua_State *L);
	static int lua_Playing(lua_State*L);
	static int lua_Paused(lua_State *L);
	static int lua_FadeVolume(lua_State *L);
	static int lua_Play(lua_State *L);
	static int lua_Pause(lua_State *L);
	static int lua_Rewind(lua_State *L);
	static int lua_Stop(lua_State *L);

	SoundRef m_sound;
};

} // world

#include <Runtime/PopPack.h>
