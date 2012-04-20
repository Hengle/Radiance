// GLState.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// OGL state management.
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../RendererDef.h"
#include "../Sources.h"
#include "GLTable.h"
#include "GLTextureDef.h"
#include "GLVertexBufferDef.h"
#include "GLVertexArrayDef.h"
#include <Runtime/Math/Matrix.h>
#include <boost/thread/tss.hpp>
#include <Runtime/PushPack.h>

namespace r {

enum
{
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

	// Depth Write Mask

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
	CWM_RGBA = (CWM_R|CWM_G|CWM_B|CWM_A),
	CWM_RGB = (CWM_R|CWM_G|CWM_B),
	CWM_Flags = (CWM_RGBA|CWM_Off),

	// Alpha Test

	AT_Disable = 0x100000,
	AT_Less = 0x200000,
	AT_Greater = 0x400000,
	AT_LEqual = 0x800000,
	AT_GEqual = 0x1000000,
	AT_Flags = (AT_Disable|AT_Less|AT_Greater|AT_LEqual|AT_GEqual),

	// Scissor Test
	SCT_Enable = 0x2000000,
	SCT_Disable = 0x4000000,
	SCT_Flags = (SCT_Enable|SCT_Disable),

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
	BMS_Flags = (BMS_One|BMS_DstColor|BMS_InvDstColor|BMS_SrcAlpha|BMS_InvSrcAlpha|
                 BMS_DstAlpha|BMS_InvDstAlpha|BMS_SrcAlphaSaturate|BMS_Zero),

	// Blend Mode Destination

	BMD_Zero = 0x40000,
	BMD_One = 0x80000,
	BMD_SrcColor = 0x100000,
	BMD_InvSrcColor = 0x200000,
	BMD_SrcAlpha = 0x400000,
	BMD_InvSrcAlpha = 0x800000,
	BMD_DstAlpha = 0x1000000,
	BMD_InvDstAlpha = 0x2000000,
	BMD_Flags = (BMD_Zero|BMD_One|BMD_SrcColor|BMD_InvSrcColor|BMD_SrcAlpha|BMD_InvSrcAlpha|
                 BMD_DstAlpha|BMD_InvDstAlpha),
	
    BM_Off = (BMS_One|BMD_Zero),
	BM_Flags = (BMS_Flags|BMD_Flags),

	//
	// TexUnit States
	//

	// Texture Environment Mode

	TEM_Modulate = 0x1,
	TEM_Decal = 0x2,
	TEM_Blend = 0x4,
	TEM_Replace = 0x8,
	TEM_Combine = 0x10,
	TEM_Flags = (TEM_Modulate|TEM_Decal|TEM_Blend|TEM_Replace|TEM_Combine),

	InvalidMapping = 255
};

class GLState
{
public:

	enum 
	{
#if defined(RAD_OPT_IOS)
		MaxTextures = 6,
		MaxAttribArrays = 8,
#else
		MaxIOSTextures = 6,
		MaxIOSAttribArrays = 8,
		MaxTextures = 6,
		MaxAttribArrays = 8,
#endif
		NumSkinArrays = 0 // weights/indexes
	};

	GLState();

	// General state management

	struct S;
	typedef boost::shared_ptr<S> Ref;
	Ref New(bool init, bool clone = false);
	void Bind(const Ref &s);
	void Commit(bool unconditional = false);
	void Set(int s, int b, bool immediate=false);
	void AlphaRef(GLclampf f);
	void DisableTextures();
	void DisableTexture(int num);
	void SetTexture(int num, const GLTextureRef &tex, bool immediate=false, bool force=false);
	void SetTextureState(int num, int state, bool immediate=false, bool force=false);
	
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

	void SetActiveTexture(int t, bool force=false);
	void SetActiveTexCoord(int t, bool force=false);
	void BindBuffer(GLenum target, GLuint id, bool force=false);
	void BindBuffer(GLenum target, const GLVertexBufferRef &vb, bool force=false);
	void BindVertexArray(const GLVertexArrayRef &va, bool force=false);
	GLVertexBufferRef VertexBufferBinding(GLenum target) const;
	GLVertexArrayRef VertexArrayBinding() const;

	// Shader States
	// These are not GL states and are not cached for commit.

	void SetMTSource(r::MTSource id, int index, const GLTextureRef &tex);
	void DisableMTSource(r::MTSource id, int index);
	void DisableAllMTSources();
	GLTextureRef MTSource(r::MTSource id, int index) const;

	void SetMGSource(
		r::MGSource id, 
		int index,
		const GLVertexBufferRef &vb,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		GLuint ofs
	);

	void MGSource(
		r::MGSource id,
		int index,
		GLVertexBufferRef &vb,
		GLint &size,
		GLenum &type,
		GLboolean &normalized,
		GLsizei &stride,
		GLuint &ofs
	);

	void DisableMGSource(r::MGSource id, int index);
	void DisableAllMGSources();

	RAD_DECLARE_PROPERTY(GLState, invertCullFace, bool, bool);

	struct MInputMappings
	{ // material input mappings
		U8 numTexs;
		U8 numAttrs;
		U8 numMTSources[MTS_Max];
		U8 numMGSources[MGS_Max];
		U8 textures[MaxTextures][2];
		U8 attributes[MaxAttribArrays][3];
	};

private:

	struct T
	{
		T();
		int s;
		GLTextureRef tex;
	};

	struct AA
	{
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

	struct MTS
	{
		GLTextureRef t[MTS_MaxIndices];
	};

	struct MGS
	{
		struct G
		{
			GLVertexBufferRef vb;
			GLint size;
			GLenum type;
			GLboolean normalized;
			GLsizei stride;
			GLuint ofs;
		};

		G g[MGS_MaxIndices];
	};

	struct _S
	{
		_S();
		T  t[MaxTextures];
		AA aa[MaxAttribArrays];
		int b;
		int s;
		GLclampf aref;
		GLhandleARB p;
		bool invertCullFace;
		int scissor[4];
	};

public:

	struct S
	{
	private:
		friend class GLState;
		S();
		MTS mts[MTS_Max];
		MGS mgs[MGS_Max];
		_S s;
		_S d;
		int t;
		GLuint bb[2];
		GLVertexBufferRef vbb[2];
		GLVertexArrayRef vao;
		bool vaoBound;
#if !defined(RAD_OPT_OGLES)
		int tc;
#endif

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

	RAD_DECLARE_GET(invertCullFace, bool);
	RAD_DECLARE_SET(invertCullFace, bool);

	struct X
	{
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

