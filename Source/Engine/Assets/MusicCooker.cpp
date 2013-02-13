// MusicCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "MusicCooker.h"
#include <Runtime/AudioCodec/Wave.h>
#include <Runtime/Stream.h>
#include <Runtime/Endian.h>
#include "../Engine.h"

using namespace pkg;
using namespace endian;

namespace asset {

MusicCooker::MusicCooker() : Cooker(0)
{
}

MusicCooker::~MusicCooker()
{
}

CookStatus MusicCooker::CheckRebuild(int flags, int allflags)
{
	if (CompareVersion(flags) ||
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

CookStatus MusicCooker::Status(int flags, int allflags)
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

	return CheckRebuild(flags, allflags);
}

int MusicCooker::Compile(int flags, int allflags)
{
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	const String *s = asset->entry->KeyValue<String>("Source.File", flags);
	if (!s)
		return SR_MetaError;

	if (!CopyOutputBinFile(s->c_str, flags))
		return SR_IOError;
	return SR_Success;
}

int MusicCooker::MatchTargetKeys(int flags, int allflags) {
	return asset->entry->MatchTargetKeys<String>("Source.File", flags, allflags);
}

void MusicCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<MusicCooker>();
}

} // asset
