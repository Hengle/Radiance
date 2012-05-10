// MBatchDraw.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include "../Assets/MaterialParser.h"
#include "WorldDrawDef.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneList.h>
#include <Runtime/PushPack.h>

namespace world {
namespace details {

class MBatch
{
public:
	MBatch();

	void AddDraw(MBatchDraw &draw);
	void RemoveDraw(MBatchDraw &draw);

	r::Material *mat;
	pkg::Asset::Ref asset;
	asset::MaterialLoader::Ref loader;
	MBatchDrawPtrList draws;
};

} // details

class RADENG_CLASS MBatchDraw
{
public:
	typedef boost::shared_ptr<MBatchDraw> Ref;
	typedef zone_vector<Ref, ZWorldT>::type RefVec;
	typedef zone_list<MBatchDraw*, ZWorldT>::type PtrList;

	MBatchDraw(int matId) : m_batch(0), m_matId(matId) {}
	virtual ~MBatchDraw();

	RAD_DECLARE_READONLY_PROPERTY(MBatchDraw, matId, int);
	RAD_DECLARE_READONLY_PROPERTY(MBatchDraw, visible, bool);
	RAD_DECLARE_READONLY_PROPERTY(MBatchDraw, rgba, const Vec4&);
	RAD_DECLARE_READONLY_PROPERTY(MBatchDraw, scale, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(MBatchDraw, xform, bool);

protected:

	virtual bool GetTransform(Vec3 &pos, Vec3 &angles) const { return false; }
	virtual void Bind(r::Shader *shader) = 0;
	virtual void CompileArrayStates(r::Shader &shader) = 0;
	virtual void FlushArrayStates(r::Shader *shader) = 0;
	virtual void Draw() = 0;
	virtual bool VisMarked(int frame) const { return this->visible; }

	virtual RAD_DECLARE_GET(visible, bool) = 0;
	virtual RAD_DECLARE_GET(rgba, const Vec4&) = 0;
	virtual RAD_DECLARE_GET(scale, const Vec3&) = 0;
	virtual RAD_DECLARE_GET(xform, bool) = 0;

private:

	RAD_DECLARE_GET(matId, int) { return m_matId; }

	friend class WorldDraw;
	friend class details::MBatch;

	details::MBatch *m_batch;
	PtrList::iterator m_it;
	int m_matId;
};

} // world

#include <Runtime/PopPack.h>