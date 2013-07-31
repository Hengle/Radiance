/*! \file GLState.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#pragma once

#include "../RendererDef.h"
#include "../Common.h"
#include "GLTable.h"
#include "GLTextureDef.h"
#include "GLVertexBufferDef.h"
#include "GLVertexArrayDef.h"
#include <Runtime/Math/Matrix.h>
#include <boost/thread/tss.hpp>
#include <Runtime/PushPack.h>

namespace r {

enum {
	// Depth Test

	kDepthTest_Disable = 0x1,
	kDepthTest_Always = 0x2,
	kDepthTest_Less = 0x4,
	kDepthTest_Greater = 0x8,
	kDepthTest_LEqual = 0x10,
	kDepthTest_GEqual = 0x20,
	kDepthTest_Equal = 0x40,
	kDepthTest_Never = 0x80,
	kDepthTest_Flags = 
		kDepthTest_Disable|kDepthTest_Always|
		kDepthTest_Less|kDepthTest_Greater|
		kDepthTest_LEqual|kDepthTest_GEqual|
		kDepthTest_Equal|kDepthTest_Never,

	// Depth Write Mask

	kDepthWriteMask_Enable = 0x100,
	kDepthWriteMask_Disable = 0x200,
	kDepthWriteMask_Flags = kDepthWriteMask_Enable|kDepthWriteMask_Disable,

	// Cull Face Mode

	kCullFaceMode_Front = 0x400,
	kCullFaceMode_Back = 0x800,
	kCullFaceMode_None = 0x1000,
	kCullFaceMode_CW = 0x2000,
	kCullFaceMode_CCW = 0x4000,
	kCullFaceMode_Flags = kCullFaceMode_Front|kCullFaceMode_Back|kCullFaceMode_None|kCullFaceMode_CW|kCullFaceMode_CCW,
	kCullFaceMode_ModeFlags = kCullFaceMode_Front|kCullFaceMode_Back|kCullFaceMode_None,
	kCullFaceMode_DirFlags = kCullFaceMode_CW|kCullFaceMode_CCW,

	// Color Write Mask

	kColorWriteMask_R = 0x8000,
	kColorWriteMask_G = 0x10000,
	kColorWriteMask_B = 0x20000,
	kColorWriteMask_A = 0x40000,
	kColorWriteMask_Off = 0x80000,
	kColorWriteMask_RGBA = (kColorWriteMask_R|kColorWriteMask_G|kColorWriteMask_B|kColorWriteMask_A),
	kColorWriteMask_RGB = (kColorWriteMask_R|kColorWriteMask_G|kColorWriteMask_B),
	kColorWriteMask_Flags = (kColorWriteMask_RGBA|kColorWriteMask_Off),

	// Scissor Test
	kScissorTest_Enable = 0x2000000,
	kScissorTest_Disable = 0x4000000,
	kScissorTest_Flags = kScissorTest_Enable|kScissorTest_Disable,

	// Stencil Test
	kStencilTest_Enable = 0x8000000,
	kStencilTest_Disable = 0x10000000,
	kStencilTest_Flags = kStencilTest_Enable|kStencilTest_Disable,

	// Blend Mode Source

	kBlendModeSource_One = 0x20,
	kBlendModeSource_DstColor = 0x40,
	kBlendModeSource_InvDstColor = 0x80,
	kBlendModeSource_SrcAlpha = 0x100,
	kBlendModeSource_InvSrcAlpha = 0x200,
	kBlendModeSource_DstAlpha = 0x400,
	kBlendModeSource_InvDstAlpha = 0x800,
	kBlendModeSource_SrcAlphaSaturate = 0x1000,
	kBlendModeSource_Zero = 0x2000,
	kBlendModeSource_Flags = kBlendModeSource_One|kBlendModeSource_DstColor|kBlendModeSource_InvDstColor|
		kBlendModeSource_SrcAlpha|kBlendModeSource_InvSrcAlpha|
		kBlendModeSource_DstAlpha|kBlendModeSource_InvDstAlpha|
		kBlendModeSource_SrcAlphaSaturate|kBlendModeSource_Zero,

	// Blend Mode Destination

	kBlendModeDest_Zero = 0x40000,
	kBlendModeDest_One = 0x80000,
	kBlendModeDest_SrcColor = 0x100000,
	kBlendModeDest_InvSrcColor = 0x200000,
	kBlendModeDest_SrcAlpha = 0x400000,
	kBlendModeDest_InvSrcAlpha = 0x800000,
	kBlendModeDest_DstAlpha = 0x1000000,
	kBlendModeDest_InvDstAlpha = 0x2000000,
	kBlendModeDest_Flags = kBlendModeDest_Zero|kBlendModeDest_One|kBlendModeDest_SrcColor|kBlendModeDest_InvSrcColor|
		kBlendModeDest_SrcAlpha|kBlendModeDest_InvSrcAlpha|
		kBlendModeDest_DstAlpha|kBlendModeDest_InvDstAlpha,
	
    kBlendMode_Off = (kBlendModeSource_One|kBlendModeDest_Zero),
	kBlendMode_Flags = (kBlendModeSource_Flags|kBlendModeDest_Flags)
};

class GLState {
public:

	GLState();

	// OpenGL state management

	struct S;
	typedef boost::shared_ptr<S> Ref;
	Ref New(bool init, bool clone = false);
	void Bind(const Ref &s);
	Ref GetRef();

	void Commit(bool unconditional = false);
	void Set(int s, int b, bool immediate=false);
	void StencilOp(GLenum fail, GLenum zfail, GLenum zpass, bool immediate=false);
	void StencilMask(GLuint mask, bool immediate=false);
	void StencilFunc(GLenum func, GLint ref, GLuint mask, bool immediate=false);
	void AlphaRef(GLclampf f);
	void DisableTextures();
	void DisableTexture(int num);
	void SetTexture(int num, const GLTextureRef &tex, bool immediate=false, bool force=false);
	
	void DisableVertexAttribArrays(bool immediate=false, bool force=false);
	void EnableVertexAttribArray(int num, bool enable, bool immediate=false, bool force=false);
	void VertexAttribPointer(
		int num,
		const GLVertexBufferRef &vb,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		GLuint ofs, 
		bool immediate=false,
		bool force=false
	);

	void Scissor(GLint x, GLint y, GLsizei w, GLsizei h, bool immediate=false, bool force=false);

	void UseProgram(GLhandleARB p, bool immediate=false, bool force=false);
	GLhandleARB Program() const;

	// these take effect immediately
	// i.e. they do not require a Commit()

	void Viewport(GLint x, GLint y, GLsizei width, GLsizei height, bool force=false);
	void SetActiveTexture(int t, bool force=false);
	void SetActiveTexCoord(int t, bool force=false);
	void BindBuffer(GLenum target, GLuint id, bool force=false);
	void BindBuffer(GLenum target, const GLVertexBufferRef &vb, bool force=false);
	void BindVertexArray(const GLVertexArrayRef &va, bool force=false);
	GLVertexBufferRef VertexBufferBinding(GLenum target) const;
	GLVertexArrayRef VertexArrayBinding() const;

	// Shader States
	// These are not GL states and are not cached for commit.

	void SetMTSource(r::MaterialTextureSource id, int index, const GLTextureRef &tex);
	void DisableMTSource(r::MaterialTextureSource id, int index);
	void DisableAllMTSources();
	GLTextureRef MaterialTextureSource(r::MaterialTextureSource id, int index) const;

	void SetMGSource(
		r::MaterialGeometrySource id, 
		int index,
		const GLVertexBufferRef &vb,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		GLuint ofs
	);

	void MaterialGeometrySource(
		r::MaterialGeometrySource id,
		int index,
		GLVertexBufferRef &vb,
		GLint &size,
		GLenum &type,
		GLboolean &normalized,
		GLsizei &stride,
		GLuint &ofs
	);

	void DisableMGSource(r::MaterialGeometrySource id, int index);
	void DisableAllMGSources();

	RAD_DECLARE_PROPERTY(GLState, invertCullFace, bool, bool);

private:

	struct T {
		T();
		GLTextureRef tex;
	};

	struct AA {
		AA();
		bool e;
		GLVertexBufferRef vb;
		GLint size;
		GLenum type;
		GLboolean normalized;
		GLsizei stride;
		GLuint ofs;

		bool operator == (const AA &aa) const;
		bool operator != (const AA &aa) const;
	};

	struct MTS {
		boost::array<GLTextureRef, kMaterialTextureSource_MaxIndices> t;
	};

	struct MGS {
		struct G {
			GLVertexBufferRef vb;
			GLint size;
			GLenum type;
			GLboolean normalized;
			GLsizei stride;
			GLuint ofs;
		};

		boost::array<G, kMaterialGeometrySource_MaxIndices> g;
	};

	struct Stencil {
		GLuint mask;
		GLenum func;
		GLenum funcRef;
		GLuint funcMask;
		GLenum opFail;
		GLenum opzFail;
		GLenum opzPass;
	};

	struct _S {
		_S();
		boost::array<T, kMaxTextures> t;
		boost::array<AA, kMaxAttribArrays> aa;
		int b;
		int s;
		GLclampf aref;
		GLhandleARB p;
		boost::array<int, 4> scissor;
		Stencil stencil;
		bool invertCullFace;
	};

public:

	struct S {
	private:
		friend class GLState;
		S();
		boost::array<MTS, kNumMaterialTextureSources> mts;
		boost::array<MGS, kNumMaterialGeometrySources> mgs;
		_S s;
		_S d;
		int t;
		boost::array<GLuint, 2> bb;
		boost::array<GLVertexBufferRef, 2> vbb;
		boost::array<int, 4> vp;
		GLVertexArrayRef vao;
		bool vaoBound;
#if !defined(RAD_OPT_OGLES)
		int tc;
#endif

		void Viewport(GLint x, GLint y, GLsizei width, GLsizei height, bool force=false);
		void SetActiveTexture(int t, bool force=false);
		void SetActiveTexCoord(int t, bool force=false);
		void BindBuffer(GLenum target, GLuint id, bool force=false);
		void BindBuffer(GLenum target, const GLVertexBufferRef &vb, bool force=false);
		void BindVertexArray(const GLVertexArrayRef &va, bool force=false);
		GLVertexBufferRef VertexBufferBinding(GLenum target) const;
		GLVertexArrayRef VertexArrayBinding() const;
	};

private:

	void Init(S &s);
	void Commit(S &s, bool f);
	void CommitSB(S &s, bool f);
	void CommitT(S &st, int t, T &s, T &d, bool f);
	void CommitAA(S &s, int i, AA &sa, AA &da, bool f);
	void CommitScissor(S &s, bool f);
	void CommitStencil(S &s, bool f);

	RAD_DECLARE_GET(invertCullFace, bool);
	RAD_DECLARE_SET(invertCullFace, bool);

	struct X {
#if defined(RAD_OPT_IOS)
		static Ref s;
#else
		boost::thread_specific_ptr<Ref> s;
#endif
		S *operator -> ();
		S *operator -> () const;
		Ref &operator = (const Ref &ref);
		typedef void (X::*unspecified_bool_type) ();
		operator unspecified_bool_type () const;
		S *get();
		void z() {}
	};

	X m_s;
};

extern RADENG_API GLState gls;

} // r

#include <Runtime/PopPack.h>
#include "GLState.inl"

