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

void GLState::Init(S &s)
{
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
		if (gl.SGIS_generate_mipmap)
		{
			glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
		}
	}
#endif
	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	gl.Color4f(0.f, 0.f, 0.f, 1.f);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
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

	s.d.s = DT_Disable|DT_Less|CWM_RGBA|CFM_None|CFM_CCW|DWM_Enable|AT_Disable|SCT_Disable;
	s.d.b = BM_Off;
	s.s.scissor[0] = -1;
	s.s.scissor[1] = -1;
	s.s.scissor[2] = -1;
	s.s.scissor[3] = -1;
	s.t = 0;
	s.s.s = s.d.s;
	s.vaoBound = false;

	CHECK_GL_ERRORS();
}

void GLState::Commit(S &s, bool f)
{
	if (s.s.p != s.d.p)
	{
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

	for (int i = 0; i < gl.maxTextures && i < MaxTextures; ++i)
	{
		T &st = s.s.t[i];
		T &dt = s.d.t[i];

		if (f || st.tex.get() != dt.tex.get() || st.s != dt.s)
			CommitT(s, i, st, dt, f);
	}

	CommitScissor(s, f);

	if (s.vao) // GL_vertex_array_object active!
	{
		f = f || !s.vao->m_initialized; // if no state, then force glVertexAttrib() calls to init VAO state.
		s.vaoBound = true;
		s.vao->m_initialized = true;
	}
	else if (s.vaoBound)
	{
		f = true; // must set ALL states.
		s.vaoBound = false;
	}

	for (int i = 0; i < gl.maxVertexAttribs && i < MaxAttribArrays; ++i)
	{
		AA &sa = s.s.aa[i];
		AA &da = s.d.aa[i];
		CommitAA(s, i, sa, da, f);
	}
}

void GLState::CommitSB(S &s, bool f)
{
	int ds  = s.d.s&~s.s.s; // not in s
	if (f) 
		s.d.s = ds; // clear everything set in source to force apply
	int ss  = s.s.s&~s.d.s; // not in d
	
	if (ss&DWM_Flags)
	{
		if (ds&DWM_Enable)
		{
			glDepthMask(GL_FALSE);
		}
		else// if (ss&DWM_Enable) (implied by ss&DWM_Flags)
		{
			glDepthMask(GL_TRUE);
		}

		CHECK_GL_ERRORS_EXTRA();

		s.d.s &= ~DWM_Flags;
		s.d.s |= s.s.s&DWM_Flags;
	}

	if ((s.s.s&DT_Flags) != (s.d.s&DT_Flags))
	{
		if(ds&DT_Disable)
		{
			glEnable(GL_DEPTH_TEST);
			CHECK_GL_ERRORS_EXTRA();
		}

		if (ss&DT_Disable)
		{
			glDisable(GL_DEPTH_TEST);
			CHECK_GL_ERRORS_EXTRA();
			// record the current depth test state
			s.s.s |= (s.d.s&DT_Flags)&~DT_Disable;
		}
		else if(ss&DT_Flags)
		{
			int df = 0;

			switch (s.s.s&DT_Flags)
			{
			case DT_Always: df = GL_ALWAYS; break;
			case DT_Less: df = GL_LESS; break;
			case DT_Greater : df = GL_GREATER; break;
			case DT_LEqual : df = GL_LEQUAL; break;
			case DT_GEqual : df = GL_GEQUAL; break;
			case DT_Equal : df = GL_EQUAL; break;
			case DT_Never : df = GL_NEVER; break;
			}

			if (df != 0)
			{
				glDepthFunc(df);
				CHECK_GL_ERRORS_EXTRA();
			}
		}

		s.d.s &= ~DT_Flags;
		s.d.s |= s.s.s&DT_Flags;
	}

	if (ss&CWM_Flags)
	{
		GLboolean r, g, b, a;

		r = (s.s.s&CWM_R) ? GL_TRUE : GL_FALSE;
		g = (s.s.s&CWM_G) ? GL_TRUE : GL_FALSE;
		b = (s.s.s&CWM_B) ? GL_TRUE : GL_FALSE;
		a = (s.s.s&CWM_A) ? GL_TRUE : GL_FALSE;

		glColorMask(r, g, b, a);

		s.d.s &= ~CWM_Flags;
		s.d.s |= s.s.s&CWM_Flags;

		CHECK_GL_ERRORS_EXTRA();
	}
	
	if (ss&CFM_Flags || (s.s.invertCullFace != s.d.invertCullFace))
	{
		if (ds&CFM_None)
		{
			glEnable(GL_CULL_FACE);
			CHECK_GL_ERRORS_EXTRA();
		}
		else if (ss&CFM_None)
		{
//			s.s.s |= s.d.s&(CFM_Front|CFM_Back); // record face mode.
			glDisable(GL_CULL_FACE);
			CHECK_GL_ERRORS_EXTRA();
		}

		if (ss&CFM_Front)
		{
			glCullFace(GL_FRONT);
			CHECK_GL_ERRORS_EXTRA();
		}
		else if (ss&CFM_Back)
		{
			glCullFace(GL_BACK);
			CHECK_GL_ERRORS_EXTRA();
		}
		
		int xs = ss;

		if (s.s.invertCullFace != s.d.invertCullFace)
		{
			xs &= ~(CFM_CW|CFM_CCW);
			xs |= s.s.s&(CFM_CW|CFM_CCW);
			if ((xs&(CFM_CW|CFM_CCW))==0)
				xs |= s.d.s&(CFM_CW|CFM_CCW);
		}

		if (xs&CFM_CW)
		{
			GLenum ff = s.s.invertCullFace ? GL_CCW : GL_CW;
			glFrontFace(ff);
			CHECK_GL_ERRORS_EXTRA();
		}
		else if (xs&CFM_CCW)
		{
			GLenum ff = s.s.invertCullFace ? GL_CW : GL_CCW;
			glFrontFace(ff);
			CHECK_GL_ERRORS_EXTRA();
		}

		if (ss & CFM_Flags)
		{
			s.d.s &= ~CFM_Flags;
			s.d.s |= s.s.s&CFM_Flags;
		}
		
		s.d.invertCullFace = s.s.invertCullFace;
	}

	bool alphaTest = (ss&AT_Flags) || 
		(s.s.aref != s.d.aref && !((s.d.s|s.s.s)&AT_Disable));

	if (alphaTest)
	{
#if !defined(RAD_OPT_OGLES2)
		if (ds&AT_Disable)
		{
			glEnable(GL_ALPHA_TEST);
		}
		else if (ss&AT_Disable)
		{
			glDisable(GL_ALPHA_TEST);
		}

		CHECK_GL_ERRORS_EXTRA();
#endif

		s.d.s &= ~AT_Flags;
		s.d.s |= s.s.s&AT_Flags;
	
		if (!(s.d.s&AT_Disable))
		{
			int mode = GL_LESS;
			switch (s.d.s&AT_Flags)
			{
			case AT_Greater:
				mode = GL_GREATER;
				break;
			case AT_LEqual:
				mode = GL_LEQUAL;
				break;
			case AT_GEqual:
				mode = GL_GEQUAL;
				break;
			}

#if !defined(RAD_OPT_OGLES2)
			glAlphaFunc(mode, s.s.aref);
			CHECK_GL_ERRORS_EXTRA();
#endif
			s.d.aref = s.s.aref;
		}
	}

	if (ss&SCT_Flags)
	{
		if (ss&SCT_Enable)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);

		s.d.s &= ~SCT_Flags;
		s.d.s |= s.s.s&SCT_Flags;
	}

	// blends

	ds  = s.d.b&~s.s.b; // not in s
	if (f) s.d.b = ds; // clear everything set in source to force apply
	ss  = s.s.b&~s.d.b; // not in d

	if (ss&BM_Flags)
	{
		if ((s.d.b&BM_Off) == BM_Off)
		{
			glEnable(GL_BLEND);
			CHECK_GL_ERRORS_EXTRA();
		}
		else if((s.s.b&BM_Off) == BM_Off)
		{
			glDisable(GL_BLEND);
			CHECK_GL_ERRORS_EXTRA();
		}

		if (s.s.b&(BMS_Flags|BMD_Flags))
		{
			int sb = GL_ONE;
			int db = GL_ZERO;

			int z = s.s.b&BMS_Flags;
			switch (z)
			{
			case BMS_One: sb = GL_ONE; break;
			case BMS_DstColor: sb = GL_DST_COLOR; break;
			case BMS_InvDstColor: sb = GL_ONE_MINUS_DST_COLOR; break;
			case BMS_SrcAlpha: sb = GL_SRC_ALPHA; break;
			case BMS_InvSrcAlpha: sb = GL_ONE_MINUS_SRC_ALPHA; break;
			case BMS_DstAlpha: sb = GL_DST_ALPHA; break;
			case BMS_InvDstAlpha: sb = GL_ONE_MINUS_DST_ALPHA; break;
			case BMS_SrcAlphaSaturate: sb = GL_SRC_ALPHA_SATURATE; break;
			case BMS_Zero: sb = GL_ZERO; break;
			}

			z = s.s.b&BMD_Flags;
			switch( z )
			{
			case BMD_One: db = GL_ONE; break;
			case BMD_SrcColor: db = GL_SRC_COLOR; break;
			case BMD_InvSrcColor: db = GL_ONE_MINUS_SRC_COLOR; break;
			case BMD_SrcAlpha: db = GL_SRC_ALPHA; break;
			case BMD_InvSrcAlpha: db = GL_ONE_MINUS_SRC_ALPHA; break;
			case BMD_DstAlpha: db = GL_DST_ALPHA; break;
			case BMD_InvDstAlpha: db = GL_ONE_MINUS_DST_ALPHA; break;
			case BMD_Zero: db = GL_ZERO; break;
			}

			glBlendFunc(sb, db);
			CHECK_GL_ERRORS_EXTRA();
		}

		s.d.b &= ~BM_Flags;
		s.d.b |= s.s.b&BM_Flags;
	}
}

void GLState::CommitT(S &st, int t, T &s, T &d, bool f)
{
	if (!f)
	{ // Note: if we get into there the s.tex != d.tex was tested.
		if (s.tex)
		{
			if (d.tex)
			{
				if (d.tex->target != s.tex->target)
				{
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
				}
				else
				{
					st.SetActiveTexture(t);
					CHECK_GL_ERRORS_EXTRA();
					glBindTexture(s.tex->target, s.tex->id);
					CHECK_GL_ERRORS_EXTRA();
				}
			}
			else
			{
				st.SetActiveTexture(t);
				CHECK_GL_ERRORS_EXTRA();
				glBindTexture(s.tex->target, s.tex->id);
				CHECK_GL_ERRORS_EXTRA();
#if defined(RAD_OPT_PC)
				glEnable(s.tex->target);
				CHECK_GL_ERRORS_EXTRA();
#endif
			}
		}
		else if (d.tex)
		{
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
	}
	else
	{
		if (d.tex)
		{
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
		if (s.tex)
		{
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

	int ds  = s.s&~d.s; // not in s
	if (f) d.s = ds; // clear everything set in source to force apply
	int ss  = s.s&~d.s; // not in d

	if (ss&TEM_Flags)
	{
#if !defined(RAD_OPT_OGLES)
		int mode = GL_DECAL;

		switch (ss&TEM_Flags)
		{
		case TEM_Modulate:	mode = GL_MODULATE;	break;
		case TEM_Blend: mode = GL_BLEND; break;
		case TEM_Replace: mode = GL_REPLACE; break;
		case TEM_Combine: mode = GL_COMBINE; break;
		}

		st.SetActiveTexture(t);
		CHECK_GL_ERRORS_EXTRA();
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		CHECK_GL_ERRORS_EXTRA();
#endif
		d.s &= ~TEM_Flags;
		d.s |= s.s&TEM_Flags;
	}
}

void GLState::CommitAA(S &s, int i, AA &sa, AA &da, bool f)
{
	if (f || (!s.vaoBound && sa.e != da.e))
	{
		if (sa.e)
		{
#if defined(RAD_OPT_PC)
			// ATI driver crash fix (always set VertexAttribPointer if we enable a stream).
			f = f || !s.vaoBound;
#endif
			gl.EnableVertexAttribArrayARB(i);
			CHECK_GL_ERRORS_EXTRA();
		}
		else
		{
			gl.DisableVertexAttribArrayARB(i);
			CHECK_GL_ERRORS_EXTRA();
		}

		da.e = sa.e;
	}

	if (!sa.e && !sa.vb && da.vb)
		da.vb.reset();

	if (sa.e && sa.vb && (f || (!s.vaoBound && sa != da)))
	{
		const GLvoid *p;

		if (gl.vbos)
		{
			BindBuffer(GL_ARRAY_BUFFER_ARB, sa.vb);
			CHECK_GL_ERRORS_EXTRA();
			p = (const GLvoid*)static_cast<AddrSize>(sa.ofs);
		}
		else
		{
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

void GLState::CommitScissor(S &s, bool f)
{
	if (f || 
		((s.s.s&SCT_Enable) && 
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

void GLState::S::BindBuffer(GLenum target, const GLVertexBufferRef &vb, bool force)
{
	switch (target)
	{
	case GL_ARRAY_BUFFER_ARB:
		if (vbb[0].get() != vb.get() || force)
		{
			vbb[0] = vb;
			if (gl.vbos)
			{
				gl.BindBufferARB(target, vb ? vb->id : 0);
				CHECK_GL_ERRORS();
			}
		} break;
	case GL_ELEMENT_ARRAY_BUFFER_ARB:
		if (vbb[1].get() != vb.get() || force)
		{
			vbb[1] = vb;
			if (gl.vbos)
			{
				gl.BindBufferARB(target, vb ? vb->id : 0);
				CHECK_GL_ERRORS();
			}
		} break;
	}
}

void GLState::S::BindVertexArray(const GLVertexArrayRef &_va, bool force)
{
	RAD_ASSERT(gl.vbos&&gl.vaos);

	if (vao.get() != _va.get() || force)
	{
		vao = _va;
		gl.BindVertexArray(vao ? vao->id : 0);
		CHECK_GL_ERRORS();
	}
}

} // r
