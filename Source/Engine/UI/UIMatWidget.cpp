/*! \file UIMatWidget.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#include RADPCH
#include "UIMatWidget.h"
#include "../World/Lua/D_Material.h"

using world::D_Material;

namespace ui {


MatWidget::MatWidget() {
	Init();
}

MatWidget::MatWidget(const Rect &r) : Widget(r) {
	Init();
}

void MatWidget::Init() {
	m_material = 0;
	m_loader = 0;
	m_drawMode = kDrawMode_Rect;
	m_circle[0] = m_circle[1] = m_circle[2] = 1.f;
	m_circleTime[0] = m_circleTime[1] = 0.f;
}

bool MatWidget::BindMaterial(const pkg::AssetRef &m) {
	if (m && m_asset && m->id == m_asset->id)
		return true;
	if (!m) {
		m_asset.reset();
		m_material = 0;
		m_loader = 0;
		return true;
	}

	if (m->type != asset::AT_Material)
		return false;

	m_asset = m;
	m_material = asset::MaterialParser::Cast(m);
	m_loader = asset::MaterialLoader::Cast(m);

	if (!m_material || !m_material->valid || !m_loader) {
		m_asset.reset();
		m_material = 0;
		m_loader = 0;
		return false;
	}

	Root::Ref r = root;
	if (r && m_asset)
		r->AddTickMaterial(m_asset);

	return true;
}

void MatWidget::FillCircleTo(float percent, float time) {
	if (time <= 0.f) {
		m_circle[0] = percent;
		m_circleTime[1] = 0.f;
	} else {
		m_circle[1] = m_circle[0];
		m_circle[2] = percent;
		m_circleTime[0] = 0.f;
		m_circleTime[1] = time;
	}
}

void MatWidget::PushCallTable(lua_State *L) {
	Widget::PushCallTable(L);
	LUART_REGISTER_SET(L, Material);
	LUART_REGISTER_GETSET(L, DrawMode);
	lua_pushcfunction(L, lua_FillCircleTo);
	lua_setfield(L, -2, "FillCircleTo");
}

void MatWidget::CreateFromTable(lua_State *L) {
	Widget::CreateFromTable(L);
	lua_getfield(L, -1, "material");
	D_Material::Ref m = lua::SharedPtr::Get<D_Material>(L, "D_Material", -1, false);
	lua_pop(L, 1);
	if (m)
		BindMaterial(m->asset);

	lua_getfield(L, -1, "drawMode");
	if (!lua_isnil(L, -1)) {
		int x = (int)luaL_checkinteger(L, -1);
		if ((x != kDrawMode_Rect) && (x != kDrawMode_Circle))
			luaL_error(L, "invalid draw mode %d", x);
		m_drawMode = (DrawMode)x;
	}
	lua_pop(L, 1);
}

void MatWidget::OnTick(float time, float dt) {
	Widget::OnTick(time, dt);

	if (m_circleTime[1] > 0.f) {
		m_circleTime[0] += dt;

		if (m_circleTime[0] >= m_circleTime[1]) {
			m_circleTime[1] = 0.f;
			m_circle[0] = m_circle[2];
		} else {
			m_circle[0] = math::Lerp(m_circle[1], m_circle[2], m_circleTime[0] / m_circleTime[1]);
		}
	}
}

void MatWidget::OnDraw(const Rect *clip) {
	if (!m_asset)
		return;

	if (m_drawMode == kDrawMode_Rect) {
		rbDraw->DrawRect(
			this->screenRect,
			clip,
			this->zRotScreen,
			*m_material->material.get(),
			m_loader,
			true,
			this->blendedColor
		);
	} else {
		RAD_ASSERT(m_drawMode == kDrawMode_Circle);
		rbDraw->DrawCircle(
			this->screenRect,
			m_circle[0],
			clip,
			this->zRotScreen,
			*m_material->material.get(),
			m_loader,
			true,
			this->blendedColor
		);
	}
}

void MatWidget::AddedToRoot() {
	Widget::AddedToRoot();
	Root::Ref r = this->root;
	if (r && m_asset)
		r->AddTickMaterial(m_asset);
}

int MatWidget::LUART_SETFN(Material)(lua_State *L) {
	Ref w = GetRef<MatWidget>(L, "MatWidget", 1, true);

	D_Material::Ref m = lua::SharedPtr::Get<D_Material>(L, "D_Material", 2, true);
	if (m)
		w->BindMaterial(m->asset);

	return 0;
}

int MatWidget::lua_FillCircleTo(lua_State *L) {
	Ref w = GetRef<MatWidget>(L, "MatWidget", 1, true);
	w->FillCircleTo(
		(float)luaL_checknumber(L, 2),
		(float)luaL_checknumber(L, 3)
	);
	return 0;
}

UIW_GETSET(MatWidget, DrawMode, int, m_drawMode);

} // ui
