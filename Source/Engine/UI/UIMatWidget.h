/*! \file UIMatWidget.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#pragma once

#include "UIWidget.h"
#include "../Assets/MaterialParser.h"
#include <Runtime/PushPack.h>

namespace ui {

class RADENG_CLASS MatWidget : public Widget {
public:
	typedef boost::shared_ptr<MatWidget> Ref;
	typedef boost::weak_ptr<MatWidget> WRef;

	MatWidget();
	MatWidget(const Rect &r);
	virtual ~MatWidget() {}

	bool BindMaterial(const pkg::AssetRef &m);

protected:

	virtual void OnDraw(const Rect *clip);
	virtual void AddedToRoot();
	virtual void PushCallTable(lua_State *L);
	virtual void CreateFromTable(lua_State *L);

private:

	friend class Root;

	UIW_DECL_SET(Material);

	static void TickMaterials();
	static void AddMaterial(const pkg::AssetRef &asset);

	pkg::AssetRef m_asset;
	asset::MaterialParser *m_material;
	asset::MaterialLoader *m_loader;
};

} // ui

#include <Runtime/PopPack.h>