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
m_thumbPos(0.f),
m_thumbSize(0.f),
m_thumbMaxPos(0.f),
m_drag(false) {
	m_rect.x = 0.f;
	m_rect.y = 0.f;
	m_rect.w = 1.f;
	m_rect.h = 1.f;
}

void VScrollBar::Initialize(
	float width,
	float arrowHeight,
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

void VScrollBar::Draw(Widget &self, const Rect *clip) {
}

bool VScrollBar::HandleInputEvent(Widget &self, const InputEvent &e, const TouchState *touch, const InputState &is) {
	return false;
}

void VScrollBar::RecalcBar() {
	if ((m_contentSize <= 0.f) || (m_rect.h <= 0.f)) {
		m_thumbSize = 1.f;
		return;
	}

	float buttonHeight = m_materials.thumbTop.loader->height + m_materials.thumbBottom.loader->height;

	if (m_rect.h >= m_contentSize) {
		m_thumbSize = m_rect.h - buttonHeight;
		m_thumbSize = std::max(8.f, m_thumbSize);
		m_thumbMaxPos = 0.f;
		ClampThumb();
		m_drag = false;
		return;
	}

	m_thumbSize = m_rect.h * (m_rect.h / m_contentSize);
	m_thumbSize -= buttonHeight;
	m_thumbSize = std::max(8.f, m_thumbSize);
	
	m_thumbMaxPos = m_rect.h - m_thumbSize;
//	m_thumbMaxPos = std::max(0.f
}

} // ui
