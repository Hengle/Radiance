// GLSLShader.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../../Types.h"
#include "../../Engine.h"
#include "../../COut.h"
#include "GLSLShader.h"
#include "GLVertexBuffer.h"
#include "GLVertexArray.h"
#include <Runtime/StringBase.h>

#if defined(RAD_OPT_TOOLS)
#include "../../Packages/PackagesDef.h"
#include <sstream>
#include "GLSLTool.h"
#endif

#if defined(RAD_OPT_TOOLS)
#define LOG_DUMP
#define LOG_SAVE
#endif

#undef min
#undef max

#include <Runtime/PushSystemMacros.h>

namespace r {

#if defined(RAD_OPT_TOOLS)
class GLSLShaderLink : public tools::shader_utils::GLSLTool
{
public:
	GLSLShaderLink(
		Engine &engine,
		const Material &material,
		Shader::Pass pass, 
		const tools::shader_utils::Shader::Ref &shader,
		bool fragment
	) : m_engine(&engine), m_material(&material), m_pass(pass), m_shader(shader), m_fragment(fragment) {}

	virtual bool AddInclude(const char *name, std::ostream &out) {
		if (m_fragment && !string::cmp(name, "fragment")) {
			return m_shader->EmitFunctions(*m_engine, out) &&
				m_shader->EmitShader("_Material", m_pass, *m_material, out);
		}
			
		return true;
	}

private:
	Engine *m_engine;
	const Material *m_material;
	Shader::Pass m_pass;
	tools::shader_utils::Shader::Ref m_shader;
	bool m_fragment;
};

GLSLShader::Ref GLSLShader::Load(
	Engine &engine, 
	const char *name,
	const Material &material
) {
	tools::shader_utils::Shader::Ref shader = GLShader::Cache()->Load(engine, name);
	if (shader) {
		GLSLShader::Ref r(new (ZRender) GLSLShader());
		r->m_name = name;
		if (r->Compile(engine, shader, material))
			return r;
	}

	return Ref();
}

bool GLSLShader::Compile(
	Engine &engine,
	const tools::shader_utils::Shader::Ref &shader,
	const Material &material
) {
	int numPasses = 0;

	for (int i = 0; i < Shader::kNumPasses; ++i) {
		if (shader->Exists((Shader::Pass)i)) {
			if (!CompilePass(engine, (Shader::Pass)i, shader, material))
				return false;
			++numPasses;
		}
	}

	return numPasses > 0;
}

bool GLSLShader::CompilePass(
	Engine &engine,
	Shader::Pass pass,
	const tools::shader_utils::Shader::Ref &shader,
	const Material &material
) {
	Pass &p = m_passes[pass];

	if (!shader->BuildInputMappings(material, pass, p.m, p.tcMapping))
		return false;
	p.outputs = shader->PassOutputs(pass);

#if defined(RAD_OPT_OGLES)
	const tools::shader_utils::GLSLTool::AssembleFlags kGLESFlag = tools::shader_utils::GLSLTool::kAssemble_GLES;
#else
	const tools::shader_utils::GLSLTool::AssembleFlags kGLESFlag = tools::shader_utils::GLSLTool::kAssemble_None;
#endif

	GLSLShaderObj::Ref vs;
	GLSLShaderObj::Ref fs;

	{
		std::stringstream ss;

		GLSLShaderLink builder(engine, material, pass, shader, false);
		if (!builder.Assemble(
			engine, 
			material,
			shader,
			p.m,
			p.tcMapping,
			pass,
			tools::shader_utils::GLSLTool::kAssemble_VertexShader|
				tools::shader_utils::GLSLTool::kAssemble_Optimize|kGLESFlag,
			ss
		)) {
			COut(C_Error) << "GLSLShader::CompilePass('" << shader->name.get() << "', " <<
				pass << ", VertexShader): Failed to emit shader code, SHADER ERROR!" << std::endl;
			return false;
		}

		String shaderSource(ss.str().c_str());

#if defined(LOG_DUMP)
		COut(C_Info) << "GLSLShader::CompilePass('" << shader->name.get() << "', " <<
			pass << ", VertexShader): " << std::endl << shaderSource << std::endl;
#if !defined(RAD_OPT_IOS) && defined(LOG_SAVE)
		{
			engine.sys->files->CreateDirectory("@r:/Temp/Shaders/Logs");
			String path(CStr("@r:/Temp/Shaders/Logs/"));
			path += shader->name;
			path += ".vert.glsl";
			tools::shader_utils::SaveText(engine, path.c_str, shaderSource.c_str);
		}
#endif
#endif

		vs.reset(new (ZRender) GLSLShaderObj(GL_VERTEX_SHADER_ARB));
		const char *sz = shaderSource.c_str;
		gl.ShaderSourceARB(vs->id, 1, &sz, 0);
		CHECK_GL_ERRORS();
		gl.CompileShaderARB(vs->id);
		CHECK_GL_ERRORS();
		
		GLint status;
		gl.GetObjectParameterivARB(vs->id, GL_OBJECT_COMPILE_STATUS_ARB, &status);
		if (!status) { 
			// error. 
			COut(C_ErrMsgBox) << "GLSLShader::CompilePass('" << shader->name.get() << "', " <<
				pass << ", VertexShader) Error: \n" << ShaderLog(vs->id) << std::endl;
			return false;
		}

#if defined(RAD_OPT_DEBUG)
		COut(C_Debug) << "GLSLShader::CompilePass('" << shader->name.get() << "', " << pass <<
			", VertexShader) Successfully Compiled. " << std::endl;
#endif
	}

	{
		std::stringstream ss;

		GLSLShaderLink builder(engine, material, pass, shader, true);
		if (!builder.Assemble(
			engine, 
			material,
			shader,
			p.m,
			p.tcMapping,
			pass,
			tools::shader_utils::GLSLTool::kAssemble_PixelShader|
				tools::shader_utils::GLSLTool::kAssemble_Optimize|kGLESFlag,
			ss
		)) {
			COut(C_Error) << "GLSLShader::CompilePass('" << shader->name.get() << "', " <<
				pass << ", PixelShader): Failed to emit shader code, SHADER ERROR!" << std::endl;
			return false;
		}

		String shaderSource(ss.str().c_str());

#if defined(LOG_DUMP)
		COut(C_Info) << "GLSLShader::CompilePass('" << shader->name.get() << "', " <<
			pass << ", PixelShader): " << std::endl << shaderSource << std::endl;
#if !defined(RAD_OPT_IOS) && defined(LOG_SAVE)
		{
			engine.sys->files->CreateDirectory("@r:/Temp/Shaders/Logs");
			String path(CStr("@r:/Temp/Shaders/Logs/"));
			path += shader->name;
			path += ".frag.glsl";
			tools::shader_utils::SaveText(engine, path.c_str, shaderSource.c_str);
		}
#endif
#endif

		fs.reset(new (ZRender) GLSLShaderObj(GL_FRAGMENT_SHADER_ARB));
		const char *sz = shaderSource.c_str;
		gl.ShaderSourceARB(fs->id, 1, &sz, 0);
		CHECK_GL_ERRORS();
		gl.CompileShaderARB(fs->id);
		CHECK_GL_ERRORS();

		GLint status;
		gl.GetObjectParameterivARB(fs->id, GL_OBJECT_COMPILE_STATUS_ARB, &status);
		if (!status) { 
			// error. 
			COut(C_ErrMsgBox) << "GLSLShader::CompilePass('" << shader->name.get() << "', " <<
				pass << ", PixelShader) Error: \n" << ShaderLog(fs->id) << std::endl;
			return false;
		}

#if defined(RAD_OPT_DEBUG)
		COut(C_Debug) << "GLSLShader::CompilePass('" << shader->name.get() << "', " << pass <<
			", PixelShader) Successfully Compiled. " << std::endl;
#endif
	}

	GLSLProgramObj::Ref r(new (ZRender) GLSLProgramObj());
	gl.AttachObjectARB(r->id, vs->id);
	CHECK_GL_ERRORS();
	gl.AttachObjectARB(r->id, fs->id);
	CHECK_GL_ERRORS();

	BindAttribLocations(r->id, p.m);

	gl.LinkProgramARB(r->id);
	CHECK_GL_ERRORS();
	{
		GLint status;
#if defined(RAD_OPT_OGLES)
		glGetProgramiv(r->id, GL_LINK_STATUS, &status);
		CHECK_GL_ERRORS();
#else
		gl.GetObjectParameterivARB(r->id, GL_OBJECT_LINK_STATUS_ARB, &status);
		CHECK_GL_ERRORS();
#endif
		if (!status)
		{
			COut(C_ErrMsgBox) << "GLSLShader::CompilePass('" << shader->name.get() << "', " <<
				pass << ", Link) Error: " << ProgramLog(r->id) << std::endl;
			return false;
		}
	}

	p.p = r;

	if (!MapInputs(p, material)) {
		gls.UseProgram(0, true);
		p.p.reset();
		COut(C_ErrMsgBox) << "GLSLShader::CompilePass('" << shader->name.get() << "', " <<
			pass << ", Program) MapInputs() failed." << std::endl;
		return false;
	}

	gls.UseProgram(0, true);

	return true;
}

#endif // defined(RAD_OPT_TOOLS)

#if defined(RAD_OPT_PC_TOOLS)

bool GLSLShader::CompileShaderSource(
	Engine &engine,
	stream::OutputStream &os, 
	int pflags,
	const Material &material
) {
	tools::shader_utils::Shader::Ref shader = GLShader::Cache()->Load(engine, m_name.c_str);
	if (!shader)
		return false;

	int numPasses = 0;
	for (int i = 0; i < Shader::kNumPasses; ++i) {
		if (i == Shader::kPass_Preview)
			continue; // don't cook this.
		if (m_passes[i].p)
			++numPasses;
	}

	os << (U8)GLShader::kBackend_GLSL;
	os << (U8)numPasses;
	
	String vertexSource;
	String fragmentSource;

	tools::shader_utils::GLSLTool::AssembleFlags kGLESFlag = tools::shader_utils::GLSLTool::kAssemble_None;
	if (pflags&pkg::P_TargetIOS)
		kGLESFlag = tools::shader_utils::GLSLTool::kAssemble_GLES;

	for (int i = 0; i < Shader::kNumPasses; ++i) {
		if (i == Shader::kPass_Preview)
			continue; // don't cook this.

		Pass &p = m_passes[i];

		if (!p.p)
			continue;

		{
			std::stringstream ss;

			GLSLShaderLink builder(engine, material, (Shader::Pass)i, shader, false);
			if (!builder.Assemble(
				engine, 
				material,
				shader,
				p.m,
				p.tcMapping,
				(Shader::Pass)i,
				tools::shader_utils::GLSLTool::kAssemble_VertexShader|
					tools::shader_utils::GLSLTool::kAssemble_Optimize|kGLESFlag,
				ss
			)) {
				COut(C_Error) << "GLSLShader::CompilePass('" << shader->name.get() << "', " <<
					i << ", VertexShader): Failed to emit shader code, SHADER ERROR!" << std::endl;
				return false;
			}

			vertexSource = ss.str().c_str();
		}

		{
			std::stringstream ss;

			GLSLShaderLink builder(engine, material, (Shader::Pass)i, shader, false);
			if (!builder.Assemble(
				engine, 
				material,
				shader,
				p.m,
				p.tcMapping,
				(Shader::Pass)i,
				tools::shader_utils::GLSLTool::kAssemble_PixelShader|
					tools::shader_utils::GLSLTool::kAssemble_Optimize|kGLESFlag,
				ss
			)) {
				COut(C_Error) << "GLSLShader::CompilePass('" << shader->name.get() << "', " <<
					i << ", PixelShader): Failed to emit shader code, SHADER ERROR!" << std::endl;
				return false;
			}

			fragmentSource = ss.str().c_str();
		}

		os << (U8)i;
		os << (U8)p.outputs;

		for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
			os << p.m.tcMods[i];
		}

		for (int i = 0; i < kNumMaterialTextureSources; ++i) {
			os << p.m.numMTSources[i];
		}

		for (int i = 0; i < kNumMaterialGeometrySources; ++i) {
			os << p.m.numMGSources[i];
		}

		for (int i = 0; i < kMaxTextures; ++i) {
			os << p.m.textures[i][0];
			os << p.m.textures[i][1];
		}
		
		for (int i = 0; i < kMaxAttribArrays; ++i) {
			os << p.m.attributes[i][0];
			os << p.m.attributes[i][1];
		}

		os << (U32)(vertexSource.length+1);
		os << (U32)(fragmentSource.length+1);
		if (os.Write(vertexSource.c_str.get(), (stream::SPos)vertexSource.length.get()+1, 0) != (stream::SPos)(vertexSource.length.get()+1))
			return false;
		if (os.Write(fragmentSource.c_str.get(), (stream::SPos)fragmentSource.length.get()+1, 0) != (stream::SPos)(fragmentSource.length.get()+1))
			return false;
	}

	return true;
}

#endif

String GLSLShader::ShaderLog(GLhandleARB s) {
	enum { MaxLen = Kilo*8 };
	char sz[MaxLen];
	gl.GetInfoLogARB(s, MaxLen, 0, sz);
	return String(sz);
}

String GLSLShader::ProgramLog(GLhandleARB s) {
#if defined(RAD_OPT_OGLES)
	enum { MaxLen = Kilo*8 };
	char sz[MaxLen];
	glGetProgramInfoLog(s, MaxLen, 0, sz);
	return String(sz);
#else
	return ShaderLog(s);
#endif
}

GLSLShader::Ref GLSLShader::LoadCooked(
	const char *name,
	stream::InputStream &is,
	const Material &material
) {
	// first byte peeled off by GLShader to select GLSL backend.

	GLSLShader::Ref r(new (ZRender) GLSLShader());
	r->m_name = name;

	U8 numPasses;
	is >> numPasses;

	for (U8 i = 0; i < numPasses; ++i) {
		if (!r->LoadPass(name, is, material)) {
			r.reset();
			break;
		}
	}

	return r;
}

bool GLSLShader::LoadPass(
	const char *name,
	stream::InputStream &is,
	const Material &material
) {
	U8 passNum;
	is >> passNum;
	if (passNum >= Shader::kNumPasses)
		return false;

	Pass &p = m_passes[passNum];
	memset(&p.m, 0, sizeof(p.m));

	U8 outputs;
	
	is >> outputs; p.outputs = outputs;

	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		is >> p.m.tcMods[i];
	}

	for (int i = 0; i < kNumMaterialTextureSources; ++i) {
		is >> p.m.numMTSources[i];
	}

	for (int i = 0; i < kNumMaterialGeometrySources; ++i) {
		is >> p.m.numMGSources[i];
	}

	for (int i = 0; i < kMaxTextures; ++i) {
		is >> p.m.textures[i][0];
		is >> p.m.textures[i][1];
	}

	for (int i = 0; i < kMaxAttribArrays; ++i) {
		is >> p.m.attributes[i][0];
		is >> p.m.attributes[i][1];
	}

	U32 programLength[2];
	is >> programLength[0] >> programLength[1];
	U32 maxLength = std::max(programLength[0], programLength[1]);
	if (maxLength > 64*Kilo)
		return false; // size check
	
	char *source = (char*)stack_alloc(maxLength);
	if (is.Read(source, (stream::SPos)programLength[0], 0) != (stream::SPos)programLength[0])
		return false;
	
	GLSLProgramObj::Ref r(new (ZRender) GLSLProgramObj());
	
	COut(C_Debug) << "GLSLShader::LoadPass(" << r->id.get() << "->\"" << name << "\")" << std::endl;

	GLSLShaderObj::Ref vs;
	
	vs.reset(new (ZRender) GLSLShaderObj(GL_VERTEX_SHADER_ARB));
	gl.ShaderSourceARB(vs->id, 1, (const char**)&source, 0);
	CHECK_GL_ERRORS();
	gl.CompileShaderARB(vs->id);
	CHECK_GL_ERRORS();
		
	GLint status;
	gl.GetObjectParameterivARB(vs->id, GL_OBJECT_COMPILE_STATUS_ARB, &status);
	if (!status) { 
		// error. 
		COut(C_Error) << "GLSLShader::LoadPass('" << name << "', Vertex) Error:" << std::endl << ShaderLog(vs->id) << std::endl;
		return false;
	}

	if (is.Read(source, (stream::SPos)programLength[1], 0) != (stream::SPos)programLength[1])
		return false;

	GLSLShaderObj::Ref fs;

	fs.reset(new (ZRender) GLSLShaderObj(GL_FRAGMENT_SHADER_ARB));
	gl.ShaderSourceARB(fs->id, 1, (const char**)&source, 0);
	CHECK_GL_ERRORS();
	gl.CompileShaderARB(fs->id);
	CHECK_GL_ERRORS();

	gl.GetObjectParameterivARB(fs->id, GL_OBJECT_COMPILE_STATUS_ARB, &status);
	if (!status) { 
		// error. 
		COut(C_Error) << "GLSLShader::LoadPass('" << name << "', Fragment) Error:" << std::endl << ShaderLog(fs->id) << std::endl;
		return false;
	}

	gl.AttachObjectARB(r->id, vs->id);
	CHECK_GL_ERRORS();
	gl.AttachObjectARB(r->id, fs->id);
	CHECK_GL_ERRORS();

	BindAttribLocations(r->id, p.m);

	gl.LinkProgramARB(r->id);
	CHECK_GL_ERRORS();
	gl.DetachObjectARB(r->id, vs->id);
	CHECK_GL_ERRORS();
	gl.DetachObjectARB(r->id, fs->id);
	CHECK_GL_ERRORS();
	{
#if defined(RAD_OPT_OGLES)
		glGetProgramiv(r->id, GL_LINK_STATUS, &status);
		CHECK_GL_ERRORS();
#else
		gl.GetObjectParameterivARB(r->id, GL_OBJECT_LINK_STATUS_ARB, &status);
		CHECK_GL_ERRORS();
#endif
		if (!status) {
			COut(C_Error) << "GLSLShader::LoadPass('" << name << "', Link) Error: " << std::endl << ProgramLog(r->id) << std::endl;
			return false;
		}
	}

	p.p = r;

	if (!MapInputs(p, material)) {
		gls.UseProgram(0, true);
		p.p.reset();
		COut(C_Error) << "GLSLShader::LoadPass('" << name << "', Program) MapInputs() failed." << std::endl;
		return false;
	}

	return true;
}

bool GLSLShader::MapInputs(Pass &p, const Material &material) {
	gls.UseProgram(p.p->id, true);

#if defined(RAD_OPT_OGLES2)
	p.u.matrixOps = 0;
	p.u.mvp = gl.GetUniformLocationARB(p.p->id, "U_mvp");
	p.u.color = gl.GetUniformLocationARB(p.p->id, "U_color");
	p.u.rgba[0] = -1.f;
	p.u.rgba[1] = -1.f;
	p.u.rgba[2] = -1.f;
	p.u.rgba[3] = -1.f;
	for (int i = 0; i < 16; ++i)
		p.u.mvpfloats[i] = 0.f;
#endif

	const float kFloatMax = std::numeric_limits<float>::max();

	p.u.eye = gl.GetUniformLocationARB(p.p->id, "U_eye");
	p.u.eyeOps = 0;
	p.u.eyePos[0] = p.u.eyePos[1] = p.u.eyePos[2] = kFloatMax;

	p.u.lights.numLights = 0;

	for (int i = 0; i < kMaxLights; ++i) {
		p.u.lights.lights[i].diffuse = Vec4(kFloatMax, kFloatMax, kFloatMax, kFloatMax);
		p.u.lights.lights[i].specular = Vec4(kFloatMax, kFloatMax, kFloatMax, kFloatMax);
		p.u.lights.lights[i].pos = Vec3(kFloatMax, kFloatMax, kFloatMax);
		p.u.lights.lights[i].flags = 0;
	}
	
	for (int i = 0; i < kMaxLights; ++i) {
		char sz[64];

		string::sprintf(sz, "U_light%d_pos", i);
		p.u.lightPos[i] = gl.GetUniformLocationARB(p.p->id, sz);

		string::sprintf(sz, "U_light%d_diffuseColor", sz);
		p.u.lightDiffuse[i] = gl.GetUniformLocationARB(p.p->id, sz);

		string::sprintf(sz, "U_light%d_specularColor", sz);
		p.u.lightSpecular[i] = gl.GetUniformLocationARB(p.p->id, sz);

		if (p.u.lightPos[i] == -1 &&
			p.u.lightDiffuse[i] == -1 &&
			p.u.lightSpecular[i] == -1) {
			for (int k = i+1; k < kMaxLights; ++k) {
				p.u.lightPos[k] = -1;
				p.u.lightDiffuse[k] = -1;
				p.u.lightSpecular[k] = -1;
			}
			break;
		} else {
			++p.u.lights.numLights;
		}
	}

	for (int i = 0; i < kMaxTextures; ++i) {
		if (p.m.textures[i][0] == kInvalidMapping)
			break;

		char sz[64];
		string::sprintf(sz, "U_t%d", i);
		p.u.textures[i] = gl.GetUniformLocationARB(p.p->id, sz);

		if (p.u.textures[i] != -1) { 
			// bind texture units
			gl.Uniform1iARB(p.u.textures[i], i);
		}
	}

	for (int i = 0; i < kMaxTextures; ++i) {
		
		p.u.tcMods[i][Material::kNumTCMods] = -1;
		int tcModFlags = material.TCModFlags(i);

		int tcIndex = -1;
		if (tcModFlags != 0) {  // find matching tc index
			for (int k = 0; k < kMaxTextures; ++k) {
				if (p.m.tcMods[k] == kInvalidMapping)
					break;
				if (p.m.tcMods[k] == i) {
					tcIndex = k;
					break;
				}
			}
		}

		if (tcIndex == -1) {
			for (int k = 0; k < Material::kNumTCMods; ++k) {
				p.u.tcMods[i][k] = -1;
			}
			continue;
		}

		for (int k = 0; k < Material::kNumTCMods; ++k) {
			if (!(tcModFlags&(1<<k))) { 
				// TCMod is unused by shader.
				p.u.tcMods[i][k] = -1;
				continue;
			}

			char sz[64];
			string::sprintf(sz, "U_tcmod%d[%d]", tcIndex, k);
			p.u.tcMods[i][k] = gl.GetUniformLocationARB(p.p->id, sz);
			if (p.u.tcMods[i][k] != -1) {
				gl.Uniform4fARB(p.u.tcMods[i][k], 0.f, 0.f, 0.f, 0.f);

				for (int j = 0; j < Material::kNumTCModVals; ++j) {
					p.u.tcModVals[i][k][j] = 0.f;
				}

				if (k == Material::kTCMod_Turb) {
					string::sprintf(sz, "U_tcmod%d[%d]", tcIndex, Material::kNumTCMods);
					p.u.tcMods[i][Material::kNumTCMods] = gl.GetUniformLocationARB(p.p->id, sz);
					RAD_VERIFY(p.u.tcMods[i][Material::kNumTCMods] != -1);
					gl.Uniform4fARB(p.u.tcMods[i][Material::kNumTCMods], 0.f, 0.f, 0.f, 0.f);
				}
			}
		}
	}

	return true;
}

bool GLSLShader::Requires(Shader::Pass _p, MaterialTextureSource source, int index) const {
	if (!HasPass(_p))
		return false;
	const Pass &p = m_passes[_p];

	for (int i = 0; i < kMaxTextures; ++i) {
		if (p.m.textures[i][0] == kInvalidMapping)
			break;
		if (p.m.textures[i][0] == source && p.m.textures[i][1] == index)
			return true;
	}

	return false;
}

bool GLSLShader::Requires(Shader::Pass _p, MaterialGeometrySource source, int index) const {
	if (!HasPass(_p))
		return false;
	const Pass &p = m_passes[_p];

	for (int i = 0; i < kMaxAttribArrays; ++i) {
		if (p.m.attributes[i][0] == kInvalidMapping)
			break;
		if (p.m.attributes[i][0] == source && p.m.attributes[i][1] == index)
			return true;
	}

	return false;
}


void GLSLShader::Begin(Shader::Pass _p, const Material &material) {
	Pass &p = m_passes[_p];
	RAD_ASSERT(p.p);
	m_curPass = _p;
	m_curMat = &material;

	gls.UseProgram(p.p->id, true);

	// bind material textures
	gls.DisableTextures();
	for (int i = 0; i < kMaxTextures; ++i) {
		if (p.m.textures[i][0] == kInvalidMapping)
			break;

		GLTextureRef r = gls.MaterialTextureSource((MaterialTextureSource)p.m.textures[i][0], (int)p.m.textures[i][1]);
		if (r)
			gls.SetTexture(i, r);
	}

	for (int i = 0 ;i < kMaterialTextureSource_MaxIndices; ++i) {
		if (p.m.tcMods[i] == kInvalidMapping)
			break;

		int tcIndex = (int)p.m.tcMods[i];

		// material contains animated tcMods.
		for (int k = 0; k < Material::kNumTCMods; ++k) {
			if (p.u.tcMods[tcIndex][k] == -1)
				continue; // unreferenced by shader

			float st[Material::kNumTCModVals];
			float *src = &p.u.tcModVals[tcIndex][k][0];
			int ops;

			material.Sample(tcIndex, k, ops, st);

			if (src[0] != st[0] || src[1] != st[1] || src[2] != st[2] || src[3] != st[3]) {
				RAD_ASSERT(p.u.tcMods[tcIndex][k] != -1);
				gl.Uniform4fARB(p.u.tcMods[tcIndex][k], st[0], st[1], st[2], st[3]);
				src[0] = st[0];
				src[1] = st[1];
				src[2] = st[2];
				src[3] = st[3];
			}

			// pack in extra turb arguments
			if (k == Material::kTCMod_Turb && (src[4] != st[4] || src[5] != st[5])) {
				RAD_ASSERT(p.u.tcMods[tcIndex][Material::kNumTCMods] != -1);
				gl.Uniform4fARB(p.u.tcMods[tcIndex][Material::kNumTCMods], st[4], st[5], 0.f, 0.f);
				src[4] = st[4];
				src[5] = st[5];
			}
		}
	}
	
#if defined(RAD_OPT_OGLES2) && defined(RAD_OPT_DEBUG)
	GLint logLen, status;
	glValidateProgram(p.p->id);
	glGetProgramiv(p.p->id, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0) {
		GLchar *log = (GLchar*)stack_alloc(logLen);
		glGetProgramInfoLog(p.p->id, logLen, &logLen, log);
		COut(C_Debug) << "GLSLShader: '" << m_name.c_str.get() << "': Validate Log: " << log << std::endl;
	}
	
	glGetProgramiv(p.p->id, GL_VALIDATE_STATUS, &status);
	RAD_ASSERT(status!=0);
#endif
}

// NVIDIA cards reserve index 3 for gl_Color and will produce an error if it's used
// Remap this.

inline int RemapIndex(int idx) {
#if defined(RAD_OPT_PC)
	if (idx == 3)
		return kMaxAttribArrays-1;
#endif
	return idx;
}

void GLSLShader::BindStates(const r::Shader::Uniforms &uniforms, bool sampleMaterialColor) {
	RAD_ASSERT(m_curMat);
	Pass &p = m_passes[m_curPass];
	
	float c[4];

	if (sampleMaterialColor) {
		m_curMat->SampleColor(Material::kColor0, c);
		c[0] *= uniforms.blendColor[0];
		c[1] *= uniforms.blendColor[1];
		c[2] *= uniforms.blendColor[2];
		c[3] *= uniforms.blendColor[3];

#if defined(RAD_OPT_PC)
		gl.Color4f(c[0], c[1], c[2], c[3]);
#else
	} else {
		gl.GetColor4fv(c);
#endif
	}

	if ((p.u.eye != -1) && (p.u.eyeOps != gl.eyeOps)) {
		p.u.eyeOps = gl.eyeOps;

		float eye[3];
		gl.GetEye(eye);

		if (eye[0] != p.u.eyePos[0] ||
			eye[1] != p.u.eyePos[1] ||
			eye[2] != p.u.eyePos[2]) {

		}
	}

	// shader has to be provided all inputs!
	RAD_ASSERT(p.u.lights.numLights == uniforms.lights.numLights);

	for (int i = 0; i < uniforms.lights.numLights; ++i) {
		const LightDef &srcLight = uniforms.lights.lights[i];
		LightDef &dstLight = p.u.lights.lights[i];

		if ((p.u.lightPos[i] != -1) && (srcLight.pos != dstLight.pos)) {
			dstLight.pos = srcLight.pos;
			gl.Uniform3fvARB(p.u.lightPos[i], 1, &srcLight.pos[0]);
		}

		if ((p.u.lightDiffuse[i] != -1) && (srcLight.diffuse != dstLight.diffuse)) {
			RAD_ASSERT(srcLight.flags & LightDef::kFlag_Diffuse);
			dstLight.diffuse = srcLight.diffuse;
			gl.Uniform4fvARB(p.u.lightDiffuse[i], 1, &srcLight.diffuse[0]);
		}

		if ((p.u.lightSpecular[i] != -1) && (srcLight.specular != dstLight.specular)) {
			RAD_ASSERT(srcLight.flags & LightDef::kFlag_Specular);
			dstLight.specular = srcLight.specular;
			gl.Uniform4fvARB(p.u.lightSpecular[i], 1, &srcLight.specular[0]);
		}

		dstLight.flags = srcLight.flags;
	}
	
#if defined(RAD_OPT_OGLES2)
	if (p.u.matrixOps != gl.matrixOps)
	{ // matrix has changed
		p.u.matrixOps = gl.matrixOps;
		// track modelview projection matrix?
		if (p.u.mvp != -1) {
			float floats[16];
			Mat4 *x = reinterpret_cast<Mat4*>(floats);
			*x = gl.GetModelViewProjectionMatrix();
			x->Transpose();
			if (memcmp(floats, p.u.mvpfloats, 16*sizeof(float))) {
				memcpy(p.u.mvpfloats, floats, 16*sizeof(float));
				gl.UniformMatrix4fvARB(p.u.mvp, 1, GL_FALSE, floats);
			}
		}
	}
	if (p.u.color != -1) {
		if (p.u.rgba[0] != c[0] ||
			p.u.rgba[1] != c[1] ||
			p.u.rgba[2] != c[2] ||
			p.u.rgba[3] != c[3])
		{
			p.u.rgba[0] = c[0];
			p.u.rgba[1] = c[1];
			p.u.rgba[2] = c[2];
			p.u.rgba[3] = c[3];
			gl.Uniform4fvARB(p.u.color, 1, &c[0]);
		}
	}
#endif

	GLVertexArray::Ref vao = gls.VertexArrayBinding();
	if (vao && vao->initialized)
		return;

	// bind material attributes

	GLVertexBufferRef vb;
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLsizei stride;
	GLuint ofs;

	GLVertexBufferRef sourceVB;
	GLint sourceSize;
	GLenum sourceType;
	GLboolean sourceNormalized;
	GLsizei sourceStride;
	GLuint sourceOfs;
	MaterialGeometrySource lastSource = kNumMaterialGeometrySources;

	gls.DisableVertexAttribArrays();
	
	for (int i = 0 ; i < kMaxAttribArrays; ++i) {
		if (p.m.attributes[i][0] == kInvalidMapping)
			break;

		gls.MaterialGeometrySource(
			(MaterialGeometrySource)p.m.attributes[i][0],
			(int)p.m.attributes[i][1],
			vb,
			size,
			type,
			normalized,
			stride,
			ofs
		);

		if (lastSource != p.m.attributes[i][0]) {
			lastSource = (MaterialGeometrySource)p.m.attributes[i][0];
			sourceVB = vb;
			sourceSize = size;
			sourceType = type;
			sourceNormalized = normalized;
			sourceStride = stride;
			sourceOfs = ofs;
		}

		int loc = RemapIndex(i);

		if (vb) {
			gls.EnableVertexAttribArray(loc, true);
			gls.VertexAttribPointer(
				loc,
				vb,
				size,
				type,
				normalized,
				stride,
				ofs
			);
		} else if (sourceVB) { 
			// some values like 2nd UV channel aren't provided
			// so pass in the 1st UV channel
			gls.EnableVertexAttribArray(loc, true);
			gls.VertexAttribPointer(
				loc,
				sourceVB,
				sourceSize,
				sourceType,
				sourceNormalized,
				sourceStride,
				sourceOfs
			);
		}
	}
}

void GLSLShader::End() {
	m_curMat = 0;
	gls.UseProgram(0);
}

void GLSLShader::BindAttribLocations(GLhandleARB p, const MaterialInputMappings &m) {
	for (int i = 0; i < kMaxAttribArrays; ++i) {
		if (m.attributes[i][0] == kInvalidMapping)
			break;

		MaterialGeometrySource s = (MaterialGeometrySource)m.attributes[i][0];
		GLint idx  = (int)m.attributes[i][1];

		int loc = RemapIndex(i);

		switch (s) {
		case kMaterialGeometrySource_Vertices:
			if (idx == 0) {
				gl.BindAttribLocationARB(p, loc, "in_position");
				CHECK_GL_ERRORS();
			}
		break;
		case kMaterialGeometrySource_Normals:
			if (idx == 0) {
				gl.BindAttribLocationARB(p, loc, "in_normal");
				CHECK_GL_ERRORS();
			}
		break;
		case kMaterialGeometrySource_Tangents:
			if (idx == 0) {
				gl.BindAttribLocationARB(p, loc, "in_tangent");
				CHECK_GL_ERRORS();
			}
		break;
		case kMaterialGeometrySource_TexCoords: 
			{
				char name[64];
				string::sprintf(name, "in_tc%d", idx);
				gl.BindAttribLocationARB(p, loc, name);
				CHECK_GL_ERRORS();
			}
		break;
		default:
			break;
		}
	}
}

} // r
