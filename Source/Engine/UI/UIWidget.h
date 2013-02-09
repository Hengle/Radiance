/*! \file UIWidget.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#pragma once

#include "UIWidgetDef.h"
#include "../Input.h"
#include "../Lua/LuaRuntime.h"
#include "../Renderer/Material.h"
#include "../Assets/MaterialParser.h"
#include "../Packages/PackagesDef.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Thread/Interlocked.h>
#include <Runtime/PushPack.h>

namespace r {
class TextModel;
} // r

namespace ui {

RAD_ZONE_DEC(RADENG_API, ZUI);

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Rect {
public:
	Rect() {}
	Rect(const Rect &r) : x(r.x), y(r.y), w(r.w), h(r.h) {}
	Rect(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h) {}

	Rect &Intersect(const Rect &r);
	
	float x, y, w, h;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS RBDraw {
public:
	typedef RBDrawRef Ref;
	typedef RBDrawWRef WRef;

	static Ref New();

	virtual void SetViewport(
		int src[4],
		int dst[4]
	) = 0;

	virtual void DrawRect(
		const Rect &r, 
		const Rect *clip,
		const Vec3 &zRot, // (X, Y) is rotation center, Z is rotation in degrees
		r::Material &m,
		asset::MaterialLoader *l,
		bool sampleMaterialColor = true,
		const Vec4 &rgba = Vec4(1, 1, 1, 1)
	) = 0;

	virtual void DrawTextModel(
		const Rect &r,
		const Rect *clip,
		const Vec3 &zRot, // (X, Y) is rotation center, Z is rotation in degrees
		r::Material &material,
		r::TextModel &model,
		bool sampleMaterialColor = true,
		const Vec4 &rgba = Vec4(1, 1, 1, 1)
	) = 0;

	virtual void BeginBatchText(
		const Rect &r,
		const Rect *clip,
		r::Material &material
	) = 0;

	virtual void BatchDrawTextModel(
		r::Material &material,
		r::TextModel &model
	) = 0;

	virtual void EndBatchText() = 0;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Root : public boost::enable_shared_from_this<Root> {
public:
	typedef RootRef Ref;
	typedef RootWRef WRef;

	~Root();

	// If lua is not used this can be NULL.
	static Ref New(lua_State *L);
	
	RAD_DECLARE_READONLY_PROPERTY(Root, rbDraw, const RBDraw::Ref&);
	RAD_DECLARE_READONLY_PROPERTY(Root, focus, const WidgetRef&);
	RAD_DECLARE_READONLY_PROPERTY(Root, mapping, const Vec2&);
	
	void SetRootWidget(int layer, const WidgetRef &root);
	WidgetRef RootWidget(int layer);

	void Tick(float time, float dt, bool tickMaterials);
	void Draw(const Rect *clip, bool children=true);
	bool HandleInputEvent(const InputEvent &e, const TouchState *touch, const InputState &is);
	bool HandleInputGesture(const InputGesture &g, const TouchState &touch, const InputState &is);

	void SetSourceViewport(
		int vpx,
		int vpy,
		int vpw,
		int vph
	);

	void SetDestViewport(
		int vpx,
		int vpy,
		int vpw,
		int vph
	);

	Vec2 Map(const Vec2 &v);

	void AddTickMaterial(const pkg::AssetRef &ref);

private:

	typedef zone_map<int, WidgetRef, ZUIT>::type WidgetMap;

	friend class Widget;

	void SetFocus(const WidgetRef &w);
	void SetCapture(const WidgetRef &w);
	InputEvent Map(const InputEvent &e);
	InputGesture Map(const InputGesture &g);

	RAD_DECLARE_GET(rbDraw, const RBDraw::Ref&) { return m_rbdraw; }
	RAD_DECLARE_GET(focus, const WidgetRef&) { return m_focus; }
	RAD_DECLARE_GET(mapping, const Vec2&) { return m_mapping; }

	Root();

	int m_srcvp[4];
	int m_dstvp[4];
	Vec2 m_mapping;
	RBDraw::Ref m_rbdraw;
	WidgetRef m_focus;
	WidgetRef m_capture;
	WidgetMap m_layers;
	pkg::AssetIdMap m_materials;
};

///////////////////////////////////////////////////////////////////////////////
// Lua Marshling helpers

#define UIW_DECL_GET(_name) \
	LUART_DECL_GET(_name)

#define UIW_DECL_SET(_name) \
	LUART_DECL_SET(_name)

#define UIW_DECL_GETSET(_name) \
	LUART_DECL_GETSET(_name)

///////////////////////////////////////////////////////////////////////////////

#define UIW_GET(_class, _name, _type, _member) \
	LUART_GET(_class, _name, _type, _member, Ref self = GetRef<_class>(L, #_class, 1, true))

#define UIW_GET_CUSTOM(_class, _name, _push) \
	LUART_GET_CUSTOM(_class, _name, Ref self = GetRef<_class>(L, #_class, 1, true), _push)

#define UIW_SET(_class, _name, _type, _member) \
	LUART_SET(_class, _name, _type, _member, Ref self = GetRef<_class>(L, #_class, 1, true))

#define UIW_SET_CUSTOM(_class, _name, _get) \
	LUART_SET_CUSTOM(_class, _name, Ref self = GetRef<_class>(L, #_class, 1, true), _get)

#define UIW_GETSET(_class, _name, _type, _member) \
	LUART_GETSET(_class, _name, _type, _member, Ref self = GetRef<_class>(L, #_class, 1, true))

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Widget : public lua::SharedPtr {
public:
	typedef WidgetRef Ref;
	typedef WidgetWRef WRef;
	typedef zone_vector<Ref, ZUIT>::type Vec;

	enum HorizontalAlign {
		kHorizontalAlign_Left,
		kHorizontalAlign_Right,
		kHorizontalAlign_Center
	};

	enum VerticalAlign {
		kVerticalAlign_Top,
		kVerticalAlign_Bottom,
		kVerticalAlign_Center
	};

	enum PositionMode {
		kPositionMode_Relative,
		kPositionMode_Absolute
	};

	Widget();
	Widget(const Rect &r);
	virtual ~Widget();

	// If widget has lua integration, then the lua widget spawn function
	// must be at -1
	bool Spawn(lua_State *L, lua_State *Lco, bool luaError = true);

	void AddChild(const Ref &widget);
	void RemoveChild(const Ref &widget);

	Rect ToScreen(const Rect &r) const;
	Rect ToWidget(const Rect &r) const;

	RAD_DECLARE_READONLY_PROPERTY(Widget, parent, Ref);
	RAD_DECLARE_READONLY_PROPERTY(Widget, root, Root::Ref);
	RAD_DECLARE_READONLY_PROPERTY(Widget, rbDraw, const RBDraw::Ref&);
	RAD_DECLARE_READONLY_PROPERTY(Widget, screenRect, Rect);
	RAD_DECLARE_READONLY_PROPERTY(Widget, id, int);
	RAD_DECLARE_READONLY_PROPERTY(Widget, color, const Vec4&);
	RAD_DECLARE_READONLY_PROPERTY(Widget, blendedColor, Vec4); // may include parent
	RAD_DECLARE_READONLY_PROPERTY(Widget, zRot, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(Widget, zRotScreen, Vec3);
	RAD_DECLARE_PROPERTY(Widget, rect, const Rect &, const Rect&);
	RAD_DECLARE_PROPERTY(Widget, visible, bool, bool);
	RAD_DECLARE_PROPERTY(Widget, valign, VerticalAlign, VerticalAlign);
	RAD_DECLARE_PROPERTY(Widget, halign, HorizontalAlign, HorizontalAlign);
	RAD_DECLARE_PROPERTY(Widget, positionMode, PositionMode, PositionMode);
	RAD_DECLARE_PROPERTY(Widget, zRotationSpeed, float, float);
	RAD_DECLARE_PROPERTY(Widget, zAngle, float, float);
	RAD_DECLARE_PROPERTY(Widget, clip, bool, bool);
	RAD_DECLARE_PROPERTY(Widget, clipRect, const Rect&, const Rect&); // <-- widget space
	RAD_DECLARE_PROPERTY(Widget, contentPos, const Vec2&, const Vec2);
	RAD_DECLARE_PROPERTY(Widget, blendWithParent, bool, bool);
	
	void Tick(float time, float dt);
	//! clip rect is in screen space
	void Draw(const Rect *clip, bool children=true);
	void BlendTo(const Vec4 &dst, float time);
	void ScaleTo(const Vec2 &scale, const Vec2 &time);
	void MoveTo(const Vec2 &pos, const Vec2 &time);
	void RotateTo(const Vec3 &zRot, const Vec3 &time, bool shortestAngle);
	void SetFocus();
	void SetCapture(bool capture);
	bool HandleInputEvent(const InputEvent &e, const TouchState *touch, const InputState &is);
	bool HandleInputGesture(const InputGesture &g, const TouchState &touch, const InputState &is);

	// We pack extra stuff in the widget table including the shared_ptr, which is why we have a
	// GetRef call.
	template <typename T>
	static boost::shared_ptr<T> GetRef(lua_State *L, const char *tname, int index, bool luaError) {
		if (lua_type(L, index) != LUA_TTABLE) {
			if (luaError)
				luaL_typerror(L, index, tname);
			lua_pop(L, 1);
			return boost::shared_ptr<T>();
		}

		lua_getfield(L, index, "@sp");
		if (lua_type(L, -1) != LUA_TTABLE) {
			if (luaError)
				luaL_typerror(L, index, tname);
			lua_pop(L, 1);
			return boost::shared_ptr<T>();
		}

		boost::shared_ptr<T> r = lua::SharedPtr::Get<T>(L, tname, -1, luaError);
		lua_pop(L, 1);
		return r;
	}

	bool PushFrame(lua_State *L);
	bool PushCall(lua_State *L, const char *name);
	static bool Call(lua_State *L, const char *context, int nargs, int nresults, int errfunc);

	bool InBounds(const InputEvent &e);
	bool InBounds(const InputGesture &g);
	bool InBounds(float x, float y);

protected:

	virtual void PushCallTable(lua_State *L);
	virtual void CreateFromTable(lua_State *L);

	virtual void OnTick(float time, float dt) {}
	virtual void OnDraw(const Rect *clip) {}
	virtual void OnFocusChanged(bool gotFocus) {}
	virtual void AddedToRoot();
	virtual void RemovedFromRoot();

	virtual bool InputEventFilter(const InputEvent &e, const TouchState *touch, const InputState &is) {
		return false;
	}

	virtual bool InputGestureFilter(const InputGesture &g, const TouchState &touch, const InputState &is) {
		return false;
	}

	virtual bool OnInputEvent(const InputEvent &e, const InputState &is) { 
		return false; 
	}

	virtual bool OnInputGesture(const InputGesture &g, const InputState &is) { 
		return false; 
	}

private:

	void Init();

	friend class Root;

	static int lua_AddChild(lua_State *L);
	static int lua_RemoveChild(lua_State *L);
	static int lua_BlendTo(lua_State *L);
	static int lua_ScaleTo(lua_State *L);
	static int lua_MoveTo(lua_State *L);
	static int lua_RotateTo(lua_State *L);
	static int lua_SetCapture(lua_State *L);
	static int lua_Unmap(lua_State *L);

	void Push(lua_State *L); // Don't call this! Call PushFrame()
	void Unmap(lua_State *L);

	UIW_DECL_GETSET(Rect);
	UIW_DECL_GET(ScreenRect);
	UIW_DECL_GET(zRot);
	UIW_DECL_GET(zRotScreen);
	UIW_DECL_GET(Color);
	UIW_DECL_GETSET(Visible);
	UIW_DECL_GETSET(VAlign);
	UIW_DECL_GETSET(HAlign);
	UIW_DECL_SET(Tick);
	UIW_DECL_GETSET(PositionMode);
	UIW_DECL_GET(zRotationSpeed);
	UIW_DECL_SET(ZRotationSpeed);
	UIW_DECL_GET(zAngle);
	UIW_DECL_SET(ZAngle);
	UIW_DECL_GETSET(ClipRect);
	UIW_DECL_GETSET(ContentPos);
	UIW_DECL_GETSET(BlendWithParent);

	RAD_DECLARE_GET(parent, Ref) { 
		return m_parent.lock(); 
	}

	RAD_DECLARE_GET(root, Root::Ref) { 
		return m_root.lock(); 
	}

	RAD_DECLARE_GET(rbDraw, const RBDraw::Ref&) { 
		return m_root.lock()->m_rbdraw; 
	}

	RAD_DECLARE_GET(rect, const Rect&) { 
		return m_rect;
	}

	RAD_DECLARE_SET(rect, const Rect&) {	
		m_rect = value;
	}

	RAD_DECLARE_GET(screenRect, Rect);

	RAD_DECLARE_GET(id, int) { 
		return m_id; 
	}

	RAD_DECLARE_GET(color, const Vec4&) { 
		return m_color[0]; 
	}

	RAD_DECLARE_GET(zRot, const Vec3&) {
		return m_zRotPlusRate;
	}

	RAD_DECLARE_GET(zRotScreen, Vec3);

	RAD_DECLARE_GET(visible, bool);

	RAD_DECLARE_SET(visible, bool) { 
		m_visible = value; 
	}

	RAD_DECLARE_GET(valign, VerticalAlign) { 
		return m_valign; 
	}

	RAD_DECLARE_SET(valign, VerticalAlign) { 
		m_valign = value; 
	}

	RAD_DECLARE_GET(halign, HorizontalAlign) { 
		return m_halign; 
	}

	RAD_DECLARE_SET(halign, HorizontalAlign) {
		m_halign = value; 
	}

	RAD_DECLARE_GET(positionMode, PositionMode) {
		return m_positionMode;
	}

	RAD_DECLARE_SET(positionMode, PositionMode) {
		m_positionMode = value;
	}

	RAD_DECLARE_GET(zRotationSpeed, float) {
		return m_zRate[1];
	}

	RAD_DECLARE_SET(zRotationSpeed, float) {
		m_zRate[1] = value;
	}

	RAD_DECLARE_GET(zAngle, float) {
		return m_zRate[0];
	}

	RAD_DECLARE_SET(zAngle, float) {
		m_zRate[0] = value;
	}

	RAD_DECLARE_GET(clip, bool) {
		return m_clip;
	}

	RAD_DECLARE_SET(clip, bool) {
		m_clip = value;
	}

	RAD_DECLARE_GET(clipRect, const Rect&) {
		return m_clipRect;
	}

	RAD_DECLARE_SET(clipRect, const Rect&) {
		m_clipRect = value;
	}

	RAD_DECLARE_GET(contentPos, const Vec2&) {
		return m_contentPos;
	}

	RAD_DECLARE_SET(contentPos, const Vec2&) {
		m_contentPos = value;
	}

	RAD_DECLARE_GET(blendWithParent, bool) {
		return m_blendWithParent;
	}

	RAD_DECLARE_SET(blendWithParent, bool) {
		m_blendWithParent = value;
	}

	RAD_DECLARE_GET(blendedColor, Vec4);

	Vec m_children;
	Rect m_rect;
	Rect m_clipRect;
	Vec4 m_color[3];
	WRef m_parent;
	Root::WRef m_root;
	int m_id;
	bool m_visible;
	float m_fadeTime[2];
	Vec2 m_contentPos;
	Vec2 m_scale[3];
	Vec2 m_scaleTime[2];
	Vec2 m_move[2];
	Vec2 m_moveTime[2];
	Vec2 m_zRotPos[2];
	float m_zRotAngle[3];
	Vec3 m_zRotTime[2];
	Vec3 m_zRot;
	Vec3 m_zRotPlusRate;
	float m_zRate[2];
	lua_State *L;
	VerticalAlign m_valign;
	HorizontalAlign m_halign;
	PositionMode m_positionMode;
	bool m_tick;
	bool m_capture;
	bool m_clip;
	bool m_blendWithParent;

	static int NextWidgetId(lua_State *L);
	static int s_id;
};

} // ui

namespace lua {

template <>
struct Marshal<ui::Rect> {
	static void Push(lua_State *L, const ui::Rect &val) {
		Marshal<Vec4>::Push(L, Vec4(val.x, val.y, val.w, val.h));
	}

	static ui::Rect Get(lua_State *L, int index, bool luaError) {
		Vec4 v = Marshal<Vec4>::Get(L, index, luaError);
		return ui::Rect(v[0], v[1], v[2], v[3]);
	}

	static bool IsA(lua_State *L, int index) {
		return Marshal<Vec4>::IsA(L, index);
	}
};

} // lua

#include <Runtime/PopPack.h>
