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

#undef min
#undef max

#include <Runtime/PushSystemMacros.h>

namespace r {

#if defined(RAD_OPT_TOOLS)

class GLSLShaderLink : public tools::shader_utils::GLSLTool {
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
	tools::shader_utils::Shader::Ref shader = GLShader::Cache()->Load(engine, name, material);
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

	for (int i = Shader::kNumPasses-1; i >= 0; --i) {
		if (shader->Exists((Shader::Pass)i)) {
			
			MaterialInputMappings m;
			tools::shader_utils::Shader::TexCoordMapping tcMapping;
			
			if (!shader->BuildInputMappings(material, (Shader::Pass)i, m, tcMapping))
				return false;

			int numVaryings = CalcNumShaderVaryings(
				(Shader::Pass)i,
				shader,
				m,
				tcMapping,
				material
			);

			if (numVaryings <= gl.maxVaryings) {
				if (!CompilePass(engine, (Shader::Pass)i, shader, m, tcMapping, numVaryings, material))
					return false;
				++numPasses;
			}
		}
	}

	return numPasses > 0;
}

bool GLSLShader::CompilePass(
	Engine &engine,
	Shader::Pass pass,
	const tools::shader_utils::Shader::Ref &shader,
	const MaterialInputMappings &m,
	const tools::shader_utils::Shader::TexCoordMapping &tcMapping,
	int numVaryingFloats,
	const Material &material
) {
	Pass &p = m_passes[pass];

	memcpy(&p.m, &m, sizeof(MaterialInputMappings));
	p.outputs = shader->PassOutputs(pass);
	p.numReqVaryings = numVaryingFloats;

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
			tcMapping,
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
	}

	{
		std::stringstream ss;

		GLSLShaderLink builder(engine, material, pass, shader, true);
		if (!builder.Assemble(
			engine, 
			material,
			shader,
			p.m,
			tcMapping,
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

int GLSLShader::CalcNumShaderVaryings(
	Shader::Pass pass,
	const tools::shader_utils::Shader::Ref &shader,
	const MaterialInputMappings &m,
	const tools::shader_utils::Shader::TexCoordMapping &tcMapping,
	const Material &material
) {

	int numFloats = 0;

	int numVertices = shader->MaterialSourceUsage(pass, tools::shader_utils::Shader::kMaterialSource_Vertex);
	numFloats += numVertices*4;

	int numTexCoords = 0;
	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		if (m.tcMods[i] == r::kInvalidMapping)
			break;
		++numTexCoords;
	}

	numFloats += numTexCoords*4;

	int numLightVec = shader->MaterialSourceUsage(pass, tools::shader_utils::Shader::kMaterialSource_LightVec);
	numFloats += numLightVec*3;

	int numLightHalfVec = shader->MaterialSourceUsage(pass, tools::shader_utils::Shader::kMaterialSource_LightHalfVec);
	numFloats += numLightHalfVec*3;

	int numLightVertex = shader->MaterialSourceUsage(pass, tools::shader_utils::Shader::kMaterialSource_LightVertex);
	numFloats += numLightVertex*4;

	int numLightTanVec = shader->MaterialSourceUsage(pass, tools::shader_utils::Shader::kMaterialSource_LightTanVec);
	numFloats += numLightTanVec*3;

	int numLightTanHalfVec = shader->MaterialSourceUsage(pass, tools::shader_utils::Shader::kMaterialSource_LightTanHalfVec);
	numFloats += numLightTanHalfVec*3;
	
	int numShaderNormals = shader->MaterialSourceUsage(pass, tools::shader_utils::Shader::kMaterialSource_Normal);
	numFloats += numShaderNormals*3;

	int numShaderTangents = shader->MaterialSourceUsage(pass, tools::shader_utils::Shader::kMaterialSource_Tangent);
	numFloats += numShaderTangents*3;

	int numShaderBitangents = shader->MaterialSourceUsage(pass, tools::shader_utils::Shader::kMaterialSource_Bitangent);
	numFloats += numShaderBitangents*3;

	return numFloats;
}

#endif // defined(RAD_OPT_TOOLS)

#if defined(RAD_OPT_PC_TOOLS)

bool GLSLShader::CompileShaderSource(
	Engine &engine,
	stream::OutputStream &os, 
	int pflags,
	const Material &material
) {
	tools::shader_utils::Shader::Ref shader = 
		GLShader::Cache()->Load(engine, m_name.c_str, material);
	if (!shader)
		return false;

	int numPasses = 0;
	for (int i = 0; i < Shader::kNumPasses; ++i) {
		if (i == Shader::kPass_Preview)
			continue; // don't cook this.

		if (shader->Exists((Shader::Pass)i)) {
			++numPasses;
		}
	}

	os << (U8)GLShader::kBackend_GLSL;
	os << (U8)numPasses;
	
	String vertexSource;
	String fragmentSource;

	tools::shader_utils::GLSLTool::AssembleFlags kGLESFlag = tools::shader_utils::GLSLTool::kAssemble_None;
	if (pflags&pkg::P_TargetiOS)
		kGLESFlag = tools::shader_utils::GLSLTool::kAssemble_GLES;

	for (int i = 0; i < Shader::kNumPasses; ++i) {
		if (i == Shader::kPass_Preview)
			continue; // don't cook this.

		if (!shader->Exists((r::Shader::Pass)i))
			continue;

		MaterialInputMappings m;
		tools::shader_utils::Shader::TexCoordMapping tcMapping;

		if (!shader->BuildInputMappings(material, (r::Shader::Pass)i, m, tcMapping))
			return false;

		const int kNumVaryings = CalcNumShaderVaryings(
			(Shader::Pass)i,
			shader,
			m,
			tcMapping,
			material
		);

		{
			std::stringstream ss;

			GLSLShaderLink builder(engine, material, (Shader::Pass)i, shader, false);
			if (!builder.Assemble(
				engine, 
				material,
				shader,
				m,
				tcMapping,
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

			GLSLShaderLink builder(engine, material, (Shader::Pass)i, shader, true);
			if (!builder.Assemble(
				engine, 
				material,
				shader,
				m,
				tcMapping,
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
		os << (U8)shader->PassOutputs((r::Shader::Pass)i);
		os << (U16)kNumVaryings;

		for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
			os << m.tcMods[i];
		}

		for (int i = 0; i < kNumMaterialTextureSources; ++i) {
			os << m.numMTSources[i];
		}

		for (int i = 0; i < kNumMaterialGeometrySources; ++i) {
			os << m.numMGSources[i];
		}

		for (int i = 0; i < kMaxTextures; ++i) {
			os << m.textures[i][0];
			os << m.textures[i][1];
		}
		
		for (int i = 0; i < kMaxAttribArrays; ++i) {
			os << m.attributes[i][0];
			os << m.attributes[i][1];
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
	enum { MaxLen = kKilo*8 };
	char sz[MaxLen];
	gl.GetInfoLogARB(s, MaxLen, 0, sz);
	return String(sz);
}

String GLSLShader::ProgramLog(GLhandleARB s) {
#if defined(RAD_OPT_OGLES)
	enum { MaxLen = kKilo*8 };
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

	U16 numVaryings;
	is >> numVaryings; p.numReqVaryings = (int)numVaryings;

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
	if (maxLength > 64*kKilo)
		return false; // size check
	
	char *source = (char*)stack_alloc(maxLength);
	if (is.Read(source, (stream::SPos)programLength[0], 0) != (stream::SPos)programLength[0])
		return false;

	if (p.numReqVaryings > gl.maxVaryings) {
		if (passNum == Shader::kPass_Default) {
			COut(C_Error) << "GLSLShader::LoadPass:\"" << name << "\" kPass_Default exceeds platform varying limit (" << p.numReqVaryings << " > " << gl.maxVaryings << ")!" << std::endl;
			return false;
		}
		is.Read(source, (stream::SPos)programLength[1], 0);
		return true; // we can't use this program on this device, skip.
	}
	
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

	p.u.matrixOps = 0;
	p.u.prMatrixOps = 0;

#if defined(RAD_OPT_OGLES2)
	p.u.mvp = gl.GetUniformLocationARB(p.p->id, "U_mvp");
	for (int i = 0; i < 16; ++i)
		p.u.mvpfloats[i] = 0.f;
#endif
	p.u.dcolor = gl.GetUniformLocationARB(p.p->id, "U_color");
	p.u.drgba = Vec4(-1.f, -1.f, -1.f, -1.f);
	p.u.scolor = gl.GetUniformLocationARB(p.p->id, "U_scolor");
	p.u.srgba = Vec4(-1.f, -1.f, -1.f, -1.f);
	p.u.mv = gl.GetUniformLocationARB(p.p->id, "U_mv");
	p.u.pr = gl.GetUniformLocationARB(p.p->id, "U_pr");
	for (int i = 0; i < 16; ++i) {
		p.u.mvfloats[i] = 0.f;
		p.u.prfloats[i] = 0.f;
	}

	const float kFloatMax = std::numeric_limits<float>::max();

	p.u.eye = gl.GetUniformLocationARB(p.p->id, "U_eye");
	p.u.eyePos[0] = p.u.eyePos[1] = p.u.eyePos[2] = kFloatMax;

	p.u.tcPrj = gl.GetUniformLocationARB(p.p->id, "U_tcPrj");
	p.u.tcPrjMat = Mat4::Identity;

	p.u.lights.numLights = 0;

	for (int i = 0; i < kMaxLights; ++i) {
		p.u.lights.lights[i].diffuse = Vec3(kFloatMax, kFloatMax, kFloatMax);
		p.u.lights.lights[i].specular = Vec4(kFloatMax, kFloatMax, kFloatMax, kFloatMax);
		p.u.lights.lights[i].pos = Vec3(kFloatMax, kFloatMax, kFloatMax);
		p.u.lights.lights[i].flags = 0;
	}
	
	for (int i = 0; i < kMaxLights; ++i) {
		char sz[64];

		string::sprintf(sz, "U_light%d_pos", i);
		p.u.lightPos[i] = gl.GetUniformLocationARB(p.p->id, sz);

		string::sprintf(sz, "U_light%d_diffuseColor", i);
		p.u.lightDiffuse[i] = gl.GetUniformLocationARB(p.p->id, sz);

		string::sprintf(sz, "U_light%d_specularColor", i);
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
	
	// init sprite verts
	for (int i = 0; i < 4; ++i) {
		char sz[64];
		string::sprintf(sz, "U_spriteVerts[%d]", i);
		int idx = gl.GetUniformLocationARB(p.p->id, sz);
		if (idx == -1)
			break;
		gl.Uniform2fARB(
			idx,
			(i < 2) ? -0.5f : 0.5f,
			(i == 1 || i == 2) ? -0.5f : 0.5f
		);
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
	RAD_STATIC_ASSERT(sizeof(Mat4) == (sizeof(float)*16));

	RAD_ASSERT(m_curMat);
	Pass &p = m_passes[m_curPass];
	
	Vec4 dcolor;
	Vec4 scolor;

	if (sampleMaterialColor) {
		dcolor = m_curMat->SampleColor(Material::kColor0);
		dcolor *= uniforms.blendColor;
		scolor = Vec4(m_curMat->SampleSpecularColor() * uniforms.blendColor, m_curMat->specularExponent);
	} else {
		scolor = Vec4(1.f, 1.f, 1.f, m_curMat->specularExponent);
		gl.GetColor4fv(&dcolor[0]);
	}

	if (p.u.eye != -1) {
		if (uniforms.eyePos != p.u.eyePos) {
			p.u.eyePos = uniforms.eyePos;
			gl.Uniform3fvARB(p.u.eye, 1, &uniforms.eyePos[0]);
		}
	}

	if (p.u.tcPrj != -1) {
		if (memcmp(&p.u.tcPrjMat, &uniforms.tcGen, sizeof(Mat4))) {
			p.u.tcPrjMat = uniforms.tcGen;
			gl.UniformMatrix4fvARB(p.u.tcPrj, 1, GL_FALSE, (const float*)&uniforms.tcGen);
		}
	}

	// shader has to be provided all inputs!
	RAD_ASSERT(p.u.lights.numLights <= uniforms.lights.numLights);

	for (int i = 0; i < p.u.lights.numLights; ++i) {
		const LightDef &srcLight = uniforms.lights.lights[i];
		LightDef &dstLight = p.u.lights.lights[i];

		if ((p.u.lightPos[i] != -1) && ((srcLight.pos != dstLight.pos) || (srcLight.radius != dstLight.radius))) {
			dstLight.pos = srcLight.pos;
			dstLight.radius = srcLight.radius;
			gl.Uniform4fARB(p.u.lightPos[i], srcLight.pos[0], srcLight.pos[1], srcLight.pos[2], srcLight.radius);
		}

		if ((p.u.lightDiffuse[i] != -1) && ((srcLight.diffuse != dstLight.diffuse) || (srcLight.intensity != dstLight.intensity))) {
//			RAD_ASSERT(srcLight.flags & LightDef::kFlag_Diffuse);
			dstLight.diffuse = srcLight.diffuse;
			dstLight.intensity = srcLight.intensity;
			gl.Uniform4fARB(p.u.lightDiffuse[i], srcLight.diffuse[0], srcLight.diffuse[1], srcLight.diffuse[2], srcLight.intensity);
		}

		if ((p.u.lightSpecular[i] != -1) && (srcLight.specular != dstLight.specular)) {
			RAD_ASSERT(srcLight.flags & LightDef::kFlag_Specular);
			dstLight.specular = srcLight.specular;
			gl.Uniform3fvARB(p.u.lightSpecular[i], 1, &srcLight.specular[0]);
		}

		dstLight.flags = srcLight.flags;
	}
	
	if (p.u.matrixOps != gl.matrixOps) { 
		// matrix has changed
		p.u.matrixOps = gl.matrixOps;
#if defined(RAD_OPT_OGLES2)
		// track modelview projection matrix?
		if (p.u.mvp != -1) {
			float floats[16];
			Mat4 *x = reinterpret_cast<Mat4*>(floats);
			*x = gl.GetModelViewProjectionMatrix();
			if (memcmp(floats, p.u.mvpfloats, 16*sizeof(float))) {
				memcpy(p.u.mvpfloats, floats, 16*sizeof(float));
				gl.UniformMatrix4fvARB(p.u.mvp, 1, GL_FALSE, floats);
			}
		}
#endif
		// track modelview matrix?
		if (p.u.mv != -1) {
			float floats[16];
			Mat4 *x = reinterpret_cast<Mat4*>(floats);
			*x = gl.GetModelViewMatrix();
			if (memcmp(floats, p.u.mvfloats, 16*sizeof(float))) {
				memcpy(p.u.mvfloats, floats, 16*sizeof(float));
				gl.UniformMatrix4fvARB(p.u.mv, 1, GL_FALSE, floats);
			}
		}
	}

	if (p.u.prMatrixOps != gl.prMatrixOps) {
		p.u.prMatrixOps = gl.prMatrixOps;
		// track projection matrix?
		if (p.u.pr != -1) {
			float floats[16];
			Mat4 *x = reinterpret_cast<Mat4*>(floats);
			*x = gl.GetProjectionMatrix();
			if (memcmp(floats, p.u.prfloats, 16*sizeof(float))) {
				memcpy(p.u.prfloats, floats, 16*sizeof(float));
				gl.UniformMatrix4fvARB(p.u.pr, 1, GL_FALSE, floats);
			}
		}
	}
	if (p.u.dcolor != -1) {
		if (p.u.drgba != dcolor) {
			p.u.drgba = dcolor;
			gl.Uniform4fvARB(p.u.dcolor, 1, &dcolor[0]);
		}
	}
	if (p.u.scolor != -1) {
		if (p.u.srgba != scolor) {
			p.u.srgba = scolor;
			gl.Uniform4fvARB(p.u.scolor, 1, &scolor[0]);
		}
	}

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
				gl.BindAttribLocationARB(p, loc, "in_nm0");
				CHECK_GL_ERRORS();
			}
		break;
		case kMaterialGeometrySource_Tangents:
			if (idx == 0) {
				gl.BindAttribLocationARB(p, loc, "in_tan0");
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
		case kMaterialGeometrySource_VertexColor:
			if (idx == 0) {
				gl.BindAttribLocationARB(p, loc, "in_vertexColor");
				CHECK_GL_ERRORS();
			}
		break;
		case kMaterialGeometrySource_SpriteSkin:
			if (idx == 0) {
				gl.BindAttribLocationARB(p, loc, "in_spriteSkin");
				CHECK_GL_ERRORS();
			}
		break;
		default:
			break;
		}
	}
}

} // r
