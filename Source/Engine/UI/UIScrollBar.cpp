/*! \file UIScrollBar.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#include RADPCH
#include "UIScrollBar.h"

namespace ui {

VScrollBar::VScrollBar() :
m_contentSize(0.f),
m_contentHeight(0.f),
m_thumbContentSize(0.f),
m_thumbPos(0.f),
m_scrollPos(0.f),
m_origThumbPos(0.f),
m_thumbSize(0.f),
m_thumbMaxPos(0.f),
m_scrollMaxPos(0.f),
m_minThumbSize(0.f),
m_arrowHeight(0.f),
m_state(kState_None),
m_autoScrollTimer(0.f),
m_autoScrollSpeed(0.f),
m_autoScrollDelay(0.f),
m_autoFade(false) {
	m_rect.x = 0.f;
	m_rect.y = 0.f;
	m_rect.w = 1.f;
	m_rect.h = 1.f;
	m_blendColor[0] = m_blendColor[1] = m_blendColor[2] = Vec4(1,1,1,1);
	m_blendTime[0] = m_blendTime[1] = 0.f;
}

void VScrollBar::Initialize(
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
) {
	m_arrowHeight = arrowHeight;
	m_minThumbSize = minThumbSize;
	m_materials.arrow.Bind(arrow);
	m_materials.arrowPressed.Bind(arrowPressed);
	m_materials.track.Bind(track);
	m_materials.thumbTop.Bind(thumbTop);
	m_materials.thumbTopPressed.Bind(thumbTopPressed);
	m_materials.thumbMiddle.Bind(thumbMiddle);
	m_materials.thumbMiddlePressed.Bind(thumbMiddlePressed);
	m_materials.thumbBottom.Bind(thumbBottom);
	m_materials.thumbBottomPressed.Bind(thumbBottomPressed);
}

void VScrollBar::Tick(float time, float dt) {

	if (m_blendTime[1] > 0.f) {
		m_blendTime[0] += dt;
		if (m_blendTime[0] >= m_blendTime[1]) {
			m_blendTime[1] = 0.f;
			m_blendColor[0] = m_blendColor[2];
		} else {
			m_blendColor[0] = math::Lerp(m_blendColor[1], m_blendColor[2], m_blendTime[0]/m_blendTime[1]);
		}
	}

	if ((m_state != kState_ClickUp) && (m_state != kState_ClickDown))
		return;

	m_autoScrollTimer += dt;
	if (m_autoScrollTimer >= m_autoScrollDelay) {
		m_autoScrollTimer -= m_autoScrollDelay;
		m_autoScrollDelay = 0.05f;

		if (m_autoScrollSpeed > 0.f) {

			float old = m_scrollPos;

			if (m_state == kState_ClickUp) {
				m_scrollPos -= m_autoScrollSpeed;
			} else {
				m_scrollPos += m_autoScrollSpeed;
			}

			CalcThumbPos();
			ClampThumb();

			if (old != m_scrollPos) {
				VScrollBarMovedEventData data;
				data.instigator = this;
				OnMoved.Trigger(data);
			}
		}
	}
}

void VScrollBar::Draw(Widget &self, const Rect *clip) {
	Rect screen = self.screenRect;
	const Vec4 kBlendedColor = self.blendedColor.get() * m_blendColor[0];

	Rect r(m_rect);
	r.Translate(screen);

	// arrows
	asset::MaterialBundle *bundle = (m_state==kState_ClickUp) ? &m_materials.arrowPressed : &m_materials.arrow;
	r.h = m_arrowHeight;
		
	self.rbDraw->DrawRect(
		r,
		clip,
		Vec3::Zero,
		*bundle->material,
		bundle->loader,
		true,
		kBlendedColor
	);

	bundle = (m_state==kState_ClickDown) ? &m_materials.arrowPressed : &m_materials.arrow;

	r.y = screen.y + m_rect.h - r.h;
	self.rbDraw->DrawRect(
		r,
		clip,
		Vec3(r.x+(r.w/2.f), r.y+(r.h/2.f), 180.f),
		*bundle->material,
		bundle->loader,
		true,
		kBlendedColor
	);

	// track
	r.y = screen.y + m_rect.y + m_arrowHeight;
	r.h = m_rect.h - (m_arrowHeight*2.f);

	self.rbDraw->DrawRect(
		r,
		clip,
		Vec3::Zero,
		*m_materials.track.material,
		m_materials.track.loader,
		true,
		kBlendedColor
	);

	// thumb
	bundle = (m_state==kState_Drag) ? &m_materials.thumbTopPressed : &m_materials.thumbTop;
	r.y = screen.y + m_rect.y + m_arrowHeight + m_thumbPos;
	r.h = m_minThumbSize / 2.f;
	
	self.rbDraw->DrawRect(
		r,
		clip,
		Vec3::Zero,
		*bundle->material,
		bundle->loader,
		true,
		kBlendedColor
	);

	if (m_thumbSize > 0.f) {
		bundle = (m_state==kState_Drag) ? &m_materials.thumbMiddlePressed : &m_materials.thumbMiddle;
		r.y = r.y + r.h;
		r.h = m_thumbSize;
		self.rbDraw->DrawRect(
			r,
			clip,
			Vec3::Zero,
			*bundle->material,
			bundle->loader,
			true,
			kBlendedColor
		);
	}

	bundle = (m_state==kState_Drag) ? &m_materials.thumbBottomPressed : &m_materials.thumbBottom;
	r.y = r.y + r.h;
	r.h = m_minThumbSize / 2.f;

	self.rbDraw->DrawRect(
		r,
		clip,
		Vec3::Zero,
		*bundle->material,
		bundle->loader,
		true,
		kBlendedColor
	);
}

void VScrollBar::BlendTo(const Vec4 &color, float time) {
	if (time <= 0.f) {
		m_blendTime[0] = m_blendTime[1] = 0.f;
		m_blendColor[0] = color;
	} else {
		m_blendTime[0] = 0.f;
		m_blendTime[1] = time;
		m_blendColor[1] = m_blendColor[0];
		m_blendColor[2] = color;
	}
}

bool VScrollBar::HandleInputEvent(Widget &self, const InputEvent &e, const TouchState *touch, const InputState &is) {
	if (!TestEventRect(self, e))
		return false;
	switch (e.type) {
	case InputEvent::T_MouseDown:
		return HandleMouseDown(self, e);
	case InputEvent::T_MouseUp:
		return HandleMouseUp(self, e);
	case InputEvent::T_MouseMove:
		return HandleMouseMove(self, e);
	}
	return false;
}

void VScrollBar::RecalcBar() {
	m_contentHeight = m_rect.h - (m_arrowHeight*2.f);

	if (m_rect.h >= m_contentSize) {
		m_thumbContentSize = m_contentHeight;
		m_thumbSize = m_contentHeight - m_minThumbSize;
		m_thumbSize = std::max(0.f, m_thumbSize);
		m_thumbMaxPos = 0.f;
		m_scrollPos = 0.f;
		ClampThumb();
		m_state = kState_None;
		if (m_autoFade)
			BlendTo(Vec4(1,1,1,0), 0.2f);
		return;
	}

	m_thumbContentSize = m_contentHeight * (m_contentHeight / m_contentSize);
	m_thumbSize = m_thumbContentSize - m_minThumbSize;
	m_thumbSize = std::max(0.f, m_thumbSize);
	
	m_thumbMaxPos = m_contentHeight - (m_thumbSize + m_minThumbSize);
	m_thumbMaxPos = std::max(0.f, m_thumbMaxPos);
	m_scrollMaxPos = m_contentSize - m_rect.h;
	m_scrollMaxPos = std::max(0.f, m_scrollMaxPos);

	ClampThumb();
	m_state = kState_None;

	if (m_autoFade)
		BlendTo(Vec4(1,1,1,1), 0.2f);
}

void VScrollBar::ClampThumb() {
	m_thumbPos = math::Clamp(m_thumbPos, 0.f, m_thumbMaxPos);
	m_scrollPos = math::Clamp(m_scrollPos, 0.f, m_scrollMaxPos);
}

void VScrollBar::CalcThumbPos() {
	if (m_scrollMaxPos > 0.f) {
		m_thumbPos = (m_scrollPos / m_scrollMaxPos) * m_thumbMaxPos;
	}
}

void VScrollBar::CalcScrollPos() {
	if (m_thumbMaxPos > 0.f) {
		m_scrollPos = (m_thumbPos / m_thumbMaxPos) * m_scrollMaxPos;
	}
}

bool VScrollBar::HandleMouseMove(Widget &self, const InputEvent &e) {
	if (m_state != kState_Drag)
		return true;
	if (e.data[2] != kMouseButton_Left)
		return true;

	float old = m_thumbPos;
	float dy = (float)(e.data[1] - m_mousePos);
	m_thumbPos = m_origThumbPos + dy;
	CalcScrollPos();
	ClampThumb();

	if (old != m_thumbPos) {
		VScrollBarMovedEventData data;
		data.instigator = this;
		OnMoved.Trigger(data);
	}

	return true;
}

bool VScrollBar::HandleMouseDown(Widget &self, const InputEvent &e) {
	if (m_state != kState_None)
		return true;
	if (e.data[2] != kMouseButton_Left)
		return true;
	if (m_thumbMaxPos <= 0.f)
		return true; // not-active

	Rect screenRect = ScreenRect(self);
	Rect r(screenRect);

	m_autoScrollDelay = 0.5f;

	// arrow clicks?
	r.h = m_arrowHeight;

	if (r.InBounds(e.data[0], e.data[1])) {
		self.SetCapture(true);
		m_state = kState_ClickUp;
		m_autoScrollTimer = 0.f;
		
		float old = m_scrollPos;

		m_scrollPos -= m_autoScrollSpeed;
		CalcThumbPos();
		ClampThumb();

		if (old != m_scrollPos) {
			VScrollBarMovedEventData data;
			data.instigator = this;
			OnMoved.Trigger(data);
		}
		return true;
	}

	r.y = screenRect.y + screenRect.h - m_arrowHeight;
	if (r.InBounds(e.data[0], e.data[1])) {
		self.SetCapture(true);
		m_state = kState_ClickDown;
		m_autoScrollTimer = 0.f;

		float old = m_scrollPos;

		m_scrollPos += m_autoScrollSpeed;
		CalcThumbPos();
		ClampThumb();

		if (old != m_scrollPos) {
			VScrollBarMovedEventData data;
			data.instigator = this;
			OnMoved.Trigger(data);
		}
		return true;
	}

	// thumb clicks?
	r.y = screenRect.y + m_arrowHeight + m_thumbPos;
	r.h = m_thumbSize + m_minThumbSize;

	if (r.InBounds(e.data[0], e.data[1])) {
		self.SetCapture(true);
		m_state = kState_Drag;
		m_mousePos = e.data[1];
		m_origThumbPos = m_thumbPos;
		return true;
	}

	return true;
}

bool VScrollBar::HandleMouseUp(Widget &self, const InputEvent &e) {
	if (e.data[2] == kMouseButton_Left) {
		self.SetCapture(false);
		m_state = kState_None;
	}

	return true;
}

bool VScrollBar::TestEventRect(Widget &self, const InputEvent &e) {
	
	if (m_state == kState_None) {
		if (!(e.IsMouse() || e.IsMouseButton()))
			return false;
		if (!InBounds(self, e))
			return false;
	} 

	return true;
}

bool VScrollBar::InBounds(Widget &self, const InputEvent &e) {
	Rect r = ScreenRect(self);
	return r.InBounds(e.data[0], e.data[1]);
}

Rect VScrollBar::ScreenRect(Widget &self) {
	Rect r(m_rect);
	r.Translate(self.screenRect);
	return r;
}

void VScrollBar::RAD_IMPLEMENT_SET(autoFade) (bool value) {
	m_autoFade = value;
	if (m_autoFade) {
		if (m_thumbMaxPos > 0.f) {
			BlendTo(Vec4(1,1,1,1), 0.f);
		} else {
			BlendTo(Vec4(1,1,1,0), 0.f);
		}
	} else {
		BlendTo(Vec4(1,1,1,1), 0.f);
	}
}

} // ui
