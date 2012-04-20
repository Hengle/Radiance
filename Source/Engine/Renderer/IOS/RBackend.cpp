// RBackend.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// PC Rendering Backend
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "RBDetails.h"
#include "../../COut.h"
#include "../GL/GLTable.h"
#include "../GL/GLState.h"

using namespace string;

void __IOS_ScreenSize(int &w, int &h);
void __IOS_SwapBuffers();
void __IOS_BindFramebuffer();

namespace r {

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(RBackend, r.RBackend);
RAD_IMPLEMENT_COMPONENT(RBContext, r.RBContext);

RBackend::RBackend()
{
	m_ctx = new (ZEngine) RBContext();
	__IOS_ScreenSize(m_curMode.w, m_curMode.h);
	m_curMode.bpp = 32;
	m_curMode.hz = 60;
}

RBackend::~RBackend()
{
}

void RBackend::Initialize(int version)
{
	if (version != Version) 
		throw BadInterfaceVersionException();
}
	
void RBackend::ClearBackBuffer()
{
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
}
	
void RBackend::ClearDepthBuffer()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}
	
void RBackend::CommitStates()
{
	gls.Commit();
}
	
void RBackend::SwapBuffers()
{
	__IOS_SwapBuffers();
}
	
void RBackend::BindFramebuffer()
{
	__IOS_BindFramebuffer();
}
	
void RBackend::UnbindStates()
{
	gls.DisableTextures();
	gls.DisableVertexAttribArrays(true);
	gls.DisableAllMGSources();
	gls.DisableAllMTSources();
	gls.UseProgram(0, true);
	gls.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, GLVertexBufferRef());
	gls.BindBuffer(GL_ARRAY_BUFFER_ARB, GLVertexBufferRef());
	if (gl.vbos&&gl.vaos)
		gls.BindVertexArray(GLVertexArrayRef());
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, 0);
	gls.BindBuffer(GL_RENDERBUFFER_EXT, 0);
}
	
bool RBackend::VidBind()
{
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

const VidMode &RBackend::RAD_IMPLEMENT_GET(curVidMode)
{
	return m_curMode;
}

const HContext &RBackend::RAD_IMPLEMENT_GET(ctx)
{
	return m_ctx;
}

RAD_BEGIN_EXPORT_COMPONENTS_FN(RADENG_API, ExportRBackendComponents)
	RAD_EXPORT_COMPONENT_NAMESPACE(r, RBackend)
RAD_END_EXPORT_COMPONENTS

} // r
