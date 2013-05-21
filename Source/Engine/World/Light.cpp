/*! \file Light.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "World.h"
#include "Light.h"
#include "Entity.h"

namespace world {

Light::Light(World *w) : 
m_interactionHead(0),
m_pos(Vec3::Zero),
m_style(kStyle_Diffuse|kStyle_Specular|kStyle_CastShadows),
m_brightness(1.f),
m_radius(400.f),
m_interactionFlags(0),
m_leaf(0),
m_prev(0),
m_next(0),
m_world(w),
m_markFrame(-1),
m_visFrame(-1),
m_drawFrame(-1) {
	m_shColor = Vec4(0,0,0,1);
	m_spColor = Vec3(1,1,1);
	m_dfColor = Vec3(1,1,1);
	m_bounds = BBox(-Vec3(m_radius, m_radius, m_radius), Vec3(m_radius, m_radius, m_radius));
}

Light::~Light() {
}

void Light::Link() {
	BBox bounds(m_bounds);
	bounds.Translate(m_pos);

	m_world->LinkLight(
		*this,
		bounds
	);
}

void Light::Unlink() {
	m_world->UnlinkLight(*this);
}

details::LightInteraction **Light::ChainHead(int matId) {
	std::pair<details::MatInteractionChain::iterator, bool> r = 
		m_matInteractionChain.insert(details::MatInteractionChain::value_type(matId, (details::LightInteraction *)0));
	return &r.first->second;
}

} // world
