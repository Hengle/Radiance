/*! \file UIVListWidget.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#pragma once

#include "UIWidget.h"
#include "../Physics/Spring.h"
#include <Runtime/PushPack.h>

namespace ui {

//! A vertically oriented list of widgets.
/*! Contains and organizes a list of widgets vertically, and allows the user
    to scroll this list. */
class VListWidget : public Widget {
public:
	typedef boost::shared_ptr<VListWidget> Ref;
	typedef boost::weak_ptr<VListWidget> WRef;

	VListWidget();
	VListWidget(const Rect &r);
	virtual ~VListWidget() {}

	RAD_DECLARE_PROPERTY(VListWidget, scroll, const Vec2&, const Vec2&);
	RAD_DECLARE_PROPERTY(VListWidget, friction, float, float);
	RAD_DECLARE_PROPERTY(VListWidget, stopSpring, const physics::Spring&, const physics::Spring&);
	RAD_DECLARE_PROPERTY(VListWidget, stopSpringVertex, const physics::SpringVertex&, const physics::SpringVertex&);
	RAD_DECLARE_PROPERTY(VListWidget, endStops, const Vec2&, const Vec2&);
	RAD_DECLARE_READONLY_PROPERTY(VListWidget, contentSize, const Vec2&);
	RAD_DECLARE_READONLY_PROPERTY(VListWidget, items, const Widget::Vec*);

	void ItemChanged();
	void AddItem(const Widget::Ref &widget);
	void RemoveItem(const Widget::Ref &widget);
	void Clear();

protected:

	virtual void OnTick(float time, float dt);
	virtual void PushCallTable(lua_State *L);
	virtual bool InputEventFilter(const InputEvent &e, const TouchState *touch, const InputState &is);

private:

	void Init();
	void RecalcLayout();
	void Drag(const InputEvent &e);
	bool ProcessInputEvent(const InputEvent &e, const TouchState *state, const InputState &is);
	void CheckPostDelayedInput();
	void Scroll(const Vec2 &delta);

	bool ApplyVelocity(float dt);

	RAD_DECLARE_GET(scroll, const Vec2&) {
		return m_scroll;
	}

	RAD_DECLARE_SET(scroll, const Vec2&) {
		scroll = value;
	}

	RAD_DECLARE_GET(stopSpring, const physics::Spring&) {
		return m_spring;
	}

	RAD_DECLARE_SET(stopSpring, const physics::Spring&) {
		m_spring = value;
	}

	RAD_DECLARE_GET(stopSpringVertex, const physics::SpringVertex&) {
		return m_vertex;
	}

	RAD_DECLARE_SET(stopSpringVertex, const physics::SpringVertex&) {
		m_vertex = value;
	}
	
	RAD_DECLARE_GET(endStops, const Vec2&) {
		return m_endStops;
	}

	RAD_DECLARE_SET(endStops, const Vec2&) {
		m_endStops = value;
	}

	RAD_DECLARE_GET(friction, float) {
		return m_friction;
	}

	RAD_DECLARE_SET(friction, float) {
		m_friction = value;
	}

	RAD_DECLARE_GET(contentSize, const Vec2&) {
		return m_contentSize;
	}

	RAD_DECLARE_GET(items, const Widget::Vec*) {
		return &m_widgets;
	}

	static int lua_ItemChanged(lua_State *L);
	static int lua_AddItem(lua_State *L);
	static int lua_RemoveItem(lua_State *L);
	static int lua_Items(lua_State *L);
	static int lua_Clear(lua_State *L);

	UIW_DECL_GETSET(Scroll);
	UIW_DECL_GETSET(StopSpring);
	UIW_DECL_GETSET(StopSpringVertex);
	UIW_DECL_GETSET(Friction);
	UIW_DECL_GETSET(EndStops);
	UIW_DECL_GET(ContentSize);

	Widget::Vec m_widgets;
	Vec2 m_scroll;
	Vec2 m_velocity;
	physics::Spring m_spring;
	physics::SpringVertex m_vertex;
	InputState m_is;
	TouchState m_ts;
	TouchState *m_pts;
	InputEvent m_e;
	Vec3 m_springRoot;
	Vec2 m_contentSize;
	Vec2 m_endStops;
	Vec2 m_dragMotion;
	float m_friction;
	bool m_dragging;
	bool m_dragMove;
	bool m_dragDidMove;
	bool m_postingEvent;
	bool m_endStop;
	bool m_checkDelayed;
};

} // ui

#include <Runtime/PopPack.h>
