// UIMatWidget.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "UIWidget.h"
#include "../Assets/MaterialParser.h"

namespace ui {

class RADENG_CLASS MatWidget : public Widget
{
public:
	typedef boost::shared_ptr<MatWidget> Ref;
	typedef boost::weak_ptr<MatWidget> WRef;

	MatWidget();
	MatWidget(const Rect &r);

	bool BindMaterial(const pkg::AssetRef &m);

protected:

	virtual void OnDraw();
	virtual void AddedToRoot();
	virtual void PushCallTable(lua_State *L);
	virtual void CreateFromTable(lua_State *L);

private:

	friend class Root;

	UIW_DECL_SET(Material);

	static void TickMaterials();
	static void AddMaterial(const pkg::AssetRef &asset);

	pkg::AssetRef m_asset;
	asset::MaterialParser::Ref m_material;
	asset::MaterialLoader::Ref m_loader;
};

} // ui
