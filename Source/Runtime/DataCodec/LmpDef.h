// LmpDef.h
// Lmp file format API forward defintions.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// See Radiance/LICENSE for licensing terms.

#pragma once
#include "IntDataCodec.h"
#include "../PushPack.h"


namespace data_codec {
namespace lmp {

// Lump Offset.

typedef U32 LOfs;

enum
{
	kMaxEntrySize = kMaxU32,
	kMaxEntryCount = kMaxU32 - 1,
	kMaxLOfs = kMaxU32,
	kMaxLumpNameLen = 511,
	kLumpTagAlignment = 8 // 8 byte alignment
};

class Writer;
class StreamReader;

} // lmp
} // data_codec


#include "../PopPack.h"
