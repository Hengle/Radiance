/*! \file Light.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once
#include "WorldDef.h"
#include <Runtime/Container/ZoneVector.h>

namespace r {
class Material;
};

namespace world {

class RADENG_CLASS Light : public boost::noncopyable {
public:
	typedef boost::shared_ptr<Light> Ref;

	RAD_BEGIN_FLAGS
		RAD_FLAG(kStyle_Diffuse),
		RAD_FLAG(kStyle_Specular),
		RAD_FLAG(kStyle_CastShadows),
		kStyle_DiffuseSpecular = kStyle_Diffuse|kStyle_Specular
	RAD_END_FLAGS(LightStyle)

	enum {
		RAD_FLAG(kInteractionFlag_World),
		RAD_FLAG(kInteractionFlag_Player),
		RAD_FLAG(kInteractionFlag_Objects)		
	};

	struct IntensityStep {
		typedef zone_vector<IntensityStep, ZWorldT>::type Vec;
		float intensity;
		double time;
	};

	struct ColorStep {
		typedef zone_vector<ColorStep, ZWorldT>::type Vec;
		Vec3 color;
		double time;
	};

	~Light();

	RAD_DECLARE_PROPERTY(Light, diffuseColor, const Vec3&, const Vec3&);
	// alpha control specular power
	RAD_DECLARE_PROPERTY(Light, specularColor, const Vec3&, const Vec3&);
	// alpha controls shadow opacity
	RAD_DECLARE_PROPERTY(Light, shadowColor, const Vec4&, const Vec4&);
	RAD_DECLARE_PROPERTY(Light, style, LightStyle, LightStyle);
	RAD_DECLARE_PROPERTY(Light, pos, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(Light, radius, float, float);
	RAD_DECLARE_PROPERTY(Light, bounds, const BBox&, const BBox&);
	RAD_DECLARE_PROPERTY(Light, intensity, float, float);
	RAD_DECLARE_PROPERTY(Light, interactionFlags, int, int);
	RAD_DECLARE_READONLY_PROPERTY(Light, baseIntensity, float);

	void Link();
	void Unlink();
	void Tick(float dt);

	void AnimateIntensity(const IntensityStep::Vec &vec, bool loop);
	void AnimateDiffuseColor(const ColorStep::Vec &vec, bool loop);
	void AnimateSpecularColor(const ColorStep::Vec &vec, bool loop);

private:

	friend class World;
	friend class WorldDraw;

	Light(World *w);

	RAD_DECLARE_GET(diffuseColor, const Vec3&) {
		return m_dfColor[0];
	}

	RAD_DECLARE_SET(diffuseColor, const Vec3&) {
		m_dfColor[0] = value;
	}

	RAD_DECLARE_GET(specularColor, const Vec3&) {
		return m_spColor[0];
	}

	RAD_DECLARE_SET(specularColor, const Vec3&) {
		m_spColor[0] = value;
	}

	RAD_DECLARE_GET(shadowColor, const Vec4&) {
		return m_shColor;
	}

	RAD_DECLARE_SET(shadowColor, const Vec4&) {
		m_shColor = value;
	}

	RAD_DECLARE_GET(style, LightStyle) {
		return m_style;
	}

	RAD_DECLARE_SET(style, LightStyle) {
		m_style = value;
	}

	RAD_DECLARE_GET(pos, const Vec3&) {
		return m_pos;
	}

	RAD_DECLARE_SET(pos, const Vec3&) {
		m_pos = value;
	}

	RAD_DECLARE_GET(radius, float) {
		return m_radius;
	}

	RAD_DECLARE_SET(radius, float) {
		m_radius = value;
	}

	RAD_DECLARE_GET(bounds, const BBox&) {
		return m_bounds;
	}

	RAD_DECLARE_SET(bounds, const BBox&) {
		m_bounds = value;
	}

	RAD_DECLARE_GET(baseIntensity, float) {
		return m_baseIntensity;
	}

	RAD_DECLARE_GET(intensity, float) {
		return m_intensity[0];
	}

	RAD_DECLARE_SET(intensity, float) {
		m_baseIntensity = value;
		m_intensity[0] = value;
	}

	RAD_DECLARE_GET(interactionFlags, int) {
		return m_interactionFlags;
	}

	RAD_DECLARE_SET(interactionFlags, int) {
		m_interactionFlags = value;
	}

	details::LightInteraction **ChainHead(int matId);

	void AnimateIntensity(float dt);

	void AnimateColor(
		float dt,
		ColorStep::Vec &steps,
		int &index,
		double &time,
		Vec3 *color,
		bool loop
	);

	void InitColorSteps(
		const ColorStep::Vec &srcVec,
		bool srcLoop,
		ColorStep::Vec &vec,
		Vec3 *color,
		double &time,
		int &index,
		bool &loop
	);
	
	IntensityStep::Vec m_intensitySteps;
	ColorStep::Vec m_dfSteps;
	ColorStep::Vec m_spSteps;

	dBSPLeaf::PtrVec m_bspLeafs;
	details::MatInteractionChain m_matInteractionChain; // linkage to draws
	details::LightInteraction *m_interactionHead; // linkage to objects
	IntSet m_areas;
	BBox m_bounds;
	Vec3 m_spColor[2];
	Vec3 m_dfColor[2];
	Vec4 m_shColor;
	Vec4 m_scissor; // set by renderer
	Vec3 m_pos;
	float m_baseIntensity;
	float m_intensity[2];
	float m_radius;
	double m_intensityTime;
	double m_dfTime;
	double m_spTime;
	LightStyle m_style;
	int m_markFrame;
	int m_visFrame;
	int m_drawFrame;
	int m_exactlyCulledFrame;
	int m_interactionFlags;
	int m_intensityStep;
	int m_dfStep;
	int m_spStep;
	bool m_intensityLoop;
	bool m_dfLoop;
	bool m_spLoop;
	dBSPLeaf *m_leaf;
	Light *m_prev;
	Light *m_next;
	World *m_world;
};

} // world

RAD_IMPLEMENT_FLAGS(world::Light::LightStyle)