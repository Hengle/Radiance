// SoundHeader.h
// Internal AudioCodec Includes.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once
#include "IntAudioCodec.h"

namespace audio_codec {

struct SoundHeader
{
	int channels;
	int rate;
	int bytesPerSample;
	int bytesPerSecond;
	int numBytes;
	int numSamples;
	float duration;
};

BOOST_STATIC_ASSERT(sizeof(SoundHeader)==28);

} // audio_codec

