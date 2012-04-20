// RBDetails.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// PC Rendering Backend
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Renderer.h"
#include "RBackend.h"
#include "../GL/GLState.h"
#include <Runtime/Interface/ComponentBuilder.h>
#include <Runtime/Interface/ComponentManager.h>
#include <Runtime/String.h>

#if defined(RAD_OPT_DEBUG)
#include <Runtime/ThreadDef.h>
#endif

#include <Runtime/PushPack.h>

namespace r {

class RBackend : 
	public IRenderer, 
	public IRBackend, 
	private AtomicRefCount
{
public:

	RBackend();
	~RBackend();

	virtual void Initialize(int version);
	virtual void ClearBackBuffer();
	virtual void ClearDepthBuffer();
	virtual void CommitStates();
	virtual void BindFramebuffer();
	virtual void UnbindStates();
	
	virtual void SwapBuffers();
	virtual bool VidBind();
	
	// IInterface
	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, NULL, this)
	RAD_INTERFACE_MAP_BEGIN(IRBackend)
		RAD_INTERFACE_MAP_ADD(IRenderer)
		RAD_INTERFACE_MAP_ADD(IRBackend)
	RAD_INTERFACE_MAP_END

protected:

	virtual RAD_DECLARE_GET(curVidMode, const VidMode&);
	virtual RAD_DECLARE_GET(ctx, const HContext&);

private:

	VidMode m_curMode;
	HContext m_ctx;
};

class RBContext : 
	public IContext, 
	private AtomicRefCount
{
public:

	RBContext()
	{
		m_s = gls.New(true);
		RAD_ASSERT(m_s);
	}

	// IInterface
	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, NULL, this)
	RAD_INTERFACE_MAP_BEGIN(RBContext)
		RAD_INTERFACE_MAP_ADD(IContext)
	RAD_INTERFACE_MAP_END

private:

	friend class RBackend;

	GLState::Ref m_s;
};

} // r

#include <Runtime/PopPack.h>

