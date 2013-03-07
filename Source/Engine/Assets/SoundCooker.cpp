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

SoundCooker::SoundCooker() : Cooker(0) {
}

SoundCooker::~SoundCooker() {
}

CookStatus SoundCooker::Status(int flags) {
	if (CompareVersion(flags) ||
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

int SoundCooker::Compile(int flags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	const String *s = asset->entry->KeyValue<String>("Source.File", flags);
	if (!s)
		return SR_MetaError;

	file::MMFileInputBuffer::Ref ib = engine->sys->files->OpenInputBuffer(s->c_str, ZTools);
	if (!ib)
		return SR_FileNotFound;
	
	stream::InputStream is(*ib);
	audio_codec::wave::Decoder decoder;

	if (!decoder.Initialize(is))
		return SR_InvalidFormat;

	String path(CStr(asset->path));
	path += ".bin";
	BinFile::Ref fp = OpenWrite(path.c_str);
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

	while(numBytes && decoder.Decode(block, BlockSize, size)) {
		size = std::min(numBytes, size);

		if (os.Write(block, (stream::SPos)size, 0) != (stream::SPos)size)
			return SR_IOError;

		numBytes -= size;
	}

	return numBytes ? SR_InvalidFormat : SR_Success;
}

void SoundCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<SoundCooker>();
}

} // asset
