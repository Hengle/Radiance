/*! \file SkAnimStatesCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "SkAnimStatesCooker.h"
#include "SkAnimStatesParser.h"
#include <Runtime/Stream.h>
#include "../Engine.h"

using namespace pkg;

namespace asset {

SkAnimStatesCooker::SkAnimStatesCooker() : Cooker(4) {
}

SkAnimStatesCooker::~SkAnimStatesCooker() {
}

CookStatus SkAnimStatesCooker::Status(int flags) {
	if (CompareVersion(flags) || 
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

int SkAnimStatesCooker::Compile(int flags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	int r = asset->Process(xtime::TimeSlice::Infinite, flags|P_Load|P_TargetDefault);
	if (r < SR_Success)
		return r;

	SkAnimStatesParser *parser = SkAnimStatesParser::Cast(asset);
	if (!parser)
		return SR_ParseError;

	String path(CStr(asset->path));
	path += ".bin";

	BinFile::Ref fp = OpenWrite(path.c_str);
	if (!fp)
		return SR_IOError;

	stream::OutputStream os(fp->ob);

	const ska::AnimState::Map *states = parser->states;
	if (!states)
		return SR_ParseError;

	os << (U16)states->size();

	for (ska::AnimState::Map::const_iterator it = states->begin(); it != states->end(); ++it) {
		const ska::AnimState &state = it->second;

		os << state.name;
		os << (U16)state.loopCount[0];
		os << (U16)state.loopCount[1];
		os << (U16)state.moveType;
		os << (U16)state.variants.size();

		for (ska::Variant::Vec::const_iterator it = state.variants.begin(); it != state.variants.end(); ++it) {
			const ska::Variant &v = *it;
			os << v.name;
			os << v.timeScale[0];
			os << v.timeScale[1];
			os << (U16)v.loopCount[0];
			os << (U16)v.loopCount[1];
			os << v.weight;
			os << v.in;
			os << v.out;
		}
	}

	return SR_Success;
}

void SkAnimStatesCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<SkAnimStatesCooker>();
}

} // asset
