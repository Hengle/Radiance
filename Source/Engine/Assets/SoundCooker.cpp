// SoundCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SoundCooker.h"
#include <Runtime/AudioCodec/Wave.h>
#include <Runtime/Stream.h>
#include <Runtime/Endian.h>
#include "../Engine.h"
#include <algorithm>
#undef min
#undef max

using namespace pkg;
using namespace endian;

namespace asset {

SoundCooker::SoundCooker() : Cooker(0)
{
}

SoundCooker::~SoundCooker()
{
}

CookStatus SoundCooker::CheckRebuild(int flags, int allflags)
{
	if (CompareVersion(flags) ||
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

CookStatus SoundCooker::Status(int flags, int allflags)
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

int SoundCooker::Compile(int flags, int allflags)
{
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	const String *s = asset->entry->KeyValue<String>("Source.File", flags);
	if (!s)
		return SR_MetaError;

	file::HStreamInputBuffer ib;
	int media = file::AllMedia;
	
	int r = engine->sys->files->OpenFileStream(
		s->c_str,
		media,
		ib,
		file::HIONotify()
	);

	if (r != SR_Success)
		return r;

	stream::InputStream is(ib->buffer);
	audio_codec::wave::Decoder decoder;

	if (!decoder.Initialize(is))
		return SR_InvalidFormat;

	String path(CStr(asset->path));
	path += ".bin";
	BinFile::Ref fp = OpenWrite(path.c_str, flags);
	if (!fp)
		return SR_IOError;

	stream::OutputStream os(fp->ob);

	audio_codec::SoundHeader header;
	header.channels = SwapLittle(decoder.header->channels);
	header.rate = SwapLittle(decoder.header->rate);
	header.bytesPerSample = SwapLittle(decoder.header->bytesPerSample);
	header.numBytes = SwapLittle(decoder.header->numBytes);
	header.numSamples = SwapLittle(decoder.header->numSamples);
	header.duration = SwapLittle(decoder.header->duration);

	if (os.Write(&header, sizeof(header), 0) != sizeof(header))
		return SR_IOError;

	enum { BlockSize = 8*Kilo };
	char block[BlockSize];

	AddrSize size = 0;
	AddrSize numBytes = (AddrSize)decoder.header->numBytes;

	while(numBytes && decoder.Decode(block, BlockSize, size))
	{
		size = std::min(numBytes, size);

		if (os.Write(block, (stream::SPos)size, 0) != (stream::SPos)size)
			return SR_IOError;

		numBytes -= size;
	}

	return numBytes ? SR_InvalidFormat : SR_Success;
}

int SoundCooker::MatchTargetKeys(int flags, int allflags)
{
	return asset->entry->MatchTargetKeys<String>("Source.File", flags, allflags);
}

void SoundCooker::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<SoundCooker>();
}

} // asset
