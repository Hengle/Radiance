// GLSLTool.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#include "GLSLTool.h"
#include "../../Engine.h"
#include "../../COut.h"
#include "../Material.h"
#include <Runtime/Stream/STLStream.h>
#include <Runtime/StringBase.h>
#include "../../../../../Extern/glsl-optimizer/src/glsl/glsl_optimizer.h"
#include <sstream>
#include <Runtime/PushSystemMacros.h>

namespace tools {
namespace shader_utils {

bool GLSLTool::Assemble(
	Engine &engine,
	const r::Material &material,
	const Shader::Ref &shader,
	const r::MaterialInputMappings &mapping,
	const Shader::TexCoordMapping &tcMapping,
	r::Shader::Pass pass,
	AssembleFlags flags,
	std::ostream &out
) {
	std::stringstream ss;

	const bool GLES = (flags & kAssemble_GLES) ? true : false;

	if (!(flags&(kAssemble_VertexShader|kAssemble_PixelShader)))
		flags |= kAssemble_VertexShader;

	bool vertexShader = (flags & kAssemble_VertexShader) ? true : false;

	if (!GLES)
		ss << "#version 120\r\n";

	if (vertexShader) {
		ss << "#define VERTEX\r\n";
	} else {
		ss << "#define FRAGMENT\r\n";
		ss << "#define MATERIAL\r\n";
	}

	switch (shader->precisionMode) {
	case Shader::kPrecision_Low:
		ss << "#define PFLOAT FIXED\r\n";
		ss << "#define PFLOAT2 FIXED2\r\n";
		ss << "#define PFLOAT3 FIXED3\r\n";
		ss << "#define PFLOAT4 FIXED4\r\n";
		ss << "#define PFLOAT4X4 FIXED4X4\r\n";
		break;
	case Shader::kPrecision_Medium:
		ss << "#define PFLOAT HALF\r\n";
		ss << "#define PFLOAT2 HALF2\r\n";
		ss << "#define PFLOAT3 HALF3\r\n";
		ss << "#define PFLOAT4 HALF4\r\n";
		ss << "#define PFLOAT4X4 HALF4X4\r\n";
		break;
	case Shader::kPrecision_High:
		ss << "#define PFLOAT FLOAT\r\n";
		ss << "#define PFLOAT2 FLOAT2\r\n";
		ss << "#define PFLOAT3 FLOAT3\r\n";
		ss << "#define PFLOAT4 FLOAT4\r\n";
		ss << "#define PFLOAT4X4 FLOAT4X4\r\n";
		break;
	}

	if (pass != r::Shader::kPass_Preview) {
		if (material.skinMode == r::Material::kSkinMode_Sprite)
			ss << "#define SKIN_SPRITE\r\n";
		if (material.skinMode == r::Material::kSkinMode_Billboard)
			ss << "#define SKIN_BILLBOARD\r\n";
	}

	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_Color) > 0)
		ss << "#define SHADER_COLOR\r\n";

	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_VertexColor) > 0)
		ss << "#define SHADER_VERTEX_COLOR\r\n";

	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_Vertex) > 0)
		ss << "#define SHADER_POSITION\r\n";

	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_MV) > 0)
		ss << "#define SHADER_MV\r\n";
	
	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_PRJ) > 0)
		ss << "#define SHADER_PRJ\r\n";

	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_MVP) > 0)
		ss << "#define SHADER_MVP\r\n";

	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_InverseMV) > 0)
		ss << "#define SHADER_INVERSE_MV\r\n";

	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_InverseMVP) > 0)
		ss << "#define SHADER_INVERSE_MVP\r\n";

	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_InversePRJ) > 0)
		ss << "#define SHADER_INVERSE_PRJ\r\n";

	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_PFXVars) > 0)
		ss << "#define PFX_VARS\r\n";
		
	if (shader->MaterialSourceUsage(pass, Shader::kMaterialSource_EyeVertex) > 0)
		ss << "#define SHADER_EYE_VERTEX\r\n";

	int numTextures = 0;
	for (int i = 0; i < r::kMaxTextures; ++i) {
		if (mapping.textures[i][0] == r::kInvalidMapping)
			break; // no more texture bindings
		switch (mapping.textures[i][0]) {
		case r::kMaterialTextureSource_Texture:
			ss << "#define T" << i << "TYPE sampler2D" << "\r\n";
			break;
		default:
			break;
		}
		switch(shader->SamplerPrecisionMode(i)) {
		case Shader::kPrecision_Low:
			ss << "#define T" << i << "PRECISION lowp" << "\r\n";
			break;
		case Shader::kPrecision_Medium:
			ss << "#define T" << i << "PRECISION mediump" << "\r\n";
			break;
		case Shader::kPrecision_High:
			ss << "#define T" << i << "PRECISION highp" << "\r\n";
			break;
		}
		++numTextures;
	}

	if (numTextures > 0)
		ss << "#define TEXTURES " << numTextures << "\r\n";
	
	const Shader::IntSet &tcUsage = shader->AttributeUsage(pass, Shader::kMaterialSource_TexCoord);
	int numTexCoords = (int)tcUsage.size();
	if (numTexCoords > 0) {
		// Shaders files can read from r::kMaxTexture unique texture coordinate slots.
		// The set of read texcoord registers may be sparse. Additionally the engine only supports
		// 2 UV channels in most model data. We do some work here to map the set of read texcoord
		// registers onto the smallest possible set:
		//
		// A tcMod may take an input UV channel and modify it, meaning that texture channel is unique,
		// however there are a lot of cases where the tcMod is identity and therefore a texture coordinate
		// register in a shader can be mapped to a commmon register.

		int numTCMods = 0;
		for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
			if (mapping.tcMods[i] == r::kInvalidMapping)
				break;
			++numTCMods;
		}
		
		// numTCMods is the number of unique texture coordinates read by the pixel shader.
		// (which have to be generated by the vertex shader).

		ss << "#define TEXCOORDS " << numTCMods << "\r\n";

		RAD_VERIFY(tcUsage.size() == tcMapping.size());

		if (vertexShader) {
			Shader::IntSet tcInputs;
			
			bool genReflect = false;
			bool genProject = false;

			for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
				if (mapping.tcMods[i] == r::kInvalidMapping)
					break;
				int tcIndex = (int)mapping.tcMods[i];
				tcInputs.insert(material.TCUVIndex(tcIndex));

				int tcMod = material.TCModFlags(tcIndex);
				if (tcMod & r::Material::kTCModFlag_Rotate)
					ss << "#define TEXCOORD" << i << "_ROTATE\r\n";
				if (tcMod & r::Material::kTCModFlag_Scale)
					ss << "#define TEXCOORD" << i << "_SCALE\r\n";
				if (tcMod & r::Material::kTCModFlag_Shift)
					ss << "#define TEXCOORD" << i << "_SHIFT\r\n";
				if (tcMod & r::Material::kTCModFlag_Scroll)
					ss << "#define TEXCOORD" << i << "_SCROLL\r\n";
				if (tcMod & r::Material::kTCModFlag_Turb)
					ss << "#define TEXCOORD" << i << "_TURB\r\n";

				int tcGen = material.TCGen(tcIndex);
				if (tcGen == r::Material::kTCGen_EnvMap) {
					if (!genReflect) {
						genReflect = true;
						ss << "#define GENREFLECT\r\n";
					}
					ss << "#define TEXCOORD" << i << "_GENREFLECT\r\n";
				} else if (tcGen == r::Material::kTCGen_Projected) {
					if (!genProject) {
						genProject = true;
						ss << "#define GENPROJECT\r\n";
					}
					ss << "#define TEXCOORD" << i << "_GENPROJECT\r\n";
				}
			}

			ss << "#define TCINPUTS " << tcInputs.size() << "\r\n";

			for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
				if (mapping.tcMods[i] == r::kInvalidMapping)
					break;
				int tcIndex = (int)mapping.tcMods[i];
				int uvIndex = material.TCUVIndex(tcIndex);

				int ofs = 0;
				for (Shader::IntSet::const_iterator it2 = tcInputs.begin(); it2 != tcInputs.end(); ++it2) {
					if (uvIndex == *it2)
						break;
					++ofs;
				}

				RAD_VERIFY(ofs < (int)tcInputs.size());
				ss << "#define TEXCOORD" << i << " tc" << ofs << "\r\n";
			}
		} else {
			// fragment shader inputs used generated tc's, which may have expanded from the
			// the vertex shader inputs.
			int ofs = 0;
			for (Shader::IntSet::const_iterator it = tcUsage.begin(); it != tcUsage.end(); ++it, ++ofs) {
				ss << "#define TEXCOORD" << ofs << " tc" << tcMapping[ofs].first << "\r\n";
			}
		}
	}
	
	int numColors = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_Color);

	if (numColors > 0)
		ss << "#define COLORS " << numColors << "\r\n";

	int numSpecularColors = std::max(
		shader->MaterialSourceUsage(pass, Shader::kMaterialSource_SpecularColor),
		shader->MaterialSourceUsage(pass, Shader::kMaterialSource_SpecularExponent)
	);
	if (numSpecularColors > 0)
		ss << "#define SHADER_SPECULAR_COLORS " << numSpecularColors << "\r\n";

	int numLightPos = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_LightPos);
	int numLightVec = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_LightVec);
	int numLightHalfVec = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_LightHalfVec);
	int numLightVertex = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_LightVertex);
	int numLightTanVec = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_LightTanVec);
	int numLightTanHalfVec = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_LightTanHalfVec);
	int numLightDiffuseColor = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_LightDiffuseColor);
	int numLightSpecularColor = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_LightSpecularColor);

	int numShaderNormals = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_Normal);
	int numShaderTangents = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_Tangent);
	int numShaderBitangents = shader->MaterialSourceUsage(pass, Shader::kMaterialSource_Bitangent);

	int numNormals = numShaderNormals;
	int numTangents = numShaderTangents;
	int numBitangents = numShaderBitangents;

	bool needTangents = vertexShader && (numLightTanVec || numLightTanHalfVec);
	if (needTangents) {
		numNormals = std::max(1, numShaderNormals);
		numTangents = std::max(1, numShaderTangents);
		numBitangents = std::max(1, numShaderBitangents);

		ss << "#define TANGENT_FRAME" << "\r\n";
	}

	if (numNormals > 0)
		ss << "#define NORMALS " << numNormals << "\r\n";
	if (numTangents > 0)
		ss << "#define TANGENTS " << numTangents << "\r\n";
	if (numBitangents > 0)
		ss << "#define BITANGENTS " << numBitangents << "\r\n";

	if (numShaderNormals > 0)
		ss << "#define NUM_SHADER_NORMALS " << numShaderNormals << "\r\n";
	if (numShaderTangents > 0)
		ss << "#define NUM_SHADER_TANGENTS " << numShaderTangents << "\r\n";
	if (numShaderBitangents > 0)
		ss << "#define NUM_SHADER_BITANGENTS " << numShaderBitangents << "\r\n";

	int numLights = std::max(
		numLightPos, 
		std::max(
			numLightVec, 
			std::max(
				numLightHalfVec,
				std::max(
					numLightVertex,
					std::max(
						numLightTanVec,
						std::max(
							numLightTanHalfVec,
							std::max(numLightDiffuseColor, numLightSpecularColor)
						)
					)
				)
			)
		)
	);

	if (numLights > 0) {
		ss << "#define LIGHTS " << numLights << "\r\n";
		if (numLightPos > 0)
			ss << "#define SHADER_LIGHT_POS " << numLightPos << "\r\n";
		if (numLightVec > 0)
			ss << "#define SHADER_LIGHT_VEC " << numLightVec << "\r\n";
		if (numLightHalfVec > 0)
			ss << "#define SHADER_LIGHT_HALFVEC " << numLightHalfVec << "\r\n";
		if (numLightVertex > 0)
			ss << "#define SHADER_LIGHT_VERTEXPOS " << numLightVertex << "\r\n";
		if (numLightTanVec > 0)
			ss << "#define SHADER_LIGHT_TANVEC " << numLightTanVec << "\r\n";
		if (numLightTanHalfVec > 0)
			ss << "#define SHADER_LIGHT_TANHALFVEC " << numLightTanHalfVec << "\r\n";
		if (numLightDiffuseColor > 0)
			ss << "#define SHADER_LIGHT_DIFFUSE_COLOR " << numLightDiffuseColor << "\r\n";
		if (numLightSpecularColor > 0)
			ss << "#define SHADER_LIGHT_SPECULAR_COLOR " << numLightSpecularColor << "\r\n";
	}

	if (GLES) {
		ss << "#define _GLES\r\n";
		ss << "#define MOBILE\r\n";
	}
	
	if (!Inject(engine, "@r:/Source/Shaders/Nodes/GLSL.c", ss))
		return false;
	if (!Inject(engine, "@r:/Source/Shaders/Nodes/Common.c", ss))
		return false;
	if (!Inject(engine, "@r:/Source/Shaders/Nodes/Shader.c", ss))
		return false;

	std::stringstream ex;
	if (!ExpandIncludes(*this, ss, ex))
		return false;

	if (flags & kAssemble_Optimize) {
		String in(ex.str());

		glslopt_ctx *glslopt = glslopt_initialize(GLES);

		glslopt_shader *opt_shader = glslopt_optimize(
			glslopt,
			vertexShader ? kGlslOptShaderVertex : kGlslOptShaderFragment,
			in.c_str, 
			0
		);

		char szPass[32];
		sprintf(szPass, "_pass%d", (int)pass);
		
		{
			engine.sys->files->CreateDirectory("@r:/Temp/Shaders/Logs");
			String path(CStr("@r:/Temp/Shaders/Logs/"));
			path += shader->name;
			path += szPass;
			path += "_unoptimized";
			if (vertexShader) {
				path += ".vert.glsl";
			} else {
				path += ".frag.glsl";
			}
			tools::shader_utils::SaveText(engine, path.c_str, in.c_str);
		}

		if (!glslopt_get_status(opt_shader)) {
			COut(C_Error) << "Error optimizing shader: " << std::endl << in << std::endl << glslopt_get_log(opt_shader) << std::endl;
			engine.sys->files->CreateDirectory("@r:/Temp/Shaders/Logs");

			String path;
			for (int i = 0;; ++i) {
				path.PrintfASCII("@r:/Temp/Shaders/Logs/%s_error_%d.log", shader->name.get(), i);
				if (!engine.sys->files->FileExists(path.c_str)) {
					SaveText(engine, path.c_str, in.c_str);
					break;
				}
			}
			glslopt_shader_delete(opt_shader);
			glslopt_cleanup(glslopt);
			return false;
		}

		std::stringstream z;
		if (GLES) {
			switch (shader->precisionMode) {
			case Shader::kPrecision_Low:
				z << "precision lowp float;\r\n";
				break;
			case Shader::kPrecision_Medium:
				z << "precision mediump float;\r\n";
				break;
			case Shader::kPrecision_High:
				z << "precision highp float;\r\n";
				break;
			}
		}
		z << glslopt_get_output(opt_shader);
		glslopt_shader_delete(opt_shader);
		glslopt_cleanup(glslopt);

		Copy(z, out);

		{
			engine.sys->files->CreateDirectory("@r:/Temp/Shaders/Logs");
			String path(CStr("@r:/Temp/Shaders/Logs/"));
			path += shader->name;
			path += szPass;
			path += "_optimized";
			if (vertexShader) {
				path += ".vert.glsl";
			} else {
				path += ".frag.glsl";
			}
			tools::shader_utils::SaveText(engine, path.c_str, z.str().c_str());
		}
	} else {
		Copy(ex, out);
	}

	return true;
}

} // shader_utils
} // tools

