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
#if defined(RAD_OPT_DEBUG)
#define LOG_SAVE
#endif
#endif

#undef min
#undef max

namespace r {

#if defined(RAD_OPT_TOOLS)
class GLSLShaderLink : public GLSLTool
{
public:
	GLSLShaderLink(
		Engine &engine,
		cg::Shader::Channel channel, 
		const cg::Shader::Ref &m,
		bool fragment
	) : m_engine(&engine), m_channel(channel), m_m(m), m_fragment(fragment) {}

	virtual bool AddInclude(const char *name, std::ostream &out)
	{
		if (m_fragment && !string::cmp(name, "fragment"))
		{
			return m_m->EmitFunctions(*m_engine, out) &&
				m_m->EmitShader("_Material", m_channel, out);
		}
			
		return true;
	}

private:
	Engine *m_engine;
	cg::Shader::Channel m_channel;
	cg::Shader::Ref m_m;
	bool m_fragment;
};

GLSLShader::Ref GLSLShader::Load(
	Engine &engine, 
	const char *name,
	bool skinned,
	const Material &material
)
{
	cg::Shader::Ref shader = GLShader::Cache()->Load(engine, name);
	if (shader)
	{
		GLSLShader::Ref r(new (ZRender) GLSLShader());
		r->m_name = name;
		bool b = r->CompilePass(
			engine,
			r->m_passes[Shader::P_Default],
			skinned,
			shader,
			cg::Shader::C_Default,
			material
		);

		if (b)
			return r;
	}

	return Ref();
}

bool GLSLShader::CompilePass(
	Engine &engine,
	Pass &p,
	bool skinned,
	const cg::Shader::Ref &s,
	cg::Shader::Channel channel,
	const Material &material
)
{
	p.m = s->InputMappings(channel);
	p.outputs = s->ChannelOutputs(channel);

#if defined(RAD_OPT_OGLES)
	const bool GLES = true;
#else
	const bool GLES = false;
#endif

	GLSLShaderObj::Ref vs;
	GLSLShaderObj::Ref fs;

	GLSLTool::StringVec textureTypes;
	// figure out texture types...
	for (int i = 0; i < GLState::MaxTextures; ++i)
	{
		if (p.m.textures[i][0] == InvalidMapping)
			break;
		switch(p.m.textures[i][0])
		{
		case MTS_Texture:
		case MTS_Framebuffer:
			textureTypes.push_back(String("sampler2D"));
		break;
		}
	}

	{
		std::stringstream ss;

		GLSLShaderLink builder(engine, channel, s, false);
		if (!builder.Assemble(
			engine, 
			true,
			skinned,
			GLES,
			textureTypes,
			s->TexCoordUsage(channel),
			s->ColorUsage(channel),
			0,
			s->NormalUsage(channel),
			&material,
			&p.m,
			true,
			ss)
		)
		{

			COut(C_Error) << "GLSLShader::CompilePass('" << s->name.get() << "', " <<
				channel << ", Vertex): Failed to emit shader code, SHADER ERROR!" << std::endl;
			return false;
		}

		String shader(ss.str().c_str());

#if defined(LOG_DUMP)
		COut(C_Info) << "GLSLShader::CompilePass('" << s->name.get() << "', " <<
			channel << ", Vertex): " << std::endl << shader << std::endl;
#if !defined(RAD_OPT_IOS) && defined(LOG_SAVE)
		{
			String path(CStr("Shaders/"));
			path += s->name;
			path += ".vert.glsl";
			cg::SaveText(engine, path.c_str, shader.c_str);
		}
#endif
#endif

		vs.reset(new (ZRender) GLSLShaderObj(GL_VERTEX_SHADER_ARB));
		const char *sz = shader.c_str;
		gl.ShaderSourceARB(vs->id, 1, &sz, 0);
		CHECK_GL_ERRORS();
		gl.CompileShaderARB(vs->id);
		CHECK_GL_ERRORS();
		
		GLint status;
		gl.GetObjectParameterivARB(vs->id, GL_OBJECT_COMPILE_STATUS_ARB, &status);
		if (!status)
		{ // error. 
			COut(C_ErrMsgBox) << "GLSLShader::CompilePass('" << s->name.get() << "', " <<
				channel << ", Vertex) Error: \n" << ShaderLog(vs->id) << std::endl;
			return false;
		}

#if defined(RAD_OPT_DEBUG)
		COut(C_Debug) << "GLSLShader::CompilePass('" << s->name.get() << "', " << channel <<
			", Vertex) Successfully Compiled. " << std::endl;
#endif
	}

	{
		std::stringstream ss;
		GLSLShaderLink builder(engine, channel, s, true);

		if (!builder.Assemble(
			engine, 
			false,
			false,
			GLES,
			textureTypes,
			s->TexCoordUsage(channel),
			s->ColorUsage(channel),
			0,
			s->NormalUsage(channel),
			0,
			0,
			true,
			ss
		))
		{
			COut(C_Error) << "GLSLShader::CompilePass('" << s->name.get() << "', " <<
				channel << ", Fragment): Failed to emit shader code, SHADER ERROR!" << std::endl;
			return false;
		}

		String shader(ss.str().c_str());

#if defined(LOG_DUMP)
		COut(C_Info) << "GLSLShader::CompilePass('" << s->name.get() << "', " <<
			channel << ", Fragment): " << std::endl << shader << std::endl;
#if !defined(RAD_OPT_IOS) && defined(LOG_SAVE)
		{
			String path(CStr("Shaders/"));
			path += s->name;
			path += ".frag.glsl";
			cg::SaveText(engine, path.c_str, shader.c_str);
		}
#endif
#endif

		fs.reset(new (ZRender) GLSLShaderObj(GL_FRAGMENT_SHADER_ARB));
		const char *sz = shader.c_str;
		gl.ShaderSourceARB(fs->id, 1, &sz, 0);
		CHECK_GL_ERRORS();
		gl.CompileShaderARB(fs->id);
		CHECK_GL_ERRORS();

		GLint status;
		gl.GetObjectParameterivARB(fs->id, GL_OBJECT_COMPILE_STATUS_ARB, &status);
		if (!status)
		{ // error. 
			COut(C_ErrMsgBox) << "GLSLShader::CompilePass('" << s->name.get() << "', " <<
				channel << ", Fragment) Error: \n" << ShaderLog(fs->id) << std::endl;
			return false;
		}

#if defined(RAD_OPT_DEBUG)
		COut(C_Debug) << "GLSLShader::CompilePass('" << s->name.get() << "', " << channel <<
			", Fragment) Successfully Compiled. " << std::endl;
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
			COut(C_ErrMsgBox) << "GLSLShader::CompilePass('" << s->name.get() << "', " <<
				channel << ", Link) Error: " << ProgramLog(r->id) << std::endl;
			return false;
		}
	}

	p.p = r;

	if (!MapInputs(p, material))
	{
		gls.UseProgram(0, true);
		p.p.reset();
		COut(C_ErrMsgBox) << "GLSLShader::CompilePass('" << s->name.get() << "', " <<
			channel << ", Program) MapInputs() failed." << std::endl;
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
)
{
	cg::Shader::Ref s = GLShader::Cache()->Load(engine, m_name.c_str);
	if (!s)
		return false;

	int numPasses = 0;
	for (int i = 0; i < Shader::NumPasses; ++i)
	{
		if (m_passes[i].p)
			++numPasses;
	}

	const U8 MaxTextures = (pflags&(pkg::P_TargetIOS)) ? GLState::MaxIOSTextures : GLState::MaxTextures;
	const U8 MaxAttribs  = (pflags&(pkg::P_TargetIOS)) ? GLState::MaxIOSAttribArrays : GLState::MaxAttribArrays;

	os << (U8)GLShader::GLSL;
	os << (U8)numPasses;

	String vertexSource;
	String fragmentSource;

	const bool GLES = (pflags&(pkg::P_TargetIPhone|pkg::P_TargetIPad)) ? true : false;

	for (int i = 0; i < Shader::NumPasses; ++i)
	{
		cg::Shader::Channel channel = (cg::Shader::Channel)(cg::Shader::C_Default+i);

		Pass &p = m_passes[i];

		if (!p.p)
			continue;

		GLSLTool::StringVec textureTypes;

		// figure out texture types...
		for (int k = 0; k < GLState::MaxAttribArrays; ++k)
		{
			if (p.m.textures[k][0] == InvalidMapping)
				break;
			switch(p.m.textures[k][0])
			{
			case MTS_Texture:
			case MTS_Framebuffer:
				textureTypes.push_back(String("sampler2D"));
			break;
			}
		}

		{
			std::stringstream ss;

			GLSLShaderLink builder(engine, channel, s, false);
			if (!builder.Assemble(
				engine, 
				true,
				false,
				GLES,
				textureTypes,
				s->TexCoordUsage(channel),
				s->ColorUsage(channel),
				0,
				s->NormalUsage(channel),
				&material,
				&p.m,
				true,
				ss)
			)
			{

				COut(C_Error) << "GLSLShader::CompileShaderSource('" << s->name.get() << "', " <<
					channel << ", Vertex): Failed to emit shader code, SHADER ERROR!" << std::endl;
				return false;
			}

			vertexSource = ss.str().c_str();
		}

		{
			std::stringstream ss;
			GLSLShaderLink builder(engine, channel, s, true);

			if (!builder.Assemble(
				engine, 
				false,
				false,
				GLES,
				textureTypes,
				s->TexCoordUsage(channel),
				s->ColorUsage(channel),
				0,
				s->NormalUsage(channel),
				0,
				0,
				true,
				ss
			))
			{
				COut(C_Error) << "GLSLShader::CompileShaderSource('" << s->name.get() << "', " <<
					channel << ", Fragment): Failed to emit shader code, SHADER ERROR!" << std::endl;
				return false;
			}

			fragmentSource = ss.str().c_str();
		}

		os << (U8)i;
		os << (U8)p.outputs;
		os << p.m.numTexs;
		os << p.m.numAttrs;
		os << MaxTextures;
		os << MaxAttribs;

		for (int i = 0; i < MTS_Max; ++i)
			os << p.m.numMTSources[i];
		for (int i = 0; i < MGS_Max; ++i)
			os << p.m.numMGSources[i];
		for (int i = 0; i < MaxTextures; ++i)
		{
			os << p.m.textures[i][0];
			os << p.m.textures[i][1];
		}
		for (int i = 0; i < MaxAttribs; ++i)
		{
			os << p.m.attributes[i][0];
			os << p.m.attributes[i][1];
			os << p.m.attributes[i][2];
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

String GLSLShader::ShaderLog(GLhandleARB s)
{
	enum { MaxLen = Kilo*8 };
	char sz[MaxLen];
	gl.GetInfoLogARB(s, MaxLen, 0, sz);
	return String(sz);
}

String GLSLShader::ProgramLog(GLhandleARB s)
{
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
	bool skinned,
	const Material &material
)
{
	// first byte peeled off by GLShader to select GLSL backend.

	GLSLShader::Ref r(new (ZRender) GLSLShader());
	r->m_name = name;

	U8 numPasses;
	is >> numPasses;

	for (U8 i = 0; i < numPasses; ++i)
	{
		if (!r->LoadPass(name, is, skinned, material))
		{
			r.reset();
			break;
		}
	}

	return r;
}

bool GLSLShader::LoadPass(
	const char *name,
	stream::InputStream &is,
	bool skinned,
	const Material &material
)
{
	U8 passNum;
	is >> passNum;
	if (passNum >= Shader::NumPasses)
		return false;

	Pass &p = m_passes[passNum];
	memset(&p.m, 0, sizeof(p.m));

	U8 outputs, unused;
	U8 MaxTextures, MaxAttribs, MaxTextureOverRead, MaxAttribsOverRead;

	is >> outputs; p.outputs = outputs;
	is >> p.m.numTexs >> p.m.numAttrs;
	is >> MaxTextures >> MaxAttribs;

	MaxTextureOverRead = (MaxTextures>GLState::MaxTextures) ? MaxTextures-GLState::MaxTextures : 0;
	MaxAttribsOverRead  = (MaxAttribs>GLState::MaxAttribArrays) ? MaxAttribs-GLState::MaxAttribArrays : 0;

	MaxTextures = std::min<U8>(MaxTextures, GLState::MaxTextures);
	MaxAttribs  = std::min<U8>(MaxAttribs, GLState::MaxAttribArrays);

	for (int i = 0; i < MTS_Max; ++i)
		is >> p.m.numMTSources[i];
	for (int i = 0; i < MGS_Max; ++i)
		is >> p.m.numMGSources[i];
	for (int i = 0; i < MaxTextures; ++i)
	{
		is >> p.m.textures[i][0];
		is >> p.m.textures[i][1];
	}
	for (U8 i = 0; i < MaxTextureOverRead; ++i)
		is >> unused;
	for (int i = 0; i < MaxAttribs; ++i)
	{
		is >> p.m.attributes[i][0];
		is >> p.m.attributes[i][1];
		is >> p.m.attributes[i][2];
	}
	for (U8 i = 0; i < MaxAttribsOverRead; ++i)
		is >> unused;

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
	if (!status)
	{ // error. 
		COut(C_Error) << "GLSLShader::CompilePass('" << name << "', Vertex) Error:" << std::endl << ShaderLog(vs->id) << std::endl;
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
	if (!status)
	{ // error. 
		COut(C_Error) << "GLSLShader::CompilePass('" << name << "', Fragment) Error:" << std::endl << ShaderLog(fs->id) << std::endl;
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
		if (!status)
		{
			COut(C_Error) << "GLSLShader::CompilePass('" << name << "', Link) Error: " << std::endl << ProgramLog(r->id) << std::endl;
			return false;
		}
	}

	p.p = r;

	if (!MapInputs(p, material))
	{
		gls.UseProgram(0, true);
		p.p.reset();
		COut(C_Error) << "GLSLShader::CompilePass('" << name << "', Program) MapInputs() failed." << std::endl;
		return false;
	}

//	gls.UseProgram(0, true);

	return true;
}

bool GLSLShader::MapInputs(Pass &p, const Material &material)
{
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

	gls.UseProgram(p.p->id, true);

	for (int i = 0; i < GLState::MaxTextures; ++i)
	{
		if (p.m.textures[i][0] != InvalidMapping)
		{
			char sz[64];
			string::sprintf(sz, "U_t%d", i);
			p.u.textures[i] = gl.GetUniformLocationARB(p.p->id, sz);

			if (p.u.textures[i] != -1)
			{ // bind texture units
				gl.Uniform1iARB(p.u.textures[i], i);
			}

			// find texture coordinate index
			int tcIndex = 0;
			for (int k = 0; k < GLState::MaxAttribArrays; ++k)
			{
				if (p.m.attributes[k][0] == InvalidMapping)
					break;
				if (p.m.attributes[k][0] != MGS_TexCoords)
					continue;
				if (p.m.attributes[k][1] == p.m.textures[i][1])
				{
					tcIndex = p.m.attributes[k][2];
					break;
				}
			}

			p.u.tcMods[i][Material::NumTcMods] = -1;
			int tcModUsage = material.TcModFlags((MTSource)p.m.textures[i][0], p.m.textures[i][1]);

			for (int k = 0; k < Material::NumTcMods; ++k)
			{
				if (!(tcModUsage&(1<<k)))
				{ // TcMod is unused by shader.
					p.u.tcMods[i][k] = -1;
					continue;
				}

				string::sprintf(sz, "U_tcmod%d[%d]", tcIndex, k);
				p.u.tcMods[i][k] = gl.GetUniformLocationARB(p.p->id, sz);
				if (p.u.tcMods[i][k] != -1)
				{
					gl.Uniform4fARB(p.u.tcMods[i][k], 0.f, 0.f, 0.f, 0.f);

					for (int j = 0; j < Material::NumTcModVals; ++j)
						p.u.tcModsVals[i][k][j] = 0.f;

					if (k == Material::TcMod_Turb)
					{
						string::sprintf(sz, "U_tcmod%d[%d]", tcIndex, Material::NumTcMods);
						p.u.tcMods[i][Material::NumTcMods] = gl.GetUniformLocationARB(p.p->id, sz);
						RAD_VERIFY(p.u.tcMods[i][Material::NumTcMods] != -1);
						gl.Uniform4fARB(p.u.tcMods[i][Material::NumTcMods], 0.f, 0.f, 0.f, 0.f);
					}
				}
			}
		}
		else
		{
			p.u.textures[i] = -1;
			for (int k = 0; k < Material::NumTcMods; ++k)
				p.u.tcMods[i][k] = -1;
		}
	}

	return true;
}

GLSLShader::GLSLShader() : m_curMat(0)
{
}

int GLSLShader::Usage(Shader::Pass p, MTSource source) const
{
	RAD_ASSERT(source >= 0 && source < MTS_Max);
	return HasPass(p) ? m_passes[p].m.numMTSources[source] : 0;
}

int GLSLShader::Usage(Shader::Pass p, MGSource source) const
{
	RAD_ASSERT(source >= 0 && source < MGS_Max);
	return HasPass(p) ? m_passes[p].m.numMGSources[source] : 0;
}

int GLSLShader::Outputs(Shader::Pass p) const
{
	return HasPass(p) ? m_passes[p].outputs : 0;
}

bool GLSLShader::HasPass(Shader::Pass p) const
{
	RAD_ASSERT(p >= 0 && p < Shader::NumPasses);
	return m_passes[p].p;
}

bool GLSLShader::Requires(Shader::Pass _p, MTSource source, int index) const
{
	if (!HasPass(_p))
		return false;
	const Pass &p = m_passes[_p];

	for (int i = 0; i < GLState::MaxTextures; ++i)
	{
		if (p.m.textures[i][0] == InvalidMapping)
			break;
		if (p.m.textures[i][0] == source && p.m.textures[i][1] == index)
			return true;
	}

	return false;
}

bool GLSLShader::Requires(Shader::Pass _p, MGSource source, int index) const
{
	if (!HasPass(_p))
		return false;
	const Pass &p = m_passes[_p];

	for (int i = 0; i < GLState::MaxAttribArrays; ++i)
	{
		if (p.m.attributes[i][0] == InvalidMapping)
			break;
		if (p.m.attributes[i][0] == source && p.m.attributes[i][1] == index)
			return true;
	}

	return false;
}


void GLSLShader::Begin(Shader::Pass _p, const Material &material)
{
	RAD_ASSERT(_p >= Shader::P_First && _p < Shader::NumPasses);
	Pass &p = m_passes[_p];
	RAD_ASSERT(p.p);
	m_curPass = _p;
	m_curMat = &material;

	gls.UseProgram(p.p->id, true);

	// bind material textures
	gls.DisableTextures();
	for (int i = 0; i < GLState::MaxTextures; ++i)
	{
		if (p.m.textures[i][0] == InvalidMapping)
			break;

		GLTextureRef r = gls.MTSource((MTSource)p.m.textures[i][0], (int)p.m.textures[i][1]);
		if (r)
			gls.SetTexture(i, r);

		// material contains animated tcMods.
		for (int k = 0; k < Material::NumTcMods; ++k)
		{
			if (p.u.tcMods[i][k] == -1)
				continue; // unreferenced by shader

			float st[Material::NumTcModVals];
			float *src = p.u.tcModsVals[i][k];
			int ops;

			material.Sample((MTSource)p.m.textures[i][0], (int)p.m.textures[i][1], k, ops, st);

			if (src[0] != st[0] || src[1] != st[1] || src[2] != st[2] || src[3] != st[3])
			{
				RAD_ASSERT(p.u.tcMods[i][k] != -1);
				gl.Uniform4fARB(p.u.tcMods[i][k], st[0], st[1], st[2], st[3]);
				src[0] = st[0];
				src[1] = st[1];
				src[2] = st[2];
				src[3] = st[3];
			}

			// pack in extra turb arguments
			if (k == Material::TcMod_Turb && (src[4] != st[4] || src[5] != st[5]))
			{
				RAD_ASSERT(p.u.tcMods[i][Material::NumTcMods] != -1);
				gl.Uniform4fARB(p.u.tcMods[i][Material::NumTcMods], st[4], st[5], 0.f, 0.f);
				src[4] = st[4];
				src[5] = st[5];
			}
		}
	}
	
#if defined(RAD_OPT_OGLES2) && defined(RAD_OPT_DEBUG)
	GLint logLen, status;
	glValidateProgram(p.p->id);
	glGetProgramiv(p.p->id, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0)
	{
		GLchar *log = (GLchar*)stack_alloc(logLen);
		glGetProgramInfoLog(p.p->id, logLen, &logLen, log);
		COut(C_Debug) << "GLSLShader: '" << m_name.c_str() << "': Validate Log: " << log << std::endl;
	}
	
	glGetProgramiv(p.p->id, GL_VALIDATE_STATUS, &status);
	RAD_ASSERT(status!=0);
#endif
}

// NVIDIA cards reserve index 3 for gl_Color and will produce an error if it's used
// Remap this.

inline int RemapIndex(int idx)
{
#if defined(RAD_OPT_PC)
	if (idx == 3)
		return GLState::MaxAttribArrays-1;
#endif
	return idx;
}

void GLSLShader::BindStates(bool sampleMaterialColor, const Vec4 &rgba)
{
	RAD_ASSERT(m_curMat);
	Pass &p = m_passes[m_curPass];
	
	float c[4];

	if (sampleMaterialColor)
	{
		m_curMat->SampleColor(Material::Color0, c);
		c[0] *= rgba[0];
		c[1] *= rgba[1];
		c[2] *= rgba[2];
		c[3] *= rgba[3];

#if defined(RAD_OPT_PC)
		gl.Color4f(c[0], c[1], c[2], c[3]);
#else
	}
	else
	{
		gl.GetColor4fv(c);
#endif
	}
	
#if defined(RAD_OPT_OGLES2)
	if (p.u.matrixOps != gl.matrixOps)
	{ // matrix has changed
		p.u.matrixOps = gl.matrixOps;
		// track modelview projection matrix?
		if (p.u.mvp != -1)
		{
			float floats[16];
			Mat4 *x = reinterpret_cast<Mat4*>(floats);
			*x = gl.GetModelViewProjectionMatrix();
			x->Transpose();
			if (memcmp(floats, p.u.mvpfloats, 16*sizeof(float)))
			{
				memcpy(p.u.mvpfloats, floats, 16*sizeof(float));
				gl.UniformMatrix4fvARB(p.u.mvp, 1, GL_FALSE, floats);
			}
		}
	}
	if (p.u.color != -1)
	{
		if (p.u.rgba[0] != c[0] ||
			p.u.rgba[1] != c[1] ||
			p.u.rgba[2] != c[2] ||
			p.u.rgba[3] != c[3])
		{
			p.u.rgba[0] = c[0];
			p.u.rgba[1] = c[1];
			p.u.rgba[2] = c[2];
			p.u.rgba[3] = c[3];
			gl.Uniform4fARB(p.u.color, c[0], c[1], c[2], c[3]);
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
	MGSource lastSource = MGS_Max;

	gls.DisableVertexAttribArrays();
	
	for (int i = 0 ; i < GLState::MaxAttribArrays; ++i)
	{
		if (p.m.attributes[i][0] == InvalidMapping)
			break;

		gls.MGSource(
			(MGSource)p.m.attributes[i][0],
			(int)p.m.attributes[i][1],
			vb,
			size,
			type,
			normalized,
			stride,
			ofs
		);

		if (lastSource != p.m.attributes[i][0])
		{
			lastSource = (MGSource)p.m.attributes[i][0];
			sourceVB = vb;
			sourceSize = size;
			sourceType = type;
			sourceNormalized = normalized;
			sourceStride = stride;
			sourceOfs = ofs;
		}

		int loc = RemapIndex(i);

		if (vb)
		{
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
		}
		else if (sourceVB)
		{ // some values like 2nd UV channel aren't provided
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

void GLSLShader::End()
{
	m_curMat = 0;
	gls.UseProgram(0);
}

void GLSLShader::BindAttribLocations(GLhandleARB p, const GLState::MInputMappings &m)
{
	for (int i = 0; i < GLState::MaxAttribArrays; ++i)
	{
		if (m.attributes[i][0] == InvalidMapping)
			break;

		MGSource s = (MGSource)m.attributes[i][0];
		GLint idx  = (int)m.attributes[i][1];

		int loc = RemapIndex(i);

		switch (s)
		{
		case MGS_Vertices:
			if (idx == 0)
			{
				gl.BindAttribLocationARB(p, loc, "in_position");
				CHECK_GL_ERRORS();
			}
		break;
		case MGS_Normals:
			if (idx == 0)
			{
				gl.BindAttribLocationARB(p, loc, "in_normal");
				CHECK_GL_ERRORS();
			}
		break;
		case MGS_Binormals:
			if (idx == 0)
			{
				gl.BindAttribLocationARB(p, loc, "in_binormal");
				CHECK_GL_ERRORS();
			}
		break;
		case MGS_Tangents:
			if (idx == 0)
			{
				gl.BindAttribLocationARB(p, loc, "in_tangent");
				CHECK_GL_ERRORS();
			}
		break;
		case MGS_TexCoords:
			{
				char name[64];
				string::sprintf(name, "in_tc%d", (int)m.attributes[i][2]);
				gl.BindAttribLocationARB(p, loc, name);
				CHECK_GL_ERRORS();
			}
		break;
		case MGS_Weights:
			if (idx == 0)
			{
				gl.BindAttribLocationARB(p, loc, "in_weights");
				CHECK_GL_ERRORS();
			}
			if (idx == 1)
			{
				gl.BindAttribLocationARB(p, loc, "in_indices");
				CHECK_GL_ERRORS();
			}
		break;
		}
	}
}

} // r
