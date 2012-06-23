// SkAnimStatesCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "SkAnimStatesCooker.h"
#include "SkAnimStatesParser.h"
#include <Runtime/Stream.h>
#include "../Engine.h"

using namespace pkg;

namespace asset {

SkAnimStatesCooker::SkAnimStatesCooker() : Cooker(0)
{
}

SkAnimStatesCooker::~SkAnimStatesCooker()
{
}

CookStatus SkAnimStatesCooker::CheckRebuild(int flags, int allflags)
{
	if (CompareVersion(flags) || 
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

CookStatus SkAnimStatesCooker::Status(int flags, int allflags)
{
	flags &= P_AllTargets;
	allflags &= P_AllTargets;

	if (flags == 0)
	{ // only build generics if all platforms are identical to eachother.
		if (MatchTargetKeys(allflags, allflags)==allflags)
			return CheckRebuild(flags, allflags);
		return CS_Ignore;
	}

	if (MatchTargetKeys(allflags, allflags)==allflags)
		return CS_Ignore;

	// only build ipad if different from iphone
	if ((flags&P_TargetIPad) && (allflags&P_TargetIPhone))
	{
		if (MatchTargetKeys(P_TargetIPad, P_TargetIPhone))
			return CS_Ignore;
	}

	return CheckRebuild(flags, allflags);
}

int SkAnimStatesCooker::Compile(int flags, int allflags)
{
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	int r = asset->Process(xtime::TimeSlice::Infinite, flags|P_Load|P_TargetDefault);
	if (r < SR_Success)
		return r;

	SkAnimStatesParser::Ref parser = SkAnimStatesParser::Cast(asset);
	if (!parser)
		return SR_ParseError;

	String path(CStr(asset->path));
	path += ".bin";

	BinFile::Ref fp = OpenWrite(path.c_str, flags);
	if (!fp)
		return SR_IOError;

	stream::OutputStream os(fp->ob);

	const ska::AnimState::Map *states = parser->states;
	if (!states)
		return SR_ParseError;

	os << (U16)states->size();

	for (ska::AnimState::Map::const_iterator it = states->begin(); it != states->end(); ++it)
	{
		const ska::AnimState &state = it->second;

		os << state.name;
		os << (U16)state.variants.size();

		for (ska::Variant::Vec::const_iterator it = state.variants.begin(); it != state.variants.end(); ++it)
		{
			const ska::Variant &v = *it;
			os << v.name;
			os << v.timeScale[0];
			os << v.timeScale[1];
			os << (S16)v.loopCount[0];
			os << (S16)v.loopCount[1];
			os << v.weight;
			os << v.in;
			os << v.out;
		}
	}

	return SR_Success;
}

int SkAnimStatesCooker::MatchTargetKeys(int flags, int allflags)
{
	return asset->entry->MatchTargetKeys<String>("Source.File", flags, allflags);
}

void SkAnimStatesCooker::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<SkAnimStatesCooker>();
}

} // asset
