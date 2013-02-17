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

	enum DrawMode {
		kDrawMode_Rect,
		kDrawMode_Circle
	};

	MatWidget();
	MatWidget(const Rect &r);
	virtual ~MatWidget() {}

	bool BindMaterial(const pkg::AssetRef &m);

	RAD_DECLARE_PROPERTY(MatWidget, drawMode, DrawMode, DrawMode);
	
	void FillCircleTo(float percent, float time);

protected:

	virtual void OnDraw(const Rect *clip);
	virtual void OnTick(float time, float dt);
	virtual void AddedToRoot();
	virtual void PushCallTable(lua_State *L);
	virtual void CreateFromTable(lua_State *L);

private:

	friend class Root;

	void Init();

	RAD_DECLARE_GET(drawMode, DrawMode) {
		return (DrawMode)m_drawMode;
	}

	RAD_DECLARE_SET(drawMode, DrawMode) {
		m_drawMode = value;
	}

	UIW_DECL_SET(Material);
	UIW_DECL_GETSET(DrawMode);

	static void TickMaterials();
	static void AddMaterial(const pkg::AssetRef &asset);
	static int lua_FillCircleTo(lua_State *L);

	pkg::AssetRef m_asset;
	asset::MaterialParser *m_material;
	asset::MaterialLoader *m_loader;
	int m_drawMode;
	float m_circle[3];
	float m_circleTime[2];

};

} // ui

#include <Runtime/PopPack.h>