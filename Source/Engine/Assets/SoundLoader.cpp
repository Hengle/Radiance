// SoundLoader.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SoundLoader.h"
#include "SoundParser.h"
#include "../Engine.h"
#include "../Sound/Sound.h"

using namespace pkg;

namespace asset {

SoundLoader::SoundLoader() : m_id(0), m_size(0)
{
}

SoundLoader::~SoundLoader()
{
}

int SoundLoader::Process(
	const xtime::TimeSlice &time, 
	Engine &engine, 
	const pkg::Asset::Ref &asset, 
	int flags
)
{
	if (flags&P_Unload)
	{
		if (m_id)
		{
			engine.sys->alDriver->SyncDeleteBuffers(ALDRIVER_SIG 1, &m_id);
			ZSound.Get().Dec(m_size, 0);
		}
		m_id = 0;
		m_size = 0;
	}
	else if (flags&P_Load)
	{
		if (!m_id)
		{
			SoundParser::Ref parser = SoundParser::Cast(asset);
			if (!parser || !parser->header)
				return SR_ParseError;

			if (!engine.sys->alDriver->SyncGenBuffers(ALDRIVER_SIG 1, &m_id))
				return SR_ErrorGeneric;

			RAD_ASSERT(m_id);

			int alFormat;

			if (parser->header->channels > 2 || parser->header->channels < 1)
				return SR_InvalidFormat;
			if (parser->header->bytesPerSample > 2 || parser->header->bytesPerSample < 1)
				return SR_InvalidFormat;

			// lettuce figure out the format.
			if (parser->header->channels == 2 && parser->header->bytesPerSample == 2)
				alFormat = AL_FORMAT_STEREO16;
			else if(parser->header->channels == 2 && parser->header->bytesPerSample == 1)
				alFormat = AL_FORMAT_STEREO8;
			else if(parser->header->channels == 1 && parser->header->bytesPerSample == 2)
				alFormat = AL_FORMAT_MONO16;
			else
				alFormat = AL_FORMAT_MONO8;
			
			m_size = parser->header->numBytes;
			
			bool r = engine.sys->alDriver->SyncBufferData(
				ALDRIVER_SIG
				m_id,
				alFormat,
				(ALvoid*)parser->data.get(),
				parser->header->numBytes,
				parser->header->rate
			);
			
			if (!r)
				return SR_InvalidFormat;

			ZSound.Get().Inc(m_size, 0);
		}
	}

	return SR_Success;
}

void SoundLoader::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<SoundLoader>();
}

} // asset
