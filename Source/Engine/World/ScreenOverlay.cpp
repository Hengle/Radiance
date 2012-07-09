// ScreenOverlay.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "ScreenOverlay.h"
#include "WorldDraw.h"

namespace world {

ScreenOverlay::ScreenOverlay(WorldDraw *draw, const details::MBatchRef &mbatch) :
m_draw(draw),
m_mbatch(mbatch),
m_alpha(0.f),
m_time(0.f),
m_fade(false)
{
	m_draw->AddScreenOverlay(*this);
}

ScreenOverlay::~ScreenOverlay()
{
	m_draw->RemoveScreenOverlay(*this);
}

void ScreenOverlay::Tick(float dt)
{
	if (m_fade)
	{ // fade in

		if (m_time <= 0.f)
		{
			m_alpha = 1.f;
			return;
		}

		m_alpha += dt/m_time;
		if (m_alpha >= 1.f)
		{
			m_alpha = 1.f;
			m_time = 0.f;
		}
	}
	else
	{ // fade out
		if (m_time <= 0.f)
		{
			m_alpha = 0.f;
			return;
		}

		m_alpha -= dt/m_time;
		if (m_alpha <= 0.f)
		{
			m_alpha = 0.f;
			m_time = 0.f;
		}
	}
}

void ScreenOverlay::FadeIn(float time)
{
	m_time = time;
	m_fade = true;
}

void ScreenOverlay::FadeOut(float time)
{
	m_fade = false;
	m_time = time;
}

} // world
