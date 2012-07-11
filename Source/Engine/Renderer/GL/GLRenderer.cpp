// RBackend.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// PC Rendering Backend
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GLRenderer.h"
#include "../GL/GLState.h"

namespace r {

//////////////////////////////////////////////////////////////////////////////////////////

void GLRenderer::Initialize(int version) {
	if (version != Version) 
		throw BadInterfaceVersionException();
}

void GLRenderer::ClearBackBuffer() {
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
}

void GLRenderer::ClearDepthBuffer() {
	glClear(GL_DEPTH_BUFFER_BIT);
}

void GLRenderer::CommitStates() {
	gls.Commit();
}

void GLRenderer::BindFramebuffer() {
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, 0);
	gls.BindBuffer(GL_RENDERBUFFER_EXT, 0);
}

void GLRenderer::UnbindStates() {
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


} // r
