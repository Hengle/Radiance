// Sound.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "App.h"
#include "Engine.h"
#include "Sound.h"
#include "MathUtils.h"
#include "Assets/SoundLoader.h"
#include "Assets/MusicParser.h"
#include <Runtime/AudioCodec/Vorbis.h>
#include "COut.h"

#undef MessageBox

#if defined(RAD_OPT_OSX)
#define alSpeedOfSound alDopplerVelocity
#endif

enum
{
	MaxSimultaneousSounds = 32,
	DefaultReferenceDistance = 50,
	DefaultMaxDistance = 150,
	StreamingBufferSize = 12*Kilo,
	MaxDecodeBuffersPerTick = 2
};

///////////////////////////////////////////////////////////////////////////////

bool CheckALErrors(const char *file, const char *function, int line)
{
	if (!alcGetCurrentContext())
		return false;

#if defined(RAD_OPT_IOS)
	return alGetError() != AL_NO_ERROR;
#else
	WString str;
	bool found = false;
	int count = 0;

	for (ALenum err = alGetError(); err != AL_NO_ERROR; err = alGetError())
	{
		if (++count > 256)
			break;

		if (!found)
		{
			str.format(
				L"AL Errors (file: %s, function: %s, line: %d):\n",
				string::Widen(file).c_str(),
				string::Widen(function).c_str(),
				line
			);
			found = true;
		}

		switch (err)
		{
		case AL_INVALID_NAME:
			str += L"AL_INVALID_NAME\n";
			break;
		case AL_INVALID_ENUM:
			str += L"AL_INVALID_ENUM\n";
			break;
		case AL_INVALID_VALUE:
			str += L"AL_INVALID_VALUE\n";
			break;
		case AL_INVALID_OPERATION:
			str += L"AL_INVALID_OPERATION\n";
			break;
		case AL_OUT_OF_MEMORY:
			str += L"AL_OUT_OF_MEMORY\n";
			break;
		default:
			str += L"Unknown Error\n";
		};
	}

	ClearALErrors();
	if (found)
	{
		MessageBox(L"AL Errors Detected", str.c_str(), MBStyleOk);
	}

	return found;
#endif
}

void ClearALErrors()
{
	if (!alcGetCurrentContext())
		return;

	while (alGetError() != AL_NO_ERROR)
	{
	}
}

///////////////////////////////////////////////////////////////////////////////

int s_devRefs = 0;
ALCdevice *s_device = 0;

ALCdevice *OpenDevice()
{
	if (++s_devRefs == 1)
	{
		s_device = alcOpenDevice(0);
	}

	return s_device;
}

void CloseDevice()
{
	if (--s_devRefs == 0)
	{
		alcCloseDevice(s_device);
		s_device = 0;
	}
}

SoundDevice::Ref SoundDevice::New()
{
	ALCcontext *alc = alcCreateContext(OpenDevice(), 0);
	if (!alc)
		COut(C_Warn) << "Warning: Failed to open OpenAL device!" << std::endl;
	return Ref(new (ZSound) SoundDevice(alc));
}

SoundDevice::SoundDevice(ALCcontext *alc) : m_alc(alc), m_suspended(false), m_active(false)
{
	Bind();
}

SoundDevice::~SoundDevice()
{
	if (m_alc)
	{
		alcDestroyContext(m_alc);
		CloseDevice();
	}
}

void SoundDevice::Bind()
{
	if (m_alc)
		alcMakeContextCurrent(m_alc);
	m_active = true;
}

void SoundDevice::Unbind()
{
	if (m_alc)
		alcMakeContextCurrent(0);
	m_active = false;
}

void SoundDevice::Suspend()
{
	if (m_alc)
		alcSuspendContext(m_alc);
	m_suspended = true;
}

void SoundDevice::Resume()
{
	if (m_alc)
		alcProcessContext(m_alc);
	m_suspended = false;
}

void SoundContext::SoundThread::AddContext(SoundContext *ctx)
{
	Lock L(m_m);
	m_ctxs.insert(ctx);
	if (m_ctxs.size() == 1)
	{
		m_exit = false;
		Run();
	}
}

void SoundContext::SoundThread::RemoveContext(SoundContext *ctx)
{
	Lock L(m_m);
	m_ctxs.erase(ctx);
	if (m_ctxs.size() == 0)
	{
		m_exit = true;
		Join();
	}
}

int SoundContext::SoundThread::ThreadProc()
{
	bool deviceBound = false;
	
	while (!m_exit)
	{
		thread::Sleep(20);

		if (m_m.try_lock())
		{
			for (ContextSet::iterator it = m_ctxs.begin(); it != m_ctxs.end(); ++it)
			{
				if (!deviceBound && (*it)->m_alc)
				{
					deviceBound = true;
					alcMakeContextCurrent((*it)->m_alc);
				}
				(*it)->ThreadTick();
			}

			m_m.unlock();
		}
	}
	return 0;
}

SoundContext::SoundThread SoundContext::s_thread;

SoundContext::Ref SoundDevice::CreateContext()
{
	SoundContext::Ref r(new (ZSound) SoundContext(shared_from_this()));
	SoundContext::s_thread.AddContext(r.get());
	return r;
}

SoundContext::SoundContext(const SoundDevice::Ref &device) : 
m_device(device),
m_alc(device->m_alc), 
m_numSources(0),
m_distanceModel(AL_INVERSE_DISTANCE_CLAMPED)
{
	m_pos[0] = Vec3::Zero;
	m_pos[1] = Vec3::Zero;
	m_vel[0] = Vec3::Zero;
	m_vel[1] = Vec3::Zero;
	m_rot[0] = Quat::Identity;
	m_rot[1] = Quat::Identity;
	m_volume[0] = m_volume[1] = m_volume[2] = 1.f;
	m_volume[3] = 0.f; // force apply volume
	m_time[0] = m_time[1] = 0.f;

	if (m_alc)
		alcMakeContextCurrent(m_alc);
	distanceModel = AL_LINEAR_DISTANCE_CLAMPED; // set distance model.

	float f[6] = { 1, 0, 0, 0, 0, 1 };
	alListenerfv(AL_ORIENTATION, f);
	ClearALErrors();
}

SoundContext::~SoundContext()
{
	s_thread.RemoveContext(this);
}

void SoundContext::PauseAll(bool pause)
{
	for (int i = 0; i < SC_Max; ++i)
	{
		Channel &channel = m_channels[i];

		if (channel.paused)
			continue;

		Source::Map &map = channel.sources;

		Lock L(m_m);

		for (Source::Map::iterator it = map.begin(); it != map.end(); ++it)
		{
			Source::List &list = it->second;
			for (Source::List::iterator it2 = list.begin(); it2 != list.end(); ++it2)
			{
				Source *source = *it2;
				RAD_ASSERT(source&&source->sound);
				if (source->paused != pause)
					source->sound->Pause(*source, pause);
			}
		}
	}
}

void SoundContext::StopAll()
{
	for (int i = 0; i < SC_Max; ++i)
	{
		Channel &channel = m_channels[i];
		Source::Map &map = channel.sources;

		Lock L(m_m);

		for (Source::Map::iterator it = map.begin(); it != map.end(); ++it)
		{
			Source::List &list = it->second;
			for (Source::List::iterator it = list.begin(); it != list.end();)
			{
				Source *source = *it;
				RAD_ASSERT(source&&source->sound);
				source->sound->Stop(*source);
				Source::List::iterator next = it; ++next;
				list.erase(it);
				it = next;
				source->mapped = false;
				m_sources[source->priority].erase(source->it[1]);
				RAD_ASSERT(m_numSources>0);
				--m_numSources;
			}
		}
	}
}

void SoundContext::Tick(float dt, bool positional)
{
	if (!m_device->active)
		return;
	
	if (m_time[1] > 0.f)
	{
		m_time[0] += dt;
		if (m_time[0] >= m_time[1])
		{
			m_volume[0] = m_volume[2];
			m_time[1] = 0.f;
		}
		else
		{
			m_volume[0] = math::Lerp(m_volume[1], m_volume[2], m_time[0]/m_time[1]);
		}
	}

	{
		Lock L(m_m);

		if (m_volume[3] != m_volume[0])
		{
			m_volume[3] = m_volume[0];
			if (m_alc)
				alListenerf(AL_GAIN, m_volume[0]);
		}

		if (m_pos[1] != m_pos[0])
		{
			m_pos[1] = m_pos[0];
			if (m_alc)
				alListener3f(AL_POSITION, m_pos[0][0], m_pos[0][1], m_pos[0][2]);
		}

		if (m_vel[1] != m_vel[0])
		{
			m_vel[1] = m_vel[0];
			if (m_alc)
				alListener3f(AL_VELOCITY, m_vel[0][0], m_vel[0][1], m_vel[0][2]);
		}

		if (m_rot[1] != m_rot[0])
		{
			m_rot[1] = m_rot[0];
			Vec3 fwd(1, 0, 0);
			Mat4 m = Mat4::Rotation(m_rot[0]);
			fwd = fwd * m;

			Vec3 up, left;
			FrameVecs(fwd, up, left);

			float f[6] = {
				fwd[0], fwd[1], fwd[2],
				 up[0],  up[1],  up[2]
			};

			if (m_alc)
				alListenerfv(AL_ORIENTATION, f);
		}
	}

	for (int i = 0; i < SC_Max; ++i)
	{
		Channel &channel = m_channels[i];

		if (channel.time[1] > 0.f)
		{
			channel.time[0] += dt;
			if (channel.time[0] >= channel.time[1])
			{
				channel.time[1] = 0.f;
				channel.volume[0] = channel.volume[2];
			}
			else
			{
				channel.volume[0] = math::Lerp(channel.volume[1], channel.volume[2], channel.time[0]/channel.time[1]);
			}
		}

		Source::Map &map = channel.sources;

		Lock L(m_m);

		for (Source::Map::iterator it = map.begin(); it != map.end(); ++it)
		{
			Source::List &list = it->second;
			for (Source::List::iterator it = list.begin(); it != list.end();)
			{
				Source *source = *it;
				RAD_ASSERT(source&&source->sound);

				if (!source->paused && source->play && !source->stream && m_alc)
				{
					alSourcePlay(source->source);
					source->play = false;
				}

				if (source->sound->Tick(dt, channel, *source, positional))
				{
					Source::List::iterator next = it; ++next;
					list.erase(it);
					it = next;
					source->mapped = false;
					m_sources[source->priority].erase(source->it[1]);
					RAD_ASSERT(m_numSources>0);
					--m_numSources;
				}
				else
				{
					++it;
				}
			}
		}
	}
}

int SoundContext::ThreadTick()
{
	enum { MaxStreamSources = 8 };
	
	if (!m_device->active)
		return 0;
	
	m_mthread.lock();

	int numSources = 0;
	Source *sources[MaxStreamSources];
	
	{
		Lock L(m_m);
		for (Source::List::const_iterator it = m_streams.begin(); it != m_streams.end(); ++it)
		{
			Source *s = *it;
			if (s->stream)
			{
				sources[numSources++] = s;
				if (numSources == MaxStreamSources)
					break;
			}
		}
	}

	for(int i = 0; i < numSources; ++i)
	{
		Source *s = sources[i];
		if (s->sound->TickStream(*s))
		{
			Lock L(m_m);
			if (s->mapped)
				UnmapSource(*s);
		}
	}

	m_mthread.unlock();

	return 0;
}

void SoundContext::FadeMasterVolume(float volume, float time)
{
	if (time > 0.f)
	{
		m_volume[1] = m_volume[0];
		m_volume[2] = volume;
		m_time[0] = 0.f;
		m_time[1] = time;
	}
	else
	{
		m_volume[0] = volume;
		m_time[1] = 0.f;
	}
}

void SoundContext::FadeChannelVolume(SoundChannel c, float volume, float time)
{
	Channel &channel = m_channels[c];

	if (time > 0.f)
	{
		channel.volume[1] = channel.volume[0];
		channel.volume[2] = volume;
		channel.time[0] = 0.f;
		channel.time[1] = time;
	}
	else
	{
		channel.volume[0] = volume;
		channel.time[1] = 0.f;
	}
}

void SoundContext::PauseChannel(SoundChannel c, bool pause)
{
	const Source::Map &map = m_channels[c].sources;

	for (Source::Map::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		const Source::List &list = it->second;
		for (Source::List::const_iterator it = list.begin(); it != list.end(); ++it)
		{
			Source *source = *it;
			if (source->paused != pause)
				source->sound->Pause(*source, pause);
		}
	}
}

void SoundContext::SetDoppler(float dopplerFactor, float speedOfSound)
{
	if (!m_alc)
		return;

	alDopplerFactor(dopplerFactor);
	CHECK_AL_ERRORS();
	alSpeedOfSound(speedOfSound);
	CHECK_AL_ERRORS();
}

void SoundContext::RAD_IMPLEMENT_SET(distanceModel)(ALenum value)
{
	if (m_distanceModel == value)
		return;
	m_distanceModel = value;

	if (m_alc)
		alDistanceModel(value);
}

bool SoundContext::Evict(int priority)
{
	Source::Map::iterator end = m_sources.lower_bound(priority);

	for (Source::Map::iterator it = m_sources.begin(); it != end; ++it)
	{
		Source::List &list = it->second;
		if (!list.empty())
		{
			UnmapSource(**list.begin());
			return true;
		}
	}

	return false;
}

bool SoundContext::MapSource(Source &source)
{
	RAD_ASSERT(!source.mapped);
	RAD_ASSERT(source.channel >= SC_First && source.channel < SC_Max);
	
	if (m_numSources >= MaxSimultaneousSounds)
	{
		if (!Evict(source.priority))
			return false;
	}
	
	++m_numSources;
	Source::List *list = &m_channels[source.channel].sources[source.priority];
	list->push_back(&source);
	source.it[0] = list->end();
	--source.it[0];
	
	list = &m_sources[source.priority];
	list->push_back(&source);
	source.it[1] = list->end();
	--source.it[1];

	source.mapped = true;

	if (source.stream)
	{
		m_streams.push_back(&source);
		source.it[2] = m_streams.end();
		--source.it[2];
	}

	return true;
}

void SoundContext::UnmapSource(Source &source)
{
	RAD_ASSERT(m_numSources>0);
	RAD_ASSERT(source.mapped);
	
	m_channels[source.channel].sources[source.priority].erase(source.it[0]);
	m_sources[source.priority].erase(source.it[1]);
	if (source.stream)
		 m_streams.erase(source.it[2]);

	source.mapped = false;
	source.sound->Stop(source);
	--m_numSources;
}

Sound::Ref SoundContext::NewSound(ALuint buffer, int maxInstances)
{
	ClearALErrors();
	RAD_ASSERT(maxInstances>0);
	Sound::Ref s(new (ZSound) Sound());
	if (!s->Init(shared_from_this(), buffer, maxInstances))
		return Sound::Ref();
	return s;
}

Sound::Ref SoundContext::NewSound(const pkg::AssetRef &sound, int maxInstances)
{
	ClearALErrors();
	RAD_ASSERT(maxInstances>0);
	Sound::Ref s(new (ZSound) Sound());

	if (sound->type == asset::AT_Sound)
	{
		if (!s->Init(shared_from_this(), sound, maxInstances))
			return Sound::Ref();
	}
	else
	{
		if (!s->InitStreaming(shared_from_this(), sound))
			return Sound::Ref();
	}

	return s;
}

///////////////////////////////////////////////////////////////////////////////

Sound::Sound() : 
m_playing(0), 
m_paused(0), 
m_loop(false), 
m_relative(true),
m_outerVolume(-1.f),
m_pitch(-1.f),
m_rolloff(-1.0f),
m_maxDistance(-1.f),
m_refDistance(-1.0f),
m_is(0),
m_vbd(0),
m_blockData(0),
m_eos(false),
m_bsi(0)
{
	m_pos[0] = Vec3::Zero;
	m_pos[1] = Vec3::Zero;
	m_vel[0] = Vec3::Zero;
	m_vel[1] = Vec3::Zero;
	m_volume[0] = m_volume[1] = m_volume[2] = m_volume[3] = 1.f;
	m_minMaxVolume[0] = 0.f;
	m_minMaxVolume[1] = 1.f;
	m_angles[0] = m_angles[1] = -1.f;
	m_time[0] = m_time[1] = 0.f;

	memset(m_sbufs, 0, sizeof(ALuint)*NumStreamingBuffers);
	m_availBuf[0] = m_availBuf[1] = 0;
}

Sound::~Sound()
{
	SoundContext::Ref ctx = m_ctx.lock();
	if (ctx)
	{
		SoundContext::Lock L(ctx->m_mthread);
		SoundContext::Lock L2(ctx->m_m);
		
		if (m_sbufs[0])
			ZMusic.Get().Dec(NumStreamingBuffers*StreamingBufferSize, 0);

		for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
		{
			SoundContext::Source &source = *it;
			if (source.mapped)
				ctx->UnmapSource(source);

			if (source.source)
			{
				alDeleteSources(1, &source.source);
				CHECK_AL_ERRORS();
			}
		}
		for (int i = 0; i < NumStreamingBuffers; ++i)
		{
			if (m_sbufs[i])
			{
				alDeleteBuffers(1, &m_sbufs[i]);
				CHECK_AL_ERRORS();
			}
		}
	}

	m_ib.Close();
	
	if (m_vbd)
		delete m_vbd;
	if (m_bsi)
		delete m_bsi;
	if (m_is)
		delete m_is;
	if (m_blockData)
		zone_free(m_blockData);
}

bool Sound::Init(
	const SoundContext::Ref &ctx,
	ALuint buffer,
	int maxInstances
)
{
	RAD_ASSERT(maxInstances>0);
	m_sources.resize(maxInstances);

	{
		SoundContext::Lock L(ctx->m_m);

		for (int i = 0; i < maxInstances; ++i)
		{
			SoundContext::Source &source = m_sources[i];
			source.idx = i;
			source.sound = this;
			if (ctx->m_alc)
			{
				if (!buffer)
					return false;

				ClearALErrors();
				alGenSources(1, &source.source);
				int error = alGetError();
				if (error != AL_NO_ERROR)
				{
					source.source = 0;
					return false;
				}
				alSourcei(source.source, AL_BUFFER, buffer);
				alSourcei(source.source, AL_SOURCE_RELATIVE, AL_TRUE);
				CHECK_AL_ERRORS();
			}
			else
			{
				source.source = 0;
			}
		}

		m_ctx = ctx;
	}

	this->refDistance = (float)DefaultReferenceDistance;
	this->maxDistance = (float)DefaultMaxDistance;

	return true;
}

bool Sound::Init(
	const SoundContext::Ref &ctx,
	const pkg::AssetRef &asset,
	int maxInstances
)
{
	if (asset->type != asset::AT_Sound)
		return false;
	asset::SoundLoader::Ref loader = asset::SoundLoader::Cast(asset);
	if (!Init(ctx, loader->id, maxInstances))
		return false;
	m_asset = asset;
	return true;
}

bool Sound::InitStreaming(
	const SoundContext::Ref &ctx,
	const pkg::AssetRef &asset
)
{
	if (asset->type != asset::AT_Music)
		return false;
	asset::MusicParser::Ref parser = asset::MusicParser::Cast(asset);
	if (!parser || !parser->file.get())
		return false;
	m_ib = App::Get()->engine->sys->files->SafeCreateStreamBuffer(8*Kilo, ZMusic);
	m_ib->Bind(parser->file.get());
	m_is = new (ZMusic) stream::InputStream(m_ib->buffer);
	if (!m_is)
		return false;
	m_vbd = new (ZMusic) audio_codec::ogg_vorbis::Decoder();
	if (!m_vbd)
		return false;
	m_bsi = new (ZMusic) audio_codec::ogg_vorbis::BSI;
	if (!m_bsi)
		return false;

	if (!m_vbd->Initialize(*m_is, false))
		return false;
	if (!m_vbd->BSInfo(*m_bsi))
		return false;

	m_eos = false;

	{
		SoundContext::Lock L(ctx->m_m);

		if (ctx->m_alc)
		{
			// it's a vorbis stream, create AL buffers.
			ClearALErrors();
			alGenBuffers(NumStreamingBuffers, m_sbufs);
			ALuint error = alGetError();
			if (error != AL_NO_ERROR)
				return false;
			ZMusic.Get().Inc(NumStreamingBuffers*StreamingBufferSize, 0);
		}
		else
		{
			memset(m_sbufs, 0, sizeof(ALuint)*NumStreamingBuffers);
		}

		memcpy(m_rbufs, m_sbufs, sizeof(ALuint)*NumStreamingBuffers);
		m_availBuf[0] = 0;
		m_availBuf[1] = NumStreamingBuffers;

		m_blockData = safe_zone_malloc(ZMusic, StreamingBufferSize);

		// make streaming source.
		m_sources.resize(1);
		SoundContext::Source &source = m_sources[0];
		source.idx = 0;
		source.sound = this;
		source.stream = true;

		if (ctx->m_alc)
		{
			ClearALErrors();
			alGenSources(1, &source.source);
			ALuint error = alGetError();
			if (error != AL_NO_ERROR)
				return false;
			alSourcei(source.source, AL_SOURCE_RELATIVE, AL_TRUE);
		}
		else
		{
			source.source = 0;
		}

		m_ctx = ctx;
		m_asset = asset;
	}

	this->refDistance = (float)DefaultReferenceDistance;
	this->maxDistance = (float)DefaultMaxDistance;

	return true;
}

void Sound::FadeVolume(float volume, float time)
{
	if (time > 0.f)
	{
		m_volume[1] = m_volume[0];
		m_volume[2] = volume;
		m_time[0] = 0.f;
		m_time[1] = time;
	}
	else
	{
		m_volume[0] = volume;
		m_time[1] = 0.f;
	}
}

bool Sound::Play(SoundChannel c, int priority)
{
	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return false;
	if (ctx->ChannelIsPaused(c))
		return false;
	if(m_loop && m_playing>0)
		return true; // already playing.
	if (m_paused>0)
		return false; // must unpause first.

	SoundContext::Lock L2(ctx->m_m);
	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (!source.mapped)
		{
			source.priority = priority;
			source.channel = c;
			source.paused = false;
			if (!ctx->MapSource(source))
				return false;
			source.play = true;
			++m_playing; // will start playing via SoundContext::Tick
			break;
		}
	}

	return m_playing>0;
}

void Sound::Pause(bool pause)
{
	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.mapped && source.paused != pause)
		{
			if (!ctx->ChannelIsPaused(source.channel) || pause)
				Pause(source, pause);
		}
	}
}

void Sound::Rewind()
{
	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	SoundContext::Lock L2(ctx->m_m);
	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.mapped)
			ctx->UnmapSource(source);

		if (!source.source)
			continue;

		if (m_is)
		{ // rewind ogg playback
			RAD_ASSERT(m_vbd);
			m_vbd->SeekBytes(0, false);

			// iOS5 bug, just delete the source entirely.
			if (source.source)
			{
				alDeleteSources(1, &source.source);
				CHECK_AL_ERRORS();
				alGenSources(1, &source.source);
				CHECK_AL_ERRORS();
				alSourcei(source.source, AL_SOURCE_RELATIVE, m_relative ? AL_TRUE : AL_FALSE);
				CHECK_AL_ERRORS();
			}
			
			m_availBuf[0] = 0;
			m_availBuf[1] = NumStreamingBuffers;
			memcpy(m_rbufs, m_sbufs, sizeof(ALuint)*NumStreamingBuffers);
			m_eos = false;
		}
		else
		{
			alSourceRewind(source.source);
			CHECK_AL_ERRORS();
		}
	}
}

void Sound::Stop()
{
	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	SoundContext::Lock L2(ctx->m_m);
	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.mapped)
			ctx->UnmapSource(source);
	}
}

bool Sound::Tick(
	float dt, 
	const SoundContext::Channel &channel,
	SoundContext::Source &source,
	bool positional
)
{
	RAD_ASSERT(source.mapped);
	RAD_ASSERT(source.source);

	if (!positional && !m_relative)
		return false; // not ticking positional sounds.

	if (m_pos[1] != m_pos[0])
	{
		m_pos[1] = m_pos[0];
		for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
		{
			SoundContext::Source &source = *it;
			if (source.source)
			{
				alSource3f(source.source, AL_POSITION, m_pos[0][0], m_pos[0][1], m_pos[0][2]);
				CHECK_AL_ERRORS();
			}
		}
	}

	if (m_vel[1] != m_vel[0])
	{
		m_vel[1] = m_vel[0];
		for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
		{
			SoundContext::Source &source = *it;
			if (source.source)
			{
				alSource3f(source.source, AL_VELOCITY, m_vel[0][0], m_vel[0][1], m_vel[0][2]);
				CHECK_AL_ERRORS();
			}
		}
	}

	if (m_rot[1] != m_rot[0])
	{
		m_rot[1] = m_rot[0];
		Vec3 fwd(1, 0, 0);
		Mat4 m = Mat4::Rotation(m_rot[0]);
		fwd = fwd * m;

		for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
		{
			SoundContext::Source &source = *it;
			if (source.source)
			{
				alSource3f(source.source, AL_DIRECTION, fwd[0], fwd[1], fwd[2]);
				CHECK_AL_ERRORS();
			}
		}
	}

	if (m_time[1] > 0.f)
	{
		m_time[0] += dt;
		if (m_time[0] >= m_time[1])
		{
			m_volume[0] = m_volume[2];
			m_time[1] = 0.f;
		}
		else
		{
			m_volume[0] = math::Lerp(m_volume[1], m_volume[2], m_time[0]/m_time[1]);
		}
	}

	if (m_volume[3] != m_volume[0]*channel.volume[0])
	{
		m_volume[3] = m_volume[0]*channel.volume[0];
		for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
		{
			SoundContext::Source &source = *it;
			if (source.source)
			{
				alSourcef(source.source, AL_GAIN, m_volume[3]);
				CHECK_AL_ERRORS();
			}
		}
	}

	if (!source.source)
	{
		--m_playing;
		return true;
	}

	if (m_is)
		return false;

	// done?
	ALint status;
	alGetSourcei(source.source, AL_SOURCE_STATE, &status);
	if (status == AL_STOPPED)
	{
		--m_playing;
		return true;
	}

	if (status == AL_PAUSED && !m_paused && !channel.paused)
		alSourcePlay(source.source);

	return false;
}

bool Sound::TickStream(SoundContext::Source &source)
{
	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return true;

	if (!source.source)
		return true;
	if (source.paused)
		return false;
	
	{
		SoundContext::Lock L(ctx->m_m);
		
		if (m_eos)
		{
			ALint num;
			alGetSourcei(source.source, AL_BUFFERS_QUEUED, &num);
			return num == 0;
		}
		
		// unqueue AL buffers if we don't have anything to read in.
		// i.e. all buffers are full of data.
		if (m_availBuf[0] == m_availBuf[1])
		{
			ALint num;
			alGetSourcei(source.source, AL_BUFFERS_PROCESSED, &num);
			if (num < 1)
			{
				ALint state;
				alGetSourcei(source.source, AL_SOURCE_STATE, &state);
				if (state != AL_PLAYING)
					alSourcePlay(source.source);
				return false; // nothing was freed.
			}
			alSourceUnqueueBuffers(source.source, num, m_rbufs);
			m_availBuf[1] = num;
			m_availBuf[0] = 0;
		}
	}

	int numBuffers = 0;
	
	while ((m_availBuf[0] != m_availBuf[1]) && !m_eos && (numBuffers < MaxDecodeBuffersPerTick))
	{
		// decode another block.
		
		AddrSize read = 0;
		
		while (read < StreamingBufferSize)
		{
			AddrSize x;
			
			m_vbd->Decode(
				((U8*)m_blockData)+read,
				StreamingBufferSize-read,
				audio_codec::ogg_vorbis::EM_Little,
				audio_codec::ogg_vorbis::ST_16Bit,
				audio_codec::ogg_vorbis::DT_Signed,
				0,
				x
			);
			
			if (0 == x)
			{
				if (m_loop)
				{
					m_vbd->SeekBytes(0, false);
				}
				else
				{
					m_eos = true;
					break;
				}
			}
			
			read += x;
		}
		
		if (read > 0)
		{
			SoundContext::Lock L(ctx->m_m);
			
			int idx = m_availBuf[0]++;
			RAD_ASSERT(idx < NumStreamingBuffers);
			
			if (idx < NumStreamingBuffers)
			{
				ALuint buf = m_rbufs[idx];
				
				alBufferData(
					buf,
					(m_bsi->channels==2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
					m_blockData,
					(ALsizei)read,
					m_bsi->rate
				);
				
				if (source.source)
					alSourceQueueBuffers(source.source, 1, &buf);
				
				++numBuffers;
			}
		}
	}

	SoundContext::Lock L(ctx->m_m);
	if (source.mapped && source.source)
	{
		ALint state;
		alGetSourcei(source.source, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
			alSourcePlay(source.source);
	}

	return false;
}

void Sound::Pause(SoundContext::Source &source, bool pause)
{
	RAD_ASSERT(source.source);
	RAD_ASSERT(source.paused != pause);

	source.paused = pause;
	if (pause)
	{
		++m_paused;
		if (source.source)
		{
			alSourcePause(source.source);
			CHECK_AL_ERRORS();
		}
	}
	else
	{
		--m_paused;
		// will resume in SoundContext::Tick();
	}
}

void Sound::Stop(SoundContext::Source &source)
{
	RAD_ASSERT(source.source);
	RAD_ASSERT(m_playing>0);
	if (source.paused)
	{
		--m_paused;
		source.paused = false;
	}
	
	if (source.source)
	{
		alSourceStop(source.source);
		CHECK_AL_ERRORS();
	}

	--m_playing;
}

void Sound::RAD_IMPLEMENT_SET(loop)(bool value)
{
	if (m_loop == value)
		return;
	m_loop = value;

	if (m_is)
		return; // streaming sources loop manually.

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcei(source.source, AL_LOOPING, m_loop ? AL_TRUE : AL_FALSE);
			CHECK_AL_ERRORS();
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(sourceRelative)(bool value)
{
	if (m_relative == value)
		return;
	m_relative = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcei(source.source, AL_SOURCE_RELATIVE, value ? AL_TRUE : AL_FALSE);
			CHECK_AL_ERRORS();
			if (!value)
			{ // force muted until we tick
				alSourcef(source.source, AL_GAIN, m_volume[3]);
				CHECK_AL_ERRORS();
			}
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(innerAngle)(float value)
{
	if (m_angles[0] == value)
		return;
	m_angles[0] = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcef(source.source, AL_CONE_INNER_ANGLE, value);
			CHECK_AL_ERRORS();
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(outerAngle)(float value)
{
	if (m_angles[1] == value)
		return;
	m_angles[1] = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcef(source.source, AL_CONE_OUTER_ANGLE, value);
			CHECK_AL_ERRORS();
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(outerVolume)(float value)
{
	if (m_outerVolume == value)
		return;
	m_outerVolume = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcef(source.source, AL_CONE_OUTER_GAIN, value);
			CHECK_AL_ERRORS();
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(minVolume)(float value)
{
	if (m_minMaxVolume[0] == value)
		return;
	m_minMaxVolume[0] = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcef(source.source, AL_MIN_GAIN, math::Clamp(value, 0.f, 1.f));
			CHECK_AL_ERRORS();
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(maxVolume)(float value)
{
	if (m_minMaxVolume[1] == value)
		return;
	m_minMaxVolume[1] = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcef(source.source, AL_MAX_GAIN, math::Clamp(value, 0.f, 1.f));
			CHECK_AL_ERRORS();
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(pitch)(float value)
{
	if (m_pitch == value)
		return;
	m_pitch = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcef(source.source, AL_PITCH, value);
			CHECK_AL_ERRORS();
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(rolloff)(float value)
{
	if (m_rolloff == value)
		return;
	m_rolloff = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcef(source.source, AL_ROLLOFF_FACTOR, value);
			CHECK_AL_ERRORS();
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(refDistance)(float value)
{
	if (m_refDistance == value)
		return;
	m_refDistance = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcef(source.source, AL_REFERENCE_DISTANCE, value);
			CHECK_AL_ERRORS();
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(maxDistance)(float value)
{
	if (m_maxDistance == value)
		return;
	m_maxDistance = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;
	SoundContext::Lock L(ctx->m_m);

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		SoundContext::Source &source = *it;
		if (source.source)
		{
			alSourcef(source.source, AL_MAX_DISTANCE, value);
			CHECK_AL_ERRORS();
		}
	}
}
