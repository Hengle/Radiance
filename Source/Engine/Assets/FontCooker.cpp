// FontCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "FontCooker.h"
#include "../Engine.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

FontCooker::FontCooker() : Cooker(0) {
}

FontCooker::~FontCooker() {
}

CookStatus FontCooker::Status(int flags, int allflags) {
	flags &= P_AllTargets;

	if (flags == 0) {
		if (CompareVersion(flags) ||
			CompareModifiedTime(flags) ||
			CompareCachedFileTimeKey(flags, "Source.File"))
		{
			return CS_NeedRebuild;
		}

		return CS_UpToDate;
	}

	return CS_Ignore;
}

int FontCooker::Compile(int flags, int allflags) {
	// Make sure these get updated (Status may have not called them all)
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	const String *s = asset->entry->KeyValue<String>("Source.File", flags);
	if (!s || s->empty)
		return SR_MetaError;

	if (!CopyOutputBinFile(s->c_str, flags))
		return SR_IOError;
	return SR_Success;
}

void FontCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<FontCooker>();
}

} // asset
