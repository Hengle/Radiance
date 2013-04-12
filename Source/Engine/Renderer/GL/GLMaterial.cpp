/*! \file GLMaterial.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#include RADPCH
#include "../Material.h"
#include "../../Assets/MaterialParser.h"
#include "GLTexture.h"
#include "GLState.h"
#include "GLShader.h"

namespace r {

void Material::BindStates(int flags, int blends) {
	int drawFlags = flags;

	if (!(flags&kColorWriteMask_Flags)) {
		drawFlags |= kColorWriteMask_RGBA;
	}

	if (!(flags&kDepthWriteMask_Flags)) {
		if (this->depthWrite)
			drawFlags |= kDepthWriteMask_Enable;
		else
			drawFlags |= kDepthWriteMask_Disable;
	}

	if (!(flags&kDepthTest_Flags)) {
		switch (this->depthFunc) {
		case Material::kDepthFunc_None:
			drawFlags |= kDepthTest_Disable;
			break;
		case Material::kDepthFunc_Less:
			drawFlags |= kDepthTest_Less;
			break;
		case Material::kDepthFunc_LEqual:
			drawFlags |= kDepthTest_LEqual;
			break;
		case Material::kDepthFunc_Greater:
			drawFlags |= kDepthTest_Greater;
			break;
		case Material::kDepthFunc_GEqual:
			drawFlags |= kDepthTest_GEqual;
			break;
		case Material::kDepthFunc_Equal:
			drawFlags |= kDepthTest_Equal;
			break;
		}
	}

	if (!(flags&kScissorTest_Flags))
		drawFlags |= kScissorTest_Disable; // disable scissor test unless requested.

	int blendFlags = kBlendMode_Off;

	if (!(blends&kBlendMode_Flags)) {
		switch (this->blendMode) {
		case Material::kBlendMode_Alpha:
			blendFlags = kBlendModeSource_SrcAlpha|kBlendModeDest_InvSrcAlpha;
			break;
		case Material::kBlendMode_InvAlpha:
			blendFlags = kBlendModeSource_InvSrcAlpha|kBlendModeDest_SrcAlpha;
			break;
		case Material::kBlendMode_Additive:
			blendFlags = kBlendModeSource_One|kBlendModeDest_One;
			break;
		case Material::kBlendMode_AddBlend:
			blendFlags = kBlendModeSource_SrcAlpha|kBlendModeDest_One;
			break;
		case Material::kBlendMode_Colorize:
			blendFlags = kBlendModeSource_DstColor|kBlendModeDest_Zero;
			break;
		case Material::kBlendMode_InvColorizeD:
			blendFlags = kBlendModeSource_InvDstColor|kBlendModeDest_Zero;
			break;
		case Material::kBlendMode_InvColorizeS:
			blendFlags = kBlendModeSource_Zero|kBlendModeDest_InvSrcColor;
			break;
		default:
			break;
		}
	} else {
		blendFlags = blends;
	}

	if (!(flags&kCullFaceMode_Flags)) {
		if (this->doubleSided)
			drawFlags |= kCullFaceMode_None;
		else
			drawFlags |= kCullFaceMode_Back|kCullFaceMode_CCW;
	}

	gls.Set(drawFlags, blendFlags);
}

void Material::BindTextures(asset::MaterialLoader *loader) {
	gls.DisableAllMTSources();

	if (!loader)
		return;
	
	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		pkg::Asset::Ref a = loader->Texture(i);
		if (!a)
			continue; // allows gaps here (may be a procedural texture)
		GLTextureAsset *tex = GLTextureAsset::Cast(a);
		RAD_ASSERT(tex);

		int index = 0;
		if (tex->numTextures > 1) {	
			index = FloatToInt(time.get() * TextureFPS(i));
			if ((timingMode == kTimingMode_Relative) && ClampTextureFrames(i))
				index = std::min(index, tex->numTextures.get()-1);
			else
				index = index % tex->numTextures.get();
		}
			
		gls.SetMTSource(kMaterialTextureSource_Texture, i, tex->Texture(index));
	}
}

Shader::Ref Material::ShaderInstance::LoadCooked(
	Engine &engine,
	const char *shaderName,
	stream::InputStream &is,
	const Material &material
) {
	return GLShader::LoadCooked(
		shaderName,
		is,
		material
	);
}

#if defined(RAD_OPT_TOOLS)
Shader::Ref Material::ShaderInstance::Load(
	Engine &engine, 
	const Material &material
) {
	return GLShader::Load(
		engine, 
		material.shaderName, 
		material, 
		GLShader::kBackend_GLSL
	);
}
#endif

} // r
