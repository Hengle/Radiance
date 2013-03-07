/*! \file MusicCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "MusicCooker.h"
#include <Runtime/AudioCodec/Wave.h>
#include <Runtime/Stream.h>
#include <Runtime/Endian.h>
#include "../Engine.h"

using namespace pkg;
using namespace endian;

namespace asset {

MusicCooker::MusicCooker() : Cooker(0) {
}

MusicCooker::~MusicCooker() {
}

CookStatus MusicCooker::Status(int flags) {
	if (CompareVersion(flags) ||
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

int MusicCooker::Compile(int flags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	const String *s = asset->entry->KeyValue<String>("Source.File", flags);
	if (!s)
		return SR_MetaError;

	if (!CopyOutputBinFile(s->c_str))
		return SR_IOError;
	return SR_Success;
}

void MusicCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<MusicCooker>();
}

} // asset
