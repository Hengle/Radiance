// D_Material.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../../Assets/MaterialParser.h"
#include "D_Asset.h"
#include <Runtime/PushPack.h>

namespace r {
class Material;
} // r

namespace world {

class RADENG_CLASS D_Material : public D_Asset {
public:
	typedef boost::shared_ptr<D_Material> Ref;

	static Ref New(const pkg::AssetRef &asset);

	RAD_DECLARE_READONLY_PROPERTY(D_Material, material, r::Material*);
	RAD_DECLARE_READONLY_PROPERTY(D_Material, assetId, int);

protected:

	virtual void PushElements(lua_State *L);

private:

	D_Material(const pkg::AssetRef &asset);

	RAD_DECLARE_GET(material, r::Material*) { 
		return m_parser ? m_parser->material.get() : 0; 
	}

	RAD_DECLARE_GET(assetId, int) {
		return m_asset->id;
	}

	static int lua_Dimensions(lua_State *L);
	static int lua_SetState(lua_State *L);
	static int lua_BlendTo(lua_State *L);
	static int lua_Name(lua_State *L);
	
	pkg::AssetRef m_asset;
	asset::MaterialParser::Ref m_parser;
};

} // world

#include <Runtime/PopPack.h>
