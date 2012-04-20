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
	MaxEntrySize = MaxU32,
	MaxEntryCount = MaxU32 - 1,
	MaxLOfs = MaxU32,
	MaxLumpNameLen = 511,
	LumpTagAlignment = 8 // 8 byte alignment
};

class Writer;
class StreamReader;

} // lmp
} // data_codec


#include "../PopPack.h"
