// FontCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "FontCooker.h"
#include "../Engine.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

FontCooker::FontCooker() : Cooker(0)
{
}

FontCooker::~FontCooker()
{
}

CookStatus FontCooker::Status(int flags, int allflags)
{
	flags &= P_AllTargets;

	if (flags == 0)
	{
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

int FontCooker::Compile(int flags, int allflags)
{
	// Make sure these get updated (Status may have not called them all)
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	const String *s = asset->entry->KeyValue<String>("Source.File", flags);
	if (!s || s->empty)
		return SR_MetaError;

	int media = file::AllMedia;
	file::HBufferedAsyncIO buf;

	int r = engine->sys->files->LoadFile(
		s->c_str,
		media,
		buf,
		file::HIONotify()
	);

	if (r < SR_Success)
		return r;

	buf->WaitForCompletion();
	if (buf->result < SR_Success)
		return buf->result;

	String path(CStr(asset->path));
	path += ".bin";

	BinFile::Ref file = OpenWrite(path.c_str, flags);
	if (!file)
		return SR_IOError;

	stream::OutputStream os(file->ob);
	if (os.Write(buf->data->ptr, (stream::SPos)buf->data->size.get(), 0) != (stream::SPos)buf->data->size.get())
		return SR_IOError;

	return SR_Success;
}

void FontCooker::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<FontCooker>();
}

} // asset
