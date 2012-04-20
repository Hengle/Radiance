// D_Material.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "D_Material.h"
#include "../../Renderer/Material.h"

namespace world {

D_Material::Ref D_Material::New(const pkg::AssetRef &asset)
{
	return Ref(new (ZWorld) D_Material(asset));
}

D_Material::D_Material(const pkg::AssetRef &asset) : D_Asset(asset),
m_asset(asset)
{
	m_parser = asset::MaterialParser::Cast(asset);
}

void D_Material::PushElements(lua_State *L)
{
	D_Asset::PushElements(L);
	lua_pushcfunction(L, lua_SetState);
	lua_setfield(L, -2, "SetState");
}

int D_Material::lua_SetState(lua_State *L)
{
	D_Material::Ref self = lua::SharedPtr::Get<D_Material>(L, "D_Material", 1, true);
	
	const char *state = luaL_checkstring(L, 2);
	if (!strcmp(state, "Color0.A"))
	{
		Vec4 c = lua::Marshal<Vec4>::Get(L, 3, true);
		self->material->SetColor(r::Material::Color0, r::Material::ColorA, &c[0]);
		return 0;
	}
	else if(!strcmp(state, "Color0.B"))
	{
		Vec4 c = lua::Marshal<Vec4>::Get(L, 3, true);
		self->material->SetColor(r::Material::Color0, r::Material::ColorB, &c[0]);
		return 0;
	}
	else if(!strcmp(state, "Time"))
	{
		self->material->time = (float)luaL_checknumber(L, 3);
		return 0;
	}
	else if(!strcmp(state, "TimingMode"))
	{
		self->material->timingMode = (r::Material::TimingMode)luaL_checkinteger(L, 3);
		return 0;
	}

	String base;
	String s;
	const char *waves[] = { "Rotate", "Turb", "Scale", "Shift", "Scroll", 0 };

	for (int i = 0; i < 6; ++i)
	{
		for (int k = 0;; ++k)
		{
			if (!waves[k])
				break;

			base.format("Texture%d.tcMod.%s.", i+1, waves[k]);

			s = base + "Amplitude";
			if (s == state)
			{
				Vec2 c = lua::Marshal<Vec2>::Get(L, 3, true);
				WaveAnim &s = self->material->Wave(r::MTS_Texture, i, r::Material::TcMod_Rotate+k, r::Material::S);
				WaveAnim &t = self->material->Wave(r::MTS_Texture, i, r::Material::TcMod_Rotate+k, r::Material::T);

				s.amplitude = (float)c[0];
				t.amplitude = (float)c[1];
				return 0;
			}

			s = base + "Base";
			if (s == state)
			{
				Vec2 c = lua::Marshal<Vec2>::Get(L, 3, true);
				WaveAnim &s = self->material->Wave(r::MTS_Texture, i, r::Material::TcMod_Rotate+k, r::Material::S);
				WaveAnim &t = self->material->Wave(r::MTS_Texture, i, r::Material::TcMod_Rotate+k, r::Material::T);

				s.base = (float)c[0];
				t.base = (float)c[1];
				return 0;
			}

			s = base + "Frequency";
			if (s == state)
			{
				Vec2 c = lua::Marshal<Vec2>::Get(L, 3, true);
				WaveAnim &s = self->material->Wave(r::MTS_Texture, i, r::Material::TcMod_Rotate+k, r::Material::S);
				WaveAnim &t = self->material->Wave(r::MTS_Texture, i, r::Material::TcMod_Rotate+k, r::Material::T);

				s.freq = (float)c[0];
				t.freq = (float)c[1];
				return 0;
			}

			s = base + "Phase";
			if (s == state)
			{
				Vec2 c = lua::Marshal<Vec2>::Get(L, 3, true);
				WaveAnim &s = self->material->Wave(r::MTS_Texture, i, r::Material::TcMod_Rotate+k, r::Material::S);
				WaveAnim &t = self->material->Wave(r::MTS_Texture, i, r::Material::TcMod_Rotate+k, r::Material::T);

				s.phase = (float)c[0];
				t.phase = (float)c[1];
				return 0;
			}

			s = base + "Type";
			if (s == state)
			{
				const char *type = luaL_checkstring(L, 3);

				WaveAnim &s = self->material->Wave(r::MTS_Texture, i, r::Material::TcMod_Rotate+k, r::Material::S);
				WaveAnim &t = self->material->Wave(r::MTS_Texture, i, r::Material::TcMod_Rotate+k, r::Material::T);

				if (!strcmp(type, "Identity"))
				{
					s.type = WaveAnim::T_Identity;
					t.type = WaveAnim::T_Identity;
				}
				else if (!strcmp(type, "Constant"))
				{
					s.type = WaveAnim::T_Constant;
					t.type = WaveAnim::T_Constant;
				}
				else if (!strcmp(type, "Square"))
				{
					s.type = WaveAnim::T_Square;
					t.type = WaveAnim::T_Square;
				}
				else if (!strcmp(type, "Sawtooth"))
				{
					s.type = WaveAnim::T_Sawtooth;
					t.type = WaveAnim::T_Sawtooth;
				}
				else if (!strcmp(type, "Triangle"))
				{
					s.type = WaveAnim::T_Triangle;
					t.type = WaveAnim::T_Triangle;
				}
				else if (!strcmp(type, "Noise"))
				{
					s.type = WaveAnim::T_Noise;
					t.type = WaveAnim::T_Noise;
				}
				else
				{
					luaL_argerror(L, 3, "Invalid material wave type!");
				}

				return 0;
			}
		}
	}

	luaL_argerror(L, 2, "Invalid material state!");
	return 0;
}

} // world
