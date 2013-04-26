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

class RADENG_CLASS Light {
public:
	typedef boost::shared_ptr<Light> Ref;

	RAD_BEGIN_FLAGS
		RAD_FLAG(kStyle_Diffuse),
		RAD_FLAG(kStyle_Specular),
		RAD_FLAG(kStyle_Shadows),
		RAD_FLAG(kStyle_AffectWorld),
		RAD_FLAG(kStyle_AffectObjects),
		RAD_FLAG(kStyle_AffectPlayer),
		kStyle_AffectAll = kStyle_AffectWorld|kStyle_AffectObjects|kStyle_AffectPlayer
	RAD_END_FLAGS(LightStyles)

	~Light();

	RAD_DECLARE_PROPERTY(Light, diffuseColor, const Vec3&, const Vec3&);
	// alpha control specular power
	RAD_DECLARE_PROPERTY(Light, specularColor, const Vec4&, const Vec4&);
	// alpha controls shadow opacity
	RAD_DECLARE_PROPERTY(Light, shadowColor, const Vec4&, const Vec4&);
	RAD_DECLARE_PROPERTY(Light, style, LightStyles, LightStyles);
	RAD_DECLARE_PROPERTY(Light, pos, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(Light, size, const BBox&, const BBox&);

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

	RAD_DECLARE_GET(specularColor, const Vec4&) {
		return m_spColor;
	}

	RAD_DECLARE_SET(specularColor, const Vec4&) {
		m_spColor = value;
	}

	RAD_DECLARE_GET(shadowColor, const Vec4&) {
		return m_shColor;
	}

	RAD_DECLARE_SET(shadowColor, const Vec4&) {
		m_shColor = value;
	}

	RAD_DECLARE_GET(style, LightStyles) {
		return m_style;
	}

	RAD_DECLARE_SET(style, LightStyles) {
		m_style = value;
	}

	RAD_DECLARE_GET(pos, const Vec3&) {
		return m_pos;
	}

	RAD_DECLARE_SET(pos, const Vec3&) {
		m_pos = value;
	}

	RAD_DECLARE_GET(size, const BBox&) {
		return m_size;
	}

	RAD_DECLARE_SET(size, const BBox&) {
		m_size = value;
	}

	details::LightInteraction **ChainHead(int matId);
	
	dBSPLeaf::PtrVec m_bspLeafs;
	details::MatInteractionChain m_interactions;
	IntSet m_areas;
	BBox m_size;
	Vec4 m_spColor;
	Vec4 m_shColor;
	Vec3 m_dfColor;
	Vec3 m_pos;
	LightStyles m_style;
	dBSPLeaf *m_leaf;
	Light *m_prev;
	Light *m_next;
	World *m_world;
};

} // world

RAD_IMPLEMENT_FLAGS(world::Light::LightStyles)