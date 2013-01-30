// D_Typeface.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../../Assets/TypefaceParser.h"
#include "D_Asset.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_Typeface : public D_Asset
{
public:
	typedef boost::shared_ptr<D_Typeface> Ref;

	static Ref New(const pkg::AssetRef &typeface);

	RAD_DECLARE_READONLY_PROPERTY(D_Typeface, asset, const pkg::AssetRef&);
	RAD_DECLARE_READONLY_PROPERTY(D_Typeface, typeface, const asset::TypefaceParser::Ref&);

protected:

	virtual void PushElements(lua_State *L);

private:

	D_Typeface(const pkg::AssetRef &asset);

	RAD_DECLARE_GET(asset, const pkg::AssetRef &) { return m_asset; }
	RAD_DECLARE_GET(typeface, const asset::TypefaceParser::Ref&) { return m_typeface; }

	static int lua_Size(lua_State *L);
	static int lua_StringDimensions(lua_State *L);
	static int lua_WordWrap(lua_State *L);
	static int lua_AscenderDescender(lua_State *L);
	static int lua_SplitStringAtSize(lua_State *L);

	pkg::AssetRef m_asset;
	asset::TypefaceParser::Ref m_typeface;
};

} // world

#include <Runtime/PopPack.h>
