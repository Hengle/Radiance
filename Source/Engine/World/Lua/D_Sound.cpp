// D_Sound.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "D_Sound.h"
#include "../../Sound/Sound.h"

namespace world {

D_Sound::Ref D_Sound::New(const pkg::AssetRef &asset, const Sound::Ref &sound)
{
	return Ref(new (ZWorld) D_Sound(asset, sound));
}

D_Sound::D_Sound(const pkg::AssetRef &asset, const Sound::Ref &sound) : D_Asset(asset),
m_sound(sound)
{
}

void D_Sound::PushElements(lua_State *L)
{
	D_Asset::PushElements(L);
	lua_pushcfunction(L, lua_SetLoop);
	lua_setfield(L, -2, "SetLoop");
	lua_pushcfunction(L, lua_SetInnerAngle);
	lua_setfield(L, -2, "SetInnerAngle");
	lua_pushcfunction(L, lua_SetOuterAngle);
	lua_setfield(L, -2, "SetOuterAngle");
	lua_pushcfunction(L, lua_SetOuterVolume);
	lua_setfield(L, -2, "SetOuterVolume");
	lua_pushcfunction(L, lua_SetMinMaxVolume);
	lua_setfield(L, -2, "SetMinMaxVolume");
	lua_pushcfunction(L, lua_SetPitch);
	lua_setfield(L, -2, "SetPitch");
	lua_pushcfunction(L, lua_SetRolloff);
	lua_setfield(L, -2, "SetRolloff");
	lua_pushcfunction(L, lua_SetRefDistance);
	lua_setfield(L, -2, "SetRefDistance");
	lua_pushcfunction(L, lua_SetMaxDistance);
	lua_setfield(L, -2, "SetMaxDistance");
	lua_pushcfunction(L, lua_Volume);
	lua_setfield(L, -2, "Volume");
	lua_pushcfunction(L, lua_Playing);
	lua_setfield(L, -2, "Playing");
	lua_pushcfunction(L, lua_Paused);
	lua_setfield(L, -2, "Paused");
	lua_pushcfunction(L, lua_FadeVolume);
	lua_setfield(L, -2, "FadeVolume");
	lua_pushcfunction(L, lua_Play);
	lua_setfield(L, -2, "Play");
	lua_pushcfunction(L, lua_Pause);
	lua_setfield(L, -2, "Pause");
	lua_pushcfunction(L, lua_Rewind);
	lua_setfield(L, -2, "Rewind");
	lua_pushcfunction(L, lua_Stop);
	lua_setfield(L, -2, "Stop");
}

int D_Sound::lua_SetLoop(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->loop = lua_toboolean(L, 2) ?  true : false;
	return 0;
}

int D_Sound::lua_SetInnerAngle(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->innerAngle = (float)luaL_checknumber(L, 2);
	return 0;
}

int D_Sound::lua_SetOuterAngle(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->outerAngle = (float)luaL_checknumber(L, 2);
	return 0;
}

int D_Sound::lua_SetOuterVolume(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->outerVolume = (float)luaL_checknumber(L, 2);
	return 0;
}

int D_Sound::lua_SetMinMaxVolume(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->minVolume = (float)luaL_checknumber(L, 2);
	self->sound->maxVolume = (float)luaL_checknumber(L, 3);
	return 0;
}

int D_Sound::lua_SetPitch(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->pitch = (float)luaL_checknumber(L, 2);
	return 0;
}

int D_Sound::lua_SetRolloff(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->rolloff = (float)luaL_checknumber(L, 2);
	return 0;
}

int D_Sound::lua_SetRefDistance(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->refDistance = (float)luaL_checknumber(L, 2);
	return 0;
}

int D_Sound::lua_SetMaxDistance(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->maxDistance = (float)luaL_checknumber(L, 2);
	return 0;
}

int D_Sound::lua_Volume(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	lua_pushnumber(L, (lua_Number)self->sound->volume.get());
	return 1;
}

int D_Sound::lua_Playing(lua_State*L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	lua_pushboolean(L, self->sound->playing ? 1 : 0);
	return 1;
}

int D_Sound::lua_Paused(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	lua_pushboolean(L, self->sound->paused ? 1 : 0);
	return 1;
}

int D_Sound::lua_FadeVolume(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->FadeVolume(
		(float)luaL_checknumber(L, 2),
		(float)luaL_checknumber(L, 3),
		lua_toboolean(L, 4) ? true : false // fade out and stop.
	);
	return 0;
}

int D_Sound::lua_Play(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	int c = (int)luaL_checkinteger(L, 2);
	if (c < SC_First || c >= SC_Max)
		luaL_error(L, "Invalid sound channel number %d", c);

	bool r = self->sound->Play(
		(SoundChannel)c,
		(int)luaL_checkinteger(L, 3)
	);

	lua_pushboolean(L, r ? 1 : 0);
	return 1;
}

int D_Sound::lua_Pause(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->Pause(lua_toboolean(L, 2) ? true : false);
	return 0;
}

int D_Sound::lua_Rewind(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->Rewind();
	return 0;
}

int D_Sound::lua_Stop(lua_State *L)
{
	Ref self = Get<D_Sound>(L, "D_Sound", 1, true);
	self->sound->Stop();
	return 0;
}

} // world
