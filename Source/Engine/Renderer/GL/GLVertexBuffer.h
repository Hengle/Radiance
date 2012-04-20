// GLVertexBuffer.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../RendererDef.h"
#include "GLState.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

#if defined(RAD_OPT_IOS)
//#define DONT_FREE_VBOs_HACK_FIX_GLES_DRIVER_BUG
// THIS BUG APPEARS TO HAVE RESOLVED ITSELF SO I'M
// DISABLING THIS FOR NOW

// This deserves some explanation.
// I am pretty sure I have discovered an unfortunate
// iOS GLES driver bug which manifests itself in Crow
// like so:
//
// Launch Overworld02, runs great.
// Kill app.
// Launch OmenLevel03, run great.
// Kill app.
// All works great.
//
// BUG:
// Launch Overworld02, progress to OmenLevel03.
// OmenLevel03 loads and runs at about 50% of normal speed.
// XCode Instruments shows that we are spending almost 100%
// frame time inside gleRunVertexSubmitARM() which means we
// have fallen back to a software CPU->GPU vertex copy.
//
// I am nearly positive (though would like to be proven wrong)
// that this is a driver bug, because commenting out glDeleteBuffers()
// and therefore leaking VBOs from Overworld02 (or any other level)
// eliminates this issue. I am confident this is a driver issue
// because the VBOs between level loads are *never* shared and
// are all generated fresh via glGenBuffers().
//
// Since actually deleting the VBOs causes this issue, on iOS
// I recycle them (instead of deleting the id's are put in a big
// list and reused before another glGenBuffers() is called. YES
// this is a high-water mark mechanism in terms of number of VBOs
// allocated and is undefined in terms of how much memory they
// may consume but in practice each VBO is typically small and
// so the waste is sufficiently low to not egregiously impact
// memory footprint.

#endif

namespace r {

class GLVertexBuffer : public boost::enable_shared_from_this<GLVertexBuffer>
{
public:
	typedef GLVertexBufferRef Ref;
	typedef zone_vector<Ref, ZVertexBuffersT>::type Vec;

	class Ptr
	{
	public:
		typedef boost::shared_ptr<Ptr> Ref;
		RAD_DECLARE_READONLY_PROPERTY(Ptr, size, AddrSize);
		RAD_DECLARE_READONLY_PROPERTY(Ptr, ptr, void*);
	private:
		friend class GLVertexBuffer;
		friend class GLState;
		friend struct GLTable;
		RAD_DECLARE_GET(size, AddrSize);
		RAD_DECLARE_GET(ptr, void*);
		static void Delete(Ptr *p);
		GLVertexBuffer::Ref m_b;
		void *m_p;
		AddrSize m_size;
	};
	
	GLVertexBuffer(GLenum target, GLenum usage, AddrSize size);
	~GLVertexBuffer();
	Ptr::Ref Map();

	RAD_DECLARE_READONLY_PROPERTY(GLVertexBuffer, id, GLuint);
	RAD_DECLARE_READONLY_PROPERTY(GLVertexBuffer, target, GLenum);
	RAD_DECLARE_READONLY_PROPERTY(GLVertexBuffer, usage, GLenum);
	RAD_DECLARE_READONLY_PROPERTY(GLVertexBuffer, size, AddrSize);

private:

	RAD_DECLARE_GET(id, GLuint);
	RAD_DECLARE_GET(target, GLenum);
	RAD_DECLARE_GET(usage, GLenum);
	RAD_DECLARE_GET(size, AddrSize);

	friend class Ptr;
	friend class GLState;
	friend struct GLTable;

	void Unmap(void *p) const;
	
#if defined(DONT_FREE_VBOs_HACK_FIX_GLES_DRIVER_BUG)
	typedef zone_vector<GLuint, ZVertexBuffersT>::type GLuintVec;

	static GLuintVec &VBONames()
	{
		static GLuintVec v;
		return v;
	}
	
	static GLuint AllocVBO()
	{
		GLuint name;
		
		if (VBONames().empty())
		{
			gl.GenBuffersARB(1, &name);
			CHECK_GL_ERRORS();
		}
		else
		{
			name = VBONames().back();
			VBONames().pop_back();
		}
		
		return name;
	}
	
	static void FreeVBO(GLuint name)
	{
		VBONames().push_back(name);
	}
#endif
	
	AddrSize m_size;
	GLuint m_id;
	GLenum m_target;
	GLenum m_usage;
	Ptr m_ptr;
};

} // r

#include <Runtime/PopPack.h>
#include "GLVertexBuffer.inl"
