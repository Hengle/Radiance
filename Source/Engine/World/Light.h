/*! \file Light.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once
#include "WorldDef.h"

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
	RAD_DECLARE_PROPERTY(Light, brightness, float, float);
	RAD_DECLARE_PROPERTY(Light, interactionFlags, int, int);

	void Link();
	void Unlink();

private:

	friend class World;
	friend class WorldDraw;

	Light(World *w);

	RAD_DECLARE_GET(diffuseColor, const Vec3&) {
		return m_dfColor;
	}

	RAD_DECLARE_SET(diffuseColor, const Vec3&) {
		m_dfColor = value;
	}

	RAD_DECLARE_GET(specularColor, const Vec3&) {
		return m_spColor;
	}

	RAD_DECLARE_SET(specularColor, const Vec3&) {
		m_spColor = value;
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

	RAD_DECLARE_GET(brightness, float) {
		return m_brightness;
	}

	RAD_DECLARE_SET(brightness, float) {
		m_brightness = value;
	}

	RAD_DECLARE_GET(interactionFlags, int) {
		return m_interactionFlags;
	}

	RAD_DECLARE_SET(interactionFlags, int) {
		m_interactionFlags = value;
	}

	details::LightInteraction **ChainHead(int matId);
	
	dBSPLeaf::PtrVec m_bspLeafs;
	details::MatInteractionChain m_interactions;
	IntSet m_areas;
	BBox m_bounds;
	Vec3 m_spColor;
	Vec4 m_shColor;
	Vec3 m_dfColor;
	Vec3 m_pos;
	float m_brightness;
	float m_radius;
	LightStyle m_style;
	int m_markFrame;
	int m_visFrame;
	int m_interactionFlags;
	dBSPLeaf *m_leaf;
	Light *m_prev;
	Light *m_next;
	World *m_world;
};

} // world

RAD_IMPLEMENT_FLAGS(world::Light::LightStyle)