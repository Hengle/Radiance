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
m_intensity(1.f),
m_intensityTime(0.0),
m_dfTime(0.0),
m_spTime(0.0),
m_radius(400.f),
m_interactionFlags(0),
m_intensityStep(0),
m_dfStep(0),
m_spStep(0),
m_intensityLoop(false),
m_dfLoop(false),
m_spLoop(false),
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

void Light::AnimateIntensity(float dt) {
	if (!m_intensitySteps.empty()) {
		m_intensityTime += (double)dt;

		const IntensityStep *cur = &m_intensitySteps[m_intensityStep];

		if (m_intensityStep+1 >= (int)m_intensitySteps.size()) {
			// off the end
			if (m_intensityLoop) {
				m_intensityStep = 0;
				cur = &m_intensitySteps[0];
				m_intensityTime = dt;
			} else {
				m_intensity = cur->intensity;
				m_intensitySteps.clear();
				return;
			}
		}

		const IntensityStep *next = &m_intensitySteps[m_intensityStep+1];

		while (m_intensityTime > next->time) {
			
			++m_intensityStep;
			
			if (m_intensityStep+1 >= (int)m_intensitySteps.size()) {
				return; // process overflow next tick
			}
			
			cur = next;
			next = &m_intensitySteps[m_intensityStep+1];
		}

		if (!m_intensitySteps.empty()) {
			float dt = next->time - cur->time;
			float offset = m_intensityTime - cur->time;
			m_intensity = math::Lerp(cur->intensity, next->intensity, offset / dt);
		}
	}
}

void Light::AnimateColor(
	float dt,
	ColorStep::Vec &steps,
	int &index,
	double &time,
	Vec3 &color,
	bool loop
) {
	if (!steps.empty()) {
		time += (double)dt;

		const ColorStep *cur = &steps[index];

		if (index+1 >= (int)steps.size()) {
			// off the end
			if (loop) {
				index = 0;
				cur = &steps[0];
				time = dt;
			} else {
				color = cur->color;
				steps.clear();
				return;
			}
		}

		const ColorStep *next = &steps[index+1];

		while (time > next->time) {
			
			++index;
			
			if (index+1 >= (int)steps.size()) {
				return; // process overflow next tick
			}
			
			cur = next;
			next = &steps[index+1];
		}

		if (!steps.empty()) {
			float dt = next->time - cur->time;
			float offset = time - cur->time;
			color = math::Lerp(cur->color, next->color, offset / dt);
		}
	}
}

void Light::Tick(float dt) {

	AnimateIntensity(dt);
	
	AnimateColor(
		dt,
		m_dfSteps,
		m_dfStep,
		m_dfTime,
		m_dfColor,
		m_dfLoop
	);

	AnimateColor(
		dt,
		m_spSteps,
		m_spStep,
		m_spTime,
		m_spColor,
		m_spLoop
	);

}

void Light::AnimateIntensity(const IntensityStep::Vec &vec, bool loop) {
	if (vec.empty()) {
		m_intensitySteps.clear();
		return;
	}
	if (vec.size() < 2) {
		m_intensity = vec[0].intensity;
		m_intensitySteps.clear();
		return;
	}

	m_intensityLoop = loop;
	m_intensityTime = 0.0;
	m_intensityStep = 0;

	m_intensitySteps.clear();
	IntensityStep current;
	current.intensity = m_intensity;
	current.time = 0.0;
	m_intensitySteps.push_back(current);
	std::copy(vec.begin(), vec.end(), std::back_inserter(m_intensitySteps));
	
	double dt = 0.0;
	for (size_t i = 1; i < m_intensitySteps.size(); ++i) {
		m_intensitySteps[i].time += dt;
		dt += m_intensitySteps[i].time;
	}
}

void Light::InitColorSteps(
	const ColorStep::Vec &srcVec,
	bool srcLoop,
	ColorStep::Vec &vec,
	Vec3 &color,
	double &time,
	int &index,
	bool &loop
) {
	if (srcVec.empty()) {
		vec.clear();
		return;
	}
	if (srcVec.size() < 2) {
		color = srcVec[0].color;
		vec.clear();
		return;
	}

	loop = srcLoop;
	time = 0.0;
	index = 0;

	vec.clear();
	ColorStep current;
	current.color = color;
	current.time = 0.0;
	vec.push_back(current);
	std::copy(srcVec.begin(), srcVec.end(), std::back_inserter(vec));

	double dt = 0.0;
	for (size_t i = 1; i < vec.size(); ++i) {
		vec[i].time += dt;
		dt += vec[i].time;
	}
}

void Light::AnimateDiffuseColor(const ColorStep::Vec &vec, bool loop) {
	InitColorSteps(
		vec,
		loop,
		m_dfSteps,
		m_dfColor,
		m_dfTime,
		m_dfStep,
		m_dfLoop
	);
}

void Light::AnimateSpecularColor(const ColorStep::Vec &vec, bool loop) {
	InitColorSteps(
		vec,
		loop,
		m_spSteps,
		m_spColor,
		m_spTime,
		m_spStep,
		m_spLoop
	);
}

details::LightInteraction **Light::ChainHead(int matId) {
	std::pair<details::MatInteractionChain::iterator, bool> r = 
		m_matInteractionChain.insert(details::MatInteractionChain::value_type(matId, (details::LightInteraction *)0));
	return &r.first->second;
}

} // world
