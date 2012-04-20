// GLState.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// OGL state management.
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace r {

inline GLState::S *GLState::X::operator -> () 
{ 
#if defined(RAD_OPT_IOS)
	return s.get();
#else
	return s.get()->get();
#endif
};

inline GLState::S *GLState::X::operator -> () const
{ 
#if defined(RAD_OPT_IOS)
	return s.get();
#else
	return s.get()->get();
#endif
};

inline GLState::Ref &GLState::X::operator = (const Ref &ref)
{
#if defined(RAD_OPT_IOS)
	s = ref;
	return s;
#else
	if (!s.get())
	{
		s.reset(new Ref());
	}
	*s.get() = ref;
	return *s.get();
#endif
}

inline GLState::X::operator GLState::X::unspecified_bool_type () const
{
#if defined(RAD_OPT_IOS)
	return s.get() ? &X::z : 0;
#else
	return *s.get() ? &X::z : 0;
#endif
}

inline GLState::S *GLState::X::get()
{
#if defined(RAD_OPT_IOS)
	return s.get();
#else
	if (!s.get())
	{
		s.reset(new Ref());
	}
	return s.get()->get();
#endif
}

inline GLState::T::T() :
s(TEM_Modulate)
{
}

inline GLState::_S::_S() :
b(0),
s(0),
aref(0.f),
p(0),
invertCullFace(false)
{
}

inline GLState::AA::AA() :
e(false),
size(0),
type(0),
normalized(GL_FALSE),
stride(0),
ofs(0)
{
}

inline bool GLState::AA::operator == (const AA &aa) const
{
	return vb.get() == aa.vb.get() &&
		size == aa.size &&
		type == aa.type &&
		normalized == aa.normalized &&
		stride == aa.stride &&
		ofs == aa.ofs;
}

inline bool GLState::AA::operator != (const AA &aa) const
{
	return !(*this == aa);
}

inline GLState::S::S() :
t(0)
#if !defined(RAD_OPT_OGLES)
, tc(0)
#endif
{
	bb[0] = bb[1] = bb[2] = bb[3] = 0;
}

inline void GLState::S::SetActiveTexture(int _t, bool force)
{
	if (_t != t || force)
	{
		t = _t;
		gl.SetActiveTexture(_t);
	}
}

inline void GLState::S::SetActiveTexCoord(int _t, bool force)
{
#if defined(RAD_OPT_OGLES)
	SetActiveTexture(_t, force);
#else
	if (_t != tc || force)
	{
		tc = _t;
		gl.SetActiveTexCoord(_t);
	}
#endif
}

inline void GLState::S::BindBuffer(GLenum target, GLuint id, bool force)
{
	switch (target)
	{
	case GL_FRAMEBUFFER_EXT:
		if (id != bb[0] || force)
		{
			bb[0] = id;
			gl.BindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);
			CHECK_GL_ERRORS();
		} break;
	case GL_RENDERBUFFER_EXT:
		if (id != bb[1] || force)
		{
			bb[1] = id;
			gl.BindRenderbufferEXT(GL_RENDERBUFFER_EXT, id);
			CHECK_GL_ERRORS();
		} break;
	}
}

inline GLVertexBufferRef GLState::S::VertexBufferBinding(GLenum target) const
{
	GLVertexBufferRef vb;

	switch (target)
	{
	case GL_ARRAY_BUFFER_ARB:
		vb = vbb[0];
		break;
	case GL_ELEMENT_ARRAY_BUFFER_ARB:
		vb = vbb[1];
		break;
	}

	return vb;
}

inline GLVertexArrayRef GLState::S::VertexArrayBinding() const
{
	return vao;
}

///////////////////////////////////////////////////////////////////////////////

inline GLState::GLState()
{
}

inline GLState::Ref GLState::New(bool init, bool clone)
{
	Ref r(new (ZRender) S());

	if (clone && m_s)
	{
		r->s = m_s->s; // copy active state.
	}

	if (init)
	{
		Init(*r.get());
	}

	return r;
}

inline void GLState::Bind(const Ref &s)
{
	m_s = s;
}

inline void GLState::Set(int s, int b, bool immediate)
{
	RAD_ASSERT(m_s);
	if (s != -1) 
		m_s->s.s = s;
	if (b != -1) 
		m_s->s.b = b;

	if (immediate && (s != -1 || b != -1))
		CommitSB(*m_s.get(), false);
}

inline void GLState::DisableTexture(int i)
{
	RAD_ASSERT(i < gl.maxTextures && i < MaxTextures && i >= 0);
	RAD_ASSERT(m_s);	
	m_s->s.t[i].tex.reset();
}

inline void GLState::DisableTextures()
{
	for (int i = 0; i < MaxTextures && i < gl.maxTextures; ++i)
	{
		DisableTexture(i);
	}
}

inline void GLState::SetTexture(int i, const GLTextureRef &tex, bool immediate, bool force)
{
	RAD_ASSERT(i < gl.maxTextures && i < MaxTextures && i >= 0);
	RAD_ASSERT(m_s);
	m_s->s.t[i].tex = tex;
	
	if (immediate)
		CommitT(*m_s.get(), i, m_s->s.t[i], m_s->d.t[i], force);
}

inline void GLState::SetTextureState(int i, int state, bool immediate, bool force)
{
	RAD_ASSERT(i < gl.maxTextures && i < MaxTextures && i >= 0);
	RAD_ASSERT(m_s);
	m_s->s.t[i].s = state;
	if (immediate)
		CommitT(*m_s.get(), i, m_s->s.t[i], m_s->d.t[i], force);
}

inline void GLState::DisableVertexAttribArrays(bool immediate, bool force)
{
	for (int i = 0; i < gl.maxVertexAttribs && i < MaxAttribArrays; ++i)
	{
		// Release VB's
		VertexAttribPointer(
			i,
			GLVertexBufferRef(),
			0,
			0,
			GL_FALSE,
			0,
			0,
			false,
			false
		);
		EnableVertexAttribArray(i, false, immediate, force);
	}
}

inline void GLState::EnableVertexAttribArray(int i, bool enable, bool immediate, bool force)
{
	RAD_ASSERT(i < gl.maxVertexAttribs && i < MaxAttribArrays && i >= 0);
	RAD_ASSERT(m_s);
	m_s->s.aa[i].e = enable;
	if (immediate)
		CommitAA(*m_s.get(), i, m_s->s.aa[i], m_s->d.aa[i], force);
}

inline void GLState::VertexAttribPointer(
	int i,
	const GLVertexBufferRef &vb,
	GLint size,
	GLenum type,
	GLboolean normalized,
	GLsizei stride,
	GLuint ofs,
	bool immediate,
	bool force
)
{
	RAD_ASSERT(i < gl.maxVertexAttribs && i < MaxAttribArrays && i >= 0);
	RAD_ASSERT(m_s);
	AA &sa = m_s->s.aa[i];
	sa.vb = vb;
	sa.size = size;
	sa.type = type;
	sa.normalized = normalized;
	sa.stride = stride;
	sa.ofs = ofs;
	if (force || (immediate && sa.e))
		CommitAA(*m_s.get(), i, sa, m_s->d.aa[i], force);
}

inline void GLState::Scissor(GLint x, GLint y, GLsizei w, GLsizei h, bool immediate, bool force)
{
	RAD_ASSERT(m_s);
	m_s->s.scissor[0] = x;
	m_s->s.scissor[1] = y;
	m_s->s.scissor[2] = w;
	m_s->s.scissor[3] = h;

	if (immediate)
		CommitScissor(*m_s.get(), force);
}

inline void GLState::UseProgram(GLhandleARB p, bool immediate, bool force)
{
	m_s->s.p = p;
	if (force || (immediate && m_s->d.p != p))
	{
		m_s->d.p = p;
		gl.UseProgramObjectARB(p);
	}
}

inline GLhandleARB GLState::Program() const
{
	return m_s->s.p;
}

inline void GLState::Commit(bool unconditional)
{
	RAD_ASSERT(m_s);
	Commit(*m_s.get(), unconditional);
}

inline void GLState::SetActiveTexture(int t, bool force)
{
	m_s->SetActiveTexture(t, force);
}

inline void GLState::SetActiveTexCoord(int t, bool force)
{
	m_s->SetActiveTexCoord(t, force);
}

inline void GLState::BindBuffer(GLenum target, GLuint id, bool force)
{
	m_s->BindBuffer(target, id, force);
}

inline void GLState::BindBuffer(GLenum target, const GLVertexBufferRef &vb, bool force)
{
	m_s->BindBuffer(target, vb, force);
}

inline void GLState::BindVertexArray(const GLVertexArrayRef &va, bool force)
{
	m_s->BindVertexArray(va, force);
}

inline GLVertexBufferRef GLState::VertexBufferBinding(GLenum target) const
{
	return m_s->VertexBufferBinding(target);
}

inline GLVertexArrayRef GLState::VertexArrayBinding() const
{
	return m_s->VertexArrayBinding();
}

inline void GLState::SetMTSource(r::MTSource id, int index, const GLTextureRef &tex)
{
	RAD_ASSERT(id >= 0 && id < MTS_Max);
	RAD_ASSERT(index >= 0 && index < MTS_MaxIndices);
	m_s->mts[id].t[index] = tex;
}

inline void GLState::DisableMTSource(r::MTSource id, int index)
{
	SetMTSource(id, index, GLTextureRef());
}

inline void GLState::DisableAllMTSources()
{
	for (int s = r::MTS_First; s < r::MTS_Max; ++s)
		for (int i = 0; i < r::MTS_MaxIndices; ++i)
			DisableMTSource((r::MTSource)s, i);
}

inline void GLState::AlphaRef(GLclampf f)
{
	m_s->s.aref = f;
}

inline GLTextureRef GLState::MTSource(r::MTSource id, int index) const
{
	RAD_ASSERT(id >= 0 && id < MTS_Max);
	RAD_ASSERT(index >= 0 && index < MTS_MaxIndices);
	return m_s->mts[id].t[index];
}

inline void GLState::SetMGSource(
	r::MGSource id, 
	int index,
	const GLVertexBufferRef &vb,
	GLint size,
	GLenum type,
	GLboolean normalized,
	GLsizei stride,
	GLuint ofs
)
{
	RAD_ASSERT(id >= 0 && id < MGS_Max);
	RAD_ASSERT(index >= 0 && index < MGS_MaxIndices);
	MGS::G &mgs = m_s->mgs[id].g[index];
	mgs.vb = vb;
	mgs.size = size;
	mgs.type = type;
	mgs.normalized = normalized;
	mgs.stride = stride;
	mgs.ofs = ofs;
}

inline void GLState::MGSource(
	r::MGSource id,
	int index,
	GLVertexBufferRef &vb,
	GLint &size,
	GLenum &type,
	GLboolean &normalized,
	GLsizei &stride,
	GLuint &ofs
)
{
	RAD_ASSERT(id >= 0 && id < MGS_Max);
	RAD_ASSERT(index >= 0 && index < MGS_MaxIndices);
	const MGS::G &mgs = m_s->mgs[id].g[index];
	vb = mgs.vb;
	size = mgs.size;
	type = mgs.type;
	normalized = mgs.normalized;
	stride = mgs.stride;
	ofs = mgs.ofs;
}

inline void GLState::DisableMGSource(r::MGSource id, int index)
{
	SetMGSource(
		id,
		index,
		GLVertexBufferRef(),
		0,
		0,
		GL_FALSE,
		0,
		0
	);
}

inline void GLState::DisableAllMGSources()
{
	for (int s = r::MGS_First; s < r::MGS_Max; ++s)
		for (int i = 0; i < r::MGS_MaxIndices; ++i)
			DisableMGSource((r::MGSource)s, i);
}

inline bool GLState::RAD_IMPLEMENT_GET(invertCullFace)
{
	return m_s->s.invertCullFace;
}

inline void GLState::RAD_IMPLEMENT_SET(invertCullFace)(bool invert)
{
	m_s->s.invertCullFace = invert;
}

} // r
