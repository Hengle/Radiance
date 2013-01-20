// UIWidget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "UIWidget.h"
#include "../COut.h"

#define UIWIDGETS "@uiw"

namespace ui {

RAD_ZONE_DEF(RADENG_API, ZUI, "UI", ZEngine);
enum { MaxWidgets = 256 };

///////////////////////////////////////////////////////////////////////////////

Root::Ref Root::New(lua_State *L)
{
	Ref r(new (ZUI) Root());

	if (L)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, UIWIDGETS);
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_createtable(L, MaxWidgets, 0);
			lua_setfield(L, LUA_REGISTRYINDEX, UIWIDGETS);
		}
		lua_pop(L, 1);
	}

	return r;
}

Root::Root()
{
	m_rbdraw = RBDraw::New();
	m_srcvp[0] = m_srcvp[1] = m_dstvp[0] = m_dstvp[1] = 0;
	m_srcvp[2] = m_dstvp[2] = 1024;
	m_srcvp[3] = m_dstvp[3] = 768;
	m_mapping[0] = m_mapping[1] = 1.f;
}

Root::~Root()
{
	for (WidgetMap::const_iterator it = m_layers.begin(); it != m_layers.end(); ++it)
	{
		it->second->m_gc = true;
	}
}

void Root::SetRootWidget(int layer, const WidgetRef &root)
{
	if (root)
	{
		m_layers[layer] = root;
		root->m_root = shared_from_this();
		root->AddedToRoot();
	}
	else
	{
		m_layers.erase(layer);
	}
}

WidgetRef Root::RootWidget(int layer)
{
	WidgetMap::const_iterator it = m_layers.find(layer);
	if (it == m_layers.end())
		return WidgetRef();
	return it->second;
}

void Root::Tick(float time, float dt, bool tickMaterials)
{
	if (tickMaterials)
	{
		for (pkg::AssetIdMap::const_iterator it = m_materials.begin(); it != m_materials.end(); ++it)
		{
			asset::MaterialParser::Ref parser = asset::MaterialParser::Cast(it->second);
			if (parser && parser->valid)
				parser->material->Sample(time, dt);
		}
	}

	for (WidgetMap::const_iterator it = m_layers.begin(); it != m_layers.end(); ++it)
	{
		it->second->Tick(time, dt);
	}
}

void Root::Draw(bool children)
{
	if (!m_layers.empty())
	{
		m_rbdraw->SetViewport(
			m_srcvp,
			m_dstvp
		);
	}

	for (WidgetMap::const_iterator it = m_layers.begin(); it != m_layers.end(); ++it)
	{
		it->second->Draw(children);
	}
}

bool Root::HandleInputEvent(const InputEvent &_e, const TouchState *touch, const InputState &is)
{
	InputEvent e = Map(_e);

	if (e.IsKeyboard())
	{
		return m_focus ? m_focus->HandleInputEvent(e, touch, is) : false;
	}

	if (m_capture)
	{
		m_capture->HandleInputEvent(e, touch, is);
		return true;
	}
	else
	{
		for (WidgetMap::const_reverse_iterator it = m_layers.rbegin(); it != m_layers.rend(); ++it)
		{
			if (it->second->HandleInputEvent(e, touch, is))
				return true;
		}
	}

	return false;
}

bool Root::HandleInputGesture(const InputGesture &_g, const TouchState &touch, const InputState &is)
{
	InputGesture g = Map(_g);

	if (m_capture)
	{
		m_capture->HandleInputGesture(g, touch, is);
		return true;
	}
	else
	{
		for (WidgetMap::const_reverse_iterator it = m_layers.rbegin(); it != m_layers.rend(); ++it)
		{
			if (it->second->HandleInputGesture(g, touch, is))
				return true;
		}
	}

	return false;
}

void Root::SetSourceViewport(
	int vpx,
	int vpy,
	int vpw,
	int vph
)
{
	m_srcvp[0] = vpx;
	m_srcvp[1] = vpy;
	m_srcvp[2] = vpw;
	m_srcvp[3] = vph;

	m_mapping[0] = m_srcvp[2] / (float)m_dstvp[2];
	m_mapping[1] = m_srcvp[3] / (float)m_dstvp[3];
}

void Root::SetDestViewport(
	int vpx,
	int vpy,
	int vpw,
	int vph
)
{
	m_dstvp[0] = vpx;
	m_dstvp[1] = vpy;
	m_dstvp[2] = vpw;
	m_dstvp[3] = vph;

	m_mapping[0] = m_srcvp[2] / (float)m_dstvp[2];
	m_mapping[1] = m_srcvp[3] / (float)m_dstvp[3];
}

InputEvent Root::Map(const InputEvent &e)
{
	if (e.IsKeyboard())
		return e;
	InputEvent r(e);
	Vec2 v((float)r.data[0], (float)r.data[1]);
	v = Map(v);
	r.data[0] = (int)v[0];
	r.data[1] = (int)v[1];
	return r;
}

InputGesture Root::Map(const InputGesture &g)
{
	InputGesture r(g);
	Vec2 mins((float)r.mins[0], (float)r.mins[1]);
	Vec2 maxs((float)r.maxs[0], (float)r.maxs[1]);
	Vec2 origin((float)r.origin[0], (float)r.origin[1]);

	mins = Map(mins);
	maxs = Map(maxs);
	origin = Map(origin);

	r.mins[0] = (int)mins[0];
	r.mins[1] = (int)mins[1];
	r.maxs[0] = (int)maxs[0];
	r.maxs[1] = (int)maxs[1];
	r.origin[0] = (int)origin[0];
	r.origin[1] = (int)origin[1];

	return r;
}

Vec2 Root::Map(const Vec2 &v)
{
	Vec2 x = v - Vec2((float)m_dstvp[0], (float)m_dstvp[1]);
	Vec2 y = x * m_mapping;
	return y + Vec2((float)m_srcvp[0], (float)m_srcvp[1]);
}

void Root::SetFocus(const Widget::Ref &w)
{
	if (m_focus && w && (m_focus.get() == w.get()))
		return;

	Widget::Ref x = m_focus;
	m_focus = w;

	if (x)
		x->OnFocusChanged(false);
	if (w)
		w->OnFocusChanged(true);
}

void Root::SetCapture(const Widget::Ref &w)
{
	m_capture = w;
}

void Root::AddTickMaterial(const pkg::Asset::Ref &ref)
{
	if (ref)
		m_materials[ref->id] = ref;
}

///////////////////////////////////////////////////////////////////////////////

int Widget::s_id(0);
int Widget::NextWidgetId(lua_State *L)
{
	if (L)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, UIWIDGETS);
		RAD_ASSERT(!lua_isnil(L, -1));

		int i;
		for (i = 0; i < MaxWidgets; ++i)
		{
			int id = (s_id+i)&(MaxWidgets-1);
			lua_pushinteger(L, id);
			lua_gettable(L, -2);
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				break;
			}
		}

		lua_pop(L, 1);
		RAD_VERIFY_MSG(i<MaxWidgets, "MaxWidgets");
		i = (s_id+i)&(MaxWidgets-1);
		s_id = (i+1)&(MaxWidgets-1);
		return i;
	}

	return s_id++;
}

Widget::Widget() : 
L(0), 
m_id(-1), 
m_visible(true), 
m_valign(VA_Center),
m_halign(HA_Center),
m_gc(false),
m_tick(false),
m_capture(false)
{
	m_fadeTime[0] = 0.f;
	m_fadeTime[1] = 0.f;

	for (int i = 0; i < 3; ++i)
		m_color[i] = Vec4(1, 1, 1, 1);

	m_scaleTime[0] = Vec2::Zero;
	m_scaleTime[1] = Vec2::Zero;
	m_scale[0] = m_scale[1] = m_scale[2] = Vec2(1, 1);

	m_moveTime[0] = Vec2::Zero;
	m_moveTime[1] = Vec2::Zero;
	m_move[0] = m_move[1] = Vec2::Zero;
}

Widget::Widget(const Rect &r) : L(0), m_rect(r), m_id(-1), m_gc(false)
{
	m_fadeTime[0] = 0.f;
	m_fadeTime[1] = 0.f;

	for (int i = 0; i < 3; ++i)
		m_color[i] = Vec4(1, 1, 1, 1);
}

Widget::~Widget()
{
	if (L)
		Unmap(L);
}

bool Widget::Spawn(lua_State *_L, lua_State *Lco, bool luaError)
{
	L = _L;
	m_id = NextWidgetId(Lco);

	if (Lco)
	{
		CreateFromTable(Lco);

		lua_getfield(Lco, LUA_REGISTRYINDEX, UIWIDGETS);
		RAD_ASSERT(!lua_isnil(Lco, -1));
		lua_pushnumber(Lco, m_id);

		lua_createtable(Lco, 0, 10);
		PushCallTable(Lco);
		lua_pushinteger(Lco, m_id);
		lua_setfield(Lco, -2, "id");
		lua::SharedPtr::Push(Lco);
		lua_setfield(Lco, -2, "@sp");

		// pull tick function forward
		lua_getfield(Lco, -4, "tick");
		if (lua_isfunction(Lco, -1))
		{
			m_tick = true;
			lua_setfield(Lco, -2, "tick");
		}
		else
		{
			lua_pop(Lco, 1);
		}

		lua_settable(Lco, -3);
		lua_pop(Lco, 1);
	}

	return true;
}

void Widget::Unmap(lua_State *L)
{
	if (!m_gc)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, UIWIDGETS);
		RAD_ASSERT(!lua_isnil(L, -1));
		lua_pushnumber(L, m_id);
		lua_pushnil(L);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}

	for (Vec::const_iterator it = m_children.begin(); it != m_children.end(); ++it)
	{
		(*it)->m_gc = m_gc;
	}
}

void Widget::PushCallTable(lua_State *L)
{
	lua_pushcfunction(L, lua_AddChild);
	lua_setfield(L, -2, "AddChild");
	lua_pushcfunction(L, lua_RemoveChild);
	lua_setfield(L, -2, "RemoveChild");
	lua_pushcfunction(L, lua_FadeTo);
	lua_setfield(L, -2, "FadeTo");
	lua_pushcfunction(L, lua_ScaleTo);
	lua_setfield(L, -2, "ScaleTo");
	lua_pushcfunction(L, lua_MoveTo);
	lua_setfield(L, -2, "MoveTo");
	lua_pushcfunction(L, lua_SetCapture);
	lua_setfield(L, -2, "SetCapture");
	LUART_REGISTER_GETSET(L, Rect);
	LUART_REGISTER_GET(L, ScreenRect);
	LUART_REGISTER_GETSET(L, Visible);
	LUART_REGISTER_GETSET(L, VAlign);
	LUART_REGISTER_GETSET(L, HAlign);
	LUART_REGISTER_SET(L, Tick);
}

void Widget::CreateFromTable(lua_State *L)
{
	lua_getfield(L, -1, "rect");
	if (!lua_isnil(L, -1))
		m_rect = lua::Marshal<Rect>::Get(L, -1, false);
	lua_pop(L, 1);
	lua_getfield(L, -1, "visible");
	if (!lua_isnil(L, -1))
		m_visible = lua_toboolean(L, -1) ? true : false;
	lua_pop(L, 1);
	lua_getfield(L, -1, "valign");
	if (!lua_isnil(L, -1))
		m_valign = (VAlign)lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "halign");
	if (!lua_isnil(L, -1))
		m_halign = (HAlign)lua_tointeger(L, -1);
	lua_pop(L, 1);
}

bool Widget::PushFrame(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, UIWIDGETS);
	RAD_ASSERT(!lua_isnil(L, -1));
	lua_pushnumber(L, m_id);
	lua_gettable(L, -2);
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 2);
		return false;
	}

	lua_remove(L, -2);
	return true;
}

bool Widget::PushCall(lua_State *L, const char *name)
{
	if (!PushFrame(L))
		return false;

	lua_getfield(L, -1, name);
	if (lua_type(L, -1) != LUA_TFUNCTION)
	{
		lua_pop(L, 2);
		return false;
	}

	lua_pushvalue(L, -2); // move widget frame as first argument.
	lua_remove(L, -3);
	return true;
}

bool Widget::Call(lua_State *L, const char *context, int nargs, int nresults, int errfunc)
{
	if (lua_pcall(L, nargs, nresults, errfunc))
	{
		COut(C_Error) << "UIScriptError(" << context << "): " << lua_tostring(L, -1) << std::endl;
		lua_pop(L, 1);
		return false;
	}

	return true;
}

bool Widget::InBounds(const InputEvent &e)
{
	if (e.IsKeyboard())
		return true;
	return InBounds((float)e.data[0], (float)e.data[1]);
}

bool Widget::InBounds(const InputGesture &g)
{
	return InBounds((float)g.origin[0], (float)g.origin[1]);
}

bool Widget::InBounds(float x, float y)
{
	Rect r = screenRect;
	return (x >= r.x && x <= r.x+r.w) &&
		(y >= r.y && y <= r.y+r.h);
}

void Widget::AddChild(const Ref &widget)
{
	RAD_ASSERT(widget);
	RAD_ASSERT(m_root.lock());

	for (Vec::const_iterator it = m_children.begin(); it != m_children.end(); ++it)
	{
		if ((*it).get() == widget.get())
			return;
	}

	widget->m_root = m_root.lock();
	widget->m_parent = boost::static_pointer_cast<Widget>(shared_from_this());
	m_children.push_back(widget);
	widget->AddedToRoot();
}

void Widget::RemoveChild(const Ref &widget)
{
	RAD_ASSERT(widget);
	RAD_ASSERT(m_root.lock());

	for (Vec::iterator it = m_children.begin(); it != m_children.end(); ++it)
	{
		if ((*it).get() == widget.get())
		{
			widget->m_parent.reset();
			m_children.erase(it);
			widget->RemovedFromRoot();
			return;
		}
	}
}

Rect Widget::RAD_IMPLEMENT_GET(screenRect)
{
	return Map(m_rect);
}

Rect Widget::Map(const Rect &r) const
{
	Rect base(0, 0, 0, 0);

	Ref p = m_parent.lock();
	if (p)
		base = p->screenRect;
		
	float sw = r.w * m_scale[0][0];
	float sh = r.h * m_scale[0][1];
	float x, y;

	// figure out alignment
	switch (m_halign)
	{
	case HA_Center:
		x = r.x + ((r.w-sw)/2.f);
		break;
	case HA_Right:
		x = r.x+r.w-sw;
		break;
	default: // HA_Left
		x = r.x;
		break;
	}

	switch (m_valign)
	{
	case VA_Center:
		y = r.y + ((r.h-sh)/2.f);
		break;
	case VA_Bottom:
		y = r.y+r.h-sh;
		break;
	default: // VA_Top
		y = r.y;
		break;

	}

	return Rect(base.x+x, base.y+y, sw, sh);
}

void Widget::Tick(float time, float dt)
{
	if (m_fadeTime[1] > 0.f)
	{
		m_fadeTime[0] += dt;
		if (m_fadeTime[0] >= m_fadeTime[1])
		{
			m_fadeTime[1] = 0.f;
			m_color[0] = m_color[2];
		}
		else
		{
			m_color[0] = math::Lerp(m_color[1], m_color[2], m_fadeTime[0]/m_fadeTime[1]);
		}
	}

	for (int i = 0; i < 2; ++i)
	{
		if (m_scaleTime[1][i] > 0.f)
		{
			m_scaleTime[0][i] += dt;

			if (m_scaleTime[0][i] >= m_scaleTime[1][i])
			{
				m_scaleTime[1][i] = 0.f;
				m_scale[0][i] = m_scale[2][i];
			}
			else
			{
				m_scale[0][i] = math::Lerp(m_scale[1][i], m_scale[2][i], m_scaleTime[0][i]/m_scaleTime[1][i]);
			}
		}
	}

	for (int i = 0; i < 2; ++i)
	{
		if (m_moveTime[1][i] > 0.f)
		{
			m_moveTime[0][i] += dt;

			if (m_moveTime[0][i] >= m_moveTime[1][i])
			{
				m_moveTime[1][i] = 0.f;
				if (i == 0)
					m_rect.x = m_move[1][0];
				else
					m_rect.y = m_move[1][1];
			}
			else
			{
				float z = math::Lerp(m_move[0][i], m_move[1][i], m_moveTime[0][i]/m_moveTime[1][i]);
				if (i == 0)
					m_rect.x = z;
				else
					m_rect.y = z;
			}
		}
	}

	OnTick(time, dt);

	if (m_tick && L && PushCall(L, "tick"))
	{
		lua_pushnumber(L, dt);
		Call(L, "Widget::Tick", 2, 0, 0);
	}

	for (Vec::const_iterator it = m_children.begin(); it != m_children.end(); ++it)
	{
		const Ref &w = *it;
		w->Tick(time, dt);
	}
}

void Widget::FadeTo(const Vec4 &dst, float time)
{
	if (time <= 0.f)
	{
		for (int i = 0; i < 3; ++i)
			m_color[i] = dst;
		m_fadeTime[1] = 0.f;
	}
	else
	{
		m_fadeTime[0] = 0.f;
		m_fadeTime[1] = time;
		m_color[1] = m_color[0];
		m_color[2] = dst;
	}
}

void Widget::ScaleTo(const Vec2 &scale, const Vec2 &time)
{
	for (int i = 0; i < 2; ++i)
	{
		if (time[i] <= 0.f)
		{
			m_scale[0][i] = m_scale[1][i] = m_scale[2][i] = scale[i];
			m_scaleTime[1][i] = 0.f;
		}
		else
		{
			m_scaleTime[0][i] = 0.f;
			m_scaleTime[1][i] = time[i];
			m_scale[1][i] = m_scale[0][i];
			m_scale[2][i] = scale[i];
		}
	}
}

void Widget::MoveTo(const Vec2 &pos, const Vec2 &time)
{
	for (int i = 0; i < 2; ++i)
	{
		if (time[i] <= 0.f)
		{
			if (i == 0)
				m_rect.x = pos[0];
			else
				m_rect.y = pos[1];

			m_move[0][i] = m_move[1][i] = pos[i];
			m_moveTime[1][i] = 0.f;
		}
		else
		{
			m_moveTime[0][i] = 0.f;
			m_moveTime[1][i] = time[i];
			m_move[0][i] = (i==0) ? m_rect.x : m_rect.y;
			m_move[1][i] = pos[i];
		}
	}
}

void Widget::Draw(bool children)
{
	if (!m_visible)
		return;

	if (visible) // tests alpha and scale
		OnDraw();

	if (children)
	{
		for (Vec::const_iterator it = m_children.begin(); it != m_children.end(); ++it)
		{
			const Ref &w = *it;
			w->Draw();
		}
	}
}

void Widget::SetFocus()
{
	root->SetFocus(boost::static_pointer_cast<Widget>(shared_from_this()));
}

void Widget::SetCapture(bool capture)
{
	if (capture && !m_capture)
	{
		m_capture = true;
		root->SetCapture(boost::static_pointer_cast<Widget>(shared_from_this()));
	}
	else if(!capture && m_capture)
	{
		m_capture = false;
		root->SetCapture(Widget::Ref());
	}
}

bool Widget::HandleInputEvent(const InputEvent &e, const TouchState *touch, const InputState &is)
{
	if (!visible)
		return false;

	for (Vec::const_reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it)
	{
		const Ref &w = *it;
		if (w->HandleInputEvent(e, touch, is))
			return true;
	}

	if (!InBounds(e))
		return false;

	InputEvent local(e);
	if (!local.IsKeyboard())
	{
		Rect r = screenRect;
		local.data[0] -= (int)r.x;
		local.data[1] -= (int)r.y;
	}

	if (OnInputEvent(local, is))
		return true;

	bool r = false;

	if (L && PushCall(L, "OnInputEvent"))
	{
		lua::Marshal<InputEvent>::Push(L, local, touch);

		if (Call(L, "Widget::HandleInputEvent", 2, 1, 0))
		{
			r = lua_toboolean(L, -1) ? true : false;
			lua_pop(L, 1);
		}
	}

	return r;
}

bool Widget::HandleInputGesture(const InputGesture &g, const TouchState &touch, const InputState &is)
{
	if (!visible)
		return false;

	for (Vec::const_reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it)
	{
		const Ref &w = *it;
		if (w->HandleInputGesture(g, touch, is))
			return true;
	}

	if (!InBounds(g))
		return false;

	InputGesture local(g);
	{
		Rect r = screenRect;
		local.mins[0] -= (int)r.x;
		local.mins[1] -= (int)r.y;
		local.maxs[0] -= (int)r.x;
		local.maxs[1] -= (int)r.y;
		local.origin[0] -= (int)r.x;
		local.origin[1] -= (int)r.y;
	}

	if (OnInputGesture(local, is))
		return true;

	bool r = false;

	if (L && PushCall(L, "OnInputGesture"))
	{
		lua::Marshal<InputGesture>::Push(L, local, touch);

		if (Call(L, "Widget::HandleInputGesture", 2, 1, 0))
		{
			r = lua_toboolean(L, -1) ? true : false;
			lua_pop(L, 1);
		}
	}

	return r;
}

bool Widget::RAD_IMPLEMENT_GET(visible)
{
	return m_visible && 
		(m_color[0][3] > 0.f) && 
		(m_scale[0][0] > 0.f) &&
		(m_scale[0][1] > 0.f);
}

int Widget::lua_AddChild(lua_State *L)
{
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	Ref x = GetRef<Widget>(L, "Widget", 2, true);
	w->AddChild(x);
	return 0;
}

int Widget::lua_RemoveChild(lua_State *L)
{
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	Ref x = GetRef<Widget>(L, "Widget", 2, true);
	w->RemoveChild(x);
	return 0;
}

int Widget::lua_FadeTo(lua_State *L)
{
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	w->FadeTo(
		lua::Marshal<Vec4>::Get(L, 2, true),
		(float)luaL_checknumber(L, 3)
	);
	return 0;
}

int Widget::lua_ScaleTo(lua_State *L)
{
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	w->ScaleTo(
		lua::Marshal<Vec2>::Get(L, 2, true),
		lua::Marshal<Vec2>::Get(L, 3, true)
	);
	return 0;
}

int Widget::lua_MoveTo(lua_State *L)
{
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	w->MoveTo(
		lua::Marshal<Vec2>::Get(L, 2, true),
		lua::Marshal<Vec2>::Get(L, 3, true)
	);
	return 0;
}

int Widget::lua_SetCapture(lua_State *L)
{
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	w->SetCapture(lua_toboolean(L, 2) ? true : false);
	return 0;
}

UIW_GETSET(Widget, Rect, Rect, m_rect);
UIW_GET(Widget, ScreenRect, Rect, screenRect);
UIW_GET(Widget, Color, Vec4, color);
UIW_GETSET(Widget, Visible, bool, m_visible);
UIW_GET(Widget, VAlign, int, m_valign);
UIW_SET_CUSTOM(Widget, VAlign, self->m_valign = (VAlign)luaL_checkinteger(L, 2))
UIW_GET(Widget, HAlign, int, m_halign);
UIW_SET_CUSTOM(Widget, HAlign, self->m_halign = (HAlign)luaL_checkinteger(L, 2))

int Widget::LUART_SETFN(Tick)(lua_State *L)
{
	Ref self = GetRef<Widget>(L, "Widget", 1, true);

	self->m_tick = lua_isfunction(L, 2);
	lua_pushvalue(L, 2);
	lua_setfield(L, 1, "tick");
	
	return 0;
}

} // ui
