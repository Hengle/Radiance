// VorbisDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntAudioCodec.h"

namespace audio_codec {
namespace ogg_vorbis {

enum SampleType
{
	ST_8Bit,
	ST_16Bit
};

enum EndianMode
{
	EM_Little,
	EM_Big
};

enum DataType
{
	DT_Signed,
	DT_Unsigned
};

// for encoding
enum OggState
{
	OS_MoreData,
	OS_Done,
	OS_Error
};

struct BSI
{
	int version;
	int channels;
	int rate;
	int brUpper;
	int brNominal;
	int brLower;
	int brWindow;
};

class Decoder;
class Encoder;
class Comments;

} // ogg_vorbis
} // audio_codec

