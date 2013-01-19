// GLTable.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Common.h"
#include <Runtime/Math/Matrix.h>
#include <Runtime/Container/ZoneVector.h>

#include "GLSystem.h"

#if defined(RAD_OPT_TOOLS)
struct glslopt_ctx;
#endif

#include <Runtime/PushPack.h>

namespace r {

class RBackend;

#if defined(RAD_OPT_GLERRORS)
	#define __CHECK_GL_ERRORS(x, y, z) r::gl.CheckErrors(x, y, z)
	#define CHECK_GL_ERRORS() __CHECK_GL_ERRORS(__FILE__, __FUNCTION__, __LINE__)
	#define CLEAR_GL_ERRORS() r::gl.ClearErrors()
#else
	#define CHECK_GL_ERRORS() ((void)0)
	#define CLEAR_GL_ERRORS() ((void)0)
#endif

class RADENG_CLASS GLMatrixStack {
public:
	GLMatrixStack();

	void Push();
	void Pop();
	Mat4 &Top();

private:
	typedef zone_vector<Mat4, ZEngineT>::type Stack;
	Stack m_s;
};

struct RADENG_CLASS GLTable {
	GLTable();
	
	bool CheckVer(const char *ver);
	static bool CheckExt(const char *ext);
	
	int vMin;
	int vMaj;
	int maxTextures;
	int maxVertexAttribs;
	int maxTextureSize;
	int matrixOps;
	int colorOps;
	int eyeOps;
	int numTris;

	bool v1_1;
	bool v1_2;
	bool v1_3;
	bool v1_4;
	bool v1_5;
	bool v2_0;
	bool vbos;
	bool vaos;
	bool wireframe;

#if defined(RAD_OPT_OGLES1_AND_2)
	bool ogles2;
#endif

#if defined(RAD_OPT_PC_TOOLS)
	CGcontext cgc;
#endif

#if defined(RAD_OPT_TOOLS)
	glslopt_ctx *glslopt;
	glslopt_ctx *glslopt_es;
#endif

	bool SGIS_generate_mipmap;
	bool ARB_texture_non_power_of_two;
	
#if defined(RAD_OPT_OGLES)
	bool IMG_texture_compression_pvrtc;
#endif

	PFNGLACTIVETEXTUREPROC ActiveTextureARB;
	
	bool ARB_vertex_buffer_object;
	PFNGLBINDBUFFERARBPROC BindBufferARB;
	PFNGLDELETEBUFFERSARBPROC DeleteBuffersARB;
	PFNGLGENBUFFERSARBPROC GenBuffersARB;
	PFNGLISBUFFERARBPROC IsBufferARB;
	PFNGLBUFFERDATAARBPROC BufferDataARB;
	PFNGLBUFFERSUBDATAARBPROC BufferSubDataARB;
	PFNGLGETBUFFERPARAMETERIVARBPROC GetBufferParameterivARB;
	PFNGLMAPBUFFERARBPROC MapBufferARB;
	PFNGLUNMAPBUFFERARBPROC UnmapBufferARB;
	PFNGLGETBUFFERPOINTERVARBPROC GetBufferPointervARB;

	bool ARB_vertex_array_object;
	PFNGLBINDVERTEXARRAYAPPLEPROC BindVertexArray;
	PFNGLDELETEVERTEXARRAYSAPPLEPROC DeleteVertexArrays;
	PFNGLGENVERTEXARRAYSAPPLEPROC GenVertexArrays;
			
	bool EXT_framebuffer_object;
	PFNGLISRENDERBUFFEREXTPROC IsRenderbufferEXT;
	PFNGLBINDRENDERBUFFEREXTPROC BindRenderbufferEXT;
	PFNGLDELETERENDERBUFFERSEXTPROC DeleteRenderbuffersEXT;
	PFNGLGENRENDERBUFFERSEXTPROC GenRenderbuffersEXT;
	PFNGLRENDERBUFFERSTORAGEEXTPROC RenderbufferStorageEXT;
	PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC GetRenderbufferParameterivEXT;
	PFNGLISFRAMEBUFFEREXTPROC IsFramebufferEXT;
	PFNGLBINDFRAMEBUFFEREXTPROC BindFramebufferEXT;
	PFNGLDELETEFRAMEBUFFERSEXTPROC DeleteFramebuffersEXT;
	PFNGLGENFRAMEBUFFERSEXTPROC GenFramebuffersEXT;
	PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC CheckFramebufferStatusEXT;
	PFNGLFRAMEBUFFERTEXTURE2DEXTPROC FramebufferTexture2DEXT;
	PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC FramebufferRenderbufferEXT;
	PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC GetFramebufferAttachmentParameterivEXT;
	PFNGLGENERATEMIPMAPEXTPROC GenerateMipmapEXT;

	bool ARB_shader_objects;
	bool ARB_vertex_shader;
	bool ARB_fragment_shader;
	PFNGLDELETEOBJECTARBPROC DeleteObjectARB;
	PFNGLDETACHOBJECTARBPROC DetachObjectARB;
	PFNGLCREATESHADEROBJECTARBPROC CreateShaderObjectARB;
	PFNGLSHADERSOURCEARBPROC ShaderSourceARB;
	PFNGLCOMPILESHADERARBPROC CompileShaderARB;
	PFNGLCREATEPROGRAMOBJECTARBPROC CreateProgramObjectARB;
	PFNGLATTACHOBJECTARBPROC AttachObjectARB;
	PFNGLLINKPROGRAMARBPROC	 LinkProgramARB;
	PFNGLUSEPROGRAMOBJECTARBPROC UseProgramObjectARB;
	PFNGLVALIDATEPROGRAMARBPROC ValidateProgramARB;
	PFNGLBINDATTRIBLOCATIONARBPROC BindAttribLocationARB;
	PFNGLGETACTIVEATTRIBARBPROC GetActiveAttribARB;
	PFNGLGETATTRIBLOCATIONARBPROC GetAttribLocationARB;
	PFNGLUNIFORM1FARBPROC Uniform1fARB;
	PFNGLUNIFORM2FARBPROC Uniform2fARB;
	PFNGLUNIFORM3FARBPROC Uniform3fARB;
	PFNGLUNIFORM4FARBPROC Uniform4fARB;
	PFNGLUNIFORM1IARBPROC Uniform1iARB;
	PFNGLUNIFORM2IARBPROC Uniform2iARB;
	PFNGLUNIFORM3IARBPROC Uniform3iARB;
	PFNGLUNIFORM4IARBPROC Uniform4iARB;
	PFNGLUNIFORM1FVARBPROC Uniform1fvARB;
	PFNGLUNIFORM2FVARBPROC Uniform2fvARB;
	PFNGLUNIFORM3FVARBPROC Uniform3fvARB;
	PFNGLUNIFORM4FVARBPROC Uniform4fvARB;
	PFNGLUNIFORM1IVARBPROC Uniform1ivARB;
	PFNGLUNIFORM2IVARBPROC Uniform2ivARB;
	PFNGLUNIFORM3IVARBPROC Uniform3ivARB;
	PFNGLUNIFORM4IVARBPROC Uniform4ivARB;
	PFNGLUNIFORMMATRIX2FVARBPROC UniformMatrix2fvARB;
	PFNGLUNIFORMMATRIX3FVARBPROC UniformMatrix3fvARB;
	PFNGLUNIFORMMATRIX4FVARBPROC UniformMatrix4fvARB;
	PFNGLGETINFOLOGARBPROC GetInfoLogARB;
	PFNGLGETATTACHEDOBJECTSARBPROC GetAttachedObjectsARB;
	PFNGLGETUNIFORMLOCATIONARBPROC GetUniformLocationARB;
	PFNGLGETACTIVEUNIFORMARBPROC GetActiveUniformARB;
	PFNGLGETUNIFORMFVARBPROC GetUniformfvARB;
	PFNGLGETUNIFORMIVARBPROC GetUniformivARB;
	PFNGLGETSHADERSOURCEARBPROC GetShaderSourceARB;
	PFNGLGETOBJECTPARAMETERIVARBPROC GetObjectParameterivARB;

	// technically part of ARB_vertex_program
	// but are available on OGLES2
	PFNGLVERTEXATTRIB1FARBPROC VertexAttrib1fARB;
	PFNGLVERTEXATTRIB1FVARBPROC VertexAttrib1fvARB;
	PFNGLVERTEXATTRIB2FARBPROC VertexAttrib2fARB;
	PFNGLVERTEXATTRIB2FVARBPROC VertexAttrib2fvARB;
	PFNGLVERTEXATTRIB3FARBPROC VertexAttrib3fARB;
	PFNGLVERTEXATTRIB3FVARBPROC VertexAttrib3fvARB;
	PFNGLVERTEXATTRIB4FARBPROC VertexAttrib4fARB;
	PFNGLVERTEXATTRIB4FVARBPROC VertexAttrib4fvARB;
	PFNGLVERTEXATTRIBPOINTERARBPROC VertexAttribPointerARB;
	PFNGLENABLEVERTEXATTRIBARRAYARBPROC EnableVertexAttribArrayARB;
	PFNGLDISABLEVERTEXATTRIBARRAYARBPROC DisableVertexAttribArrayARB;
	
	bool EXT_framebuffer_multisample;
	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC RenderbufferStorageMultisampleEXT;

	bool EXT_texture_filter_anisotropic;
	int maxAnisotropy;
	
	bool EXT_swap_control;
	int (APIENTRYP SetSwapInterval) (int i);
	
#if !defined(RAD_OPT_OGLES)
	
	PFNGLFRAMEBUFFERTEXTURE1DEXTPROC FramebufferTexture1DEXT;
	PFNGLFRAMEBUFFERTEXTURE3DEXTPROC FramebufferTexture3DEXT;
	
	bool EXT_texture_compression_s3tc;
	bool ARB_multitexture;
	
	PFNGLGETBUFFERSUBDATAARBPROC GetBufferSubDataARB;

	PFNGLCLIENTACTIVETEXTUREPROC ClientActiveTextureARB;
	PFNGLMULTITEXCOORD1FARBPROC MultiTexCoord1fARB;
	PFNGLMULTITEXCOORD1FVARBPROC MultiTexCoord1fvARB;
	PFNGLMULTITEXCOORD2FARBPROC MultiTexCoord2fARB;
	PFNGLMULTITEXCOORD2FVARBPROC MultiTexCoord2fvARB;
	PFNGLTEXIMAGE3DPROC TexImage3D;
	PFNGLTEXSUBIMAGE3DPROC TexSubImage3D;
	
	bool EXT_compiled_vertex_array;
	PFNGLLOCKARRAYSEXTPROC LockArraysEXT;
	PFNGLUNLOCKARRAYSEXTPROC UnlockArraysEXT;
		
	bool ARB_occlusion_query;
	PFNGLGENQUERIESARBPROC GenQueriesARB;
	PFNGLDELETEQUERIESARBPROC DeleteQueriesARB;
	PFNGLISQUERYARBPROC IsQueryARB;
	PFNGLBEGINQUERYARBPROC BeginQueryARB;
	PFNGLENDQUERYARBPROC EndQueryARB;
	PFNGLGETQUERYIVARBPROC GetQueryivARB;
	PFNGLGETQUERYOBJECTIVARBPROC GetQueryObjectivARB;
	PFNGLGETQUERYOBJECTUIVARBPROC GetQueryObjectuivARB;
	
	bool ARB_texture_compression;
	PFNGLCOMPRESSEDTEXIMAGE3DARBPROC CompressedTexImage3DARB;
	PFNGLCOMPRESSEDTEXIMAGE2DARBPROC CompressedTexImage2DARB;
	PFNGLCOMPRESSEDTEXIMAGE1DARBPROC CompressedTexImage1DARB;
	PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC CompressedTexSubImage3DARB;
	PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC CompressedTexSubImage2DARB;
	PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC CompressedTexSubImage1DARB;
	PFNGLGETCOMPRESSEDTEXIMAGEARBPROC GetCompressedTexImageARB;

	bool ARB_vertex_program;
	bool ARB_fragment_program;
	PFNGLVERTEXATTRIB1DARBPROC VertexAttrib1dARB;
	PFNGLVERTEXATTRIB1DVARBPROC VertexAttrib1dvARB;
	PFNGLVERTEXATTRIB1SARBPROC VertexAttrib1sARB;
	PFNGLVERTEXATTRIB1SVARBPROC VertexAttrib1svARB;
	PFNGLVERTEXATTRIB2DARBPROC VertexAttrib2dARB;
	PFNGLVERTEXATTRIB2DVARBPROC VertexAttrib2dvARB;
	PFNGLVERTEXATTRIB2SARBPROC VertexAttrib2sARB;
	PFNGLVERTEXATTRIB2SVARBPROC VertexAttrib2svARB;
	PFNGLVERTEXATTRIB3DARBPROC VertexAttrib3dARB;
	PFNGLVERTEXATTRIB3DVARBPROC VertexAttrib3dvARB;
	PFNGLVERTEXATTRIB3SARBPROC VertexAttrib3sARB;
	PFNGLVERTEXATTRIB3SVARBPROC VertexAttrib3svARB;
	PFNGLVERTEXATTRIB4NBVARBPROC VertexAttrib4NbvARB;
	PFNGLVERTEXATTRIB4NIVARBPROC VertexAttrib4NivARB;
	PFNGLVERTEXATTRIB4NSVARBPROC VertexAttrib4NsvARB;
	PFNGLVERTEXATTRIB4NUBARBPROC VertexAttrib4NubARB;
	PFNGLVERTEXATTRIB4NUBVARBPROC VertexAttrib4NubvARB;
	PFNGLVERTEXATTRIB4NUIVARBPROC VertexAttrib4NuivARB;
	PFNGLVERTEXATTRIB4NUSVARBPROC VertexAttrib4NusvARB;
	PFNGLVERTEXATTRIB4BVARBPROC VertexAttrib4bvARB;
	PFNGLVERTEXATTRIB4DARBPROC VertexAttrib4dARB;
	PFNGLVERTEXATTRIB4DVARBPROC VertexAttrib4dvARB;
	PFNGLVERTEXATTRIB4IVARBPROC VertexAttrib4ivARB;
	PFNGLVERTEXATTRIB4SARBPROC VertexAttrib4sARB;
	PFNGLVERTEXATTRIB4SVARBPROC VertexAttrib4svARB;
	PFNGLVERTEXATTRIB4UBVARBPROC VertexAttrib4ubvARB;
	PFNGLVERTEXATTRIB4UIVARBPROC VertexAttrib4uivARB;
	PFNGLVERTEXATTRIB4USVARBPROC VertexAttrib4usvARB;
	PFNGLPROGRAMSTRINGARBPROC ProgramStringARB;
	PFNGLBINDPROGRAMARBPROC BindProgramARB;
	PFNGLDELETEPROGRAMSARBPROC DeleteProgramsARB;
	PFNGLGENPROGRAMSARBPROC GenProgramsARB;
	PFNGLPROGRAMENVPARAMETER4DARBPROC ProgramEnvParameter4dARB;
	PFNGLPROGRAMENVPARAMETER4DVARBPROC ProgramEnvParameter4dvARB;
	PFNGLPROGRAMENVPARAMETER4FARBPROC ProgramEnvParameter4fARB;
	PFNGLPROGRAMENVPARAMETER4FVARBPROC ProgramEnvParameter4fvARB;
	PFNGLPROGRAMLOCALPARAMETER4DARBPROC ProgramLocalParameter4dARB;
	PFNGLPROGRAMLOCALPARAMETER4DVARBPROC ProgramLocalParameter4dvARB;
	PFNGLPROGRAMLOCALPARAMETER4FARBPROC ProgramLocalParameter4fARB;
	PFNGLPROGRAMLOCALPARAMETER4FVARBPROC ProgramLocalParameter4fvARB;
	PFNGLGETPROGRAMENVPARAMETERDVARBPROC GetProgramEnvParameterdvARB;
	PFNGLGETPROGRAMENVPARAMETERFVARBPROC  GetProgramEnvParameterfvARB;
	PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC GetProgramLocalParameterdvARB;
	PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC GetProgramLocalParameterfvARB;
	PFNGLGETPROGRAMIVARBPROC GetProgramivARB;
	PFNGLGETPROGRAMSTRINGARBPROC GetProgramStringARB;
	PFNGLGETVERTEXATTRIBDVARBPROC GetVertexAttribdvARB;
	PFNGLGETVERTEXATTRIBFVARBPROC GetVertexAttribfvARB;
	PFNGLGETVERTEXATTRIBIVARBPROC GetVertexAttribivARB;
	PFNGLISPROGRAMARBPROC IsProgramARB;
	PFNGLGETVERTEXATTRIBPOINTERVARBPROC GetVertexAttribPointervARB;
#endif

	// Supports VBO's or CSA paths.
	void DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
	void DrawArrays(GLenum mode, GLint first, GLsizei count);
	
	void SetActiveTexture(int num);
	void SetActiveTexCoord(int num);
	bool CheckErrors(const char *file, const char *function, int line);
	void ClearErrors();
	
	void OrthoViewport(int x, int y, int w, int h);

	void MatrixMode(GLenum mode);
	void LoadIdentity();
	void PushMatrix();
	void PopMatrix();
	void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void Scalef(GLfloat x, GLfloat y, GLfloat z);
	void Translatef(GLfloat x, GLfloat y, GLfloat z);
	void Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void Perspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
	void MultMatrix(const Mat4 &m);
	Mat4 GetModelViewMatrix();
	Mat4 GetProjectionMatrix();
	Mat4 GetModelViewProjectionMatrix();

	void Color4f(float r, float g, float b, float a, bool force=false);
	void GetColor4fv(float *v);

	void SetEye(const float *eye);
	void GetEye(float *eye);

	void RotateForCamera(const Vec3 &pos, const Mat4 &rot, float near, float far, float fov, float yaspect);
	void RotateForCamera(const Vec3 &pos, const Mat4 &rot);

private:

#if defined(RAD_OPT_APPLE)
	static int _SetSwapInterval(int i);
#endif

	int mm;
	GLMatrixStack mv;
	GLMatrixStack prj;

	float color[4];
	float eye[3];
	
	friend class RBackend;

	void Load();
	void Reset();
	void LoadGLVersion();
};

extern RADENG_API GLTable gl;

} // r

#include <Runtime/PopPack.h>
#include "GLTable.inl"
