/*! \file MaterialCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/


#include RADPCH
#include "MaterialCooker.h"
#include "MaterialParser.h"
#include <Runtime/Stream.h>
#include "../Engine.h"

using namespace pkg;

namespace asset {

extern const char *s_tcModNames[r::Material::kNumTCMods]; // defined in MaterialParser.cpp

MaterialCooker::MaterialCooker() : Cooker(6) {
}

MaterialCooker::~MaterialCooker() {
}

CookStatus MaterialCooker::Status(int flags) {
	
	// NOTE: Materials must be cooked every time.
	// The reason for this is somewhat stupid. The shader
	// indexes can change as materials are built. This can
	// make them inconsistent.

	return CS_NeedRebuild;
}

int MaterialCooker::Compile(int flags) {

	// manually allocate sinks so we can set some initial flags.

	int r = asset->Process(
		xtime::TimeSlice::Infinite,
		P_SAlloc
	);

	MaterialLoader *loader = MaterialLoader::Cast(asset);
	if (!loader)
		return SR_ParseError;

	loader->shaderOnly = true; // don't load textures
	
	r = asset->Process(
		xtime::TimeSlice::Infinite, 
		flags|P_Parse|P_TargetDefault|P_NoDefaultMedia
	);

	if (r != SR_Success)
		return r;

	MaterialParser *parser = MaterialParser::Cast(asset);
	if (!parser || !parser->valid)
		return SR_ParseError;
	
	// HACK: we shouldn't be hardcoding this here
	String shaderPath("@r:/Cooked/");
	const char *platformName = PlatformNameForFlags(flags);
	if (!platformName)
		return SR_MetaError;
	shaderPath += CStr(platformName);
	shaderPath += CStr("/Shaders");

	int shaderId = parser->material->CookShader(
		shaderPath.c_str,
		*engine.get(),
		flags&pkg::P_AllTargets
	);

	if (shaderId < 0)
		return SR_MetaError;

	String path(CStr(asset->path));
	path += ".bin";
	BinFile::Ref fp = OpenWrite(path.c_str);
	if (!fp)
		return SR_IOError;

	Vec3 rgb;
	Vec4 rgba;

	stream::OutputStream os(fp->ob);
	os << (U16)shaderId;
	os << (U8)(parser->procedural.get() ? 1 : 0);
	os << (U8)parser->material->sort.get();
	os << (U8)parser->material->blendMode.get();
	os << (U8)parser->material->depthFunc.get();
	os << (U8)(parser->material->doubleSided.get() ? 1 : 0);
	os << (U8)(parser->material->depthWrite.get() ? 1 : 0);
	os << (U8)(parser->material->lit.get() ? 1 : 0);
	os << (U8)(parser->material->castShadows.get() ? 1 : 0);
	os << (U8)(parser->material->receiveShadows.get() ? 1 : 0);
	os << (U8)(parser->material->selfShadow.get() ? 1 : 0);
	os << parser->material->specularExponent.get();

	for (int i = 0; i < r::Material::kNumColorIndices; ++i) {
		rgb = parser->material->SpecularColor(i);
		os << (U8)(rgb[0]*255.f);
		os << (U8)(rgb[1]*255.f);
		os << (U8)(rgb[2]*255.f);
	}

	{
		const WaveAnim &w = *parser->material->specularColorWave;
		os << (U8)w.type.get();
		os << w.amplitude.get();
		os << w.freq.get();
		os << w.phase.get();
		os << w.base.get();
	}

	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		String path;
		path.Printf("Texture%d.Source.Texture", i+1);
		const String *s = asset->entry->KeyValue<String>(path.c_str, flags);
		if (!s)
			return SR_MetaError;

		if (s->empty) {
			os << (U8)255;
		} else {
			os << (U8)AddImport(s->c_str);
		}

		os << (U8)parser->material->TCUVIndex(i);
		os << parser->material->TextureFPS(i);
		os << (U8)parser->material->ClampTextureFrames(i);
		os << (U8)parser->material->TCGen(i);

		for (int k = 0; k < r::Material::kNumTCMods; ++k) {
			const WaveAnim &S = parser->material->Wave(i, k, r::Material::kTexCoord_S);
			const WaveAnim &T = parser->material->Wave(i, k, r::Material::kTexCoord_T);

			os << (U8)S.type.get();
			os << (U8)T.type.get();
			os << S.amplitude.get();
			os << T.amplitude.get();
			os << S.freq.get();
			os << T.freq.get();
			os << S.phase.get();
			os << T.phase.get();
			os << S.base.get();
			os << T.base.get();
		}
	}

	for (int i = r::Material::kColor0; i < r::Material::kNumColors; ++i) {
		for (int k = r::Material::kColorA; k < r::Material::kNumColorIndices; ++k) {
			rgba = parser->material->Color(i, k);
			os << (U8)(rgba[0]*255.f);
			os << (U8)(rgba[1]*255.f);
			os << (U8)(rgba[2]*255.f);
			os << (U8)(rgba[3]*255.f);
		}

		const WaveAnim &C = parser->material->ColorWave(i);
		os << (U8)C.type.get();

		os << C.amplitude.get();
		os << C.freq.get();
		os << C.phase.get();
		os << C.base.get();
	}

	return SR_Success;
}

void MaterialCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<MaterialCooker>();
}

} // asset
