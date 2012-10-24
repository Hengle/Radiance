// RGLBackend.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// PC Rendering Backend
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <Main/GL/GLContext.h>
#include "GLRenderer.h"
#include "GLState.h"
#include "RBackend.h"
#include <Runtime/Interface/ComponentBuilder.h>
#include <Runtime/PushPack.h>

#if defined(RAD_OPT_PC_TOOLS)
class QGLFormat;
#endif

namespace r {

#if defined(RAD_OPT_PC_TOOLS)
RADENG_API void RADENG_CALL SetQGLFormat(QGLFormat &fmt);
#endif

class RBackend : 
	public GLRenderer, 
	public IRBackend, 
	private AtomicRefCount  {
public:

	RBackend();
	~RBackend();

	virtual void Initialize(int version);

#if !defined(RAD_OPT_PC_TOOLS)
	virtual void SwapBuffers();
#endif
	
	virtual bool VidBind();
	virtual void BindFramebuffer();

#if defined(RAD_OPT_PC)
	virtual void VidReset();
	virtual bool CheckCaps();
#endif

	virtual HContext CreateContext(
		const NativeDeviceContext::Ref &nativeContext
	);

	// IInterface
	RAD_IMPLEMENT_IINTERFACE(NULL, NULL, NULL, this)
	RAD_INTERFACE_MAP_BEGIN(IRBackend)
		RAD_INTERFACE_MAP_ADD(IRenderer)
		RAD_INTERFACE_MAP_ADD(IRBackend)
	RAD_INTERFACE_MAP_END

protected:

	virtual RAD_DECLARE_GET(ctx, const HContext&);
	virtual RAD_DECLARE_SET(ctx, const HContext&);

private:

#if defined(RAD_OPT_PC)
	static boost::thread_specific_ptr<HContext> s_ctx;
#else
	HContext m_ctx;
#endif
};

class RBContext : 
	public IContext, 
	private AtomicRefCount {
public:

	RBContext(
		const GLDeviceContext::Ref &context
	) : m_glCtx(context)  {
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
	GLDeviceContext::Ref m_glCtx;
};

} // r

#include <Runtime/PopPack.h>
