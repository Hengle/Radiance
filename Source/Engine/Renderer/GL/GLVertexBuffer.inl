// GLVertexBuffer.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace r {

inline GLVertexBuffer::GLVertexBuffer(GLenum target, GLenum usage, AddrSize size)
: m_target(target), m_size(size), m_usage(usage)
{
	RAD_ASSERT(size);

	if (gl.vbos)
	{
#if defined(DONT_FREE_VBOs_HACK_FIX_GLES_DRIVER_BUG)
		m_id = AllocVBO();
#else
		gl.GenBuffersARB(1, &m_id);
		CHECK_GL_ERRORS();
#endif
		ZVertexBuffers.Get().Inc(size, 0);
	}
	else
	{
		m_id = 0;
		m_ptr.m_p = safe_zone_malloc(ZVertexBuffers, m_size);
	}
}

inline GLVertexBuffer::~GLVertexBuffer()
{
	if (gl.vbos)
	{
#if defined(DONT_FREE_VBOs_HACK_FIX_GLES_DRIVER_BUG)
		FreeVBO(m_id);
#else
		gl.DeleteBuffersARB(1, &m_id);
#endif
		ZVertexBuffers.Get().Dec(m_size, 0);
	}
	else
	{
		if (m_ptr.m_p)
			zone_free(m_ptr.m_p);
	}
}

inline GLVertexBuffer::Ptr::Ref GLVertexBuffer::Map()
{
	m_ptr.m_b = shared_from_this();

	if (gl.vbos)
	{
		RAD_ASSERT(m_id);
		gls.BindBuffer(m_target, m_ptr.m_b);
		CHECK_GL_ERRORS();
		gl.BufferDataARB(m_target, (GLuint)m_size, 0, m_usage);
		CHECK_GL_ERRORS();
		void *p = gl.MapBufferARB(m_target, GL_WRITE_ONLY_ARB);
		CHECK_GL_ERRORS();
		RAD_ASSERT(p);
		m_ptr.m_p = p;
	}

	m_ptr.m_size = m_size;

	return Ptr::Ref(&m_ptr, Ptr::Delete);
}

inline GLuint GLVertexBuffer::RAD_IMPLEMENT_GET(id)
{
	return m_id;
}

inline GLenum GLVertexBuffer::RAD_IMPLEMENT_GET(target)
{
	return m_target;
}

inline GLenum GLVertexBuffer::RAD_IMPLEMENT_GET(usage)
{
	return m_usage;
}

inline AddrSize GLVertexBuffer::RAD_IMPLEMENT_GET(size)
{
	return m_size;
}

inline void GLVertexBuffer::Unmap(void *p) const
{
	if (gl.vbos)
	{
		gls.BindBuffer(m_target, m_ptr.m_b);
		CHECK_GL_ERRORS();
		gl.UnmapBufferARB(m_target);
		CHECK_GL_ERRORS();
	}
}

inline void GLVertexBuffer::Ptr::Delete(Ptr *p)
{
	if (p->m_b)
		p->m_b->Unmap(p->m_p);
	p->m_size = 0;
	p->m_b.reset();

	if (gl.vbos)
		p->m_p = 0;
}

inline AddrSize GLVertexBuffer::Ptr::RAD_IMPLEMENT_GET(size)
{
	return m_size;
}

inline void *GLVertexBuffer::Ptr::RAD_IMPLEMENT_GET(ptr)
{
	return m_p;
}

} // r
