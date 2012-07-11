// RBackend.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// PC Rendering Backend
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "RGLBackend.h"
#include "../../COut.h"
#include "../GL/GLTable.h"
#include "../GL/GLState.h"
#include <Runtime/Interface/ComponentManager.h>

#if defined(RAD_OPT_PC_TOOLS)
#include <QtOpenGL/QGLFormat>
#endif

#undef min
#undef max

using namespace string;

namespace r {

#if defined(RAD_OPT_PC_TOOLS)

RADENG_API void RADENG_CALL SetQGLFormat(QGLFormat &fmt) {
	fmt.setAccum(false);
	fmt.setDepth(true);
	fmt.setDepthBufferSize(24);
	fmt.setDirectRendering(true);
	fmt.setDoubleBuffer(true);
	fmt.setSampleBuffers(false);
	fmt.setStencil(true);
	fmt.setStencilBufferSize(8);
	fmt.setStereo(false);
	fmt.setRgba(true);
	fmt.setPlane(0);
	fmt.setAlpha(true);
	fmt.setAlphaBufferSize(8);
	fmt.setRedBufferSize(8);
	fmt.setGreenBufferSize(8);
	fmt.setBlueBufferSize(8);
	fmt.setSwapInterval(0);
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(RBackend, r.RBackend);
RAD_IMPLEMENT_COMPONENT(RBContext, r.RBContext);

boost::thread_specific_ptr<HContext> RBackend::s_ctx;

RBackend::RBackend() {
}

RBackend::~RBackend() {
	VidReset();
}

void RBackend::Initialize(int version) {
	GLRenderer::Initialize(version);
}

#if !defined(RAD_OPT_PC_TOOLS)
void RBackend::SwapBuffers() {
	RAD_ASSERT(*s_ctx.get());
	InterfaceComponent<RBContext>(*s_ctx.get())->m_glCtx->SwapBuffers();
}
#endif

void RBackend::VidReset() {
	COut(C_Info) << "VidReset..." << std::endl;
	gl.Reset();
}

bool RBackend::VidBind() {
	COut(C_Info) << "VidBind()..." << std::endl;
	COut(C_Info) << "Vendor: " << glGetString(GL_VENDOR) << std::endl <<
		"Renderer: " << glGetString(GL_RENDERER) << std::endl <<
		"Version:" << glGetString(GL_VERSION) << std::endl;
	COut(C_Info) << "Extensions: " << glGetString(GL_EXTENSIONS) << std::endl;

	gl.Load();

	COut(C_Info) << "MaxTextures: " << gl.maxTextures << std::endl;
	COut(C_Info) << "MaxVertexAttribs: " << gl.maxVertexAttribs << std::endl;

	return true;
}

bool RBackend::CheckCaps() {
	return gl.v1_2 && 
		gl.ARB_vertex_buffer_object &&
#if !defined(RAD_OPT_OSX)
		gl.ARB_vertex_array_object && // OSX does not support this.
#endif
		gl.ARB_fragment_program &&
		gl.ARB_vertex_program &&
		gl.ARB_texture_compression &&
		gl.EXT_texture_compression_s3tc &&
		gl.ARB_multitexture &&
		gl.SGIS_generate_mipmap &&
		gl.ARB_texture_non_power_of_two;
}

HContext RBackend::CreateContext(
	const NativeDeviceContext::Ref &nativeContext
) {
	return HContext(new (ZRender) RBContext(boost::static_pointer_cast<GLDeviceContext>(nativeContext)));
}

const HContext &RBackend::RAD_IMPLEMENT_GET(ctx) {
	if (!s_ctx.get())
		s_ctx.reset(new HContext());
	return *s_ctx.get();
}

void RBackend::RAD_IMPLEMENT_SET(ctx) (const HContext & ctx) {
	if (!s_ctx.get())
	{
		s_ctx.reset(new HContext());
	}

	*s_ctx.get() = ctx;

	if (ctx)
	{
		gls.Bind(InterfaceComponent<RBContext>(ctx)->m_s);
	}
	else
	{
		gls.Bind(GLState::Ref());
	}

	CLEAR_GL_ERRORS();
}

RAD_BEGIN_EXPORT_COMPONENTS_FN(RADENG_API, ExportRBackendComponents)
	RAD_EXPORT_COMPONENT_NAMESPACE(r, RBackend)
RAD_END_EXPORT_COMPONENTS

} // r
