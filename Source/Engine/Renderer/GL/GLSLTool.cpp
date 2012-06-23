// GLSLTool.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if defined(RAD_OPT_TOOLS)

#include "GLSLTool.h"
#include "../../Engine.h"
#include "../../COut.h"
#include "../RendererDef.h"
#include "../Material.h"
#include <sstream>
#include <Runtime/Stream/STLStream.h>
#include <Runtime/StringBase.h>
#include "../../../../Extern/glsl-optimizer/v1/src/glsl/glsl_optimizer.h"

namespace r {

bool GLSLTool::Assemble(
	Engine &engine,
	bool vertex,
	bool skinned,
	bool gles,
	const StringVec &textureTypes,
	int numTexCoords,
	int numColors,
	int numUColors,
	int numNormals,
	const Material *material,
	const GLState::MInputMappings *mapping,
	bool optimize,
	std::ostream &out
)
{
	std::stringstream ss;
	if (vertex)
	{
		ss << "#define VERTEX" << "\r\n";
	}
	else
	{
		ss << "#define FRAGMENT" << "\r\n";
		ss << "#define MATERIAL" << "\r\n";
	}

	if (textureTypes.size() > 0)
	{
		ss << "#define TEXTURES " << textureTypes.size() << "\r\n";
		for (size_t i = 0; i < textureTypes.size(); ++i)
		{
			char sz[64];
			string::sprintf(sz, "#define T%dTYPE %s", i, textureTypes[i].c_str.get());
			ss << sz << "\r\n";
		}
	}
	if (numTexCoords > 0)
	{
		ss << "#define TEXCOORDS " << numTexCoords << "\r\n";

		if (vertex)
		{
			RAD_ASSERT(material&&mapping);
			for (int i = 0; i < numTexCoords; ++i)
			{
				// TODO: this won't work when I add MTS_Framebuffer"
#pragma message ("TODO: this won't work when I add MTS_Framebuffer")
				int txIndex = -1;
				for (int k = 0; k < GLState::MaxAttribArrays; ++k)
				{
					if (mapping->attributes[k][0] == MGS_TexCoords &&
						mapping->attributes[k][2] == i)
					{
						txIndex = mapping->attributes[k][1];
						break;
					}
				}

				if (txIndex < 0)
					continue;
				
				RAD_ASSERT(mapping->textures[txIndex][0] != InvalidMapping);
				int flags = material->TcModFlags((MTSource)mapping->textures[txIndex][0], mapping->textures[txIndex][1]);

				if (flags&Material::TcModFlag_Rotate)
					ss << "#define TEXCOORD" << i << "_ROTATE\r\n";
				if (flags&Material::TcModFlag_Turb)
					ss << "#define TEXCOORD" << i << "_TURB\r\n";
				if (flags&Material::TcModFlag_Scale)
					ss << "#define TEXCOORD" << i << "_SCALE\r\n";
				if (flags&Material::TcModFlag_Shift)
					ss << "#define TEXCOORD" << i << "_SHIFT\r\n";
				if (flags&Material::TcModFlag_Scroll)
					ss << "#define TEXCOORD" << i << "_SCROLL\r\n";

				int tcGen = material->TcGen((MTSource)mapping->textures[txIndex][0], mapping->textures[txIndex][1]);

				if (tcGen == Material::TcGen_EnvMap)
					ss << "#define TEXCOORD" << i << "_GENREFLECT\r\n";
			}
		}
	}
	if (numColors > 0)
		ss << "#define COLORS " << numColors << "\r\n";
	if (numUColors > 0)
		ss << "#define UCOLORS " << numUColors << "\r\n";
	if (numNormals > 0)
		ss << "#define NORMALS " << numNormals << "\r\n";
	if (gles)
		ss << "#define _GLES\r\n";
	
	if (!cg::Inject(engine, "Shaders/Nodes/GLSL.cg", ss))
		return false;
	if (!cg::Inject(engine, "Shaders/Nodes/Common.cg", ss))
		return false;
	if (!cg::Inject(engine, "Shaders/Nodes/Shader.cg", ss))
		return false;
//	cg::SaveText(engine, L"Materials/Nodes/glsltool.0", ss.str().c_str());

	std::stringstream ex;
	if (!cg::ExpandIncludes(*this, ss, ex))
		return false;
//	COut(C_Debug) << ex.str() << std::endl;
//	cg::SaveText(engine, L"Materials/Nodes/glsltool.1", ex.str().c_str());

	if (optimize)
	{
		String in(ex.str());

		glslopt_shader *shader = glslopt_optimize(
			gles ? gl.glslopt_es : gl.glslopt,
			vertex ? kGlslOptShaderVertex : kGlslOptShaderFragment,
			in.c_str, 
			0
		);

		if (!glslopt_get_status(shader))
		{
			COut(C_Error) << "Error optimizing shader: " << std::endl << in << std::endl << glslopt_get_log(shader) << std::endl;
			glslopt_shader_delete(shader);
			return false;
		}

		std::stringstream z;
		if (gles)
		{
			if (vertex)
				z << "precision mediump float;\r\n";
			else
				z << "precision lowp float;\r\n";
		}
		z << glslopt_get_output(shader);
		glslopt_shader_delete(shader);

		cg::Copy(z, out);
	}
	else
	{
		cg::Copy(ex, out);
	}

	return true;
}

} // r

#endif // RAD_OPT_TOOLS
