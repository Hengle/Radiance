// UIMatWidget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "UIMatWidget.h"
#include "../World/Lua/D_Material.h"

using world::D_Material;

namespace ui {


MatWidget::MatWidget()
{
}

MatWidget::MatWidget(const Rect &r) : Widget(r)
{
}

bool MatWidget::BindMaterial(const pkg::AssetRef &m)
{
	if (m && m_asset && m->id == m_asset->id)
		return true;
	if (!m)
	{
		m_asset.reset();
		m_material.reset();
		m_loader.reset();
		return true;
	}

	if (m->type != asset::AT_Material)
		return false;
	m_asset = m;
	m_material = asset::MaterialParser::Cast(m);
	m_loader = asset::MaterialLoader::Cast(m);

	if (!m_material || !m_material->valid || !m_loader)
	{
		m_asset.reset();
		m_material.reset();
		m_loader.reset();
		return false;
	}

	Root::Ref r = root;
	if (r && m_asset)
		r->AddTickMaterial(m_asset);

	return true;
}

void MatWidget::PushCallTable(lua_State *L)
{
	Widget::PushCallTable(L);
	LUART_REGISTER_SET(L, Material);
}

void MatWidget::CreateFromTable(lua_State *L)
{
	Widget::CreateFromTable(L);
	lua_getfield(L, -1, "material");
	D_Material::Ref m = lua::SharedPtr::Get<D_Material>(L, "D_Material", -1, false);
	lua_pop(L, 1);
	if (m)
		BindMaterial(m->asset);
}

void MatWidget::OnDraw()
{
	if (!m_asset)
		return;

	rbDraw->DrawRect(
		this->screenRect,
		*m_material->material.get(),
		m_loader,
		true,
		this->color
	);
}

void MatWidget::AddedToRoot()
{
	Root::Ref r = this->root;
	if (r && m_asset)
		r->AddTickMaterial(m_asset);
}

int MatWidget::LUART_SETFN(Material)(lua_State *L)
{
	Ref w = GetRef<MatWidget>(L, "MatWidget", 1, true);

	D_Material::Ref m = lua::SharedPtr::Get<D_Material>(L, "D_Material", 2, true);
	if (m)
		w->BindMaterial(m->asset);

	return 0;
}

} // ui
