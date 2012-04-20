// D_SkModel.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../../SkAnim/SkControllers.h"
#include "../../Renderer/SkMesh.h"
#include "../EntityDef.h"
#include "D_Asset.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_SkModel : public D_Asset
{
public:
	typedef boost::shared_ptr<D_SkModel> Ref;

	static Ref New(const r::SkMesh::Ref &mesh);

	RAD_DECLARE_READONLY_PROPERTY(D_SkModel, mesh, const r::SkMesh::Ref&);

	// Lerr is optional (can be NULL). if set lua_error may be triggered for errors
	virtual bool SetRootController(lua_State *Lerr, Entity *entity, const char *type);
	virtual bool BlendToState(
		const char *state, 
		const char *blendTarget, 
		bool restart, 
		const ska::Notify::Ref &notify
	);

protected:

	D_SkModel(const r::SkMesh::Ref &mesh);

	virtual void PushElements(lua_State *L);

	r::SkMesh::Ref m_mesh;
	ska::BlendToController::Ref m_blendTo;
	String m_curState;

private:

	class Notify : public ska::Notify, public lua::SharedPtr
	{
	public:
		typedef boost::shared_ptr<Notify> Ref;

		Notify(Entity &entity, int callbackId);
		virtual ~Notify();

	protected:

		virtual void PushElements(lua_State *L);
		virtual void OnTag(const ska::AnimTagEventData &data);
		virtual void OnEndFrame(const ska::AnimStateEventData &data);
		virtual void OnFinish(const ska::AnimStateEventData &data, bool masked);

	private:

		static int lua_SetMasked(lua_State *L);

		EntityWRef m_entity;
		int m_callbackId;
	};

	RAD_DECLARE_GET(mesh, const r::SkMesh::Ref&) { return m_mesh; }

	static int lua_BlendToState(lua_State *L);
	static int lua_SetRootController(lua_State *L);
};

} // world

#include <Runtime/PopPack.h>

