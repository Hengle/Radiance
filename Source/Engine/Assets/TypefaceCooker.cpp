/*! \file TypefaceCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "TypefaceCooker.h"
#include "MaterialParser.h"
#include "../App.h"
#include "../Engine.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

TypefaceCooker::TypefaceCooker() : Cooker(0) {
}

TypefaceCooker::~TypefaceCooker() {
}

CookStatus TypefaceCooker::Status(int flags) {
	if (CompareVersion(flags) || CompareModifiedTime(flags))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

int TypefaceCooker::Compile(int flags) {
	
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);

	const String *s = asset->entry->KeyValue<String>("Source.Font", flags);
	if (!s || s->empty)
		return SR_MetaError;

	// import font.
	AddImport(s->c_str);

	s = asset->entry->KeyValue<String>("Source.Material", flags);
	if (!s || s->empty)
		return SR_MetaError;

	Asset::Ref matRef = App::Get()->engine->sys->packages->Resolve(s->c_str, asset->zone);
	if (!matRef)
		return SR_FileNotFound;

	int r = matRef->Process(
		xtime::TimeSlice::Infinite,
		flags|P_Info|P_TargetDefault
	);

	if (r != SR_Success)
		return r;

	MaterialParser *matParser = MaterialParser::Cast(matRef);
	if (!matParser)
		return SR_MetaError;
	if (!matParser->procedural) {
		cout.get() << "ERROR: Typeface materials must be marked as procedural!" << std::endl;
		return SR_MetaError;
	}
	
	// import material
	AddImport(s->c_str);

	int w, h;

	const int *i = asset->entry->KeyValue<int>("Typeface.Width", flags);
	if (!i)
		return SR_MetaError;
	w = *i;

	i = asset->entry->KeyValue<int>("Typeface.Height", flags);
	if (!i)
		return SR_MetaError;
	h = *i;

	if (w < 1 || h < 1)
		return SR_MetaError;

	// save width/height to tag
	BinFile::Ref f = OpenTagWrite();
	if (!f)
		return SR_IOError;

	stream::OutputStream os(f->ob);

	os << (U16)w << (U16)h;

	return SR_Success;
}

void TypefaceCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<TypefaceCooker>();
}

} // asset
