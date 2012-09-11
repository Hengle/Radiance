/*! \file EditorMapEditorRenderer.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_editor
*/

#pragma once

#include "EditorMapEditorTypes.h"
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {
namespace map_editor {

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS RenderStyle {
public:
	enum MaterialMode {
		kMaterialMode_Picking,
		kMaterialMode_SolidColor,
		kMaterialMode_Textured,
		kMaterialMode_Material
	};

	enum ShadingMode {
		kShadingMode_Flat,
		kShadingMode_Shaded,
		kShadingMode_FullLighting
	};

	RenderStyle(
		MaterialMode materialMode,
		ShadingMode shadingMode
	);

	RAD_DECLARE_PROPERTY(RenderStyle, materialMode, MaterialMode, MaterialMode);
	RAD_DECLARE_PROPERTY(RenderStyle, shadingMode, ShadingMode, ShadingMode);

private:

	RAD_DECLARE_GET(materialMode, MaterialMode) {
		return m_mm;
	}

	RAD_DECLARE_SET(materialMode, MaterialMode) {
		m_mm = value;
	}

	RAD_DECLARE_GET(shadingMode, ShadingMode) {
		return m_sm;
	}

	RAD_DECLARE_SET(shadingMode, ShadingMode) {
		m_sm = value;
	}

	MaterialMode m_mm;
	ShadingMode  m_sm;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Viewport {
public:

	enum Type {
		kType_2D,
		kType_3D
	};

	Viewport(Type type);
	~Viewport();

	RAD_DECLARE_PROPERTY(Viewport, type, Type, Type);
	RAD_DECLARE_PROPERTY(Viewport, scale, float, float);
	RAD_DECLARE_PROPERTY(Viewport, gridSize, float, float);

	//! Controls the view of a kType_3D viewport.
	RAD_DECLARE_PROPERTY(Viewport, camera, const Camera&, const Camera&);

	//! Controls the view of a kType_2D viewport.
	RAD_DECLARE_PROPERTY(Viewport, pos2D, const Vec2&, const Vec2&);

	RAD_DECLARE_PROPERTY(Viewport, size, const QSize&, const QSize&);
	RAD_DECLARE_PROPERTY(Viewport, renderStyle, const RenderStyle&, const RenderStyle&);

	Vec2 MapToView(const Vec3 &worldPos);
	Vec3 MapToWorld(const QPoint &viewPos, float viewPlaneDistance);

private:

	RAD_DECLARE_GET(type, Type) {
		return m_type;
	}

	RAD_DECLARE_SET(type, Type) {
		m_type = value;
	}

	RAD_DECLARE_GET(scale, float) {
		return m_scale;
	}

	RAD_DECLARE_SET(scale, float) {
		m_scale = value;
	}

	RAD_DECLARE_GET(gridSize, float) {
		return m_gridSize;
	}

	RAD_DECLARE_SET(gridSize, float) {
		m_gridSize = value;
	}

	RAD_DECLARE_GET(camera, const Camera&) {
		return m_camera;
	}

	RAD_DECLARE_SET(camera, const Camera&) {
		m_camera = value;
	}

	RAD_DECLARE_GET(pos2D, const Vec2&) {
		return m_pos2D;
	}

	RAD_DECLARE_SET(pos2D, const Vec2&) {
		m_pos2D = value;
	}

	RAD_DECLARE_GET(size, const QSize&) {
		return m_size;
	}

	RAD_DECLARE_SET(size, const QSize&) {
		m_size = value;
	}

	RAD_DECLARE_GET(renderStyle, const RenderStyle&) {
		return m_renderStyle;
	}

	RAD_DECLARE_SET(renderStyle, const RenderStyle&) {
		m_renderStyle = value;
	}

	Camera m_camera;
	Vec2 m_pos2D;
	QSize m_size;
	RenderStyle m_renderStyle;
	float m_scale;
	float m_gridSize;
	Type m_type;
};

/////////////////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS ObjectRenderer {
public:

	virtual ~ObjectRenderer() {}

	virtual int Draw(
		const RenderStyle &renderStyle,
		const Viewport &viewport
	) = 0;

};

} // map_editor
} // editor
} // tools

#include <Runtime/PopPack.h>
