// MaterialCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "MaterialCooker.h"
#include "MaterialParser.h"
#include <Runtime/Stream.h>
#include "../Engine.h"

using namespace pkg;

namespace asset {

extern const char *s_tcModNames[r::Material::kNumTCMods]; // defined in MaterialParser.cpp

MaterialCooker::MaterialCooker() : Cooker(2) {
}

MaterialCooker::~MaterialCooker() {
}

CookStatus MaterialCooker::Status(int flags, int allflags) {
	flags &= P_AllTargets;
	allflags &= P_AllTargets;

	// NOTE: Materials must be cooked every time.
	// The reason for this is somewhat stupid. The shader
	// indexes can change as materials are built. This can
	// make them inconsistent.

	// The Shader indexes are different for GLES targets
	// (i.e. they produce different shader code), so even
	// though the material keys match we still need to cook
	// them seperately.

	// only build ipad if different from iphone
	if ((flags&P_TargetIPad) && (allflags&P_TargetIPhone)) {
		if (MatchTargetKeys(P_TargetIPad, P_TargetIPhone))
			return CS_Ignore;
		return CS_NeedRebuild;
	}

	// if iphone/ipad is selected, and we have non-gles targets, then cook
	if ((allflags&(P_TargetIPhone|P_TargetIPad)) && (allflags&~(P_TargetIPhone|P_TargetIPad)))
		return CS_NeedRebuild; // all different

	if (flags == 0) { 
		// only build generics if all platforms are identical to eachother.
		// && we have all GLES or non GLES targets.
		if ((allflags&(P_TargetIPhone|P_TargetIPad)) && (allflags&~(P_TargetIPhone|P_TargetIPad)))
			return CS_Ignore; // no generics (all different)
		return MatchTargetKeys(allflags, allflags)==allflags ? CS_NeedRebuild : CS_Ignore;
	}

	if (MatchTargetKeys(flags, allflags)==allflags)
		return CS_Ignore;

	return CS_NeedRebuild;
}

int MaterialCooker::Compile(int flags, int allflags) {
	int r = asset->Process(
		xtime::TimeSlice::Infinite, 
		flags|P_Parse|P_TargetDefault|P_NoDefaultMedia
	);

	if (r != SR_Success)
		return r;

	MaterialParser::Ref parser = MaterialParser::Cast(asset);
	if (!parser || !parser->valid)
		return SR_ParseError;
	
	int shaderTarget = flags&pkg::P_AllTargets;
	if (shaderTarget == 0) {   
		// if we are cooking the generics path, we need to explicity set the target
		// for GLES variants.
		// NOTE: we will never cook the generics path if we have mixed GLES/non ES targets
		// See Status() above for that logic.
		if (allflags&(P_TargetIPhone|P_TargetIPad))
			shaderTarget = P_TargetIPhone;
	}

	int shaderId = parser->material->CookShader(
		"@r:/Cooked/Out/Shaders",
		*engine.get(),
		shaderTarget
	);

	if (shaderId < 0)
		return SR_MetaError;

	String path(CStr(asset->path));
	path += ".bin";
	BinFile::Ref fp = OpenWrite(path.c_str, flags);
	if (!fp)
		return SR_IOError;

	stream::OutputStream os(fp->ob);
	os << (U16)shaderId;
	os << (U8)(parser->procedural.get() ? 1 : 0);
	os << (U8)parser->material->sort.get();
	os << (U8)parser->material->blendMode.get();
	os << (U8)parser->material->depthFunc.get();
	os << (U8)(parser->material->doubleSided.get() ? 1 : 0);
	os << (U8)(parser->material->depthWrite.get() ? 1 : 0);

	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		String path;
		path.Printf("Texture%d.Source.Texture", i+1);
		const String *s = asset->entry->KeyValue<String>(path.c_str, flags);
		if (!s)
			return SR_MetaError;

		if (s->empty) {
			os << (U8)255;
		} else {
			os << (U8)AddImport(s->c_str, flags);
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
			float rgba[4];
			parser->material->Color(i, k, rgba);
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

int MaterialCooker::MatchTargetKeys(int flags, int allflags) {
	int x = asset->entry->MatchTargetKeys<String>("Source.Shader", flags, allflags)&
		asset->entry->MatchTargetKeys<String>("Sort", flags, allflags)&
		asset->entry->MatchTargetKeys<String>("BlendMode", flags, allflags)&
		asset->entry->MatchTargetKeys<bool>("DoubleSided", flags, allflags)&
		asset->entry->MatchTargetKeys<bool>("DepthWrite", flags, allflags)&
		asset->entry->MatchTargetKeys<String>("DepthFunc", flags, allflags);

	if (!x)
		return 0;

	String path;
	String z;

	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		path.Printf("Texture%d.Source.Texture", i+1);
		x &= asset->entry->MatchTargetKeys<String>(path.c_str, flags, allflags);
		path.Printf("Texture%d.Source.uvChannel", i+1);
		x &= asset->entry->MatchTargetKeys<String>(path.c_str, flags, allflags);
		path.Printf("Texture%d.Source.FramesPerSecond", i+1);
		x &= asset->entry->MatchTargetKeys<String>(path.c_str, flags, allflags);
		path.Printf("Texture%d.tcGen", i+1);
		x &= asset->entry->MatchTargetKeys<String>(path.c_str, flags, allflags);

		if (!x)
			return 0;

		for (int k = 0; k < r::Material::kNumTCMods; ++k) {
			path.Printf("Texture%d.tcMod.%s", i+1, s_tcModNames[k]);
			z = path + ".Type";
			x &= asset->entry->MatchTargetKeys<String>(z.c_str, flags, allflags);
			z = path + ".Amplitude";
			x &= asset->entry->MatchTargetKeys<String>(z.c_str, flags, allflags);
			z = path + ".Frequency";
			x &= asset->entry->MatchTargetKeys<String>(z.c_str, flags, allflags);
			z = path + ".Phase";
			x &= asset->entry->MatchTargetKeys<String>(z.c_str, flags, allflags);
			z = path + ".Base";
			x &= asset->entry->MatchTargetKeys<String>(z.c_str, flags, allflags);

			if (!x)
				return 0;
		}
	}

	for (int i = r::Material::kColor0; i < r::Material::kNumColors; ++i) {
		for (int k = r::Material::kColorA; k < r::Material::kNumColorIndices; ++k) {
			path.Printf("Color%d.%c", i, 'A'+k);
			x &= asset->entry->MatchTargetKeys<String>(path.c_str, flags, allflags);
		}

		if (!x)
			return 0;

		path.Printf("Color%d.Gen", i);
		z = path + ".Type";
		x &= asset->entry->MatchTargetKeys<String>(z.c_str, flags, allflags);
		z = path + ".Amplitude";
		x &= asset->entry->MatchTargetKeys<String>(z.c_str, flags, allflags);
		z = path + ".Frequency";
		x &= asset->entry->MatchTargetKeys<String>(z.c_str, flags, allflags);
		z = path + ".Phase";
		x &= asset->entry->MatchTargetKeys<String>(z.c_str, flags, allflags);
		z = path + ".Base";
		x &= asset->entry->MatchTargetKeys<String>(z.c_str, flags, allflags);
	}

	return x;
}

void MaterialCooker::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<MaterialCooker>();
}

} // asset
