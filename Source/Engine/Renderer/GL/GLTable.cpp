// GLTable.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GLTable.h"
#include "GLState.h"
#include "GLVertexBuffer.h"

#undef MessageBox

#if defined(RAD_OPT_WIN)
#define GL_GetProcAddress wglGetProcAddress
#elif defined(RAD_OPT_OSX)
#include <mach-o/dyld.h>
#include <OpenGL/OpenGL.h>
#define GL_GetProcAddress aglGetProcAddress
namespace {
void *aglGetProcAddress(const char *name)
{
	char x[1024];
	strcpy(x, "_");
	strcat(x, name);
	if (!NSIsSymbolNameDefined(x))
	{
		return 0;
	}
	NSSymbol sym = NSLookupAndBindSymbol(x);
	if (!sym)
	{
		return 0;
	}
	return NSAddressOfSymbol(sym);
}
}
#elif defined(RAD_OPT_LINUX)
#include <GL/glx.h>
#define GL_GetProcAddress(_x) glXGetProcAddress((const GLubyte*)(_x))
#elif defined(RAD_OPT_IOS)
bool __IOS_IPhone();
bool __IOS_IPhone4();
bool __IOS_IPad();
#define GL_TEXTURE0_ARB GL_TEXTURE0
#else
#error RAD_ERROR_UNSUP_PLAT
#endif

#if defined(RAD_OPT_TOOLS)
#include "../../../../../Extern/glsl-optimizer/src/glsl/glsl_optimizer.h"
#endif

namespace r {

#define L(_type, _name) \
	_name = (_type)GL_GetProcAddress("gl"#_name); \
	*b = *b && (_name != 0)

#define CHECK(_ext) _ext = CheckExt("GL_"#_ext)
	
#define CHECK_EXT(_ext, _name) _ext = CheckExt("GL_"#_name)

#define BEGIN_NULL { bool _x = true; bool *b = &_x;

#define BEGIN(_ext) \
	CHECK(_ext); \
	if (_ext) { bool *b = &_ext;
	
#define BEGIN_EXT(_ext, _name) \
	CHECK_EXT(_ext, _name); \
	if (_ext) { bool *b = &_ext;

#define END *b = *b; }

RADENG_API GLTable gl;

GLTable::GLTable() {
#if defined(RAD_OPT_PC_TOOLS)
	cgc = 0;
#endif
#if defined(RAD_OPT_TOOLS)
	glslopt = 0;
	glslopt_es = 0;
#endif
	Reset();
}

void GLTable::DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
	if (mode == GL_TRIANGLES)
		numTris += count / 3;

	if (vbos) {
		if (wireframe && mode == GL_TRIANGLES) {
			const U8 *ofs = reinterpret_cast<const U8*>(indices);
			int size = (type == GL_UNSIGNED_BYTE) ? 3 : (type == GL_UNSIGNED_SHORT) ? 6 : 12;

			// lines
			for (GLsizei i = 0; i <= count-3; i += 3, ofs += size)
				glDrawElements(GL_LINE_LOOP, 3, type, ofs);
		} else {
			glDrawElements(mode, count, type, indices);
		}
	} else {
		GLVertexBuffer::Ref ib = gls.VertexBufferBinding(GL_ELEMENT_ARRAY_BUFFER_ARB);
		if (ib)
			indices = (const GLvoid*)(static_cast<U8*>(ib->m_ptr.m_p)+reinterpret_cast<AddrSize>(indices));		
		RAD_ASSERT(indices);

		if (wireframe && mode == GL_TRIANGLES) {
			const U8 *ofs = reinterpret_cast<const U8*>(indices);
			int size = (type == GL_UNSIGNED_BYTE) ? 3 : (type == GL_UNSIGNED_SHORT) ? 6 : 12;

			// lines
			for (GLsizei i = 0; i <= count-3; i += 3, ofs += size)
				glDrawElements(GL_LINE_STRIP, 3, type, ofs);
		} else {
			glDrawElements(mode, count, type, indices);
		}
	}
}

void GLTable::DrawArrays(GLenum mode, GLint first, GLsizei count) {
	if (mode == GL_TRIANGLES)
		numTris += count / 3;

	if (wireframe && mode == GL_TRIANGLES) {
		// lines
		for (GLint i = 0; i <= count-3; i += 3)
			glDrawArrays(GL_LINE_LOOP, i+first, 3);
	} else {
		glDrawArrays(mode, first, count);
	}
}

void GLTable::Reset() {
#if defined(RAD_OPT_PC_TOOLS)
	if (cgc) {
		cgDestroyContext(cgc);
		cgc = 0;
	}
#endif

	vMin = vMaj = 0;
	maxTextures = 0;
	maxTextureSize = 0;
	maxVertexAttribs = 0;

	SGIS_generate_mipmap = false;
	ARB_texture_non_power_of_two = false;
	
	color[0] = 1.f;
	color[1] = 1.f;
	color[2] = 1.f;
	color[3] = 1.f;
	
	mm = GL_PROJECTION;
	matrixOps = 0;
	prMatrixOps = 0;
	colorOps = 0;
	vbos = true;
#if defined(RAD_OPT_OSX)
	vaos = false; // not on OSX.
#else
	vaos = true;
#endif
	wireframe = false;
	numTris = 0;

#if defined(RAD_OPT_TOOLS)
	if (glslopt) {
		glslopt_cleanup(glslopt);
		glslopt = 0;
	}
	if (glslopt_es) {
		glslopt_cleanup(glslopt_es);
		glslopt_es = 0;
	}
#endif

#if defined(RAD_OPT_OGLES)
#if defined(RAD_OPT_OGLES1_AND_2)
	ogles2 = false;
#endif
#else
	EXT_texture_compression_s3tc = false;
	ARB_multitexture = false;
	EXT_compiled_vertex_array = false;
	ARB_occlusion_query = false;
	ARB_vertex_program = false;
	ARB_fragment_program = false;
	ARB_texture_compression = false;
#endif
	
	ARB_vertex_buffer_object = false;
	ARB_vertex_array_object = false;
	EXT_framebuffer_object = false;
	EXT_framebuffer_multisample = false;
	EXT_texture_filter_anisotropic = false;
	maxAnisotropy = 0;
	EXT_swap_control = false;
	ARB_shader_objects = false;
	ARB_vertex_shader = false;
	ARB_fragment_shader = false;

	ActiveTextureARB = 0;
	BindBufferARB = 0;
	DeleteBuffersARB = 0;
	GenBuffersARB = 0;
	IsBufferARB = 0;
	BufferDataARB = 0;
	BufferSubDataARB = 0;
	GetBufferParameterivARB = 0;
	MapBufferARB = 0;
	UnmapBufferARB = 0;
	GetBufferPointervARB = 0;
	BindVertexArray = 0;
	DeleteVertexArrays = 0;
	GenVertexArrays = 0;
	IsRenderbufferEXT = 0;
	BindRenderbufferEXT = 0;
	DeleteRenderbuffersEXT = 0;
	GenRenderbuffersEXT = 0;
	RenderbufferStorageEXT = 0;
	GetRenderbufferParameterivEXT = 0;
	IsFramebufferEXT = 0;
	BindFramebufferEXT = 0;
	DeleteFramebuffersEXT = 0;
	GenFramebuffersEXT = 0;
	CheckFramebufferStatusEXT = 0;
	FramebufferTexture2DEXT = 0;
	FramebufferRenderbufferEXT = 0;
	GetFramebufferAttachmentParameterivEXT = 0;
	GenerateMipmapEXT = 0;
	RenderbufferStorageMultisampleEXT = 0;

	DeleteObjectARB = 0;
	DetachObjectARB = 0;
	CreateShaderObjectARB = 0;
	ShaderSourceARB = 0;
	CompileShaderARB = 0;
	CreateProgramObjectARB = 0;
	AttachObjectARB = 0;
	LinkProgramARB = 0;
	BindAttribLocationARB = 0;
	GetActiveAttribARB = 0;
	GetAttribLocationARB = 0;
	UseProgramObjectARB = 0;
	ValidateProgramARB = 0;
	Uniform1fARB = 0;
	Uniform2fARB = 0;
	Uniform3fARB = 0;
	Uniform4fARB = 0;
	Uniform1iARB = 0;
	Uniform2iARB = 0;
	Uniform3iARB = 0;
	Uniform4iARB = 0;
	Uniform1fvARB = 0;
	Uniform2fvARB = 0;
	Uniform3fvARB = 0;
	Uniform4fvARB = 0;
	Uniform1ivARB = 0;
	Uniform2ivARB = 0;
	Uniform3ivARB = 0;
	Uniform4ivARB = 0;
	UniformMatrix2fvARB = 0;
	UniformMatrix3fvARB = 0;
	UniformMatrix4fvARB = 0;
	GetInfoLogARB = 0;
	GetAttachedObjectsARB = 0;
	GetUniformLocationARB = 0;
	GetActiveUniformARB = 0;
	GetUniformfvARB = 0;
	GetUniformivARB = 0;
	GetShaderSourceARB = 0;
	GetObjectParameterivARB = 0;
	
	VertexAttrib1fARB = 0;
	VertexAttrib1fvARB = 0;
	VertexAttrib2fARB = 0;
	VertexAttrib2fvARB = 0;
	VertexAttrib3fARB = 0;
	VertexAttrib3fvARB = 0;
	VertexAttrib4fARB = 0;
	VertexAttrib4fvARB = 0;
	VertexAttribPointerARB = 0;
	EnableVertexAttribArrayARB = 0;
	DisableVertexAttribArrayARB = 0;

#if !defined(RAD_OPT_OGLES)
	FramebufferTexture1DEXT = 0;
	FramebufferTexture3DEXT = 0;
	GetBufferSubDataARB = 0;
	ClientActiveTextureARB = 0;
	MultiTexCoord1fARB = 0;
	MultiTexCoord1fvARB = 0;
	MultiTexCoord2fARB = 0;
	MultiTexCoord2fvARB = 0;
	LockArraysEXT = 0;
	UnlockArraysEXT = 0;
	TexImage3D = 0;
	TexSubImage3D = 0;
	GenQueriesARB = 0;
	DeleteQueriesARB = 0;
	IsQueryARB = 0;
	BeginQueryARB = 0;
	EndQueryARB = 0;
	GetQueryivARB = 0;
	GetQueryObjectivARB = 0;
	GetQueryObjectuivARB = 0;
	CompressedTexImage3DARB = 0;
	CompressedTexImage2DARB = 0;
	CompressedTexImage1DARB = 0;
	CompressedTexSubImage3DARB = 0;
	CompressedTexSubImage2DARB = 0;
	CompressedTexSubImage1DARB = 0;
	GetCompressedTexImageARB = 0;
	VertexAttrib1dARB = 0;
	VertexAttrib1dvARB = 0;
	VertexAttrib1sARB = 0;
	VertexAttrib1svARB = 0;
	VertexAttrib2dARB = 0;
	VertexAttrib2dvARB = 0;
	VertexAttrib2sARB = 0;
	VertexAttrib2svARB = 0;
	VertexAttrib3dARB = 0;
	VertexAttrib3dvARB = 0;
	VertexAttrib3sARB = 0;
	VertexAttrib3svARB = 0;
	VertexAttrib4NbvARB = 0;
	VertexAttrib4NivARB = 0;
	VertexAttrib4NsvARB = 0;
	VertexAttrib4NubARB = 0;
	VertexAttrib4NubvARB = 0;
	VertexAttrib4NuivARB = 0;
	VertexAttrib4NusvARB = 0;
	VertexAttrib4bvARB = 0;
	VertexAttrib4dARB = 0;
	VertexAttrib4dvARB = 0;
	VertexAttrib4ivARB = 0;
	VertexAttrib4sARB = 0;
	VertexAttrib4svARB = 0;
	VertexAttrib4ubvARB = 0;
	VertexAttrib4uivARB = 0;
	VertexAttrib4usvARB = 0;
	ProgramStringARB = 0;
	BindProgramARB = 0;
	DeleteProgramsARB = 0;
	GenProgramsARB = 0;
	ProgramEnvParameter4dARB = 0;
	ProgramEnvParameter4dvARB = 0;
	ProgramEnvParameter4fARB = 0;
	ProgramEnvParameter4fvARB = 0;
	ProgramLocalParameter4dARB = 0;
	ProgramLocalParameter4dvARB = 0;
	ProgramLocalParameter4fARB = 0;
	ProgramLocalParameter4fvARB = 0;
	GetProgramEnvParameterdvARB = 0;
	GetProgramEnvParameterfvARB = 0;
	GetProgramLocalParameterdvARB = 0;
	GetProgramLocalParameterfvARB = 0;
	GetProgramivARB = 0;
	GetProgramStringARB = 0;
	GetVertexAttribdvARB = 0;
	GetVertexAttribfvARB = 0;
	GetVertexAttribivARB = 0;
	GetVertexAttribPointervARB = 0;
	IsProgramARB = 0;
#endif
	
	SetSwapInterval = 0;
}

void GLTable::Load() {
	LoadGLVersion();

	v1_1 = CheckVer("1.1");
	v1_2 = CheckVer("1.2");
	v1_3 = CheckVer("1.3");
	v1_4 = CheckVer("1.4");
	v1_5 = CheckVer("1.5");
	v2_0 = CheckVer("2.0");

#if defined(RAD_OPT_PC_TOOLS)
	cgc = cgCreateContext();
#endif

#if defined(RAD_OPT_TOOLS)
	glslopt = glslopt_initialize(false);
	glslopt_es = glslopt_initialize(true);
#endif

	mm = GL_PROJECTION;
	matrixOps = 0;
	prMatrixOps = 0;
	colorOps = 0;
	wireframe = false;
	numTris = 0;

	color[0] = 1.f;
	color[1] = 1.f;
	color[2] = 1.f;
	color[3] = 1.f;

#if !defined(RAD_OPT_OGLES2)
	glMatrixMode(mm);
#endif

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	
#if defined(RAD_OPT_OGLES)
#if defined(RAD_OPT_OGLES1_AND_2)
	ogles2 = true;
#endif
	maxTextures = 1;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextures);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	
	CHECK(IMG_texture_compression_pvrtc);
	SGIS_generate_mipmap = true;
	ARB_texture_non_power_of_two = false;
	ActiveTextureARB = &glActiveTexture;
	ARB_vertex_buffer_object = true;
	BindBufferARB = &glBindBuffer;
	DeleteBuffersARB = &glDeleteBuffers;
	GenBuffersARB = &glGenBuffers;
	IsBufferARB = &glIsBuffer;
	BufferDataARB = &glBufferData;
	BufferSubDataARB = &glBufferSubData;
	GetBufferParameterivARB = &glGetBufferParameteriv;
	MapBufferARB = &glMapBufferOES;
	UnmapBufferARB = &glUnmapBufferOES;
	ARB_vertex_array_object = true;
	GetBufferPointervARB = &glGetBufferPointervOES;
	BindVertexArray = &glBindVertexArrayOES;
	DeleteVertexArrays = &glDeleteVertexArraysOES;
	GenVertexArrays = &glGenVertexArraysOES;
	EXT_framebuffer_object = true;
	IsRenderbufferEXT = &glIsRenderbuffer;
	BindRenderbufferEXT = &glBindRenderbuffer;
	DeleteRenderbuffersEXT = &glDeleteRenderbuffers;
	GenRenderbuffersEXT = &glGenRenderbuffers;
	RenderbufferStorageEXT = &glRenderbufferStorage;
	GetRenderbufferParameterivEXT = &glGetRenderbufferParameteriv;
	IsFramebufferEXT = &glIsFramebuffer;
	BindFramebufferEXT = &glBindFramebuffer;
	DeleteFramebuffersEXT = &glDeleteFramebuffers;
	GenFramebuffersEXT = &glGenFramebuffers;
	CheckFramebufferStatusEXT = &glCheckFramebufferStatus;
	FramebufferTexture2DEXT = &glFramebufferTexture2D;
	FramebufferRenderbufferEXT = &glFramebufferRenderbuffer;
	GetFramebufferAttachmentParameterivEXT = &glGetFramebufferAttachmentParameteriv;
	GenerateMipmapEXT = &glGenerateMipmap;
	VertexAttrib1fARB = &glVertexAttrib1f;
	VertexAttrib1fvARB = &glVertexAttrib1fv;
	VertexAttrib2fARB = &glVertexAttrib2f;
	VertexAttrib2fvARB = &glVertexAttrib2fv;
	VertexAttrib3fARB = &glVertexAttrib3f;
	VertexAttrib3fvARB = &glVertexAttrib3fv;
	VertexAttrib4fARB = &glVertexAttrib4f;
	VertexAttrib4fvARB = &glVertexAttrib4fv;
	VertexAttribPointerARB = &glVertexAttribPointer;
	EnableVertexAttribArrayARB = &glEnableVertexAttribArray;
	DisableVertexAttribArrayARB = &glDisableVertexAttribArray;

	ARB_shader_objects = true;
	ARB_vertex_shader = true;
	ARB_fragment_shader = true;
	DeleteObjectARB = &glDeleteShader;
	DetachObjectARB = &glDetachShader;
	CreateShaderObjectARB = &glCreateShader;
	ShaderSourceARB = &glShaderSource;
	CompileShaderARB = &glCompileShader;
	CreateProgramObjectARB = &glCreateProgram;
	AttachObjectARB = &glAttachShader;
	LinkProgramARB = &glLinkProgram;
	UseProgramObjectARB = &glUseProgram;
	ValidateProgramARB = &glValidateProgram;
	BindAttribLocationARB = &glBindAttribLocation;
	GetActiveAttribARB = &glGetActiveAttrib;
	GetAttribLocationARB = &glGetAttribLocation;
	Uniform1fARB = &glUniform1f;
	Uniform2fARB = &glUniform2f;
	Uniform3fARB = &glUniform3f;
	Uniform4fARB = &glUniform4f;
	Uniform1iARB = &glUniform1i;
	Uniform2iARB = &glUniform2i;
	Uniform3iARB = &glUniform3i;
	Uniform4iARB = &glUniform4i;
	Uniform1fvARB = &glUniform1fv;
	Uniform2fvARB = &glUniform2fv;
	Uniform3fvARB = &glUniform3fv;
	Uniform4fvARB = &glUniform4fv;
	Uniform1ivARB = &glUniform1iv;
	Uniform2ivARB = &glUniform2iv;
	Uniform3ivARB = &glUniform3iv;
	Uniform4ivARB = &glUniform4iv;
	UniformMatrix2fvARB = &glUniformMatrix2fv;
	UniformMatrix3fvARB = &glUniformMatrix3fv;
	UniformMatrix4fvARB = &glUniformMatrix4fv;
	GetInfoLogARB = &glGetShaderInfoLog;
	GetAttachedObjectsARB = &glGetAttachedShaders;
	GetUniformLocationARB = &glGetUniformLocation;
	GetActiveUniformARB = &glGetActiveUniform;
	GetUniformfvARB = &glGetUniformfv;
	GetUniformivARB = &glGetUniformiv;
	GetShaderSourceARB = &glGetShaderSource;
	GetObjectParameterivARB = &glGetShaderiv;
#else

	CHECK(EXT_texture_compression_s3tc);
	CHECK(SGIS_generate_mipmap);
	CHECK(ARB_texture_non_power_of_two);
	CHECK(EXT_texture_filter_anisotropic);

	maxAnisotropy = 0;
	if (EXT_texture_filter_anisotropic)
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

	maxTextures = 1;
	BEGIN(ARB_multitexture)
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextures);
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	END

	BEGIN_NULL
		L(PFNGLACTIVETEXTUREPROC, ActiveTextureARB);
		L(PFNGLCLIENTACTIVETEXTUREPROC, ClientActiveTextureARB);
		L(PFNGLTEXIMAGE3DPROC, TexImage3D);
		L(PFNGLTEXSUBIMAGE3DPROC, TexSubImage3D);
	END

	BEGIN(EXT_compiled_vertex_array)
		L(PFNGLLOCKARRAYSEXTPROC, LockArraysEXT);
		L(PFNGLUNLOCKARRAYSEXTPROC, UnlockArraysEXT);
	END

	BEGIN(ARB_occlusion_query)
		L(PFNGLGENQUERIESARBPROC, GenQueriesARB);
		L(PFNGLDELETEQUERIESARBPROC, DeleteQueriesARB);
		L(PFNGLISQUERYARBPROC, IsQueryARB);
		L(PFNGLBEGINQUERYARBPROC, BeginQueryARB);
		L(PFNGLENDQUERYARBPROC, EndQueryARB);
		L(PFNGLGETQUERYIVARBPROC, GetQueryivARB);
		L(PFNGLGETQUERYOBJECTIVARBPROC, GetQueryObjectivARB);
		L(PFNGLGETQUERYOBJECTUIVARBPROC, GetQueryObjectuivARB);
	END

	BEGIN(ARB_vertex_buffer_object)
		L(PFNGLBINDBUFFERARBPROC, BindBufferARB);
		L(PFNGLDELETEBUFFERSARBPROC, DeleteBuffersARB);
		L(PFNGLGENQUERIESARBPROC, GenBuffersARB);
		L(PFNGLISBUFFERARBPROC, IsBufferARB);
		L(PFNGLBUFFERDATAARBPROC, BufferDataARB);
		L(PFNGLBUFFERSUBDATAARBPROC, BufferSubDataARB);
		L(PFNGLGETBUFFERSUBDATAARBPROC, GetBufferSubDataARB);
		L(PFNGLMAPBUFFERARBPROC, MapBufferARB);
		L(PFNGLUNMAPBUFFERARBPROC, UnmapBufferARB);
		L(PFNGLGETBUFFERPARAMETERIVARBPROC, GetBufferParameterivARB);
		L(PFNGLGETBUFFERPOINTERVARBPROC, GetBufferPointervARB);
	END

	BEGIN(ARB_vertex_array_object)
		L(PFNGLBINDVERTEXARRAYAPPLEPROC, BindVertexArray);
		L(PFNGLDELETEVERTEXARRAYSAPPLEPROC, DeleteVertexArrays);
		L(PFNGLGENVERTEXARRAYSAPPLEPROC, GenVertexArrays);
	END
	
	if (!ARB_vertex_array_object)
	{
		BEGIN_EXT(ARB_vertex_array_object, APPLE_vertex_array_object)
			L(PFNGLBINDVERTEXARRAYAPPLEPROC, BindVertexArray);
			L(PFNGLDELETEVERTEXARRAYSAPPLEPROC, DeleteVertexArrays);
			L(PFNGLGENVERTEXARRAYSAPPLEPROC, GenVertexArrays);
		END
	}

	BEGIN(EXT_framebuffer_object)
		L(PFNGLISRENDERBUFFEREXTPROC, IsRenderbufferEXT);
		L(PFNGLBINDRENDERBUFFEREXTPROC, BindRenderbufferEXT);
		L(PFNGLDELETERENDERBUFFERSEXTPROC, DeleteRenderbuffersEXT);
		L(PFNGLGENRENDERBUFFERSEXTPROC, GenRenderbuffersEXT);
		L(PFNGLRENDERBUFFERSTORAGEEXTPROC, RenderbufferStorageEXT);
		L(PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC, GetRenderbufferParameterivEXT);
		L(PFNGLISFRAMEBUFFEREXTPROC, IsFramebufferEXT);
		L(PFNGLBINDFRAMEBUFFEREXTPROC, BindFramebufferEXT);
		L(PFNGLDELETEFRAMEBUFFERSEXTPROC, DeleteFramebuffersEXT);
		L(PFNGLGENFRAMEBUFFERSEXTPROC, GenFramebuffersEXT);
		L(PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC, CheckFramebufferStatusEXT);
		L(PFNGLFRAMEBUFFERTEXTURE1DEXTPROC, FramebufferTexture1DEXT);
		L(PFNGLFRAMEBUFFERTEXTURE2DEXTPROC, FramebufferTexture2DEXT);
		L(PFNGLFRAMEBUFFERTEXTURE3DEXTPROC, FramebufferTexture3DEXT);
		L(PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC, FramebufferRenderbufferEXT);
		L(PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC, GetFramebufferAttachmentParameterivEXT);
		L(PFNGLGENERATEMIPMAPEXTPROC, GenerateMipmapEXT);
	END

	BEGIN(EXT_framebuffer_multisample)
		L(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, RenderbufferStorageMultisampleEXT);
	END

	BEGIN(ARB_texture_compression)
		L(PFNGLCOMPRESSEDTEXIMAGE3DARBPROC, CompressedTexImage3DARB);
		L(PFNGLCOMPRESSEDTEXIMAGE2DARBPROC, CompressedTexImage2DARB);
		L(PFNGLCOMPRESSEDTEXIMAGE1DARBPROC, CompressedTexImage1DARB);
		L(PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC, CompressedTexSubImage3DARB);
		L(PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC, CompressedTexSubImage2DARB);
		L(PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC, CompressedTexSubImage1DARB);
		L(PFNGLGETCOMPRESSEDTEXIMAGEARBPROC, GetCompressedTexImageARB);
	END

	BEGIN(ARB_shader_objects)
		CHECK(ARB_vertex_shader);
		CHECK(ARB_fragment_shader);
		L(PFNGLDELETEOBJECTARBPROC, DeleteObjectARB);
		L(PFNGLDETACHOBJECTARBPROC, DetachObjectARB);
		L(PFNGLCREATESHADEROBJECTARBPROC, CreateShaderObjectARB);
		L(PFNGLSHADERSOURCEARBPROC, ShaderSourceARB);
		L(PFNGLCOMPILESHADERARBPROC, CompileShaderARB);
		L(PFNGLCREATEPROGRAMOBJECTARBPROC, CreateProgramObjectARB);
		L(PFNGLATTACHOBJECTARBPROC, AttachObjectARB);
		L(PFNGLLINKPROGRAMARBPROC, LinkProgramARB);
		L(PFNGLUSEPROGRAMOBJECTARBPROC, UseProgramObjectARB);
		L(PFNGLVALIDATEPROGRAMARBPROC, ValidateProgramARB);
		L(PFNGLBINDATTRIBLOCATIONARBPROC, BindAttribLocationARB);
		L(PFNGLGETACTIVEATTRIBARBPROC, GetActiveAttribARB);
		L(PFNGLGETATTRIBLOCATIONARBPROC, GetAttribLocationARB);
		L(PFNGLUNIFORM1FARBPROC, Uniform1fARB);
		L(PFNGLUNIFORM2FARBPROC, Uniform2fARB);
		L(PFNGLUNIFORM3FARBPROC, Uniform3fARB);
		L(PFNGLUNIFORM4FARBPROC, Uniform4fARB);
		L(PFNGLUNIFORM1IARBPROC, Uniform1iARB);
		L(PFNGLUNIFORM2IARBPROC, Uniform2iARB);
		L(PFNGLUNIFORM3IARBPROC, Uniform3iARB);
		L(PFNGLUNIFORM4IARBPROC, Uniform4iARB);
		L(PFNGLUNIFORM1FVARBPROC, Uniform1fvARB);
		L(PFNGLUNIFORM2FVARBPROC, Uniform2fvARB);
		L(PFNGLUNIFORM3FVARBPROC, Uniform3fvARB);
		L(PFNGLUNIFORM4FVARBPROC, Uniform4fvARB);
		L(PFNGLUNIFORM1IVARBPROC, Uniform1ivARB);
		L(PFNGLUNIFORM2IVARBPROC, Uniform2ivARB);
		L(PFNGLUNIFORM3IVARBPROC, Uniform3ivARB);
		L(PFNGLUNIFORM4IVARBPROC, Uniform4ivARB);
		L(PFNGLUNIFORMMATRIX2FVARBPROC, UniformMatrix2fvARB);
		L(PFNGLUNIFORMMATRIX3FVARBPROC, UniformMatrix3fvARB);
		L(PFNGLUNIFORMMATRIX4FVARBPROC, UniformMatrix4fvARB);
		L(PFNGLGETINFOLOGARBPROC, GetInfoLogARB);
		L(PFNGLGETATTACHEDOBJECTSARBPROC, GetAttachedObjectsARB);
		L(PFNGLGETUNIFORMLOCATIONARBPROC, GetUniformLocationARB);
		L(PFNGLGETACTIVEUNIFORMARBPROC, GetActiveUniformARB);
		L(PFNGLGETUNIFORMFVARBPROC, GetUniformfvARB);
		L(PFNGLGETUNIFORMIVARBPROC, GetUniformivARB);
		L(PFNGLGETSHADERSOURCEARBPROC, GetShaderSourceARB);
		L(PFNGLGETOBJECTPARAMETERIVARBPROC, GetObjectParameterivARB);
	END

	BEGIN(ARB_vertex_program)
		CHECK(ARB_fragment_program);
		L(PFNGLVERTEXATTRIB1DARBPROC, VertexAttrib1dARB);
		L(PFNGLVERTEXATTRIB1DVARBPROC, VertexAttrib1dvARB);
		L(PFNGLVERTEXATTRIB1FARBPROC, VertexAttrib1fARB);
		L(PFNGLVERTEXATTRIB1FVARBPROC, VertexAttrib1fvARB);
		L(PFNGLVERTEXATTRIB1SARBPROC, VertexAttrib1sARB);
		L(PFNGLVERTEXATTRIB1SVARBPROC, VertexAttrib1svARB);
		L(PFNGLVERTEXATTRIB2DARBPROC, VertexAttrib2dARB);
		L(PFNGLVERTEXATTRIB2DVARBPROC, VertexAttrib2dvARB);
		L(PFNGLVERTEXATTRIB2FARBPROC, VertexAttrib2fARB);
		L(PFNGLVERTEXATTRIB2FVARBPROC, VertexAttrib2fvARB);
		L(PFNGLVERTEXATTRIB2SARBPROC, VertexAttrib2sARB);
		L(PFNGLVERTEXATTRIB2SVARBPROC, VertexAttrib2svARB);
		L(PFNGLVERTEXATTRIB3DARBPROC, VertexAttrib3dARB);
		L(PFNGLVERTEXATTRIB3DVARBPROC, VertexAttrib3dvARB);
		L(PFNGLVERTEXATTRIB3FARBPROC, VertexAttrib3fARB);
		L(PFNGLVERTEXATTRIB3FVARBPROC, VertexAttrib3fvARB);
		L(PFNGLVERTEXATTRIB3SARBPROC, VertexAttrib3sARB);
		L(PFNGLVERTEXATTRIB3SVARBPROC, VertexAttrib3svARB);
		L(PFNGLVERTEXATTRIB4NBVARBPROC, VertexAttrib4NbvARB);
		L(PFNGLVERTEXATTRIB4NIVARBPROC, VertexAttrib4NivARB);
		L(PFNGLVERTEXATTRIB4NSVARBPROC, VertexAttrib4NsvARB);
		L(PFNGLVERTEXATTRIB4NUBARBPROC, VertexAttrib4NubARB);
		L(PFNGLVERTEXATTRIB4NUBVARBPROC, VertexAttrib4NubvARB);
		L(PFNGLVERTEXATTRIB4NUIVARBPROC, VertexAttrib4NuivARB);
		L(PFNGLVERTEXATTRIB4NUSVARBPROC, VertexAttrib4NusvARB);
		L(PFNGLVERTEXATTRIB4BVARBPROC, VertexAttrib4bvARB);
		L(PFNGLVERTEXATTRIB4DARBPROC, VertexAttrib4dARB);
		L(PFNGLVERTEXATTRIB4DVARBPROC, VertexAttrib4dvARB);
		L(PFNGLVERTEXATTRIB4FARBPROC, VertexAttrib4fARB);
		L(PFNGLVERTEXATTRIB4FVARBPROC, VertexAttrib4fvARB);
		L(PFNGLVERTEXATTRIB4IVARBPROC, VertexAttrib4ivARB);
		L(PFNGLVERTEXATTRIB4SARBPROC, VertexAttrib4sARB);
		L(PFNGLVERTEXATTRIB4SVARBPROC, VertexAttrib4svARB);
		L(PFNGLVERTEXATTRIB4UBVARBPROC, VertexAttrib4ubvARB);
		L(PFNGLVERTEXATTRIB4UIVARBPROC, VertexAttrib4uivARB);
		L(PFNGLVERTEXATTRIB4USVARBPROC, VertexAttrib4usvARB);
		L(PFNGLVERTEXATTRIBPOINTERARBPROC, VertexAttribPointerARB);
		L(PFNGLENABLEVERTEXATTRIBARRAYARBPROC, EnableVertexAttribArrayARB);
		L(PFNGLDISABLEVERTEXATTRIBARRAYARBPROC, DisableVertexAttribArrayARB);
		L(PFNGLPROGRAMSTRINGARBPROC, ProgramStringARB);
		L(PFNGLBINDPROGRAMARBPROC, BindProgramARB);
		L(PFNGLDELETEPROGRAMSARBPROC, DeleteProgramsARB);
		L(PFNGLGENPROGRAMSARBPROC, GenProgramsARB);
		L(PFNGLPROGRAMENVPARAMETER4DARBPROC, ProgramEnvParameter4dARB);
		L(PFNGLPROGRAMENVPARAMETER4DVARBPROC, ProgramEnvParameter4dvARB);
		L(PFNGLPROGRAMENVPARAMETER4FARBPROC, ProgramEnvParameter4fARB);
		L(PFNGLPROGRAMENVPARAMETER4FVARBPROC, ProgramEnvParameter4fvARB);
		L(PFNGLPROGRAMLOCALPARAMETER4DARBPROC, ProgramLocalParameter4dARB);
		L(PFNGLPROGRAMLOCALPARAMETER4DVARBPROC, ProgramLocalParameter4dvARB);
		L(PFNGLPROGRAMLOCALPARAMETER4FARBPROC, ProgramLocalParameter4fARB);
		L(PFNGLPROGRAMLOCALPARAMETER4FVARBPROC, ProgramLocalParameter4fvARB);
		L(PFNGLGETPROGRAMENVPARAMETERDVARBPROC, GetProgramEnvParameterdvARB);
		L(PFNGLGETPROGRAMENVPARAMETERFVARBPROC, GetProgramEnvParameterfvARB);
		L(PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC, GetProgramLocalParameterdvARB);
		L(PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC, GetProgramLocalParameterfvARB);
		L(PFNGLGETPROGRAMIVARBPROC, GetProgramivARB);
		L(PFNGLGETPROGRAMSTRINGARBPROC, GetProgramStringARB);
		L(PFNGLGETVERTEXATTRIBDVARBPROC, GetVertexAttribdvARB);
		L(PFNGLGETVERTEXATTRIBFVARBPROC, GetVertexAttribfvARB);
		L(PFNGLGETVERTEXATTRIBIVARBPROC, GetVertexAttribivARB);
		L(PFNGLGETVERTEXATTRIBPOINTERVARBPROC, GetVertexAttribPointervARB);
		L(PFNGLISPROGRAMARBPROC, IsProgramARB);
	END
	
#endif

#if defined(RAD_OPT_APPLE)
#if defined(RAD_OPT_OGLES)
	EXT_swap_control = false;
#else
	EXT_swap_control = true;
#endif
	SetSwapInterval = &_SetSwapInterval;
#elif defined(RAD_OPT_WIN)
	EXT_swap_control = CheckExt("WGL_EXT_swap_control");
	if (EXT_swap_control) {
		SetSwapInterval = (int (APIENTRYP)(int))GL_GetProcAddress("wglSwapIntervalEXT");
		EXT_swap_control = SetSwapInterval != 0;
	}
#elif defined(RAD_OPT_LINUX)
	EXT_swap_control = true; // assume. some vendors messed up the swap control extension strings
	if (EXT_swap_control) {
		SetSwapInterval = (int (APIENTRYP)(int))GL_GetProcAddress("glXSwapIntervalSGI");
		EXT_swap_control = SetSwapInterval != 0;
	}
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif

	ClearErrors();
}

#if defined(RAD_OPT_APPLE)
int GLTable::_SetSwapInterval(int i) {
#if !defined(RAD_OPT_OGLES)
#if defined(CGL_VERSION_1_2)
	const GLint sync = i;
#else
	const long sync = (long)i;
#endif
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &sync);
#endif
	return 0;
}
#endif

bool GLTable::CheckVer(const char *ver) {
	int min = 0, maj = 0;
#define CAWARN_DISABLE 6031 // return value ignored
#include <Runtime/PushCAWarnings.h>
	sscanf(ver, "%d.%d", &maj, &min);
#include <Runtime/PopCAWarnings.h>
	return (vMaj > maj) || (vMaj == maj && vMin >= min);
}

bool GLTable::CheckExt(const char* ext) {
	int i, k;
	int len_ext, len_glstring;
	const char* glstring;

	if(!ext || *ext == 0)
		return false;

	len_ext = (int)string::len( ext );

	glstring = (const char*)glGetString(GL_EXTENSIONS);
	if(!glstring || *glstring == 0)
		return false;

	len_glstring = (int)string::len( glstring );

	for(i = 0; i < len_glstring; i++) {
		if(glstring[i] == ' ')
			continue;

		/*
		found a char.
		compare with ext.
		mac doesn't have strcmp?
		*/

		for(k = 0; k < len_ext; k++) {
			if( string::tolower( ext[k] ) != string::tolower( glstring[i+k] ) )
				break;
		}

		if(k == len_ext) {	/* found? */
			/*
			the next char in the glstring better
			be a space or a null otherwise we just
			were fooled by a substring.
			*/
			if( i+k == len_glstring || glstring[i+k] == ' ' )
				return true;
		}
	}

	return false;
}

void GLTable::LoadGLVersion() {
#if defined(RAD_OPT_IOS)
	vMaj = 1;
	vMin = 1;
#else
	int off;
	const char* s = (const char*)glGetString(GL_VERSION);
	RAD_ASSERT(s);
	char temp_string[1024];
	temp_string[0] = 0;
	char* temp = temp_string;
	int low, high;

	low = 0;
	high = 0;
	off = 0;

	// get first version #.
	while( s[off] == ' ' )
		off++; // skip white space.

	// extract first.
	while( s[off] != '.' && s[off] != ' ' && s[off] != 0 )
	{
		*temp = s[off++];
		temp++;
	}
	*temp = 0;

	high = atoi( temp_string );
	if( s[off] != 0 )
	{
		RAD_ASSERT( s[off] == '.' );
		off++;
		temp = temp_string;
		while( s[off] != '.' && s[off] != ' ' && s[off] != 0 )
		{
			*temp = s[off++];
			temp++;
		}
		*temp = 0;

		low = atoi( temp_string );
	}

	vMaj = high;
	vMin = low;
#endif
}

void GLTable::SetActiveTexture(int num) {
	if (ActiveTextureARB)
		ActiveTextureARB(GL_TEXTURE0_ARB+num);
}

void GLTable::SetActiveTexCoord(int num) {
#if defined(RAD_OPT_OGLES)
	SetActiveTexture(num);
#else
	if (ClientActiveTextureARB)
		ClientActiveTextureARB(GL_TEXTURE0_ARB+num);
#endif
}

bool GLTable::CheckErrors(const char *file, const char *function, int line) {
#if defined(RAD_OPT_IOS) || defined(RAD_OPT_OSX)
	return glGetError() != GL_NO_ERROR;
#else
	String str;
	bool found = false;
	int count = 0;

	for (GLenum err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
		if (++count > 256)
			break;

		if (!found) {
			str.Printf(
				"GL Errors (file: %s, function: %s, line: %d):\n",
				file,
				function,
				line
			);
			found = true;
		}

		str += (const char*)gluErrorString(err);
		str += "\n";
	}

	ClearErrors();
	if (found) {
		MessageBox("GL Errors Detected", str.c_str, MBStyleOk);
	}

	return found;
#endif
}

void GLTable::ClearErrors() {
	int c = 0;
	while (glGetError() != GL_NO_ERROR && (c<64)) {
		++c;
	}
}

void GLTable::OrthoViewport(int x, int y, int w, int h) {
	MatrixMode(GL_PROJECTION);
	LoadIdentity();
	Ortho((double)x, (double)w, (double)(y+h), (double)y, -1.0, 1.0);
	MatrixMode(GL_MODELVIEW);
	LoadIdentity();
}

#undef near
#undef far

void GLTable::RotateForCamera(const Vec3 &pos, const Mat4 &rot, float near, float far, float fov, float yaspect) {
	MatrixMode(GL_PROJECTION);
	LoadIdentity();
	Perspective(fov*yaspect, 1.0/yaspect, near, far);
	RotateForCamera(pos, rot);
}

void GLTable::RotateForCamera(const Vec3 &pos, const Mat4 &rot) {
	MatrixMode(GL_MODELVIEW);
	LoadIdentity();
#if defined(RAD_OPT_IOS)
	gl.Rotatef(-90, 0, 0, 1);
#endif
	gl.Rotatef(-90, 1, 0, 0); // put Z going up
    gl.Rotatef( 90, 0, 0, 1); // put Z going up
	MultMatrix(rot);
	Translatef(-pos[0], -pos[1], -pos[2]);
}

} // r
