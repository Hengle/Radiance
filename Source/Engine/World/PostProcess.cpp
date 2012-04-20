// PostProcess.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "PostProcess.h"

namespace world {

PostProcessEffect::PostProcessEffect() : m_enabled(false)
{
	m_time[0] = m_time[1] = 0.f;
	m_color[0] = m_color[1] = m_color[2] = Vec4(1, 1, 1, 1);
}

bool PostProcessEffect::BindMaterial(const pkg::AssetRef &asset)
{
	m_asset = asset;
	m_parser = asset::MaterialParser::Cast(asset);
	m_loader = asset::MaterialLoader::Cast(asset);
	return m_loader && m_parser && m_parser->valid;
}

void PostProcessEffect::Tick(float dt)
{
	if (m_time[1] > 0.f)
	{
		m_time[0] += dt;
		if (m_time[0] >= m_time[1])
		{
			m_time[1] = 0.f;
			m_color[0] = m_color[2];
		}
		else
		{
			m_color[0] = math::Lerp(m_color[1], m_color[2], m_time[0]/m_time[1]);
		}
	}
}

void PostProcessEffect::FadeTo(const Vec4 &color, float time)
{
	if (time <= 0.f)
	{
		m_time[1] = 0.f;
		m_color[0] = color;
	}
	else
	{
		m_time[0] = 0;
		m_time[1] = time;
		m_color[1] = m_color[0];
		m_color[2] = color;
	}
}

void PostProcessEffect::BindStates()
{
}

} // world
