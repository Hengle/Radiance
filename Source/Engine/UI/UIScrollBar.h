/*! \file UIScrollBar.h
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

class VScrollBar;
struct VScrollBarMovedEventData {
	VScrollBar *instigator;
};

typedef Event<VScrollBarMovedEventData, EventNoAccess> VScrollBarMovedEvent;

//! Manages and renders a scroll bar (not a widget and doesn't fit in a widget hierarchy)
class VScrollBar {
public:
	typedef boost::shared_ptr<VScrollBar> Ref;
	typedef boost::weak_ptr<VScrollBar> WRef;

	VScrollBar();

	void Initialize(
		float arrowHeight,
		float minThumbSize,
		const pkg::AssetRef &arrow,
		const pkg::AssetRef &arrowPressed,
		const pkg::AssetRef &track,
		const pkg::AssetRef &thumbTop,
		const pkg::AssetRef &thumbTopPressed,
		const pkg::AssetRef &thumbMiddle,
		const pkg::AssetRef &thumbMiddlePressed,
		const pkg::AssetRef &thumbBottom,
		const pkg::AssetRef &thumbBottomPressed
	);

	RAD_DECLARE_PROPERTY(VScrollBar, contentSize, float, float);
	RAD_DECLARE_PROPERTY(VScrollBar, rect, const Rect&, const Rect&);
	RAD_DECLARE_PROPERTY(VScrollBar, scrollPos, float, float);
	RAD_DECLARE_PROPERTY(VScrollBar, autoScrollSpeed, float, float);
	RAD_DECLARE_PROPERTY(VScrollBar, autoFade, bool, bool);
	RAD_DECLARE_READONLY_PROPERTY(VScrollBar, thumbSize, float);
	RAD_DECLARE_READONLY_PROPERTY(VScrollBar, thumbMaxPos, float);

	void Tick(float time, float dt);
	void Draw(Widget &self, const Rect *clip);
	void BlendTo(const Vec4 &color, float time);
	bool HandleInputEvent(Widget &self, const InputEvent &e, const TouchState *touch, const InputState &is);
	
	VScrollBarMovedEvent OnMoved;

private:

	void RecalcBar();
	void ClampThumb();
	void CalcThumbPos();
	void CalcScrollPos();
	bool HandleMouseMove(Widget &self, const InputEvent &e);
	bool HandleMouseDown(Widget &self, const InputEvent &e);
	bool HandleMouseUp(Widget &self, const InputEvent &e);
	bool TestEventRect(Widget &self, const InputEvent &e);
	bool InBounds(Widget &self, const InputEvent &e);
	Rect ScreenRect(Widget &self);

	RAD_DECLARE_GET(contentSize, float) {
		return m_contentSize;
	}

	RAD_DECLARE_SET(contentSize, float) {
		m_contentSize = value;
		RecalcBar();
	}

	RAD_DECLARE_GET(rect, const Rect&) {
		return m_rect;
	}

	RAD_DECLARE_SET(rect, const Rect&) {
		m_rect = value;
		RecalcBar();
	}

	RAD_DECLARE_GET(scrollPos, float) {
		return m_scrollPos;
	}

	RAD_DECLARE_SET(scrollPos, float) {
		m_scrollPos = value;
		CalcThumbPos();
		ClampThumb();
	}

	RAD_DECLARE_GET(thumbSize, float) {
		return m_thumbSize;
	}

	RAD_DECLARE_GET(thumbMaxPos, float) {
		return m_thumbMaxPos;
	}

	RAD_DECLARE_GET(autoScrollSpeed, float) {
		return m_autoScrollSpeed;
	}

	RAD_DECLARE_SET(autoScrollSpeed, float) {
		m_autoScrollSpeed = value;
	}

	RAD_DECLARE_GET(autoFade, bool) {
		return m_autoFade;
	}

	RAD_DECLARE_SET(autoFade, bool);

	struct Materials {
		asset::MaterialBundle arrow;
		asset::MaterialBundle arrowPressed;
		asset::MaterialBundle track;
		asset::MaterialBundle thumbTop;
		asset::MaterialBundle thumbTopPressed;
		asset::MaterialBundle thumbMiddle;
		asset::MaterialBundle thumbMiddlePressed;
		asset::MaterialBundle thumbBottom;
		asset::MaterialBundle thumbBottomPressed;
	};

	enum State {
		kState_None,
		kState_ClickUp,
		kState_ClickDown,
		kState_Drag
	};

	Materials m_materials;
	Rect m_rect;
	boost::array<Vec4, 3> m_blendColor;
	int m_mousePos;
	float m_contentHeight;
	float m_contentSize;
	float m_thumbContentSize;
	float m_thumbPos;
	float m_scrollPos;
	float m_origThumbPos;
	float m_thumbSize;
	float m_thumbMaxPos;
	float m_scrollMaxPos;
	float m_minThumbSize;
	float m_arrowHeight;
	float m_autoScrollTimer;
	float m_autoScrollSpeed;
	float m_autoScrollDelay;
	bool m_autoFade;
	boost::array<float, 2> m_blendTime;
	State m_state;
};

} // ui

#include <Runtime/PopPack.h>
