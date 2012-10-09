// ScreenOverlay.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "../Packages/PackagesDef.h"
#include "../Assets/MaterialParser.h"
#include "../Renderer/Material.h"
#include "WorldDrawDef.h"
#include <Runtime/Container/ZoneList.h>

namespace world {

class RADENG_CLASS ScreenOverlay
{
public:
	typedef boost::shared_ptr<ScreenOverlay> Ref;
	typedef zone_list<ScreenOverlay*, ZWorldT>::type List;

	~ScreenOverlay();

	RAD_DECLARE_READONLY_PROPERTY(ScreenOverlay, alpha, float);
	RAD_DECLARE_READONLY_PROPERTY(ScreenOverlay, fading, bool);

	void Tick(float dt);
	void FadeIn(float time);
	void FadeOut(float time);

private:

	friend class WorldDraw;

	ScreenOverlay(WorldDraw *draw, const details::MatRef &mat);

	RAD_DECLARE_GET(alpha, float) { return m_alpha; }
	RAD_DECLARE_GET(fading, bool) { return m_time != 0.f; }

	float m_alpha;
	float m_time;
	bool m_fade;
	List::iterator m_it;
	WorldDraw *m_draw;
	const details::MatRef *m_mat;
};

} // world
