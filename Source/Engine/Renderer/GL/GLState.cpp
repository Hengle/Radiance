// GLState.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// OGL state management.
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GLState.h"
#include "GLTexture.h"
#include "GLVertexBuffer.h"
#include "GLVertexArray.h"

#define EXTRA_GL_CHECKS

#if defined(EXTRA_GL_CHECKS)
	#define CHECK_GL_ERRORS_EXTRA() CHECK_GL_ERRORS()
#else
	#define CHECK_GL_ERRORS_EXTRA()
#endif

namespace r {

RADENG_API GLState gls;

#if defined(RAD_OPT_IOS)
GLState::Ref GLState::X::s;
#endif

void GLState::Init(S &s) {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	
#if defined(RAD_OPT_OGLES1)
	if (!gl.ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	{
		glDisable(GL_LIGHTING);
		glShadeModel(GL_SMOOTH);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		if (gl.SGIS_generate_mipmap) {
			glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
		}
	}
#endif
	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	gl.Color4f(0.f, 0.f, 0.f, 1.f);

	if (gl.EXT_framebuffer_object) {
		glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
	}
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glClearStencil(0);
	glDepthFunc(GL_LESS);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
#if !defined(RAD_OPT_OGLES2)
	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_LESS, 0.f);
#endif
	glDepthMask(GL_TRUE);
	glFrontFace(GL_CCW);
	gl.SetActiveTexture(0);
	gl.SetActiveTexCoord(0);

	s.d.s = kDepthTest_Disable|
			kColorWriteMask_RGBA|
			kCullFaceMode_None|
			kCullFaceMode_CCW|
			kDepthWriteMask_Enable|
			kScissorTest_Disable|
			kStencilTest_Disable;

	s.d.b = kBlendMode_Off;
	s.d.scissor[0] = -1;
	s.d.scissor[1] = -1;
	s.d.scissor[2] = -1;
	s.d.scissor[3] = -1;

	s.d.stencil.func = GL_ALWAYS;
	s.d.stencil.funcRef = 0;
	s.d.stencil.funcMask = 0xff;
	glStencilFunc(GL_ALWAYS, 0, 0xff);

	s.d.stencil.mask = 0;
	glStencilMask(0);

	s.d.stencil.opFail = GL_KEEP;
	s.d.stencil.opzFail = GL_KEEP;
	s.d.stencil.opzPass = GL_KEEP;
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	
	s.t = 0;
	s.s.s = s.d.s;
	s.bb[0] = s.bb[1] = 0;
	s.vaoBound = false;
	s.vp[0] = s.vp[1] = s.vp[2] = s.vp[3] = 0;

	CHECK_GL_ERRORS();
}

void GLState::Commit(S &s, bool f) {
	if (s.s.p != s.d.p) {
		gl.UseProgramObjectARB(s.s.p);
		CHECK_GL_ERRORS_EXTRA();
		s.d.p = s.s.p;
	}
	
	if (f || (s.s.s != 0 && s.s.s != s.d.s) ||
		(s.s.invertCullFace != s.d.invertCullFace) ||
		(s.s.b != 0 && s.s.b != s.d.b) ||
		(s.s.aref != s.d.aref))
	{
		CommitSB(s, f);
	}

	for (int i = 0; i < gl.maxTextures && i < kMaxTextures; ++i) {
		T &st = s.s.t[i];
		T &dt = s.d.t[i];

		if (f || st.tex.get() != dt.tex.get())
			CommitT(s, i, st, dt, f);
	}

	CommitScissor(s, f);
	CommitStencil(s, f);

	if (s.vao) { 
		// GL_vertex_array_object active!
		f = f || !s.vao->m_initialized; // if no state, then force glVertexAttrib() calls to init VAO state.
		s.vaoBound = true;
		s.vao->m_initialized = true;
	} else if (s.vaoBound) {
		f = true; // must set ALL states.
		s.vaoBound = false;
	}

	for (int i = 0; i < gl.maxVertexAttribs && i < kMaxAttribArrays; ++i) {
		AA &sa = s.s.aa[i];
		AA &da = s.d.aa[i];
		CommitAA(s, i, sa, da, f);
	}
}

void GLState::CommitSB(S &s, bool f) {
	int ds  = s.d.s&~s.s.s; // not in s
	if (f) 
		s.d.s = ds; // clear everything set in source to force apply
	int ss  = s.s.s&~s.d.s; // not in d
	
	if (ss&kDepthWriteMask_Flags) {
		if (ds&kDepthWriteMask_Enable) {
			glDepthMask(GL_FALSE);
		} else {
			glDepthMask(GL_TRUE);
		}

		CHECK_GL_ERRORS_EXTRA();

		s.d.s &= ~kDepthWriteMask_Flags;
		s.d.s |= s.s.s&kDepthWriteMask_Flags;
	}

	// ensure proper depth test bits are here
	bool depthTestChanged = false;
	if ((s.s.s&(kDepthTest_Flags&~kDepthTest_Disable)) && (s.d.s&kDepthTest_Disable)) {
		// the ss bitfield doesn't contain accurate bits here for depth test because
		// we have recorded the depth func. That means that the driver bits are already
		// depthFunc|depthTestDisable, which means setting the same depthFunc in source
		// with end up with zeros in ss. Account for this.
		depthTestChanged = true;
	}

	if ((ss&kDepthTest_Flags) || depthTestChanged) {
		if(ds&kDepthTest_Disable) {
			glEnable(GL_DEPTH_TEST);
			CHECK_GL_ERRORS_EXTRA();
		}

		if (ss&kDepthTest_Disable) {
			glDisable(GL_DEPTH_TEST);
			CHECK_GL_ERRORS_EXTRA();
			// record the current depth test state
			s.s.s |= (s.d.s&kDepthTest_Flags);
		} else if(ss&kDepthTest_Flags) {
			int df = 0;

			switch (ss&kDepthTest_Flags) {
			case kDepthTest_Always: df = GL_ALWAYS; break;
			case kDepthTest_Less: df = GL_LESS; break;
			case kDepthTest_Greater : df = GL_GREATER; break;
			case kDepthTest_LEqual : df = GL_LEQUAL; break;
			case kDepthTest_GEqual : df = GL_GEQUAL; break;
			case kDepthTest_Equal : df = GL_EQUAL; break;
			case kDepthTest_Never : df = GL_NEVER; break;
			}

			if (df != 0) {
				glDepthFunc(df);
				CHECK_GL_ERRORS_EXTRA();
			}
		}

		s.d.s &= ~kDepthTest_Flags;
		s.d.s |= s.s.s&kDepthTest_Flags;
	}

	if (ss&kColorWriteMask_Flags) {
		GLboolean r, g, b, a;

		r = (s.s.s&kColorWriteMask_R) ? GL_TRUE : GL_FALSE;
		g = (s.s.s&kColorWriteMask_G) ? GL_TRUE : GL_FALSE;
		b = (s.s.s&kColorWriteMask_B) ? GL_TRUE : GL_FALSE;
		a = (s.s.s&kColorWriteMask_A) ? GL_TRUE : GL_FALSE;

		glColorMask(r, g, b, a);

		s.d.s &= ~kColorWriteMask_Flags;
		s.d.s |= s.s.s&kColorWriteMask_Flags;

		CHECK_GL_ERRORS_EXTRA();
	}
	
	// ensure proper cull face bits are here (see depth test)
	bool cullModeChanged = false;
	if ((s.s.s&(kCullFaceMode_Flags&~kCullFaceMode_None)) && (s.d.s&kCullFaceMode_None)) {
		cullModeChanged = true;
	}

	if ((ss&kCullFaceMode_Flags) || cullModeChanged || (s.s.invertCullFace != s.d.invertCullFace)) {
		
		if (ds&kCullFaceMode_None) {
			glEnable(GL_CULL_FACE);
			CHECK_GL_ERRORS_EXTRA();
		} else if (ss&kCullFaceMode_None) {
			glDisable(GL_CULL_FACE);
			CHECK_GL_ERRORS_EXTRA();
		}

		if (ss&kCullFaceMode_Front) {
			glCullFace(GL_FRONT);
			CHECK_GL_ERRORS_EXTRA();
		} else if (ss&kCullFaceMode_Back) {
			glCullFace(GL_BACK);
			CHECK_GL_ERRORS_EXTRA();
		}
		
		int xs = ss;

		if (s.s.invertCullFace != s.d.invertCullFace) {
			xs &= ~(kCullFaceMode_CW|kCullFaceMode_CCW);
			xs |= s.s.s&(kCullFaceMode_CW|kCullFaceMode_CCW);
			if ((xs&(kCullFaceMode_CW|kCullFaceMode_CCW))==0)
				xs |= s.d.s&(kCullFaceMode_CW|kCullFaceMode_CCW);
		}

		if (xs&kCullFaceMode_CW) {
			GLenum ff = s.s.invertCullFace ? GL_CCW : GL_CW;
			glFrontFace(ff);
			CHECK_GL_ERRORS_EXTRA();
		} else if (xs&kCullFaceMode_CCW) {
			GLenum ff = s.s.invertCullFace ? GL_CW : GL_CCW;
			glFrontFace(ff);
			CHECK_GL_ERRORS_EXTRA();
		}

		// it is acceptable to disable culling by just passing kCullFadeMode_None
		// in those cases, make sure to record the current polarity from the driver
		// so we don't duplicate reduntant state

		if (!(s.s.s&(kCullFaceMode_Front|kCullFaceMode_Back))) // record polarity
			s.s.s |= s.d.s&(kCullFaceMode_Front|kCullFaceMode_Back);
		if (!(s.s.s&(kCullFaceMode_CW|kCullFaceMode_CCW)))
			s.s.s |= s.d.s&(kCullFaceMode_CW|kCullFaceMode_CCW); // record order

		s.d.s &= ~kCullFaceMode_Flags;
		s.d.s |= s.s.s&kCullFaceMode_Flags;
		s.d.invertCullFace = s.s.invertCullFace;
	}

	if (ss&kScissorTest_Flags) {
		if (ss&kScissorTest_Enable) {
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}

		s.d.s &= ~kScissorTest_Flags;
		s.d.s |= s.s.s&kScissorTest_Flags;
	}

	if (ss&kStencilTest_Flags) {
		if (ss&kStencilTest_Enable) {
			glEnable(GL_STENCIL_TEST);
		} else {
			glDisable(GL_STENCIL_TEST);
		}

		s.d.s &= ~kStencilTest_Flags;
		s.d.s |= s.s.s&kStencilTest_Flags;
	}

	// blends

	ds  = s.d.b&~s.s.b; // not in s
	if (f) s.d.b = ds; // clear everything set in source to force apply
	ss  = s.s.b&~s.d.b; // not in d

	if (ss&kBlendMode_Flags) {
		if ((s.d.b&kBlendMode_Off) == kBlendMode_Off) {
			glEnable(GL_BLEND);
			CHECK_GL_ERRORS_EXTRA();
		} else if((s.s.b&kBlendMode_Off) == kBlendMode_Off) {
			glDisable(GL_BLEND);
			CHECK_GL_ERRORS_EXTRA();
		}

		if (s.s.b&(kBlendModeSource_Flags|kBlendModeDest_Flags)) {
			int sb = GL_ONE;
			int db = GL_ZERO;

			int z = s.s.b&kBlendModeSource_Flags;
			switch (z) {
			case kBlendModeSource_One: sb = GL_ONE; break;
			case kBlendModeSource_DstColor: sb = GL_DST_COLOR; break;
			case kBlendModeSource_InvDstColor: sb = GL_ONE_MINUS_DST_COLOR; break;
			case kBlendModeSource_SrcAlpha: sb = GL_SRC_ALPHA; break;
			case kBlendModeSource_InvSrcAlpha: sb = GL_ONE_MINUS_SRC_ALPHA; break;
			case kBlendModeSource_DstAlpha: sb = GL_DST_ALPHA; break;
			case kBlendModeSource_InvDstAlpha: sb = GL_ONE_MINUS_DST_ALPHA; break;
			case kBlendModeSource_SrcAlphaSaturate: sb = GL_SRC_ALPHA_SATURATE; break;
			case kBlendModeSource_Zero: sb = GL_ZERO; break;
			}

			z = s.s.b&kBlendModeDest_Flags;
			switch( z ) {
			case kBlendModeDest_One: db = GL_ONE; break;
			case kBlendModeDest_SrcColor: db = GL_SRC_COLOR; break;
			case kBlendModeDest_InvSrcColor: db = GL_ONE_MINUS_SRC_COLOR; break;
			case kBlendModeDest_SrcAlpha: db = GL_SRC_ALPHA; break;
			case kBlendModeDest_InvSrcAlpha: db = GL_ONE_MINUS_SRC_ALPHA; break;
			case kBlendModeDest_DstAlpha: db = GL_DST_ALPHA; break;
			case kBlendModeDest_InvDstAlpha: db = GL_ONE_MINUS_DST_ALPHA; break;
			case kBlendModeDest_Zero: db = GL_ZERO; break;
			}

			glBlendFunc(sb, db);
			CHECK_GL_ERRORS_EXTRA();
		}

		s.d.b &= ~kBlendMode_Flags;
		s.d.b |= s.s.b&kBlendMode_Flags;
	}
}

void GLState::CommitT(S &st, int t, T &s, T &d, bool f) {
	if (!f) {
		// Note: if we get into there the s.tex != d.tex was tested.
		if (s.tex) {
			if (d.tex) {
				if (d.tex->target != s.tex->target) {
					st.SetActiveTexture(t);
					CHECK_GL_ERRORS_EXTRA();
					glBindTexture(s.tex->target, s.tex->id);
					CHECK_GL_ERRORS_EXTRA();
#if defined(RAD_OPT_PC)
					glDisable(d.tex->target);
					CHECK_GL_ERRORS_EXTRA();
					glEnable(s.tex->target);
					CHECK_GL_ERRORS_EXTRA();
#else
					glBindTexture(d.tex->target, 0);
					CHECK_GL_ERRORS_EXTRA();
#endif
				} else {
					st.SetActiveTexture(t);
					CHECK_GL_ERRORS_EXTRA();
					glBindTexture(s.tex->target, s.tex->id);
					CHECK_GL_ERRORS_EXTRA();
				}
			} else {
				st.SetActiveTexture(t);
				CHECK_GL_ERRORS_EXTRA();
				glBindTexture(s.tex->target, s.tex->id);
				CHECK_GL_ERRORS_EXTRA();
#if defined(RAD_OPT_PC)
				glEnable(s.tex->target);
				CHECK_GL_ERRORS_EXTRA();
#endif
			}
		} else if (d.tex) {
			st.SetActiveTexture(t);
			CHECK_GL_ERRORS_EXTRA();
#if defined(RAD_OPT_PC)
			glDisable(d.tex->target);
			CHECK_GL_ERRORS_EXTRA();
#else
			glBindTexture(d.tex->target, 0);
			CHECK_GL_ERRORS_EXTRA();
#endif
		}

		d.tex = s.tex;
	} else {
		if (d.tex) {
			st.SetActiveTexture(t);
			CHECK_GL_ERRORS_EXTRA();
#if defined(RAD_OPT_PC)
			glDisable(d.tex->target);
			CHECK_GL_ERRORS_EXTRA();
#else
			glBindTexture(d.tex->target, 0);
			CHECK_GL_ERRORS_EXTRA();
#endif
		} if (s.tex) {
			st.SetActiveTexture(t);
			CHECK_GL_ERRORS_EXTRA();
			glBindTexture(s.tex->target, s.tex->id);
			CHECK_GL_ERRORS_EXTRA();
#if defined(RAD_OPT_PC)
			glEnable(s.tex->target);
			CHECK_GL_ERRORS_EXTRA();
#endif
		}

		d.tex = s.tex;
	}
}

void GLState::CommitAA(S &s, int i, AA &sa, AA &da, bool f) {
	if (f || (!s.vaoBound && sa.e != da.e)) {
		if (sa.e) {
#if defined(RAD_OPT_PC)
			// ATI driver crash fix (always set VertexAttribPointer if we enable a stream).
			f = f || !s.vaoBound;
#endif
			gl.EnableVertexAttribArrayARB(i);
			CHECK_GL_ERRORS_EXTRA();
		} else {
			gl.DisableVertexAttribArrayARB(i);
			CHECK_GL_ERRORS_EXTRA();
		}

		da.e = sa.e;
	}

	if (!sa.e && !sa.vb && da.vb)
		da.vb.reset();

	if (sa.e && sa.vb && (f || (!s.vaoBound && sa != da))) {
		const GLvoid *p;

		if (gl.vbos) {
			BindBuffer(GL_ARRAY_BUFFER_ARB, sa.vb);
			CHECK_GL_ERRORS_EXTRA();
			p = (const GLvoid*)static_cast<AddrSize>(sa.ofs);
		} else {
			p = (const GLvoid*)(static_cast<U8*>(sa.vb->m_ptr.m_p) + sa.ofs);
		}
		
		gl.VertexAttribPointerARB(
			(GLuint)i,
			sa.size,
			sa.type,
			sa.normalized,
			sa.stride,
			p
		);
		CHECK_GL_ERRORS_EXTRA();
		
		da = sa;
	}
}

void GLState::CommitScissor(S &s, bool f) {
	if (f || 
		((s.s.s&kScissorTest_Enable) && 
		(s.s.scissor[0] != s.d.scissor[0] ||
		 s.s.scissor[1] != s.d.scissor[1] ||
		 s.s.scissor[2] != s.d.scissor[2] ||
		 s.s.scissor[3] != s.d.scissor[3])))
	{
		glScissor(
			s.s.scissor[0],
			s.s.scissor[1],
			s.s.scissor[2],
			s.s.scissor[3]
		);

		s.d.scissor[0] = s.s.scissor[0];
		s.d.scissor[1] = s.s.scissor[1];
		s.d.scissor[2] = s.s.scissor[2];
		s.d.scissor[3] = s.s.scissor[3];
	}
}

void GLState::CommitStencil(S &s, bool f) {
	if (f || (s.s.s&kStencilTest_Enable)) {

		if ((s.s.stencil.func != s.d.stencil.func) ||
			(s.s.stencil.funcRef != s.d.stencil.funcRef) ||
			(s.s.stencil.funcMask != s.d.stencil.funcMask)) {
			s.d.stencil.func = s.s.stencil.func;
			s.d.stencil.funcRef = s.s.stencil.funcRef;
			s.d.stencil.funcMask = s.s.stencil.funcMask;

			glStencilFunc(
				s.s.stencil.func,
				s.s.stencil.funcRef,
				s.s.stencil.funcMask
			);
		}

		if ((s.s.stencil.opFail != s.d.stencil.opFail) ||
			(s.s.stencil.opzFail != s.d.stencil.opzFail) ||
			(s.s.stencil.opzPass != s.d.stencil.opzPass)) {
			glStencilOp(
				s.s.stencil.opFail,
				s.s.stencil.opzFail,
				s.s.stencil.opzPass
			);
			s.d.stencil.opFail = s.s.stencil.opFail;
			s.d.stencil.opzFail = s.s.stencil.opzFail;
			s.d.stencil.opzPass = s.s.stencil.opzPass;
		}
	}

	// stencil mask effect glClear, must be set if it ever changes
	if (s.s.stencil.mask != s.d.stencil.mask) {
		s.d.stencil.mask = s.s.stencil.mask;
		glStencilMask(s.s.stencil.mask);
	}
}

void GLState::S::BindBuffer(GLenum target, const GLVertexBufferRef &vb, bool force) {
	switch (target) {
	case GL_ARRAY_BUFFER_ARB:
		if (vbb[0].get() != vb.get() || force) {
			vbb[0] = vb;
			if (gl.vbos) {
				gl.BindBufferARB(target, vb ? vb->id : 0);
				CHECK_GL_ERRORS();
			}
		} break;
	case GL_ELEMENT_ARRAY_BUFFER_ARB:
		if (vbb[1].get() != vb.get() || force) {
			vbb[1] = vb;
			if (gl.vbos) {
				gl.BindBufferARB(target, vb ? vb->id : 0);
				CHECK_GL_ERRORS();
			}
		} break;
	}
}

void GLState::S::BindVertexArray(const GLVertexArrayRef &_va, bool force) {
	RAD_ASSERT(gl.vbos&&gl.vaos);

	if (vao.get() != _va.get() || force) {
		vao = _va;
		gl.BindVertexArray(vao ? vao->id : 0);
		CHECK_GL_ERRORS();
	}
}

} // r
