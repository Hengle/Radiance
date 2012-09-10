// GLMaterial.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../Material.h"
#include "../../Assets/MaterialParser.h"
#include "GLTexture.h"
#include "GLState.h"
#include "GLShader.h"

namespace r {

void Material::BindStates(int flags, int blends)
{
	int drawFlags = flags|CFM_CCW|CWM_RGBA;

	if (!(flags&r::DWM_Flags))
	{
		if (this->depthWrite)
			drawFlags |= r::DWM_Enable;
		else
			drawFlags |= r::DWM_Disable;
	}

	if (!(flags&r::DT_Flags))
	{
		switch (this->depthFunc)
		{
		case Material::DT_None:
			drawFlags |= r::DT_Disable;
			break;
		case Material::DT_Less:
			drawFlags |= r::DT_Less;
			break;
		case Material::DT_LEqual:
			drawFlags |= r::DT_LEqual;
			break;
		case Material::DT_Greater:
			drawFlags |= r::DT_Greater;
			break;
		case Material::DT_GEqual:
			drawFlags |= r::DT_GEqual;
			break;
		}
	}

	if (!(flags&r::AT_Flags))
	{
		switch (this->alphaTest)
		{
		case Material::AT_None:
			drawFlags |= r::AT_Disable;
			break;
		case Material::AT_Less:
			drawFlags |= r::AT_Less;
			break;
		case Material::AT_LEqual:
			drawFlags |= r::AT_LEqual;
			break;
		case Material::AT_Greater:
			drawFlags |= r::AT_Greater;
			break;
		case Material::AT_GEqual:
			drawFlags |= r::AT_GEqual;
			break;
		}
	}

	gls.AlphaRef(this->alphaVal.get() / 255.f);

	if (!(flags&r::SCT_Flags))
		flags |= SCT_Disable; // disable scissor test unless requested.

	int blendFlags = r::BM_Off;

	if (!(blends&r::BM_Flags))
	{
		switch (this->blendMode)
		{
		case Material::BM_Alpha:
			blendFlags = BMS_SrcAlpha|BMD_InvSrcAlpha;
			break;
		case Material::BM_InvAlpha:
			blendFlags = BMS_InvSrcAlpha|BMD_SrcAlpha;
			break;
		case Material::BM_Additive:
			blendFlags = BMS_One|BMD_One;
			break;
		case Material::BM_AddBlend:
			blendFlags = BMS_SrcAlpha|BMD_One;
			break;
		case Material::BM_Colorize:
			blendFlags = BMS_DstColor|BMD_Zero;
			break;
		case Material::BM_InvColorizeD:
			blendFlags = BMS_InvDstColor|BMD_Zero;
			break;
		case Material::BM_InvColorizeS:
			blendFlags = BMS_Zero|BMD_InvSrcColor;
			break;
		default:
			break;
		}
	}
	else
	{
		blendFlags = blends;
	}

	if (!(flags&r::CFM_Flags))
	{
		if (this->doubleSided)
			drawFlags |= r::CFM_None;
		else
			drawFlags |= r::CFM_Front;
	}

	gls.Set(drawFlags, blendFlags);
}

void Material::BindTextures(const asset::MaterialLoader::Ref &loader)
{
	gls.DisableAllMTSources();
	if (!loader)
		return;

	for (int i = 0; i < MTS_Max; ++i)
	{
		for (int k = 0; k < MTS_MaxIndices; ++k)
		{
			pkg::Asset::Ref a = loader->Texture((MTSource)i, k);
			if (!a)
			{
				if (i == MTS_Texture)
					continue; // allows gaps here (may be procedural texture)
				break;
			}
			GLTextureAsset::Ref tex = GLTextureAsset::Cast(a);
			RAD_ASSERT(tex);

			int index = 0;
			if (tex->numTextures > 1)
			{	
				index = FloatToInt(time.get() * TextureFPS((MTSource)i, k));
				if ((timingMode == TM_Relative) && ClampTextureFrames((MTSource)i, k))
					index = std::min(index, tex->numTextures.get()-1);
				else
					index = index % tex->numTextures.get();
			}
			
			gls.SetMTSource((MTSource)i, k, tex->Texture(index));
		}
	}
}

Shader::Ref Material::ShaderInstance::LoadCooked(
	Engine &engine,
	const char *shaderName,
	stream::InputStream &is,
	bool skinned,
	const Material &material
)
{
	return GLShader::LoadCooked(
		shaderName,
		is,
		skinned,
		material
	);
}

#if defined(RAD_OPT_TOOLS)
Shader::Ref Material::ShaderInstance::Load(
	Engine &engine, 
	bool skinned,
	const Material &material
)
{
	return GLShader::Load(
		engine, 
		material.shaderName, 
		skinned, 
		material, 
		GLShader::GLSL
	);
}
#endif

} // r
