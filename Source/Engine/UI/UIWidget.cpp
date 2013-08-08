/*! \file UIWidget.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#include RADPCH
#include "UIWidget.h"
#include "../COut.h"
#include "../MathUtils.h"

#define UIWIDGETS "@uiw"

namespace ui {

RAD_ZONE_DEF(RADENG_API, ZUI, "UI", ZEngine);
enum { kMaxWidgets = 256 };

Rect &Rect::Intersect(const Rect &r) {
	
	float x1[2] = {x, r.x};
	float x2[2] = {x+w, r.x+r.w};
	float y1[2] = {y, r.y};
	float y2[2] = {y+h, r.y+r.h};

	x = std::max(x1[0], x1[1]);
	y = std::max(y1[0], y1[1]);

	w = std::min(x2[0], x2[1]) - x;
	h = std::min(y2[0], y2[1]) - y;

	return *this;
}

///////////////////////////////////////////////////////////////////////////////

Root::Ref Root::New(lua_State *L) {
	Ref r(new (ZUI) Root());

	if (L) {
		lua_getfield(L, LUA_REGISTRYINDEX, UIWIDGETS);
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			lua_createtable(L, kMaxWidgets, 0);
			lua_setfield(L, LUA_REGISTRYINDEX, UIWIDGETS);
		} else {
			lua_pop(L, 1);
		}
	}

	return r;
}

Root::Root() {
	m_rbdraw = RBDraw::New();
	m_srcvp[0] = m_srcvp[1] = m_dstvp[0] = m_dstvp[1] = 0;
	m_srcvp[2] = m_dstvp[2] = 1024;
	m_srcvp[3] = m_dstvp[3] = 768;
	m_mapping[0] = m_mapping[1] = 1.f;
}

Root::~Root() {
}

void Root::SetRootWidget(int layer, const WidgetRef &root) {
	if (root) {
		m_layers[layer] = root;
		root->m_root = shared_from_this();
		root->AddedToRoot();
	} else {
		m_layers.erase(layer);
	}
}

WidgetRef Root::RootWidget(int layer) {
	WidgetMap::const_iterator it = m_layers.find(layer);
	if (it == m_layers.end())
		return WidgetRef();
	return it->second;
}

void Root::Tick(float time, float dt, bool tickMaterials) {
	if (tickMaterials) {
		for (pkg::AssetIdMap::const_iterator it = m_materials.begin(); it != m_materials.end(); ++it) {
			asset::MaterialParser *parser = asset::MaterialParser::Cast(it->second);
			if (parser && parser->valid)
				parser->material->Sample(time, dt);
		}
	}

	for (WidgetMap::const_iterator it = m_layers.begin(); it != m_layers.end(); ++it) {
		it->second->Tick(time, dt);
	}
}

void Root::Draw(const Rect *clip, bool children) {
	if (!m_layers.empty()) {
		m_rbdraw->Begin();
		m_rbdraw->SetViewport(
			m_srcvp,
			m_dstvp
		);
	}

	for (WidgetMap::const_iterator it = m_layers.begin(); it != m_layers.end(); ++it) {
		it->second->Draw(clip, children);
	}

	if (!m_layers.empty()) {
		m_rbdraw->End();
	}
}

bool Root::HandleInputEvent(const InputEvent &_e, const TouchState *touch, const InputState &is) {
	InputEvent e = Map(_e);

	if (e.IsKeyboard() && m_focus) {
		return m_focus->HandleInputEvent(e, touch, is);
	}

	if (m_capture) {
		m_capture->HandleInputEvent(e, touch, is);
		return true;
	} else {
		for (WidgetMap::const_reverse_iterator it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
			if (it->second->HandleInputEvent(e, touch, is))
				return true;
			if (it->second->visible && it->second->opaqueLayerInput)
				break;
		}
	}

	return false;
}

bool Root::HandleInputGesture(const InputGesture &_g, const TouchState &touch, const InputState &is) {
	InputGesture g = Map(_g);

	if (m_capture) {
		m_capture->HandleInputGesture(g, touch, is);
		return true;
	} else {
		for (WidgetMap::const_reverse_iterator it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
			if (it->second->HandleInputGesture(g, touch, is))
				return true;
			if (it->second->visible && it->second->opaqueLayerInput)
				break;
		}
	}

	return false;
}

void Root::SetSourceViewport(
	int vpx,
	int vpy,
	int vpw,
	int vph
) {
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
) {
	m_dstvp[0] = vpx;
	m_dstvp[1] = vpy;
	m_dstvp[2] = vpw;
	m_dstvp[3] = vph;

	m_mapping[0] = m_srcvp[2] / (float)m_dstvp[2];
	m_mapping[1] = m_srcvp[3] / (float)m_dstvp[3];
}

InputEvent Root::Map(const InputEvent &e) {
	if (e.IsKeyboard())
		return e;
	InputEvent r(e);
	Vec2 v((float)r.data[0], (float)r.data[1]);
	v = Map(v);
	r.data[0] = (int)v[0];
	r.data[1] = (int)v[1];
	return r;
}

InputGesture Root::Map(const InputGesture &g) {
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

Vec2 Root::Map(const Vec2 &v) {
	Vec2 x = v - Vec2((float)m_dstvp[0], (float)m_dstvp[1]);
	Vec2 y = x * m_mapping;
	return y + Vec2((float)m_srcvp[0], (float)m_srcvp[1]);
}

void Root::SetFocus(const Widget::Ref &w) {
	if (m_focus && w && (m_focus.get() == w.get()))
		return;

	Widget::Ref x = m_focus;
	m_focus = w;

	if (x)
		x->OnFocusChanged(false);
	if (w)
		w->OnFocusChanged(true);
}

void Root::SetCapture(const Widget::Ref &w) {
	m_capture = w;
}

void Root::AddTickMaterial(const pkg::Asset::Ref &ref) {
	if (ref)
		m_materials[ref->id] = ref;
}

///////////////////////////////////////////////////////////////////////////////
namespace {
	int s_numWidgets = 0;
}

int Widget::s_id(0);
int Widget::NextWidgetId(lua_State *L) {
	if (L) {
		lua_getfield(L, LUA_REGISTRYINDEX, UIWIDGETS);
		RAD_ASSERT(!lua_isnil(L, -1));

		int i;
		for (i = 0; i < kMaxWidgets; ++i) {
			int id = (s_id+i)&(kMaxWidgets-1);
			lua_pushinteger(L, id);
			lua_gettable(L, -2);
			bool nil = lua_isnil(L, -1);
			lua_pop(L, 1);
			if (nil)
				break;
		}

		lua_pop(L, 1);
#if !defined(RAD_OPT_SHIP)
		RAD_VERIFY_MSG(i<kMaxWidgets, "kMaxWidgets");
#endif
		i = (s_id+i)&(kMaxWidgets-1);
		s_id = (i+1)&(kMaxWidgets-1);
		return i;
	}

	return s_id++;
}

Widget::Widget() {
	Init();
}

Widget::Widget(const Rect &r) {
	Init();
	m_rect = r;
}

Widget::~Widget() {
	if (m_id > -1) {
		--s_numWidgets;
	}

	if (m_capture) {
		Root::Ref root = m_root.lock();
		if (root)
			root->SetCapture(Widget::Ref());
	}
}

void Widget::Init() {
	L = 0;
	m_id = -1;
	m_visible = true;
	m_valign = kVerticalAlign_Center;
	m_halign = kHorizontalAlign_Center;
	m_positionMode = kPositionMode_Relative;
	m_tick = false;
	m_capture = false;
	m_clip = false;
	m_blendWithParent = false;
	m_opaqueLayerInput = false;
	m_contentPos = Vec2::Zero;

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

	m_zRot = Vec3::Zero;
	m_zRotPlusRate = Vec3::Zero;
	m_zRotTime[0] = m_zRotTime[1] = Vec3::Zero;
	m_zRate[0] = m_zRate[1] = 0.f;
}

bool Widget::Spawn(lua_State *_L, lua_State *Lco, bool luaError) {
	L = _L;
	
	if (Lco) {
		m_id = NextWidgetId(Lco);

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
		lua_getfield(Lco, -4, "Tick");
		if (lua_isfunction(Lco, -1)) {
			m_tick = true;
			lua_setfield(Lco, -2, "Tick");
		} else {
			lua_pop(Lco, 1);
		}

		lua_settable(Lco, -3);
		lua_pop(Lco, 1);

		++s_numWidgets;
#if !defined(RAD_OPT_SHIP)
		COut(C_Debug) << "Widget::Spawn: " << s_numWidgets << " widget(s)" << std::endl;
#endif
	}

	return true;
}

void Widget::Unmap(lua_State *L) {
	if (m_id > -1) {
		lua_getfield(L, LUA_REGISTRYINDEX, UIWIDGETS);
		RAD_ASSERT(!lua_isnil(L, -1));
		lua_pushnumber(L, m_id);
		lua_pushnil(L);
		lua_settable(L, -3);
		lua_pop(L, 1);
		m_id = -1;

		--s_numWidgets;
#if !defined(RAD_OPT_SHIP)
		COut(C_Debug) << "Widget::Unmap: " << s_numWidgets << " widget(s)" << std::endl;
#endif
	}

	for (Vec::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
		(*it)->Unmap(L);
	}
}

void Widget::PushCallTable(lua_State *L) {
	lua_pushcfunction(L, lua_AddChild);
	lua_setfield(L, -2, "AddChild");
	lua_pushcfunction(L, lua_RemoveChild);
	lua_setfield(L, -2, "RemoveChild");
	lua_pushcfunction(L, lua_BlendTo);
	lua_setfield(L, -2, "BlendTo");
	lua_pushcfunction(L, lua_ScaleTo);
	lua_setfield(L, -2, "ScaleTo");
	lua_pushcfunction(L, lua_MoveTo);
	lua_setfield(L, -2, "MoveTo");
	lua_pushcfunction(L, lua_RotateTo);
	lua_setfield(L, -2, "RotateTo");
	lua_pushcfunction(L, lua_SetCapture);
	lua_setfield(L, -2, "SetCapture");
	lua_pushcfunction(L, lua_Unmap);
	lua_setfield(L, -2, "Unmap");
	LUART_REGISTER_GETSET(L, Rect);
	LUART_REGISTER_GET(L, ScreenRect);
	LUART_REGISTER_GET(L, zRot);
	LUART_REGISTER_GET(L, zRotScreen);
	LUART_REGISTER_GETSET(L, Visible);
	LUART_REGISTER_GETSET(L, VAlign);
	LUART_REGISTER_GETSET(L, HAlign);
	LUART_REGISTER_SET(L, Tick);
	LUART_REGISTER_GET(L, zRotationSpeed);
	LUART_REGISTER_SET(L, ZRotationSpeed);
	LUART_REGISTER_GET(L, zAngle);
	LUART_REGISTER_SET(L, ZAngle);
	LUART_REGISTER_GETSET(L, ClipRect);
	LUART_REGISTER_GETSET(L, ContentPos);
	LUART_REGISTER_GETSET(L, BlendWithParent);
	LUART_REGISTER_GETSET(L, OpaqueLayerInput);
}

void Widget::CreateFromTable(lua_State *L) {
	lua_getfield(L, -1, "rect");
	if (!lua_isnil(L, -1))
		m_rect = lua::Marshal<Rect>::Get(L, -1, false);
	lua_pop(L, 1);
	lua_getfield(L, -1, "clipRect");
	if (!lua_isnil(L, -1)) {
		m_clip = true;
		m_clipRect = lua::Marshal<Rect>::Get(L, -1, false);
	} else {
		m_clip = false;
	}
	lua_pop(L, 1);
	lua_getfield(L, -1, "visible");
	if (!lua_isnil(L, -1))
		m_visible = lua_toboolean(L, -1) ? true : false;
	lua_pop(L, 1);
	lua_getfield(L, -1, "valign");
	if (!lua_isnil(L, -1))
		m_valign = (VerticalAlign)lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "halign");
	if (!lua_isnil(L, -1))
		m_halign = (HorizontalAlign)lua_tointeger(L, -1);
	lua_pop(L, 1);
}

bool Widget::PushFrame(lua_State *L) {
	if (m_id > -1) {
		lua_getfield(L, LUA_REGISTRYINDEX, UIWIDGETS);
		RAD_ASSERT(!lua_isnil(L, -1));
		lua_pushnumber(L, m_id);
		lua_gettable(L, -2);
		if (lua_isnil(L, -1)) {
			lua_pop(L, 2);
			return false;
		}

		lua_remove(L, -2);
		return true;
	}

	return false;
}

bool Widget::PushCall(lua_State *L, const char *name) {
	if (!PushFrame(L))
		return false;

	lua_getfield(L, -1, name);
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 2);
		return false;
	}

	lua_pushvalue(L, -2); // move widget frame as first argument.
	lua_remove(L, -3);
	return true;
}

bool Widget::Call(lua_State *L, const char *context, int nargs, int nresults, int errfunc) {
	if (lua_pcall(L, nargs, nresults, errfunc)) {
		COut(C_Error) << "UIScriptError(" << context << "): " << lua_tostring(L, -1) << std::endl;
		lua_pop(L, 1);
		return false;
	}

	return true;
}

bool Widget::InBounds(const InputEvent &e) {
	if (e.IsKeyboard())
		return true;
	return InBounds((float)e.data[0], (float)e.data[1]);
}

bool Widget::InBounds(const InputGesture &g) {
	return InBounds((float)g.origin[0], (float)g.origin[1]);
}

bool Widget::InBounds(float x, float y) {
	Rect r = screenRect;
	return r.InBounds(x,y);
}

void Widget::AddChild(const Ref &widget) {
	RAD_ASSERT(widget);
	
#if !defined(RAD_TARGET_GOLDEN)
	for (Vec::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
		RAD_VERIFY_MSG((*it).get() != widget.get(), "Child widget added to parent widget multiple times!");
	}
#endif

	widget->m_root = m_root;
	widget->m_parent = boost::static_pointer_cast<Widget>(shared_from_this());
	m_children.push_back(widget);

	if (widget->m_root.lock())
		widget->AddedToRoot();
}

void Widget::RemoveChild(const Ref &widget) {
	RAD_ASSERT(widget);
	
	for (Vec::iterator it = m_children.begin(); it != m_children.end(); ++it) {
		if ((*it).get() == widget.get()) {
			widget->m_parent.reset();
			m_children.erase(it);
			widget->RemovedFromRoot();
			return;
		}
	}
}

void Widget::AddedToRoot() {
	for (Vec::iterator it = m_children.begin(); it != m_children.end(); ++it) {
		(*it)->m_root = m_root;
		(*it)->AddedToRoot();
	}
}

void Widget::RemovedFromRoot() {
	m_root.reset();
	for (Vec::iterator it = m_children.begin(); it != m_children.end(); ++it) {
		(*it)->RemovedFromRoot();
	}
}

void Widget::ClearCapture() {
	if (m_capture) {
		SetCapture(false);
		return;
	}

	for (Vec::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
		(*it)->ClearCapture();
	}
}

Rect Widget::RAD_IMPLEMENT_GET(screenRect) {
	return ToScreen(m_rect);
}

Vec3 Widget::RAD_IMPLEMENT_GET(zRotScreen) {
	Vec3 z = zRot;
	Rect r = screenRect;
	z[0] += r.x;
	z[1] += r.y;
	return z;
}


Rect Widget::ToScreen(const Rect &r) const {
	Rect base(0, 0, 0, 0);

	if (m_positionMode == kPositionMode_Relative) {
		Ref p = m_parent.lock();
		if (p) {
			base = p->screenRect;
			Vec2 offset = p->contentPos;
			base.x += offset[0];
			base.y += offset[1];
		}
	}
		
	float sw = r.w * m_scale[0][0];
	float sh = r.h * m_scale[0][1];
	float x, y;

	// figure out alignment
	switch (m_halign) {
	case kHorizontalAlign_Center:
		x = r.x + ((r.w-sw)/2.f);
		break;
	case kHorizontalAlign_Right:
		x = r.x+r.w-sw;
		break;
	default: // kHorizontalAlign_Left
		x = r.x;
		break;
	}

	switch (m_valign) {
	case kVerticalAlign_Center:
		y = r.y + ((r.h-sh)/2.f);
		break;
	case kVerticalAlign_Bottom:
		y = r.y+r.h-sh;
		break;
	default: // kVerticalAlign_Top
		y = r.y;
		break;

	}

	return Rect(base.x+x, base.y+y, sw, sh);
}

Rect Widget::ToWidget(const Rect &r) const {
	Rect x = screenRect;
	x.x = r.x - x.x;
	x.y = r.y - x.y;
	x.w = r.w;
	x.h = r.h;
	return x;
}

void Widget::Tick(float time, float dt) {
	if (m_fadeTime[1] > 0.f) {
		m_fadeTime[0] += dt;
		if (m_fadeTime[0] >= m_fadeTime[1]) {
			m_fadeTime[1] = 0.f;
			m_color[0] = m_color[2];
		} else {
			m_color[0] = math::Lerp(m_color[1], m_color[2], m_fadeTime[0]/m_fadeTime[1]);
		}
	}

	for (int i = 0; i < 2; ++i) {
		if (m_scaleTime[1][i] > 0.f) {
			m_scaleTime[0][i] += dt;

			if (m_scaleTime[0][i] >= m_scaleTime[1][i]) {
				m_scaleTime[1][i] = 0.f;
				m_scale[0][i] = m_scale[2][i];
			} else {
				m_scale[0][i] = math::Lerp(m_scale[1][i], m_scale[2][i], m_scaleTime[0][i]/m_scaleTime[1][i]);
			}
		}
	}

	for (int i = 0; i < 2; ++i) {
		if (m_moveTime[1][i] > 0.f) {
			m_moveTime[0][i] += dt;

			if (m_moveTime[0][i] >= m_moveTime[1][i]) {
				m_moveTime[1][i] = 0.f;
				if (i == 0) {
					m_rect.x = m_move[1][0];
				} else {
					m_rect.y = m_move[1][1];
				}
			} else {
				float z = math::Lerp(m_move[0][i], m_move[1][i], m_moveTime[0][i]/m_moveTime[1][i]);
				if (i == 0) {
					m_rect.x = z;
				} else {
					m_rect.y = z;
				}
			}
		}
	}

	for (int i = 0; i < 2; ++i) {
		if (m_zRotTime[1][i] > 0.f) {
			m_zRotTime[0][i] += dt;

			if (m_zRotTime[0][i] >= m_zRotTime[1][i]) {
				m_zRot[i] = m_zRotPos[1][i];
				m_zRotTime[1][i] = 0.f;
			} else {
				m_zRot[i] = math::Lerp(m_zRotPos[0][i], m_zRotPos[1][i], m_zRotTime[0][i]/m_zRotTime[1][i]);
			}
		}
	}

	if (m_zRotTime[1][2] > 0.f) {
		m_zRotTime[0][2] += dt;

		if (m_zRotTime[0][2] >= m_zRotTime[1][2]) {
			m_zRot[2] = m_zRotAngle[2];
			m_zRotTime[1][2] = 0.f;
		} else {
			m_zRot[2] = m_zRotAngle[0] + math::Lerp(0.f, m_zRotAngle[1], m_zRotTime[0][2] / m_zRotTime[1][2]);
		}
	}

	m_zRate[0] += m_zRate[1] * dt;
	if ((m_zRate[0] < -360.f) ||
		(m_zRate[1] >  360.f)) {
		m_zRate[0] = math::Mod(m_zRate[0], 360.f);
	}

	m_zRotPlusRate = m_zRot;
	m_zRotPlusRate[2] += m_zRate[0];

	OnTick(time, dt);

	if (m_tick && L && PushCall(L, "Tick")) {
		lua_pushnumber(L, dt);
		Call(L, "Widget::Tick", 2, 0, 0);
	}

	for (Vec::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
		const Ref &w = *it;
		w->Tick(time, dt);
	}
}

void Widget::BlendTo(const Vec4 &dst, float time) {
	if (time <= 0.f) {
		for (int i = 0; i < 3; ++i)
			m_color[i] = dst;
		m_fadeTime[1] = 0.f;
	} else {
		m_fadeTime[0] = 0.f;
		m_fadeTime[1] = time;
		m_color[1] = m_color[0];
		m_color[2] = dst;
	}
}

void Widget::ScaleTo(const Vec2 &scale, const Vec2 &time) {
	for (int i = 0; i < 2; ++i) {
		if (time[i] <= 0.f) {
			m_scale[0][i] = m_scale[1][i] = m_scale[2][i] = scale[i];
			m_scaleTime[1][i] = 0.f;
		} else {
			m_scaleTime[0][i] = 0.f;
			m_scaleTime[1][i] = time[i];
			m_scale[1][i] = m_scale[0][i];
			m_scale[2][i] = scale[i];
		}
	}
}

void Widget::MoveTo(const Vec2 &pos, const Vec2 &time) {
	for (int i = 0; i < 2; ++i) {
		if (time[i] <= 0.f) {
			if (i == 0) {
				m_rect.x = pos[0];
			} else {
				m_rect.y = pos[1];
			}

			m_move[0][i] = m_move[1][i] = pos[i];
			m_moveTime[1][i] = 0.f;
		} else {
			m_moveTime[0][i] = 0.f;
			m_moveTime[1][i] = time[i];
			m_move[0][i] = (i==0) ? m_rect.x : m_rect.y;
			m_move[1][i] = pos[i];
		}
	}
}

void Widget::RotateTo(const Vec3 &zRot, const Vec3 &time, bool shortestAngle) {
	float _zRot = zRot[2];

	if (_zRot < 0.f)
		_zRot += 360.f;
	if (_zRot > 360.f)
		_zRot -= 360.f;
	

	for (int i = 0; i < 2; ++i) {
		if (time[i] <= 0.f) {
			m_zRot[i] = zRot[i];
			m_zRotPos[0][i] = 0.f;
			m_zRotPos[1][i] = 0.f;
		} else {
			m_zRotPos[0][i] = m_zRot[i];
			m_zRotPos[1][i] = zRot[i];
			m_zRotTime[0][i] = 0.f;
			m_zRotTime[1][i] = time[i];
		}
	}

	if (time[2] <= 0.f) {
		m_zRot[2] = _zRot;
		m_zRotTime[1][2] = 0.f;
		m_zRotTime[1][2] = 0.f;
	} else {

		if (m_zRot[2] < 0.f)
			m_zRot[2] += 360.f;
		if (m_zRot[2] > 360.f)
			m_zRot[2] -= 360.f;

		float delta;
		if (shortestAngle) {
			Vec3 startAngles(0, 0, m_zRot[2]);
			Vec3 endAngles(0, 0, _zRot);
			Vec3 deltaAngles = DeltaAngles(startAngles, endAngles);
			delta = deltaAngles[2];
		} else {
			delta = _zRot - m_zRot[2];
		}

		m_zRotAngle[0] = m_zRot[2];
		m_zRotAngle[1] = delta;
		m_zRotAngle[2] = _zRot;
		m_zRotTime[0][2] = 0.f;
		m_zRotTime[1][2] = time[2];
	}
}

void Widget::Draw(const Rect *_clip, bool children) {
	if (!m_visible)
		return;

	Rect clip;
	if (m_clip) {
		clip = m_clipRect;
		if (_clip) {
			Rect outerClip = ToWidget(*_clip);
			clip.Intersect(outerClip);
		}
		clip = ToScreen(clip);
	}

	if (visible) { // tests alpha and scale
		OnDraw(m_clip ? &clip : _clip);
	}

	if (children) {
		for (Vec::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
			const Ref &w = *it;
			w->Draw(m_clip ? &clip : _clip, true);
		}
	}
}

void Widget::SetFocus() {
	root->SetFocus(boost::static_pointer_cast<Widget>(shared_from_this()));
}

void Widget::SetCapture(bool capture) {
	if (capture && !m_capture) {
		m_capture = true;
		root->SetCapture(boost::static_pointer_cast<Widget>(shared_from_this()));
	} else if(!capture && m_capture) {
		m_capture = false;
		root->SetCapture(Widget::Ref());
	}
}

bool Widget::HandleInputEvent(const InputEvent &e, const TouchState *touch, const InputState &is) {
	if (!visible)
		return false;

	if ((m_capture || e.IsKeyboard() || InBounds(e)) && InputEventFilter(e, touch, is))
		return true;

	for (Vec::const_reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it) {
		const Ref &w = *it;
		if (w->HandleInputEvent(e, touch, is))
			return true;
	}

	if (!m_capture && (!e.IsKeyboard() && !InBounds(e)))
		return false;

	InputEvent local(e);
	if (!local.IsKeyboard()) {
		Rect r = screenRect;
		local.data[0] -= (int)r.x;
		local.data[1] -= (int)r.y;
	}

	if (OnInputEvent(local, is))
		return true;

	bool r = false;

	if (L && PushCall(L, "OnInputEvent")) {
		lua::Marshal<InputEvent>::Push(L, local, touch);

		if (Call(L, "Widget::HandleInputEvent", 2, 1, 0)) {
			r = lua_toboolean(L, -1) ? true : false;
			lua_pop(L, 1);
		}
	}

	return r;
}

bool Widget::HandleInputGesture(const InputGesture &g, const TouchState &touch, const InputState &is) {
	if (!visible)
		return false;

	if ((m_capture || InBounds(g)) && InputGestureFilter(g, touch, is))
		return false;

	for (Vec::const_reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it) {
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

	if (L && PushCall(L, "OnInputGesture")) {
		lua::Marshal<InputGesture>::Push(L, local, touch);

		if (Call(L, "Widget::HandleInputGesture", 2, 1, 0)) {
			r = lua_toboolean(L, -1) ? true : false;
			lua_pop(L, 1);
		}
	}

	return r;
}

bool Widget::RAD_IMPLEMENT_GET(visible) {
	bool r = m_visible && 
		(m_color[0][3] > 0.f) && 
		(m_scale[0][0] > 0.f) &&
		(m_scale[0][1] > 0.f);

	if (r) {
		if (m_blendWithParent) {
			Vec4 rgba = this->blendedColor;
			r = rgba[3] > 0.f;
		}
	}

	return r;
}

Vec4 Widget::RAD_IMPLEMENT_GET(blendedColor) {
	if (!m_blendWithParent)
		return m_color[0];

	Vec4 rgba = m_color[0];
	Widget::Ref parent = m_parent.lock();
	if (parent) {
		rgba *= parent->blendedColor;
	}
	return rgba;
}

int Widget::lua_AddChild(lua_State *L) {
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	Ref x = GetRef<Widget>(L, "Widget", 2, true);

#if !defined(RAD_TARGET_GOLDEN)
	for (Vec::const_iterator it = w->m_children.begin(); it != w->m_children.end(); ++it) {
		if ((*it).get() == x.get()) {
			luaL_error(L, "Child widget added to parent widget multiple times!");
		}
	}
#endif

	w->AddChild(x);
	return 0;
}

int Widget::lua_RemoveChild(lua_State *L) {
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	Ref x = GetRef<Widget>(L, "Widget", 2, true);
	w->RemoveChild(x);
	return 0;
}

int Widget::lua_BlendTo(lua_State *L) {
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	w->BlendTo(
		lua::Marshal<Vec4>::Get(L, 2, true),
		(float)luaL_checknumber(L, 3)
	);
	return 0;
}

int Widget::lua_ScaleTo(lua_State *L) {
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	w->ScaleTo(
		lua::Marshal<Vec2>::Get(L, 2, true),
		lua::Marshal<Vec2>::Get(L, 3, true)
	);
	return 0;
}

int Widget::lua_MoveTo(lua_State *L) {
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	w->MoveTo(
		lua::Marshal<Vec2>::Get(L, 2, true),
		lua::Marshal<Vec2>::Get(L, 3, true)
	);
	return 0;
}

int Widget::lua_RotateTo(lua_State *L) {
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	w->RotateTo(
		lua::Marshal<Vec3>::Get(L, 2, true),
		lua::Marshal<Vec3>::Get(L, 3, true),
		lua_toboolean(L, 4) ? true : false
	);
	return 0;
}

int Widget::lua_SetCapture(lua_State *L) {
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	w->SetCapture(lua_toboolean(L, 2) ? true : false);
	return 0;
}

int Widget::lua_Unmap(lua_State *L) {
	Ref w = GetRef<Widget>(L, "Widget", 1, true);
	w->Unmap(L);
	return 0;
}

UIW_GETSET(Widget, Rect, Rect, m_rect);
UIW_GET(Widget, ScreenRect, Rect, screenRect);
UIW_GET(Widget, Color, Vec4, color);
UIW_GETSET(Widget, Visible, bool, m_visible);
UIW_GET(Widget, VAlign, int, m_valign);
UIW_SET_CUSTOM(Widget, VAlign, self->m_valign = (VerticalAlign)luaL_checkinteger(L, 2))
UIW_GET(Widget, HAlign, int, m_halign);
UIW_SET_CUSTOM(Widget, HAlign, self->m_halign = (HorizontalAlign)luaL_checkinteger(L, 2))
UIW_GET(Widget, zRot, Vec3, m_zRot);
UIW_GET(Widget, zRotScreen, Vec3, zRotScreen);
UIW_GET(Widget, zRotationSpeed, float, m_zRate[1]);
UIW_SET(Widget, ZRotationSpeed, float, m_zRate[1]);
UIW_GET(Widget, zAngle, float, m_zRate[0]);
UIW_SET(Widget, ZAngle, float, m_zRate[0]);
UIW_GETSET(Widget, ContentPos, Vec2, m_contentPos);
UIW_GETSET(Widget, BlendWithParent, bool, m_blendWithParent);
UIW_GETSET(Widget, OpaqueLayerInput, bool, m_opaqueLayerInput);

int Widget::LUART_SETFN(Tick)(lua_State *L) {
	Ref self = GetRef<Widget>(L, "Widget", 1, true);

	self->m_tick = lua_isfunction(L, 2);
	lua_pushvalue(L, 2);
	lua_setfield(L, 1, "tick");
	
	return 0;
}

int Widget::LUART_GETFN(ClipRect)(lua_State *L) {
	Ref self = GetRef<Widget>(L, "Widget", 1, true);
	if (self->clip) {
		lua::Marshal<Rect>::Push(L, self->clipRect);
		return 1;
	}
	return 0;
}

int Widget::LUART_SETFN(ClipRect)(lua_State *L) {
	Ref self = GetRef<Widget>(L, "Widget", 1, true);
	if (lua_isnil(L, 2)) {
		self->clip = false;
	} else {
		self->clip = true;
		self->clipRect = lua::Marshal<Rect>::Get(L, 2, true);
	}
	return 0;
}

} // ui
