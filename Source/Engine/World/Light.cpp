/*! \file Light.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "World.h"
#include "Light.h"

namespace world {

Light::Light(World *w) : 
m_pos(Vec3::Zero) ,
m_style(kStyle_Diffuse|kStyle_Specular|kStyle_Shadows|kStyle_AffectAll),
m_leaf(0),
m_prev(0),
m_next(0),
m_world(w) {
	m_spColor = Vec4(1,1,1,1);
	m_shColor = Vec4(0,0,0,1);
	m_dfColor = Vec3(1,1,1);
}

Light::~Light() {
}

void Light::Link() {
	BBox bounds(m_size);
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
		m_interactions.insert(details::MatInteractionChain::value_type(matId, (details::LightInteraction *)0));
	return &r.first->second;
}

} // world
