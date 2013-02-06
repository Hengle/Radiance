/*! \file UIVListWidget.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#include RADPCH
#include "UIVListWidget.h"

namespace ui {

VListWidget::VListWidget() {
	Init();
}

VListWidget::VListWidget(const Rect &r) : Widget(r) {
	Init();
}

void VListWidget::Init() {
	m_friction = 2000.f;
	m_scroll = Vec2::Zero;
	m_velocity = Vec2::Zero;
	m_contentSize = Vec2::Zero;
	m_endStops = Vec2::Zero;
	m_dragMotion = Vec2::Zero;
	m_dragging = false;
	m_dragMove = false;
	m_pts = 0;
	m_postingEvent = false;
	m_dragDidMove = false;
	m_endStop = false;
	m_checkDelayed = false;

	m_spring.length = 0.01f;
	m_spring.elasticity = 200.f;
	m_spring.tolerance = 0.003f;

	m_vertex.mass = 5.f;
	m_vertex.friction = 0.5;
	m_vertex.drag[0] = 10;
	m_vertex.drag[1] = 10;
	m_vertex.inner = false;
	m_vertex.outer = true;

	m_scrollTo[0] = m_scrollTo[1] = Vec2::Zero;
	m_scrollTime[0] = m_scrollTime[1] = 0.f;
}

void VListWidget::Clear() {
	m_e.type = InputEvent::T_Invalid;
	m_velocity = Vec2::Zero;
	m_endStop = false;
	m_scroll = Vec2::Zero;
	m_contentSize = Vec2::Zero;
	m_dragging = false;
	m_dragMove = false;
	m_dragDidMove = false;
	this->contentPos = Vec2::Zero;

	for (Widget::Vec::iterator it = m_widgets.begin(); it != m_widgets.end(); ++it) {
		RemoveChild(*it);
	}

	m_widgets.clear();
}

void VListWidget::AddItem(const Widget::Ref &widget) {
	for (Widget::Vec::iterator it = m_widgets.begin(); it != m_widgets.end(); ++it) {
		if ((*it).get() == widget.get())
			return;
	}
	m_widgets.push_back(widget);
	AddChild(widget);
}

void VListWidget::RemoveItem(const Widget::Ref &widget) {
	for (Widget::Vec::iterator it = m_widgets.begin(); it != m_widgets.end(); ++it) {
		if ((*it).get() == widget.get()) {
			RemoveChild(*it);
			m_widgets.erase(it);
			break;
		}
	}
}

void VListWidget::ScrollTo(const Vec2 &pos, float time) {
	// cancel any drag or motion
	m_e.type = InputEvent::T_Invalid;
	m_velocity = Vec2::Zero;
	m_endStop = false;
	m_dragging = false;
	m_dragMove = false;
	m_dragDidMove = false;

	Rect r = this->rect;

	Vec2 scrollTo = pos;

	if ((scrollTo[0]+r.w) > m_contentSize[0]) {
		scrollTo[0] = m_contentSize[0] - r.w;
	}

	if (scrollTo[0] < 0.f) { 
		scrollTo[0] = 0.f;
	}

	if ((scrollTo[1]+r.h) > m_contentSize[1]) {
		scrollTo[1] = m_contentSize[1] - r.h;
	}

	if (scrollTo[1] < 0.f) {
		scrollTo[1] = 0.f;
	}

	m_scrollTime[0] = 0.f;

	if (time <= 0.f) {
		m_scrollTime[1] = 0.f;
		m_scroll = -scrollTo;
		RecalcLayout();
		this->contentPos = m_scroll;
	} else {
		m_scrollTime[1] = time;
		m_scrollTo[0] = m_scroll;
		m_scrollTo[1] = -scrollTo;
	}
}

void VListWidget::DoVerticalLayout() {
	Vec2 pos(Vec2::Zero);

	for (Widget::Vec::const_iterator it = m_widgets.begin(); it != m_widgets.end(); ++it) {
		const Widget::Ref &w = *it;

		Rect r = w->rect;
		r.x += pos[0];
		r.y += pos[1];

		pos[1] += r.h;

		w->rect = r;
	}

	RecalcLayout();
}

void VListWidget::RecalcLayout() {
	Vec2 pos = m_scroll;

	Rect self = this->rect;

	m_contentSize = Vec2::Zero;

	for (Widget::Vec::const_iterator it = m_widgets.begin(); it != m_widgets.end(); ++it) {
		const Widget::Ref &w = *it;

		Rect r = w->rect;
		
		if ((r.y+r.h+pos[1]) < 0.f) {
			w->visible = false;
		} else if ((r.y+pos[1]) > self.h) {
			w->visible = false;
		} else {
			w->visible = true;
		}

		m_contentSize[0] = std::max(m_contentSize[0], r.x+r.w);
		m_contentSize[1] = std::max(m_contentSize[1], r.y+r.h);
	}
}

void VListWidget::Drag(const InputEvent &e) {

	if (e.IsTouchEnd(0)) {
		m_dragging = false;
		m_dragMove = false;
		m_dragDidMove = false;
		m_e.type = InputEvent::T_Invalid;
		return;
	}

	RAD_ASSERT(e.IsTouchMove(0));

	float dy = (float)(e.data[1] - m_e.data[1]);

	xtime::TimeVal delta = e.time - m_e.time;

	if (delta < 1)
		return;

	m_e.time = e.time;
	m_e.data[0] = e.data[0];
	m_e.data[1] = e.data[1];

	float dt = delta / 1000.f;
	
	if (dt < 0.1f) {
		if (((dy < 0.f) && (m_dragMotion[0] <= 0.f)) ||
			((dy > 0.f) && (m_dragMotion[0] >= 0.f))) {
			m_dragMotion[0] += dy;
			m_dragMotion[1] += dt;
			m_velocity[1] = m_dragMotion[0] / m_dragMotion[1];
			if (m_dragMotion[1] > 0.25) {
				m_dragMotion[0] = 0.f;
				m_dragMotion[1] = 0.f;
			}
		} else {
			m_dragMotion[0] = dy;
			m_dragMotion[1] = dt;
			m_velocity[1] = m_dragMotion[0] / m_dragMotion[1];
		}
	}

	m_dragMove = dy != 0.f;
	m_dragDidMove = m_dragDidMove || m_dragMove;

	Scroll(Vec2(0.f, dy));
}

void VListWidget::Scroll(const Vec2 &delta) {
	// end stops
	float minY = 0.f;

	Rect r = this->rect;

	if (m_contentSize[1] > r.h) {
		minY = -(m_contentSize[1] - r.h);
	}

	Vec2 scroll(m_scroll);

	scroll[1] += delta[1];

	if (scroll[1] > 0.f) {
		if (m_dragging || (m_velocity.Magnitude() < 0.001f) || (scroll[1] > m_endStops[1])) {
			m_endStop = true;
			if (scroll[1] > m_endStops[1]) {
				scroll[1] = m_endStops[1];
			} else {
				scroll[1] = m_scroll[1] + (delta[1]*0.5f);
			}
			m_springRoot = Vec3::Zero;
		}
	} else if (scroll[1] < minY) {
		if (m_dragging || (m_velocity.Magnitude() < 0.001f) || ((minY - scroll[1]) > m_endStops[1])) {
			m_endStop = true;
			if ((minY - scroll[1]) > m_endStops[1]) {
				scroll[1] = minY - m_endStops[1];
			} else {
				scroll[1] = m_scroll[1] + (delta[1]*0.5f);
			}
			m_springRoot = Vec3(0.f, minY, 0.f);
		}
	} else {
		m_endStop = false;
	}

	m_vertex.pos = Vec3(0.f, scroll[1], 0.f);
	m_vertex.vel = Vec3::Zero;
	m_scroll = scroll;
}

bool VListWidget::ApplyVelocity(float dt) {
	if (m_dragMove)
		return true;
	if (m_dragging)
		return false;
	if (m_endStop) {
		m_velocity = Vec2::Zero;
		m_endStop = m_vertex.Update(dt, m_springRoot, m_spring);
		if (m_endStop) {
			m_scroll[0] = m_vertex.pos[0];
			m_scroll[1] = m_vertex.pos[1];
		} else {
			m_scroll[0] = m_springRoot[0];
			m_scroll[1] = m_springRoot[1];
		}
		return m_endStop;
	}

	float mag = m_velocity.Magnitude();

	if (mag > 0.01f) {
		float decel = mag - m_friction*dt;

		if (decel < 0.f)
			decel = 0.f;

		decel /= mag;
		m_velocity *= decel;
	} else {
		m_velocity = Vec2::Zero;
		return false;
	}

	Scroll(m_velocity*dt);

	return true;
}

void VListWidget::OnTick(float time, float dt) {
	CheckPostDelayedInput();

	if (m_dragging) {
		xtime::TimeVal delta = xtime::ReadMilliseconds() - m_e.time;
		if (delta > 250) {
			m_dragMotion[0] = 0.f;
			m_dragMotion[1] = 0.f;
			m_velocity[1] = 0.f;
		}
	}

	if (m_scrollTime[1] > 0.f) {
		m_scrollTime[0] += dt;
		if (m_scrollTime[0] >= m_scrollTime[1]) {
			m_scrollTime[1] = 0.f;
			m_scroll = m_scrollTo[1];
		} else {
			m_scroll = math::Lerp(m_scrollTo[0], m_scrollTo[1], m_scrollTime[0]/m_scrollTime[1]);
		}

		RecalcLayout();
		this->contentPos = m_scroll;
	}
	else if (ApplyVelocity(dt)) {
		RecalcLayout();
		this->contentPos = m_scroll;
	}
}

bool VListWidget::ProcessInputEvent(const InputEvent &e, const TouchState *state, const InputState &is) {
	m_postingEvent = true;
	bool r = HandleInputEvent(e, state, is);
	m_postingEvent = false;
	return r;
}

bool VListWidget::InputEventFilter(const InputEvent &e, const TouchState *state, const InputState &is) {
	if (m_postingEvent)
		return false;

	if (!e.IsTouch())
		return false;

	if (m_e.type == InputEvent::T_Invalid) {
		if (!e.IsTouchBegin())
			return false;
		m_e = e;
		m_is = is;
		if (state) {
			m_ts = *state;
			m_pts = &m_ts;
		} else {
			m_pts = 0;
		}
		m_dragging = true;
		m_checkDelayed = false;
		m_velocity = Vec2::Zero;
		m_dragMotion = Vec2::Zero;
		m_scrollTime[1] = 0.f; // kill any scrolling
		return true;
	} else if (e.touch != m_e.touch) {
		return false; // not our touch
	}

	if (m_dragDidMove) {
		Drag(e);
		return true;
	}

	if (e.IsTouchEnd(0)) {
		// touch and release
		RAD_ASSERT(m_e.type == InputEvent::T_TouchBegin);
		ProcessInputEvent(m_e, state, is);
		m_e.type = InputEvent::T_Invalid;
		m_dragging = false;
		return false;
	}

	if (m_dragging) {
		Drag(e);
		return true;
	}


	return false;
}

void VListWidget::CheckPostDelayedInput() {
	if (m_checkDelayed)
		return;
	if (m_dragDidMove)
		return;
	if (m_e.type == InputEvent::T_Invalid)
		return;
	RAD_ASSERT(m_dragging);
	xtime::TimeVal delta = xtime::ReadMilliseconds() - m_e.time;
	if (delta > 150) {
		m_checkDelayed = true;
		if (ProcessInputEvent(m_e, m_pts, m_is)) {
			m_dragging = false;
			m_e.type = InputEvent::T_Invalid;
		}
	}
}

void VListWidget::PushCallTable(lua_State *L) {
	Widget::PushCallTable(L);
	LUART_REGISTER_GETSET(L, Scroll);
	LUART_REGISTER_GETSET(L, StopSpring);
	LUART_REGISTER_GETSET(L, StopSpringVertex);
	LUART_REGISTER_GETSET(L, Friction);
	LUART_REGISTER_GETSET(L, EndStops);
	LUART_REGISTER_GET(L, ContentSize);
	lua_pushcfunction(L, lua_RecalcLayout);
	lua_setfield(L, -2, "RecalcLayout");
	lua_pushcfunction(L, lua_AddItem);
	lua_setfield(L, -2, "AddItem");
	lua_pushcfunction(L, lua_RemoveItem);
	lua_setfield(L, -2, "RemoveItem");
	lua_pushcfunction(L, lua_ScrollTo);
	lua_setfield(L, -2, "ScrollTo");
	lua_pushcfunction(L, lua_Items);
	lua_setfield(L, -2, "Items");
	lua_pushcfunction(L, lua_DoVerticalLayout);
	lua_setfield(L, -2, "DoVerticalLayout");
	lua_pushcfunction(L, lua_Clear);
	lua_setfield(L, -2, "Clear");
}

int VListWidget::lua_RecalcLayout(lua_State *L) {
	Ref self = GetRef<VListWidget>(L, "VListWidget", 1, true);
	self->RecalcLayout();
	return 0;
}

int VListWidget::lua_AddItem(lua_State *L) {
	Ref self = GetRef<VListWidget>(L, "VListWidget", 1, true);
	Widget::Ref widget = GetRef<Widget>(L, "Widget", 2, true);
	self->AddItem(widget);
	return 0;
}

int VListWidget::lua_RemoveItem(lua_State *L) {
	Ref self = GetRef<VListWidget>(L, "VListWidget", 1, true);
	Widget::Ref widget = GetRef<Widget>(L, "Widget", 2, true);
	self->RemoveItem(widget);
	return 0;
}

int VListWidget::lua_ScrollTo(lua_State *L) {
	Ref self = GetRef<VListWidget>(L, "VListWidget", 1, true);
	self->ScrollTo(
		lua::Marshal<Vec2>::Get(L, 2, true),
		(float)luaL_checknumber(L, 3)
	);
	return 0;
}

int VListWidget::lua_DoVerticalLayout(lua_State *L) {
	Ref self = GetRef<VListWidget>(L, "VListWidget", 1, true);
	self->DoVerticalLayout();
	return 0;
}

int VListWidget::lua_Items(lua_State *L) {
	Ref self = GetRef<VListWidget>(L, "VListWidget", 1, true);
	if (self->m_widgets.empty())
		return 0;
	lua_createtable(L, (int)self->m_widgets.size(), 0);
	for (int i = 0; i < (int)self->m_widgets.size(); ++i) {
		const Widget::Ref &w = self->m_widgets[i];
		lua_pushinteger(L, i+1);
		w->PushFrame(L);
		lua_settable(L, -3);
	}

	return 1;
}

int VListWidget::lua_Clear(lua_State *L) {
	Ref self = GetRef<VListWidget>(L, "VListWidget", 1, true);
	self->Clear();
	return 0;
}

UIW_GETSET(VListWidget, Scroll, Vec2, m_scroll);
UIW_GETSET(VListWidget, StopSpring, physics::Spring, m_spring);
UIW_GETSET(VListWidget, StopSpringVertex, physics::SpringVertex, m_vertex);
UIW_GETSET(VListWidget, Friction, float, m_friction);
UIW_GETSET(VListWidget, EndStops, Vec2, m_endStops);
UIW_GET(VListWidget, ContentSize, Vec2, m_contentSize);

} // ui
