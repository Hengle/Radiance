// RGL.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <Runtime/Base.h>
#include <Runtime/Math/Vector.h>
#include <Runtime/Math/Matrix.h>
#include <Runtime/Math/Quaternion.h>

#include "RGLBackend.h"
typedef math::Vector3<float> GLVec3;
typedef math::Matrix4X4<float> GLMat4;
typedef math::Quaternion<float> GLQuat;
typedef math::AxisAngle<float> GLAxisAngle;

const GLVec3 GLXAxis = GLVec3(float(1), float(0), float(0));
const GLVec3 GLYAxis = GLVec3(float(0), float(1), float(0));
const GLVec3 GLZAxis = GLVec3(float(0), float(0), float(1));

#include <string.h>
#undef min
#include <limits>

#include <Runtime/PushSystemMacros.h>

void CheckGLErrors();

#if defined(RAD_OPT_DEBUG)
	#define CHECK_GL_ERRORS() CheckGLErrors()
#else
	#define CHECK_GL_ERRORS()
#endif

//
// Common vertex types.
//
#pragma pack (push, 1)

template <typename P>
struct V_P2
{
	V_P2() {}
	
	V_P2(
		const P &_x, 
		const P &_y
	) : x(_x), y(_y) {}

	P x, y;
};

template <typename P, typename T>
struct V_P2T2
{
	V_P2T2() {}
	
	V_P2T2(
		const P &_x, 
		const P &_y,
		const T &_s,
		const T &_t
	) : x(_x), y(_y), s(_s), t(_t) {}

	P x, y;
	T s, t;
};

template <typename P, typename C, typename T>
struct V_P2CT2
{
	V_P2CT2() {}
	
	V_P2CT2(
		const P &_x, 
		const P &_y,
		const T &_s,
		const T &_t,
		const C &_r,
		const C &_g,
		const C &_b,
		const C &_a
	) : x(_x), y(_y), s(_s), t(_t), r(_r), g(_g), b(_b), a(_a) {}

	P x, y;
	T s, t;
	C r, g, b, a;
};

template <typename P, typename C>
struct V_P2C
{
	V_P2C() {}
	
	V_P2C(
		const P &_x, 
		const P &_y,
		const C &_r,
		const C &_g,
		const C &_b,
		const C &_a
	) : x(_x), y(_y), r(_r), g(_g), b(_b), a(_a) {}

	P x, y;
	C r, g, b, a;
};

template <typename P, typename N>
struct V_P2N
{
	V_P2N() {}
	V_P2N(
		const P &_x, 
		const P &_y, 
		const N &_nx, 
		const N &_ny, 
		const N &_nz
	) :	x(_x), y(_y), nx(_nx), ny(_ny), nz(_nz) {}

	P x, y;
	N nx, ny, nz;
};

template <typename P, typename N, typename C>
struct V_P2NC
{
	V_P2NC() {}
	V_P2NC(
		const P &_x, 
		const P &_y, 
		const N &_nx, 
		const N &_ny, 
		const N &_nz, 
		const C &_r, 
		const C &_g, 
		const C &_b, 
		const C &_a) : x(_x), y(_y), nx(_nx), ny(_ny), nz(_nz), r(_r), g(_g), b(_b), a(_a) {}

	P x, y;
	N nx, ny, nz;
	C r, g, b, a;
};

template <typename P, typename N, typename C, typename T>
struct V_P2NCT2
{
	V_P2NCT2() {}
	V_P2NCT2(
		const P &_x, 
		const P &_y, 
		const N &_nx, 
		const N &_ny, 
		const N &_nz, 
		const C &_r, 
		const C &_g, 
		const C &_b, 
		const C &_a,
		const T &_t0s,
		const T &_t0t) : x(_x), y(_y), nx(_nx), ny(_ny), nz(_nz), r(_r), g(_g), b(_b), a(_a), t0s(_t0s), t0t(_t0t) {}
	
	P x, y;
	N nx, ny, nz;
	C r, g, b, a;
	T t0s, t0t;
};

template <typename P, typename N, typename C, typename T>
struct V_P2NCT2T2
{
	V_P2NCT2T2() {}
	V_P2NCT2T2(
		const P &_x, 
		const P &_y, 
		const N &_nx, 
		const N &_ny, 
		const N &_nz, 
		const C &_r, 
		const C &_g, 
		const C &_b, 
		const C &_a,
		const T &_t0s,
		const T &_t0t,
		const T &_t1s,
		const T &_t1t) : x(_x), y(_y), nx(_nx), ny(_ny), nz(_nz), r(_r), g(_g), b(_b), a(_a), t0s(_t0s), t0t(_t0t), t1s(_t1s), t1t(_t1t) {}
	
	P x, y;
	N nx, ny, nz;
	C r, g, b, a;
	T t0s, t0t;
	T t1s, t1t;
};

#pragma pack (pop)

enum
{
	//
	// States
	//

	// Depth Test

	DT_Disable = 0x1,
	DT_Always = 0x2,
	DT_Less = 0x4,
	DT_Greater = 0x8,
	DT_LEqual = 0x10,
	DT_GEqual = 0x20,
	DT_Equal = 0x40,
	DT_Never = 0x80,
	DT_Flags = (DT_Disable|DT_Always|DT_Less|DT_Greater|DT_LEqual|DT_GEqual|DT_Equal|DT_Never),

	// Depth Write Mode

	DWM_Enable = 0x100,
	DWM_Disable = 0x200,
	DWM_Flags = (DWM_Enable|DWM_Disable),

	// Cull Face Mode

	CFM_Front = 0x400,
	CFM_Back = 0x800,
	CFM_None = 0x1000,
	CFM_CW = 0x2000,
	CFM_CCW = 0x4000,
	CFM_Flags = (CFM_Front|CFM_Back|CFM_None|CFM_CW|CFM_CCW),
	CFM_ModeFlags = (CFM_Front|CFM_Back|CFM_None),
	CFM_DirFlags = (CFM_CW|CFM_CCW),

	// Color Write Mask

	CWM_R = 0x8000,
	CWM_G = 0x10000,
	CWM_B = 0x20000,
	CWM_A = 0x40000,
	CWM_Off = 0x80000,
	CWM_All = (CWM_R|CWM_G|CWM_B|CWM_A),
	CWM_RGB = (CWM_R|CWM_G|CWM_B),
	CWM_Flags = (CWM_All|CWM_Off),

	// Vertex Array

	VA_On = 0x100000,
	VA_Off = 0x200000,
	VA_Flags = (VA_On|VA_Off),

	// Index Array

	// Unsupported On iPhone
/*	IA_ON = 0x200000,
	IA_OFF = 0x400000,
	IA_FLAGS = (IA_ON|IA_OFF), */

	// Color Array

	CA_On = 0x800000,
	CA_Off = 0x1000000,
	CA_Flags = (CA_On|CA_Off),

	// Normal Array

	NA_On = 0x2000000,
	NA_Off = 0x4000000,
	NA_Flags = (NA_On|NA_Off),

	// Array Combos

	NoArrays = (VA_Off|CA_Off|NA_Off),
	AllArrays = (VA_On|CA_On|NA_On),

	// Blend Mode Source

	BMS_One = 0x20,
	BMS_DstColor = 0x40,
	BMS_InvDstColor = 0x80,
	BMS_SrcAlpha = 0x100,
	BMS_InvSrcAlpha = 0x200,
	BMS_DstAlpha = 0x400,
	BMS_InvDstAlpha = 0x800,
	BMS_SrcAlphaSaturate = 0x1000,
	BMS_Zero = 0x2000,
	/*BMS_CONSTANT_COLOR = 0x4000,
	BMS_INV_CONSTANT_COLOR = 0x8000,
	BMS_CONSTANT_ALPHA = 0x10000,
	BMS_INV_CONSTANT_ALPHA = 0x20000,*/
	BMS_Flags = (BMS_One|BMS_DstColor|BMS_InvDstColor|BMS_SrcAlpha|BMS_InvSrcAlpha|
                 BMS_DstAlpha|BMS_InvDstAlpha|BMS_SrcAlphaSaturate|BMS_Zero/*|BMS_CONSTANT_COLOR|
				 BMS_INV_CONSTANT_COLOR|BMS_CONSTANT_ALPHA|BMS_INV_CONSTANT_ALPHA*/),

	// Blend Mode Destination

	BMD_Zero = 0x40000,
	BMD_One = 0x80000,
	BMD_SrcColor = 0x100000,
	BMD_InvSrcColor = 0x200000,
	BMD_SrcAlpha = 0x400000,
	BMD_InvSrcAlpha = 0x800000,
	BMD_DstAlpha = 0x1000000,
	BMD_InvDstAlpha = 0x2000000,
	/*BMD_CONSTANT_COLOR = 0x4000000,
	BMD_INV_CONSTANT_COLOR = 0x8000000,
	BMD_CONSTANT_ALPHA = 0x10000000,
	BMD_INV_CONSTANT_ALPHA = 0x20000000,*/
	BMD_Flags = (BMD_Zero|BMD_One|BMD_SrcColor|BMD_InvSrcColor|BMD_SrcAlpha|BMD_InvSrcAlpha|
                 BMD_DstAlpha|BMD_InvDstAlpha/*|BMD_CONSTANT_COLOR|BMD_INV_CONSTANT_COLOR|
                 BMD_CONSTANT_ALPHA|BMD_INV_CONSTANT_ALPHA*/),
	
    BM_Off = (BMS_One|BMD_Zero),
	BM_Flags = (BMS_Flags|BMD_Flags),

	//
	// TMU States
	//

	// Texture Environment Mode

	TEM_Modulate = 0x1,
	TEM_Decal = 0x2,
	TEM_Blend = 0x4,
	TEM_Replace = 0x8,
	TEM_Combine = 0x10,
	TEM_Flags = (TEM_Modulate|TEM_Decal|TEM_Blend|TEM_Replace|TEM_Combine),

	// Texture Coordinate Array

	TCA_On = 0x20,
	TCA_Off = 0x40,
	TCA_Flags = (TCA_On|TCA_Off),

	// Array Types

	Vertex = 0,
	Color,
	//INDEX,
	Normal,
	NumGLArrays,

	TexCoord = 0,
	NumGLTMUArrays,

	NumGLTMUs = 2,

	CompressedTextureFlag = 1,
	MipmapTextureFlag = 2,
	WrapTextureFlag = 4,
	FilterTextureFlag = 8
};

class GLState;
class GLTexture
{
public:

	GLTexture(GLenum target, int w, int h) : m_target(target), m_w(w), m_h(h)
	{
		RAD_ASSERT((w&(w-1)) == 0);
		RAD_ASSERT((h&(h-1)) == 0);
		glGenTextures(1, &m_id);
	}

	virtual ~GLTexture()
	{
		glDeleteTextures(1, &m_id);
	}

	int Width() const { return m_w; }
	int Height() const { return m_h; }

	GLenum Target() const { return m_target; }
	GLuint Id() const { return m_id; }

protected:

	void Bind() const
	{
		glBindTexture(Target(), Id());
	}

private:

	friend class GLState;

	int m_w, m_h;
	GLenum m_target;
	GLuint m_id;
};

class GLRenderTexture : public GLTexture
{
public:

	GLRenderTexture(
		int w, 
		int h,
		bool wrap,
		bool mipmap,
		int format, // GL_RGB, GL_RGBA, GL_RGBA4, GL_RGB5_A1, GL_RGB565
		int depthFormat=0 // or GL_DEPTH_COMPONENT16
	);
	virtual ~GLRenderTexture();

	void Begin() const;
	void End() const;

private:

	mutable GLint m_vp[4];
	GLuint m_fb;
	GLuint m_db;
};

class GLState
{
public:
	struct Combine
	{
		Combine() :
			rgb_mode(0),
			alpha_mode(0),
			src0_rgb(0),
			src1_rgb(0),
			src2_rgb(0),
			op0_rgb(0),
			op1_rgb(0),
			op2_rgb(0),
			rgb_scale(-std::numeric_limits<float>::max()),
			src0_alpha(0),
			src1_alpha(0),
			src2_alpha(0),
			op0_alpha(0),
			op1_alpha(0),
			op2_alpha(0),
			alpha_scale(-std::numeric_limits<float>::max())
		{
		}

		int rgb_mode;
		int alpha_mode;

		int src0_rgb;
		int src1_rgb;
		int src2_rgb;
		int op0_rgb;
		int op1_rgb;
		int op2_rgb;
		float rgb_scale;

		int src0_alpha;
		int src1_alpha;
		int src2_alpha;
		int op0_alpha;
		int op1_alpha;
		int op2_alpha;
		float alpha_scale;
	};

	explicit GLState(bool initWithDriverState = false)
	{
		if (initWithDriverState)
		{
			m_s = s_ds;
		}
		else
		{
			memset(&m_s, 0, sizeof(m_s));
			SetState(NoArrays|DT_Disable|CFM_None|CWM_All, BM_Off);
			DisableAllTMUs();
		}
	}

	GLState(const GLState &state)
	{
		memcpy(&m_s, &state, sizeof(m_s));
	}

	~GLState()
	{
	}

	void LoadDriverState()
	{
		m_s = s_ds;
	}

	void Commit(bool unconditional = false);
	
	void SetState(int state, int blend)
	{
		if (state != -1) { m_s.state = state; }
		if (blend != -1) { m_s.blend = blend; }
	}
	
	void SetArray(
		int array,
		int size,
		int type,
		int stride,
		const void *data
	)
	{
		RAD_ASSERT(array >= 0 && array < NumGLArrays);
		Array &a = m_s.arrays[array];
		a.size = size;
		a.type = type;
		a.stride = stride;
		a.data = data;
	}

	void DisableAllTMUs()
	{
		for (int i = 0; i < NumGLTMUs; ++i)
		{
			SetTMUTexture(i, 0);
			SetTMUState(i, TCA_Off);
		}
	}

	void SetTMUState(int tmu, int state)
	{
		RAD_ASSERT(tmu >= 0 && tmu < NumGLTMUs);
		TMU &t = m_s.tmus[tmu];
		t.state = state;
	}

	void SetTMUTexture(int tmu, GLTexture *tex)
	{
		RAD_ASSERT(tmu >= 0 && tmu < NumGLTMUs);
		TMU &t = m_s.tmus[tmu];
		t.tex = tex;
	}
	
	void SetTMUArray(
		int tmu,
		int array,
		int size,
		int type,
		int stride,
		const void *data
	)
	{
		RAD_ASSERT(tmu >= 0 && tmu < NumGLTMUs);
		RAD_ASSERT(array >= 0 && array < NumGLTMUArrays);
		TMU &t = m_s.tmus[tmu];
		Array &a = t.arrays[array];
		a.size = size;
		a.type = type;
		a.stride = stride;
		a.data = data;
	}

	void SetTMUCombineMode(
		int tmu, 
		const Combine &combine
	);
	
	//
	// Driver State Access
	//

	static void Initialize();

	static void SetDriverState(
		int state, 
		int blend, 
		bool unconditional = false
	);

	static void SetDriverArray(
		int array, 
		int size, 
		int type, 
		int stride, 
		const void *data, 
		bool unconditional = false
	);

	static void DisableAllDriverTMUs(bool unconditional = false)
	{
		for (int i = 0; i < NumGLTMUs; ++i)
		{
			SetDriverTMUTexture(i, 0, unconditional);
			SetDriverTMUState(i, TCA_Off, unconditional);
		}
	}

	static void SetDriverTMUState(
		int tmu, 
		int state, 
		bool unconditional = false
	);

	static void SetDriverTMUTexture(
		int tmu, 
		GLTexture *tex, 
		bool unconditional = false
	);

	static void SetDriverTMUArray(
		int tmu,
		int array,
		int size,
		int type,
		int stride,
		const void *data,
		bool unconditional = false
	);

	void SetDriverTMUCombineMode(
		int tmu, 
		const Combine &combine,
		bool unconditional = false
	);

private:

	struct Array
	{
		int size;
		int type;
		int stride;
		const void *data;
	};

	struct TMU
	{
		Combine combine;
		Array arrays[NumGLTMUArrays];
		GLTexture *tex;
		int state;
		int target;
	};

	struct State
	{
		TMU tmus[NumGLTMUs];
		Array arrays[NumGLArrays];
		int state;
		int blend;
	};

	State m_s;
	static State s_ds;
};

//
// Free functions
//

void R_glSetActiveTexture(int texture);
void R_glUploadTexture(
	int target, 
	int width, 
	int height, 
	int depth, 
	int type, 
	int components, 
	int flags, 
	void* data, 
	int dataLen, 
	void **mips=0, 
	int *mipSizes=0, 
	int numMips=0
);

#include <Runtime/PopSystemMacros.h>
