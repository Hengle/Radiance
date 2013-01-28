// D_Material.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "D_Material.h"
#include "../../Assets/TextureParser.h"
#include "../../Renderer/Material.h"
#include <algorithm>

namespace world {

D_Material::Ref D_Material::New(const pkg::AssetRef &asset) {
	return Ref(new (ZWorld) D_Material(asset));
}

D_Material::D_Material(const pkg::AssetRef &asset) : D_Asset(asset),
m_asset(asset) {
	m_parser = asset::MaterialParser::Cast(asset);
}

void D_Material::PushElements(lua_State *L) {
	D_Asset::PushElements(L);
	lua_pushcfunction(L, lua_Dimensions);
	lua_setfield(L, -2, "Dimensions");
	lua_pushcfunction(L, lua_BlendTo);
	lua_setfield(L, -2, "BlendTo");
	lua_pushcfunction(L, lua_SetState);
	lua_setfield(L, -2, "SetState");
	lua_pushcfunction(L, lua_Name);
	lua_setfield(L, -2, "Name");
}

int D_Material::lua_Dimensions(lua_State *L) {
	D_Material::Ref self = lua::SharedPtr::Get<D_Material>(L, "D_Material", 1, true);

	// this function is not fast:

	asset::MaterialLoader::Ref loader = asset::MaterialLoader::Cast(self->m_asset);
	if (!loader || !loader->info)
		return 0;

	int mx = 0;
	int my = 0;

	// find the largest texture.
	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		asset::TextureParser::Ref t = asset::TextureParser::Cast(loader->Texture(i));
		if (!t)
			break;
		if (!t->headerValid)
			break;
		mx = std::max(mx, t->header->width);
		my = std::max(my, t->header->height);
	}
	
	if (mx > 0 && my > 0) {
		lua_createtable(L, 2, 0);
		lua_pushinteger(L, 1);
		lua_pushinteger(L, mx);
		lua_settable(L, -3);
		lua_pushinteger(L, 2); 
		lua_pushinteger(L, my);
		lua_settable(L, -3);
		return 1;
	}

	return 0;
}

int D_Material::lua_BlendTo(lua_State *L) {
	D_Material::Ref self = lua::SharedPtr::Get<D_Material>(L, "D_Material", 1, true);
	self->material->BlendTo(
		lua::Marshal<Vec4>::Get(L, 2, true),
		(float)luaL_checknumber(L, 3)
	);
	return 0;
}

int D_Material::lua_SetState(lua_State *L) {
	D_Material::Ref self = lua::SharedPtr::Get<D_Material>(L, "D_Material", 1, true);
	
	const char *state = luaL_checkstring(L, 2);
	if (!strcmp(state, "Color0.A")) {
		Vec4 c = lua::Marshal<Vec4>::Get(L, 3, true);
		self->material->SetColor(r::Material::kColor0, r::Material::kColorA, &c[0]);
		return 0;
	} else if(!strcmp(state, "Color0.B")) {
		Vec4 c = lua::Marshal<Vec4>::Get(L, 3, true);
		self->material->SetColor(r::Material::kColor0, r::Material::kColorB, &c[0]);
		return 0;
	} else if(!strcmp(state, "Time")) {
		self->material->time = (float)luaL_checknumber(L, 3);
		return 0;
	} else if(!strcmp(state, "TimingMode")) {
		self->material->timingMode = (r::Material::TimingMode)luaL_checkinteger(L, 3);
		return 0;
	}

	String base;
	String s;
	const char *waves[] = { "Rotate", "Turb", "Scale", "Shift", "Scroll", 0 };

	const String kAmplitude(CStr("Amplitude"));
	const String kBase(CStr("Base"));
	const String kFrequency(CStr("Frequency"));
	const String kPhase(CStr("Phase"));
	const String kType(CStr("Type"));

	for (int i = 0; i < 6; ++i) {
		for (int k = 0;; ++k) {
			if (!waves[k])
				break;

			base.Printf("Texture%d.tcMod.%s.", i+1, waves[k]);

			s = base + kAmplitude;
			if (s == state) {
				Vec2 c = lua::Marshal<Vec2>::Get(L, 3, true);
				WaveAnim &s = self->material->Wave(i, r::Material::kTCMod_Rotate+k, r::Material::kTexCoord_S);
				WaveAnim &t = self->material->Wave(i, r::Material::kTCMod_Rotate+k, r::Material::kTexCoord_T);

				s.amplitude = (float)c[0];
				t.amplitude = (float)c[1];
				return 0;
			}

			s = base + kBase;
			if (s == state) {
				Vec2 c = lua::Marshal<Vec2>::Get(L, 3, true);
				WaveAnim &s = self->material->Wave(i, r::Material::kTCMod_Rotate+k, r::Material::kTexCoord_S);
				WaveAnim &t = self->material->Wave(i, r::Material::kTCMod_Rotate+k, r::Material::kTexCoord_T);

				s.base = (float)c[0];
				t.base = (float)c[1];
				return 0;
			}

			s = base + kFrequency;
			if (s == state) {
				Vec2 c = lua::Marshal<Vec2>::Get(L, 3, true);
				WaveAnim &s = self->material->Wave(i, r::Material::kTCMod_Rotate+k, r::Material::kTexCoord_S);
				WaveAnim &t = self->material->Wave(i, r::Material::kTCMod_Rotate+k, r::Material::kTexCoord_T);

				s.freq = (float)c[0];
				t.freq = (float)c[1];
				return 0;
			}

			s = base + kPhase;
			if (s == state) {
				Vec2 c = lua::Marshal<Vec2>::Get(L, 3, true);
				WaveAnim &s = self->material->Wave(i, r::Material::kTCMod_Rotate+k, r::Material::kTexCoord_S);
				WaveAnim &t = self->material->Wave(i, r::Material::kTCMod_Rotate+k, r::Material::kTexCoord_T);

				s.phase = (float)c[0];
				t.phase = (float)c[1];
				return 0;
			}

			s = base + kType;
			if (s == state) {
				const char *type = luaL_checkstring(L, 3);

				WaveAnim &s = self->material->Wave(i, r::Material::kTCMod_Rotate+k, r::Material::kTexCoord_S);
				WaveAnim &t = self->material->Wave(i, r::Material::kTCMod_Rotate+k, r::Material::kTexCoord_T);

				if (!strcmp(type, "Identity")) {
					s.type = WaveAnim::T_Identity;
					t.type = WaveAnim::T_Identity;
				} else if (!strcmp(type, "Constant")) {
					s.type = WaveAnim::T_Constant;
					t.type = WaveAnim::T_Constant;
				} else if (!strcmp(type, "Square")) {
					s.type = WaveAnim::T_Square;
					t.type = WaveAnim::T_Square;
				} else if (!strcmp(type, "Sawtooth")) {
					s.type = WaveAnim::T_Sawtooth;
					t.type = WaveAnim::T_Sawtooth;
				} else if (!strcmp(type, "Triangle")) {
					s.type = WaveAnim::T_Triangle;
					t.type = WaveAnim::T_Triangle;
				} else if (!strcmp(type, "Noise")) {
					s.type = WaveAnim::T_Noise;
					t.type = WaveAnim::T_Noise;
				} else {
					luaL_argerror(L, 3, "Invalid material wave type!");
				}

				return 0;
			}
		}
	}

	luaL_argerror(L, 2, "Invalid material state!");
	return 0;
}

int D_Material::lua_Name(lua_State *L) {
	D_Material::Ref self = lua::SharedPtr::Get<D_Material>(L, "D_Material", 1, true);
	lua_pushstring(L, self->m_asset->path);
	return 1;
}

} // world
