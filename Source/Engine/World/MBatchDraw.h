// MBatchDraw.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include "../Assets/MaterialParser.h"
#include "WorldDef.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneList.h>
#include <Runtime/PushPack.h>

namespace world {

class Entity;

namespace details {

struct MBatchDrawLink {
	MBatchDrawLink *next;
	MBatchDraw *draw;
};

struct MatRef {
	r::Material *mat;
	pkg::Asset::Ref asset;
	asset::MaterialLoader *loader;
};

struct MBatch {
	MBatch();
	~MBatch();

	void AddDraw(MBatchDraw &draw);
	
	WorldDraw *owner;
	const MatRef *matRef;
	MBatchDrawLink *head;
	MBatchDrawLink *tail;
};

} // details

class RADENG_CLASS MBatchDraw : public boost::noncopyable {
public:
	typedef boost::shared_ptr<MBatchDraw> Ref;
	typedef zone_vector<Ref, ZWorldT>::type RefVec;
	
	MBatchDraw(WorldDraw &draw, int matId);
	virtual ~MBatchDraw() {}

	RAD_DECLARE_PROPERTY(MBatchDraw, matId, int, int);
	RAD_DECLARE_READONLY_PROPERTY(MBatchDraw, maxLights, int);
	RAD_DECLARE_READONLY_PROPERTY(MBatchDraw, visible, bool);
	RAD_DECLARE_READONLY_PROPERTY(MBatchDraw, rgba, const Vec4&);
	RAD_DECLARE_READONLY_PROPERTY(MBatchDraw, scale, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(MBatchDraw, bounds, const BBox&);

protected:

	virtual bool GetTransform(Vec3 &pos, Vec3 &angles) const { 
		return false; 
	}

	virtual void Bind(r::Shader *shader) = 0;
	virtual void CompileArrayStates(r::Shader &shader) = 0;
	virtual void FlushArrayStates(r::Shader *shader) = 0;
	virtual void Draw() = 0;
	
	virtual RAD_DECLARE_GET(visible, bool) = 0;
	virtual RAD_DECLARE_GET(rgba, const Vec4&) = 0;
	virtual RAD_DECLARE_GET(scale, const Vec3&) = 0;
	virtual RAD_DECLARE_GET(bounds, const BBox&) = 0;

private:
	
	friend class WorldDraw;
	friend struct details::MBatch;

	RAD_DECLARE_GET(matId, int) { 
		return m_matId; 
	}

	RAD_DECLARE_SET(matId, int) {
		m_matId = value;
	}

	RAD_DECLARE_GET(maxLights, int) {
		RAD_ASSERT(m_matRef && m_matRef->mat);
		return m_matRef->mat->maxLights;
	}

	details::MatRef *m_matRef;
	details::LightInteraction *m_interactions;
	int m_matId;
	int m_markFrame;
	int m_visibleFrame;
};

} // world

#include <Runtime/PopPack.h>
