// RGL.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "RGL.h"
#include <string.h>

#if defined(RAD_OPT_IPHONE)
	//#define AUTOGEN_MIPMAPS
#else
	#define AUTOGEN_MIPMAPS
#endif

namespace
{
	int s_activeTexture = 0;
}

GLRenderTexture::GLRenderTexture(
	int w, 
	int h,
	bool wrap,
	bool mipmap,
	int format, // GL_RGB, GL_RGBA, GL_RGBA4, GL_RGB5_A1, GL_RGB565
	int depthFormat
	) : GLTexture(GL_TEXTURE_2D, w, h), m_fb(0), m_db(0)
{
	RAD_ASSERT(format==GL_RGB||format==GL_RGBA||format==GL_RGBA4||format==GL_RGB5_A1||format==GL_RGB565);
	
	glGenFramebuffers(1, &m_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fb);
	
	if (depthFormat != 0)
	{
		glGenRenderbuffers(1, &m_db);
		glBindRenderbuffer(GL_RENDERBUFFER, m_db);
		glRenderbufferStorage(GL_RENDERBUFFER, depthFormat, w, h);
		glFramebufferRenderbuffer(
			GL_FRAMEBUFFER, 
			GL_DEPTH_ATTACHMENT,
			GL_RENDERBUFFER,
			m_db
		);
	}

	// reserve texture space.
	GLState::SetDriverTMUTexture(0, this, false);
	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, 0);

	if (wrap)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (mipmap)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 0);
	}

	glFramebufferTexture2D(
		GL_FRAMEBUFFER, 
		GL_COLOR_ATTACHMENT0, 
		GL_TEXTURE_2D, 
		Id(),
		0
	);

	RAD_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	GLState::SetDriverTMUTexture(0, 0, false);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	CHECK_GL_ERRORS();
}

GLRenderTexture::~GLRenderTexture()
{
	if (m_fb)
	{
		glDeleteFramebuffers(1, &m_fb);
	}

	if (m_db)
	{
		glDeleteRenderbuffers(1, &m_db);
	}
}

void GLRenderTexture::Begin() const
{
	glGetIntegerv(GL_VIEWPORT, &m_vp[0]);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fb);
	glViewport(0, 0, Width(), Height());
	CHECK_GL_ERRORS();
}

void GLRenderTexture::End() const
{
	glViewport(m_vp[0], m_vp[1], m_vp[2], m_vp[3]);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	CHECK_GL_ERRORS();
}

void R_glSetActiveTexture(int texture)
{
	if (s_activeTexture != texture)
	{
		s_activeTexture = texture;
		glActiveTexture(texture+GL_TEXTURE0);
		glClientActiveTexture(texture+GL_TEXTURE0);
	}
}

//
// this texture should already be bound
//
void R_glUploadTexture(int target, int width, int height, int depth, int type, int components, int flags, void* data, int dataLen, void **mips, int *mipSizes, int numMips )
{
	RAD_ASSERT((width&(width-1))==0);
	RAD_ASSERT((height&(height-1))==0);
	RAD_ASSERT( type );
	RAD_ASSERT( components );
	RAD_ASSERT( data||mips );

#if !defined(AUTOGEN_MIPMAPS)
	if (!mips) flags &= ~MipmapTextureFlag;
#endif

	CHECK_GL_ERRORS();

	RAD_ASSERT( components == GL_RGB || components == GL_RGBA || components == GL_LUMINANCE ||
			components == GL_LUMINANCE_ALPHA || components == GL_ALPHA || components == GL_COLOR_INDEX);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	CHECK_GL_ERRORS();
	
	
	// set texture params.
	if( flags&WrapTextureFlag )
	{
		glTexParameteri( target, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( target, GL_TEXTURE_WRAP_T, GL_REPEAT );
	}
	else
	{
		glTexParameteri( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}
	
	CHECK_GL_ERRORS();
	
	if( flags&FilterTextureFlag )
	{
		if( flags&MipmapTextureFlag )
		{
			/*if( (flags&_upf_trilinear) )
				glTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			else*/
			glTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
		
#if defined(AUTOGEN_MIPMAPS)
			glTexParameteri( target, GL_GENERATE_MIPMAP, (0==numMips) ? GL_TRUE : GL_FALSE);

#endif
			glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		}
		else
		{
#if defined(AUTOGEN_MIPMAPS)
			glTexParameteri( target, GL_GENERATE_MIPMAP, GL_FALSE);
#endif
			glTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		}
	}
	else
	{
		if( flags&MipmapTextureFlag )
		{
#if defined(AUTOGEN_MIPMAPS)
			glTexParameteri( target, GL_GENERATE_MIPMAP, (0==numMips) ? GL_TRUE : GL_FALSE);
#endif
			glTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
			glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		}
		else
		{
#if defined(AUTOGEN_MIPMAPS)
			glTexParameteri( target, GL_GENERATE_MIPMAP, GL_FALSE);
#endif
			glTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		}
	}
	
	CHECK_GL_ERRORS();
	
	if( target == GL_TEXTURE_2D )
	{
	
		if( flags&MipmapTextureFlag && numMips > 0 && mips )
		{
			if( mips )
			{
				int i;
				int mipwidth, mipheight;
				
				mipwidth = width;
				mipheight = height;
				
				//glTexParameteri( target, GL_TEXTURE_BASE_LEVEL, 0 );
				CHECK_GL_ERRORS();
				
				for(i = 0; i < numMips; i++)
				{
					RAD_ASSERT( mips[i] && mipSizes );
					
					glTexImage2D( GL_TEXTURE_2D, i, components, mipwidth, mipheight, 0, components, type, mips[i] );
					
					mipwidth >>= 1;
					mipheight >>= 1;
					
					if( mipwidth < 1 || mipheight < 1 )
						break;
				}

				//glTexParameteri( target, GL_TEXTURE_MAX_LEVEL, i-1 );
				CHECK_GL_ERRORS();

			}
		}
		else if (data)
		{
			glTexImage2D( GL_TEXTURE_2D, 0, components, width, height, 0, components, type, data );
		}
		
	}
	
	CHECK_GL_ERRORS();
}

//////////////////////////////////////////////////////////////////////////////////////////
// GLState
//////////////////////////////////////////////////////////////////////////////////////////

GLState::State GLState::s_ds;

void GLState::Commit(bool unconditional)
{
	if (m_s.state || m_s.blend)
	{
		CHECK_GL_ERRORS();
		SetDriverState(m_s.state, m_s.blend, unconditional);
		CHECK_GL_ERRORS();
	}

	for (int i = 0; i < NumGLArrays; ++i)
	{
		const Array &array = m_s.arrays[i];

		CHECK_GL_ERRORS();

		SetDriverArray(
			i,
			array.size,
			array.type,
			array.stride,
			array.data,
			unconditional
		);

		CHECK_GL_ERRORS();
	}

	for (int i = 0; i < NumGLTMUs; ++i)
	{
		const TMU &tmu = m_s.tmus[i];
		
		CHECK_GL_ERRORS();

		SetDriverTMUTexture(
			i,
			tmu.tex,
			unconditional
		);

		CHECK_GL_ERRORS();

		SetDriverTMUState(
			i,
			tmu.state,
			unconditional
		);

		CHECK_GL_ERRORS();

		SetDriverTMUCombineMode(
			i,
			tmu.combine,
			unconditional
		);

		CHECK_GL_ERRORS();

		for (int k = 0; k < NumGLTMUArrays; ++k)
		{
			const Array &array = tmu.arrays[k];

			CHECK_GL_ERRORS();

			SetDriverTMUArray(
				i,
				k,
				array.size,
				array.type,
				array.stride,
				array.data,
				unconditional
			);

			CHECK_GL_ERRORS();
		}
	}
}

void GLState::SetTMUCombineMode(
	int tmu, 
	const Combine &src
)
{
	RAD_ASSERT(tmu >= 0 && tmu < NumGLTMUs);
	TMU &t = m_s.tmus[tmu];
	t.combine = src;
//	Combine &dst = t.combine;
//	
//#define COPY(_x) if (src.##_x != dst.##_x) { dst.##_x = src.##_x; }
//
//	COPY(rgb_mode)
//	COPY(alpha_mode)
//	COPY(src0_rgb)
//	COPY(src1_rgb)
//	COPY(src2_rgb)
//	COPY(op0_rgb)
//	COPY(op1_rgb)
//	COPY(op2_rgb)
//	COPY(src0_alpha)
//	COPY(src1_alpha)
//	COPY(src2_alpha)
//	COPY(op0_alpha)
//	COPY(op1_alpha)
//	COPY(op2_alpha)
//
//#undef COPY
//	
//	if (src.rgb_scale != std::numeric_limits<float>::min())
//	{
//		dst.rgb_scale = src.rgb_scale;
//	}
//
//	if (src.alpha_scale != std::numeric_limits<float>::min())
//	{
//		dst.alpha_scale = src.alpha_scale;
//	}
}

//
// Driver State Access
//

void GLState::Initialize()
{
	memset(&s_ds, 0, sizeof(s_ds));
	SetDriverState(NoArrays|DT_Disable|CFM_None|CWM_All|DWM_Enable, BM_Off);
	DisableAllDriverTMUs();
}

void GLState::SetDriverState(
	int ss, 
	int sb, 
	bool unconditional
)
{
	CHECK_GL_ERRORS();
	
	int &ds = s_ds.state;
	int &db = s_ds.blend;

	// Depth Test.

	if (unconditional || ((ss&DT_Flags) != 0 &&
		(ss&DT_Flags) != (ds&DT_Flags)))
	{
		if ((ss&DT_Disable) && (unconditional || !(ds&DT_Disable)))
		{
			glDisable(GL_DEPTH_TEST);
		}
		else if (!(ss&DT_Disable) && (unconditional || (ds&DT_Disable)))
		{
			glEnable(GL_DEPTH_TEST);
		}

		if (unconditional || !(ss&DT_Disable))
		{
			int m;
			switch (ss&DT_Flags)
			{
			case DT_Always: m = GL_ALWAYS; break;
			case DT_Less: m = GL_LESS; break;
			case DT_Greater : m = GL_GREATER; break;
			case DT_LEqual : m = GL_LEQUAL; break;
			case DT_GEqual : m = GL_GEQUAL; break;
			case DT_Equal : m = GL_EQUAL; break;
			case DT_Never : m = GL_NEVER; break;
			}
			glDepthFunc(m);
		}

		ds &= ~DT_Flags;
		ds |= ss&DT_Flags;

		CHECK_GL_ERRORS();
	}

	// Depth Write Mask.

	if (unconditional || ((ss&DWM_Flags) &&
		((ss&DWM_Flags) != (ds&DWM_Flags))))
	{
		if (ss&DWM_Enable)
		{
			glDepthMask(GL_TRUE);
		}
		else
		{
			glDepthMask(GL_FALSE);
		}

		ds &= ~DWM_Flags;
		ds |= ss&DWM_Flags;

		CHECK_GL_ERRORS();
	}

	// Cull Face

	if (unconditional || ((ss&CFM_Flags) &&
		((ss&CFM_Flags) != (ds&CFM_Flags))))
	{
		if ((ss&CFM_None) && (unconditional || !(ds&CFM_None)))
		{
			glDisable(GL_CULL_FACE);
		}
		else if (!(ss&CFM_None) && (unconditional || (ds&CFM_None)))
		{
			glEnable(GL_CULL_FACE);
		}

		if (unconditional || !(ss&CFM_None))
		{
			if ((ss&CFM_Front) && (unconditional || !(ds&CFM_Front)))
			{
				glCullFace(GL_FRONT);
			}
			else if ((ss&CFM_Back) && (unconditional || !(ds&CFM_Back)))
			{
				glCullFace(GL_BACK);
			}

			if ((ss&CFM_CW) && (unconditional || !(ds&CFM_CW)))
			{
				glFrontFace(GL_CW);
			}
			else if ((ss&CFM_CCW) && (unconditional || !(ds&CFM_CCW)))
			{
				glFrontFace(GL_CCW);
			}
		}

		ds &= ~CFM_Flags;
		ds |= ss&CFM_Flags;

		CHECK_GL_ERRORS();
	}

	// Color Write Mask

	if (unconditional || ((ss&CWM_Flags) &&
		((ss&CFM_Flags) != (ds&CFM_Flags))))
	{
		GLboolean r, g, b, a;

		r = (ss&CWM_R) ? GL_TRUE : GL_FALSE;
		g = (ss&CWM_G) ? GL_TRUE : GL_FALSE;
		b = (ss&CWM_B) ? GL_TRUE : GL_FALSE;
		a = (ss&CWM_A) ? GL_TRUE : GL_FALSE;

		glColorMask(r, g, b, a);

		ds &= ~CFM_Flags;
		ds |= ss&CFM_Flags;

		CHECK_GL_ERRORS();
	}

	// Normal Array

	if (unconditional || ((ss&NA_Flags) &&
		((ss&NA_Flags) != (ds&NA_Flags))))
	{
		if ((ss&NA_On) && (unconditional || !(ds&NA_On)))
		{
			glEnableClientState(GL_NORMAL_ARRAY);
		}
		else if ((ss&NA_Off) && (unconditional || !(ds&NA_Off)))
		{
			glDisableClientState(GL_NORMAL_ARRAY);
		}

		ds &= ~NA_Flags;
		ds |= ss&NA_Flags;

		CHECK_GL_ERRORS();
	}

	// Vertex Array

	if (unconditional || ((ss&VA_Flags) &&
		((ss&VA_Flags) != (ds&VA_Flags))))
	{
		if ((ss&VA_On) && (unconditional || !(ds&VA_On)))
		{
			glEnableClientState(GL_VERTEX_ARRAY);
		}
		else if ((ss&VA_Off) && (unconditional || !(ds&VA_Off)))
		{
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		ds &= ~VA_Flags;
		ds |= ss&VA_Flags;

		CHECK_GL_ERRORS();
	}

	// Color Array

	if (unconditional || ((ss&CA_Flags) &&
		((ss&CA_Flags) != (ds&CA_Flags))))
	{
		if ((ss&CA_On) && (unconditional || !(ds&CA_On)))
		{
			glEnableClientState(GL_COLOR_ARRAY);
		}
		else if ((ss&CA_Off) && (unconditional || !(ds&CA_Off)))
		{
			glDisableClientState(GL_COLOR_ARRAY);
		}

		ds &= ~CA_Flags;
		ds |= ss&CA_Flags;

		CHECK_GL_ERRORS();
	}

	// Index Array

	/*if (unconditional || ((ss&IA_FLAGS) &&
		((ss&IA_FLAGS) != (ds&IA_FLAGS))))
	{
		if ((ss&IA_ON) && (unconditional || !(ds&IA_ON)))
		{
			glEnableClientState(GL_INDEX_ARRAY);
		}
		else if ((ss&IA_OFF) && (unconditional || !(ds&IA_OFF)))
		{
			glDisableClientState(GL_INDEX_ARRAY);
		}

		ds &= ~IA_FLAGS;
		ds |= ss&IA_FLAGS;

		CHECK_GL_ERRORS();
	}*/

	if (unconditional || ((sb&BM_Flags) &&
		((sb&BM_Flags) != (db&BM_Flags))))
	{
		if (((sb&BM_Off) == BM_Off) && (unconditional || ((db&BM_Off) != BM_Off)))
		{
			glDisable(GL_BLEND);
		}
		else if (((sb&BM_Off) != BM_Off) && (unconditional || ((db&BM_Off) == BM_Off)))
		{
			glEnable(GL_BLEND);
		}

		if ((sb&BM_Off) != BM_Off)
		{
			int sblend = GL_ONE;
			int dblend = GL_ZERO;
			int f;

			f = (sb&BMS_Flags) ? sb : db;
			f &= BMS_Flags;

			switch (f)
			{
			case BMS_One: sblend = GL_ONE; break;
			case BMS_DstColor: sblend = GL_DST_COLOR; break;
			case BMS_InvDstColor: sblend = GL_ONE_MINUS_DST_COLOR; break;
			case BMS_SrcAlpha: sblend = GL_SRC_ALPHA; break;
			case BMS_InvSrcAlpha: sblend = GL_ONE_MINUS_SRC_ALPHA; break;
			case BMS_DstAlpha: sblend = GL_DST_ALPHA; break;
			case BMS_InvDstAlpha: sblend = GL_ONE_MINUS_DST_ALPHA; break;
			case BMS_SrcAlphaSaturate: sblend = GL_SRC_ALPHA_SATURATE; break;
			case BMS_Zero: sblend = GL_ZERO; break;
			}

			/*db &= ~BMS_Flags;
			db |= f;*/

			f = (sb&BMD_Flags) ? sb : db;
			f &= BMD_Flags;

			switch( f )
			{
			case BMD_One: dblend = GL_ONE; break;
			case BMD_SrcColor: dblend = GL_SRC_COLOR; break;
			case BMD_InvSrcColor: dblend = GL_ONE_MINUS_SRC_COLOR; break;
			case BMD_SrcAlpha: dblend = GL_SRC_ALPHA; break;
			case BMD_InvSrcAlpha: dblend = GL_ONE_MINUS_SRC_ALPHA; break;
			case BMD_DstAlpha: dblend = GL_DST_ALPHA; break;
			case BMD_InvDstAlpha: dblend = GL_ONE_MINUS_DST_ALPHA; break;
			case BMD_Zero: dblend = GL_ZERO; break;
			}

			/*db &= ~BMD_Flags;
			db |= f;*/

			glBlendFunc(sblend, dblend);
		}

		db &= ~BM_Flags;
		db |= sb&BM_Flags;

		CHECK_GL_ERRORS();
	}
}

void GLState::SetDriverArray(
	int arrayNum, 
	int size, 
	int type, 
	int stride, 
	const void *data, 
	bool unconditional
)
{
	const int &ds = s_ds.state;
	Array &dst = s_ds.arrays[arrayNum];
	
	if (unconditional ||
		(dst.size != size || dst.type != type || dst.stride != stride || dst.data != data))
	{
		switch (arrayNum)
		{
		case Vertex:
			if (!(ds&VA_On)) return;
			glVertexPointer(size, type, stride, data);
			break;
		case Color:
			if (!(ds&CA_On)) return;
			glColorPointer(size, type, stride, data);
			break;
		/*case INDEX: 
			if (!(ds&IA_ON)) return;
			glIndexPointer(type, stride, data);
			break;*/
		case Normal: 
			if (!(ds&NA_On)) return;
			glNormalPointer(type, stride, data);
			break;
		}

		dst.size = size;
		dst.type = type;
		dst.stride = stride;
		dst.data = data;
		CHECK_GL_ERRORS();
	}
}

void GLState::SetDriverTMUState(
	int tmuNum, 
	int ss, 
	bool unconditional
)
{
	RAD_ASSERT(tmuNum >=0 && tmuNum < NumGLTMUs);
	TMU &tmu = s_ds.tmus[tmuNum];
	int &ds = tmu.state;

	R_glSetActiveTexture(tmuNum);

	if ((ss&TEM_Flags) && (unconditional || ((ss&TEM_Flags) != (ds&TEM_Flags))))
	{
		int mode = GL_DECAL;
		switch (ss&TEM_Flags)
		{
			case TEM_Modulate:	mode = GL_MODULATE;	break;
			case TEM_Decal: mode = GL_DECAL; break;
			case TEM_Blend: mode = GL_BLEND; break;
			case TEM_Replace: mode = GL_REPLACE; break;
			case TEM_Combine: mode = GL_COMBINE; break;
		}

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		ds &= ~TEM_Flags;
		ds |= ss&TEM_Flags;
		CHECK_GL_ERRORS();
	}

	if ((ss&TCA_Flags) && (unconditional || ((ss&TCA_Flags) != (ds&TCA_Flags))))
	{
		if ((ss&TCA_On) && (unconditional || (!(ds&TCA_On))))
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		else if ((ss&TCA_Off) || (unconditional || !(ds&TCA_Off)))
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		ds &= ~TCA_Flags;
		ds |= ss&TCA_Flags;
		CHECK_GL_ERRORS();
	}
}

void GLState::SetDriverTMUTexture(
	int tmuNum, 
	GLTexture *tex, 
	bool unconditional
)
{
	RAD_ASSERT(tmuNum >=0 && tmuNum < NumGLTMUs);
	TMU &tmu = s_ds.tmus[tmuNum];

	R_glSetActiveTexture(tmuNum);

	if (tmu.tex != tex)
	{
		if (tmu.tex && tex && (tmu.target != tex->Target()))
		{
			glDisable(tmu.target);
			tmu.target = tex->Target();
			glEnable(tmu.target);
			CHECK_GL_ERRORS();
		}
		else if (!tmu.tex)
		{
			tmu.target = tex->Target();
			glEnable(tmu.target);
			CHECK_GL_ERRORS();
		}
		else if (!tex)
		{
			glDisable(tmu.target);
			tmu.target = 0;
			CHECK_GL_ERRORS();
		}

		tmu.tex = tex;

		if (tex)
		{
			tex->Bind();
			CHECK_GL_ERRORS();
		}
	}
}

void GLState::SetDriverTMUArray(
	int tmuNum,
	int array,
	int size,
	int type,
	int stride,
	const void *data,
	bool unconditional
)
{
	RAD_ASSERT(tmuNum >=0 && tmuNum < NumGLTMUs);
	RAD_ASSERT(array >= 0 && array < NumGLTMUArrays);
	TMU &tmu = s_ds.tmus[tmuNum];

	Array &dst = tmu.arrays[array];
	
	if (unconditional ||
		((tmu.state&TCA_On) && (dst.size != size || dst.type != type || dst.stride != stride || dst.data != data)))
	{
		switch (array)
		{
		case TexCoord:
			glTexCoordPointer(size, type, stride, data);
			break;
		}

		dst.size = size;
		dst.type = type;
		dst.stride = stride;
		dst.data = data;
	}
	
	R_glSetActiveTexture(tmuNum);
}

void GLState::SetDriverTMUCombineMode(
	int tmuNum, 
	const Combine &src,
	bool unconditional
)
{
	CHECK_GL_ERRORS();
	RAD_ASSERT(tmuNum >= 0 && tmuNum < NumGLTMUs);
	TMU &tmu = s_ds.tmus[tmuNum];
	if (!unconditional && !(tmu.state&TEM_Combine)) return;
	Combine &dst = tmu.combine;

#define SET(_x, _state) \
	if ((src._x != 0) && (unconditional || (src._x != dst._x))) \
	{ glTexEnvi(GL_TEXTURE_ENV, _state, src._x); dst._x = src._x; }

	SET(rgb_mode, GL_COMBINE_RGB)
	CHECK_GL_ERRORS();
	SET(alpha_mode, GL_COMBINE_ALPHA)
	CHECK_GL_ERRORS();

	SET(src0_rgb, GL_SRC0_RGB)
	CHECK_GL_ERRORS();
	SET(src1_rgb, GL_SRC1_RGB)
	CHECK_GL_ERRORS();
	SET(src2_rgb, GL_SRC2_RGB)
	CHECK_GL_ERRORS();
	SET(op0_rgb, GL_OPERAND0_RGB)
	CHECK_GL_ERRORS();
	SET(op1_rgb, GL_OPERAND1_RGB)
	CHECK_GL_ERRORS();
	SET(op2_rgb, GL_OPERAND2_RGB)
	CHECK_GL_ERRORS();

	SET(src0_alpha, GL_SRC0_ALPHA)
	CHECK_GL_ERRORS();
	SET(src1_alpha, GL_SRC1_ALPHA)
	CHECK_GL_ERRORS();
	SET(src2_alpha, GL_SRC2_ALPHA)
	CHECK_GL_ERRORS();
	SET(op0_alpha, GL_OPERAND0_ALPHA)
	CHECK_GL_ERRORS();
	SET(op1_alpha, GL_OPERAND1_ALPHA)
	CHECK_GL_ERRORS();
	SET(op2_alpha, GL_OPERAND2_ALPHA)
	CHECK_GL_ERRORS();

	if ((src.rgb_scale != std::numeric_limits<float>::min()) && 
		(unconditional || (src.rgb_scale != dst.rgb_scale)))
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, src.rgb_scale);
		dst.rgb_scale = src.rgb_scale;
		CHECK_GL_ERRORS();
	}

	if ((src.alpha_scale != std::numeric_limits<float>::min()) &&
		(unconditional || (src.alpha_scale != dst.alpha_scale)))
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, src.alpha_scale);
		dst.alpha_scale = src.alpha_scale;
		CHECK_GL_ERRORS();
	}

#undef SET

}