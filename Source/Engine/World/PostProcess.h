// PostProcess.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "../Renderer/RendererDef.h"
#include "../Renderer/Material.h"
#include "../Assets/MaterialParser.h"
#include <Runtime/Container/ZoneMap.h>

namespace world {

class PostProcessEffect
{
public:
	typedef boost::shared_ptr<PostProcessEffect> Ref;
	typedef zone_map<int, Ref, r::ZRenderT>::type Map;

	PostProcessEffect();

	bool BindMaterial(const pkg::AssetRef &material);

	void Tick(float dt);
	void FadeTo(const Vec4 &color, float time);

	virtual void BindStates();

	RAD_DECLARE_PROPERTY(PostProcessEffect, enabled, bool, bool);
	RAD_DECLARE_PROPERTY(PostProcessEffect, srcScale, const Vec2&, const Vec2&);
	RAD_DECLARE_READONLY_PROPERTY(PostProcessEffect, color, const Vec4&);
	RAD_DECLARE_READONLY_PROPERTY(PostProcessEffect, material, r::Material*);
	RAD_DECLARE_READONLY_PROPERTY(PostProcessEffect, asset, const pkg::AssetRef&);
	RAD_DECLARE_READONLY_PROPERTY(PostProcessEffect, parser, asset::MaterialParser*);
	RAD_DECLARE_READONLY_PROPERTY(PostProcessEffect, loader, asset::MaterialLoader*);

private:

	RAD_DECLARE_GET(enabled, bool) { return m_enabled; }
	RAD_DECLARE_SET(enabled, bool) { m_enabled = value; }
	RAD_DECLARE_GET(srcScale, const Vec2&) { return m_srcScale; }
	RAD_DECLARE_SET(srcScale, const Vec2&) { m_srcScale = value; }
	RAD_DECLARE_GET(color, const Vec4&) { return m_color[0]; }
	RAD_DECLARE_GET(material, r::Material*) { return m_parser->material; }
	RAD_DECLARE_GET(asset, const pkg::AssetRef&) { return m_asset; }
	RAD_DECLARE_GET(parser, asset::MaterialParser*) { return m_parser; }
	RAD_DECLARE_GET(loader, asset::MaterialLoader*) { return m_loader; }

	pkg::AssetRef m_asset;
	Vec4 m_color[3];
	Vec2 m_srcScale;
	asset::MaterialParser *m_parser;
	asset::MaterialLoader *m_loader;
	float m_time[2];
	bool m_enabled;
};

} // world
