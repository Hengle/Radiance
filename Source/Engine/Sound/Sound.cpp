// Sound.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../App.h"
#include "../Engine.h"
#include "../MathUtils.h"
#include "../Assets/SoundLoader.h"
#include "../Assets/MusicParser.h"
#include "../COut.h"
#include <Runtime/AudioCodec/Vorbis.h>
#include "Sound.h"

#undef MessageBox

#if defined(RAD_OPT_OSX)
#define alSpeedOfSound alDopplerVelocity
#endif

enum {
	kMaxSimultaneousSounds = 32,
	kDefaultReferenceDistance = 50,
	kDefaultMaxDistance = 150,
	kStreamingBufferSize = 16*Kilo,
	kMaxBytesDecodedPerTick = Kilo
};

///////////////////////////////////////////////////////////////////////////////

SoundContext::StreamCallback::StreamCallback(SoundContext &ctx) : 
ALDriver::Callback(ctx.m_alDriver), m_ctx(ctx) {
}

void SoundContext::StreamCallback::tick(ALDriver &alDriver) {
	if (m_ctx.tickStreams(alDriver))
		alDriver.wake(); // don't sleep we are still working.
}


SoundContext::Ref SoundContext::create(const ALDriver::Ref &driver) {
	return SoundContext::Ref(new (ZSound) SoundContext(driver));
}

SoundContext::SoundContext(const ALDriver::Ref &driver) : 
m_alDriver(driver),
m_numSources(0),
m_distanceModel(AL_INVERSE_DISTANCE_CLAMPED) {
	m_pos[0] = Vec3::Zero;
	m_pos[1] = Vec3::Zero;
	m_vel[0] = Vec3::Zero;
	m_vel[1] = Vec3::Zero;
	m_rot[0] = Quat::Identity;
	m_rot[1] = Quat::Identity;
	m_volume[0] = m_volume[1] = m_volume[2] = 1.f;
	m_volume[3] = 0.f; // force apply volume
	m_time[0] = m_time[1] = 0.f;

	distanceModel = AL_LINEAR_DISTANCE_CLAMPED; // set distance model.

	float f[6] = { 1, 0, 0, 0, 0, 1 };
	driver->listenerfv(ALDRIVER_SIG AL_ORIENTATION, f);

	m_streamCallback.reset(new (ZSound) StreamCallback(*this));
	driver->addCallback(*m_streamCallback);
}

SoundContext::~SoundContext() {
	m_streamCallback.reset(); // make sure we aren't in tickStreams before we start cleaning up.
}

void SoundContext::pauseAll(bool pause) {
	for (int i = 0; i < SC_Max; ++i) {
		Channel &channel = m_channels[i];

		if (channel.paused)
			continue;

		Source::Map &map = channel.sources;

		for (Source::Map::iterator it = map.begin(); it != map.end(); ++it) {

			Source::List &list = it->second;
			
			for (Source::List::iterator it2 = list.begin(); it2 != list.end(); ++it2) {
				Source *source = *it2;
				RAD_ASSERT(source&&source->sound);
				if (source->paused != pause)
					source->sound->pause(*source, pause);
			}
		}
	}
}

void SoundContext::stopAll() {

	for (int i = 0; i < SC_Max; ++i) {
		Channel &channel = m_channels[i];
		Source::Map &map = channel.sources;

		for (Source::Map::iterator it = map.begin(); it != map.end(); ++it) {
			Source::List &list = it->second;
			
			for (Source::List::iterator it = list.begin(); it != list.end();) {
				Source *source = *it;
				RAD_ASSERT(source&&source->sound);
				source->sound->stop(*source);
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

void SoundContext::tick(float dt, bool positional)
{
	if (m_alDriver->suspended)
		return;
	
	if (m_time[1] > 0.f) {
		m_time[0] += dt;
		if (m_time[0] >= m_time[1]) {
			m_volume[0] = m_volume[2];
			m_time[1] = 0.f;
		} else {
			m_volume[0] = math::Lerp(m_volume[1], m_volume[2], m_time[0]/m_time[1]);
		}
	}

	{
		if (m_volume[3] != m_volume[0]) {
			m_volume[3] = m_volume[0];
			m_alDriver->listenerf(ALDRIVER_SIG AL_GAIN, m_volume[0]);
		}

		if (m_pos[1] != m_pos[0]) {
			m_pos[1] = m_pos[0];
			m_alDriver->listener3f(ALDRIVER_SIG AL_POSITION, m_pos[0][0], m_pos[0][1], m_pos[0][2]);
		}

		if (m_vel[1] != m_vel[0]) {
			m_vel[1] = m_vel[0];
			m_alDriver->listener3f(ALDRIVER_SIG AL_VELOCITY, m_vel[0][0], m_vel[0][1], m_vel[0][2]);
		}

		if (m_rot[1] != m_rot[0]) {
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

			m_alDriver->listenerfv(ALDRIVER_SIG AL_ORIENTATION, f);
		}
	}

	for (int i = 0; i < SC_Max; ++i) {
		Channel &channel = m_channels[i];

		if (channel.time[1] > 0.f) {
			channel.time[0] += dt;
			if (channel.time[0] >= channel.time[1]) {
				channel.time[1] = 0.f;
				channel.volume[0] = channel.volume[2];
			} else {
				channel.volume[0] = math::Lerp(channel.volume[1], channel.volume[2], channel.time[0]/channel.time[1]);
			}
		}

		Source::Map &map = channel.sources;

		ReadLock L(m_m);

		for (Source::Map::iterator it = map.begin(); it != map.end(); ++it) {
			Source::List &list = it->second;
			
			for (Source::List::iterator it = list.begin(); it != list.end();) {
				Source *source = *it;
				RAD_ASSERT(source&&source->sound);

				if (!source->paused && source->play && !source->stream) {
					m_alDriver->sourcePlay(ALDRIVER_SIG source->source);
					source->play = false;
					source->status = AL_PLAYING;
				}

				if (source->sound->tick(dt, channel, *source, positional)) {
					UpgradeToWriteLock WL(L);
					Source::List::iterator next = it; ++next;
					list.erase(it);
					it = next;
					source->mapped = false;
					m_sources[source->priority].erase(source->it[1]);
					RAD_ASSERT(m_numSources>0);
					--m_numSources;
				} else {
					++it;
				}
			}
		}
	}
}

int SoundContext::tickStreams(ALDriver &driver)
{
	int numBusy = 0;

	ReadLock L(m_m);

	for (Source::List::const_iterator it = m_streams.begin(); it != m_streams.end();) {
		Source *s = *it; ++it; // we may unmap this and invalidate it
		RAD_ASSERT(s->stream);
		Sound::StreamResult r = s->sound->tickStream(*s);
		if (r == Sound::kStreamResult_Finished) {
			UpgradeToWriteLock WL(L);
			unmapSource(*s);
		} else if (r == Sound::kStreamResult_Decoding) {
			++numBusy;
		}
	}

	return numBusy;
}

void SoundContext::fadeMasterVolume(float volume, float time) {
	if (time > 0.f) {
		m_volume[1] = m_volume[0];
		m_volume[2] = volume;
		m_time[0] = 0.f;
		m_time[1] = time;
	} else {
		m_volume[0] = volume;
		m_time[1] = 0.f;
	}
}

void SoundContext::fadeChannelVolume(SoundChannel c, float volume, float time) {
	Channel &channel = m_channels[c];

	if (time > 0.f) {
		channel.volume[1] = channel.volume[0];
		channel.volume[2] = volume;
		channel.time[0] = 0.f;
		channel.time[1] = time;
	} else {
		channel.volume[0] = volume;
		channel.time[1] = 0.f;
	}
}

void SoundContext::pauseChannel(SoundChannel c, bool pause) {
	const Source::Map &map = m_channels[c].sources;

	for (Source::Map::const_iterator it = map.begin(); it != map.end(); ++it) {
		const Source::List &list = it->second;
		
		for (Source::List::const_iterator it = list.begin(); it != list.end(); ++it) {
			Source *source = *it;
			if (source->paused != pause)
				source->sound->pause(*source, pause);
		}
	}
}

void SoundContext::setDoppler(float dopplerFactor, float speedOfSound) {
	m_alDriver->dopplerFactor(ALDRIVER_SIG dopplerFactor);
	m_alDriver->speedOfSound(ALDRIVER_SIG speedOfSound);
}

void SoundContext::RAD_IMPLEMENT_SET(distanceModel)(ALenum value) {
	if (m_distanceModel == value)
		return;
	m_distanceModel = value;

	m_alDriver->distanceModel(ALDRIVER_SIG value);
}

bool SoundContext::evict(int priority) {
	Source::Map::iterator end = m_sources.lower_bound(priority);

	for (Source::Map::iterator it = m_sources.begin(); it != end; ++it) {
		Source::List &list = it->second;
		if (!list.empty()) {
			unmapSource(**list.begin());
			return true;
		}
	}

	return false;
}

bool SoundContext::mapSource(Source &source) {
	RAD_ASSERT(!source.mapped);
	RAD_ASSERT(source.channel >= SC_First && source.channel < SC_Max);

	WriteLock L(m_m);
	
	if (m_numSources >= kMaxSimultaneousSounds) {
		if (!evict(source.priority))
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

	if (source.stream) {
		m_streams.push_back(&source);
		source.it[2] = m_streams.end();
		--source.it[2];
	}

	return true;
}

void SoundContext::unmapSource(Source &source) {
	RAD_ASSERT(m_numSources>0);
	RAD_ASSERT(source.mapped);
	
	m_channels[source.channel].sources[source.priority].erase(source.it[0]);
	m_sources[source.priority].erase(source.it[1]);
	if (source.stream)
		 m_streams.erase(source.it[2]);

	source.mapped = false;
	source.sound->stop(source);
	--m_numSources;
}

Sound::Ref SoundContext::newSound(ALuint buffer, int maxInstances) {
	RAD_ASSERT(maxInstances>0);
	Sound::Ref s(new (ZSound) Sound());
	if (!s->init(shared_from_this(), buffer, maxInstances))
		return Sound::Ref();
	return s;
}

Sound::Ref SoundContext::newSound(const pkg::AssetRef &sound, int maxInstances)
{
	ClearALErrors();
	RAD_ASSERT(maxInstances>0);
	Sound::Ref s(new (ZSound) Sound());

	if (sound->type == asset::AT_Sound) {
		if (!s->init(shared_from_this(), sound, maxInstances))
			return Sound::Ref();
	} else {
		if (!s->initStreaming(shared_from_this(), sound))
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
m_decodeOfs(0),
m_eos(false),
m_bsi(0) {
	m_pos[0] = Vec3::Zero;
	m_pos[1] = Vec3::Zero;
	m_vel[0] = Vec3::Zero;
	m_vel[1] = Vec3::Zero;
	m_volume[0] = m_volume[1] = m_volume[2] = m_volume[3] = 1.f;
	m_minMaxVolume[0] = 0.f;
	m_minMaxVolume[1] = 1.f;
	m_angles[0] = m_angles[1] = -1.f;
	m_time[0] = m_time[1] = 0.f;

	memset(m_sbufs, 0, sizeof(ALuint)*kNumStreamingBuffers);
	m_availBuf[0] = m_availBuf[1] = 0;
}

Sound::~Sound() {
	SoundContext::Ref ctx = m_ctx.lock();
	if (ctx) {
		ZMusic.Get().Dec(kNumStreamingBuffers*kStreamingBufferSize, 0);

		for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
			SoundContext::Source &source = *it;
			if (source.mapped) {
				SoundContext::WriteLock L(ctx->m_m);
				ctx->unmapSource(source);
			}

			if (source.source)
				m_alDriver->sync_deleteSources(ALDRIVER_SIG 1, &source.source);
		}

		for (int i = 0; i < kNumStreamingBuffers; ++i) {
			if (m_sbufs[i])
				m_alDriver->sync_deleteBuffers(ALDRIVER_SIG 1, &m_sbufs[i]);
		}

		// NOTE: in Sound::tick we issue a custom ALDriver command that queries that status
		// of the source (alSourcei(AL_SOURCE_STATUS). This is an asynchronous command that
		// writes to a member of a Source object contained in the m_sources[] vector of this
		// object.
		//
		// There is no chance of contention here (or crashing because we write to this
		// data after the destructor is finished) because in order for us to issue this command
		// the source must have a valid openAL source/buffer attached. That means we call
		// ALDriver::deleteSources() & ALDriver::deleteBuffers() which are both synchronous operations
		// and therefore our custom command is guaranteed to be executed before the destructor returns

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

bool Sound::init(
	const SoundContext::Ref &ctx,
	ALuint buffer,
	int maxInstances
) {
	RAD_ASSERT(maxInstances>0);
	m_sources.resize(maxInstances);
	m_alDriver = ctx->m_alDriver;

	for (int i = 0; i < maxInstances; ++i) {
		SoundContext::Source &source = m_sources[i];
		source.idx = i;
		source.sound = this;
		
		if (!m_alDriver->sync_genSources(ALDRIVER_SIG 1, &source.source))
			return false;
		m_alDriver->sourcei(ALDRIVER_SIG source.source, AL_BUFFER, buffer);
		m_alDriver->sourcei(ALDRIVER_SIG source.source, AL_SOURCE_RELATIVE, AL_TRUE);
	}

	m_ctx = ctx;

	this->refDistance = (float)kDefaultReferenceDistance;
	this->maxDistance = (float)kDefaultMaxDistance;

	return true;
}

bool Sound::init(
	const SoundContext::Ref &ctx,
	const pkg::AssetRef &asset,
	int maxInstances
) {
	if (asset->type != asset::AT_Sound)
		return false;
	asset::SoundLoader::Ref loader = asset::SoundLoader::Cast(asset);
	if (!init(ctx, loader->id, maxInstances))
		return false;
	m_asset = asset;
	return true;
}

bool Sound::initStreaming(
	const SoundContext::Ref &ctx,
	const pkg::AssetRef &asset
) {
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
	m_alDriver = ctx->m_alDriver;

	if (!m_alDriver->sync_genBuffers(ALDRIVER_SIG kNumStreamingBuffers, m_sbufs))
		return false;
	ZMusic.Get().Inc(kNumStreamingBuffers*kStreamingBufferSize, 0);

	memcpy(m_rbufs, m_sbufs, sizeof(ALuint)*kNumStreamingBuffers);
	m_availBuf[0] = 0;
	m_availBuf[1] = kNumStreamingBuffers;

	m_blockData = safe_zone_malloc(ZMusic, kStreamingBufferSize);

	// make streaming source.
	m_sources.resize(1);
	SoundContext::Source &source = m_sources[0];
	source.idx = 0;
	source.sound = this;
	source.stream = true;

	if (!m_alDriver->sync_genSources(ALDRIVER_SIG 1, &source.source))
		return false;
	m_alDriver->sourcei(ALDRIVER_SIG source.source, AL_SOURCE_RELATIVE, AL_TRUE);
	
	m_ctx = ctx;
	m_asset = asset;

	this->refDistance = (float)kDefaultReferenceDistance;
	this->maxDistance = (float)kDefaultMaxDistance;

	return true;
}

void Sound::fadeVolume(float volume, float time)
{
	if (time > 0.f) {
		m_volume[1] = m_volume[0];
		m_volume[2] = volume;
		m_time[0] = 0.f;
		m_time[1] = time;
	} else {
		m_volume[0] = volume;
		m_time[1] = 0.f;
	}
}

bool Sound::play(SoundChannel c, int priority) {
	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return false;
	if (ctx->channelIsPaused(c))
		return false;
	if(m_loop && m_playing>0)
		return true; // already playing.
	if (m_paused>0)
		return false; // must unpause first.

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (!source.mapped) {
			source.priority = priority;
			source.channel = c;
			source.paused = false;
			if (!ctx->mapSource(source))
				return false;
			source.play = true;
			++m_playing; // will start playing via SoundContext::Tick
			break;
		}
	}

	return m_playing>0;
}

void Sound::pause(bool pause) {
	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.mapped && source.paused != pause) {
			if (!ctx->channelIsPaused(source.channel) || pause)
				this->pause(source, pause);
		}
	}
}

void Sound::rewind() {
	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.mapped) {
			SoundContext::WriteLock L(ctx->m_m);
			ctx->unmapSource(source);
		}

		if (!source.source)
			continue;

		if (m_is) { // rewind ogg playback
			
			RAD_ASSERT(m_vbd);
			m_vbd->SeekBytes(0, false);

			// iOS5 bug, just delete the source entirely.
			if (source.source) {
				m_alDriver->sync_deleteSources(ALDRIVER_SIG 1, &source.source);
				m_alDriver->sync_genSources(ALDRIVER_SIG 1, &source.source);
				RAD_ASSERT(source.source);
				m_alDriver->sourcei(
					ALDRIVER_SIG 
					source.source, 
					AL_SOURCE_RELATIVE, 
					m_relative ? AL_TRUE : AL_FALSE
				);
			}
			
			m_availBuf[0] = 0;
			m_availBuf[1] = kNumStreamingBuffers;
			memcpy(m_rbufs, m_sbufs, sizeof(ALuint)*kNumStreamingBuffers);
			m_eos = false;
		} else {
			m_alDriver->sourceRewind(ALDRIVER_SIG source.source);
		}
	}
}

void Sound::stop() {
	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.mapped) {
			SoundContext::WriteLock L(ctx->m_m);
			ctx->unmapSource(source);
		}
	}
}

bool Sound::tick(
	float dt, 
	const SoundContext::Channel &channel,
	SoundContext::Source &source,
	bool positional
) {
	RAD_ASSERT(source.mapped);
	RAD_ASSERT(source.source);

	if (!positional && !m_relative)
		return false; // don't tick positional sounds.

	if (m_pos[1] != m_pos[0]) {
		m_pos[1] = m_pos[0];
		for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
			SoundContext::Source &source = *it;
			if (source.source) {
				m_alDriver->source3f(
					ALDRIVER_SIG 
					source.source, 
					AL_POSITION,
					m_pos[0][0], 
					m_pos[0][1], 
					m_pos[0][2]
				);
			}
		}
	}

	if (m_vel[1] != m_vel[0]) {
		m_vel[1] = m_vel[0];
		for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
			SoundContext::Source &source = *it;
			if (source.source) {
				m_alDriver->source3f(
					ALDRIVER_SIG
					source.source, 
					AL_VELOCITY, 
					m_vel[0][0], 
					m_vel[0][1], 
					m_vel[0][2]
				);
			}
		}
	}

	if (m_rot[1] != m_rot[0]) {
		m_rot[1] = m_rot[0];
		Vec3 fwd(1, 0, 0);
		Mat4 m = Mat4::Rotation(m_rot[0]);
		fwd = fwd * m;

		for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
			SoundContext::Source &source = *it;
			if (source.source) {
				m_alDriver->source3f(
					ALDRIVER_SIG 
					source.source, 
					AL_DIRECTION, 
					fwd[0], 
					fwd[1], 
					fwd[2]
				);
			}
		}
	}

	if (m_time[1] > 0.f) {
		m_time[0] += dt;
		if (m_time[0] >= m_time[1]) {
			m_volume[0] = m_volume[2];
			m_time[1] = 0.f;
		} else {
			m_volume[0] = math::Lerp(m_volume[1], m_volume[2], m_time[0]/m_time[1]);
		}
	}

	if (m_volume[3] != m_volume[0]*channel.volume[0]) {
		m_volume[3] = m_volume[0]*channel.volume[0];
		for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
			SoundContext::Source &source = *it;
			if (source.source)
				m_alDriver->sourcef(ALDRIVER_SIG source.source, AL_GAIN, m_volume[3]);
		}
	}

	if (!source.source) {
		--m_playing;
		return true;
	}

	if (m_is)
		return false;

	if (source.status == AL_STOPPED) {
		--m_playing;
		return true;
	}

	if (source.status == AL_PAUSED && !m_paused && !channel.paused) {
		m_alDriver->sourcePlay(ALDRIVER_SIG source.source);
		source.status = AL_PLAYING;
	} else {
		ALDriver::Command *cmd = m_alDriver->createCommand(&fn_getSourceStatus);
		ALDRIVER_CMD_SIG_SRC(*cmd);
		cmd->args.pvoid = &source;
		m_alDriver->submit(*cmd);
	}

	return false;
}

void Sound::fn_getSourceStatus(ALDriver &driver, ALDriver::Command *cmd) {
	SoundContext::Source *source = reinterpret_cast<SoundContext::Source*>(cmd->args.pvoid);
	ALint status;
	alGetSourcei(source->source, AL_SOURCE_STATE, &status);
	source->status = status;
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

Sound::StreamResult Sound::tickStream(SoundContext::Source &source)
{ // NOTE: Called on ALDriver thread, safe to call OpenAL directly in this function.
	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return kStreamResult_Finished;

	if (!source.source)
		return kStreamResult_Finished;
	if (source.paused)
		return kStreamResult_Playing;
	
	if (m_eos) {
		ALint num;
		alGetSourcei(source.source, AL_BUFFERS_QUEUED, &num);
		return (num > 0) ? kStreamResult_Playing : kStreamResult_Finished;
	}
	
	// unqueue AL buffers if we don't have anything to read in.
	// i.e. all buffers are full of data.
	if (m_availBuf[0] == m_availBuf[1]) {
		ALint num;
		alGetSourcei(source.source, AL_BUFFERS_PROCESSED, &num);
		if (num < 1) {
			ALint state;
			alGetSourcei(source.source, AL_SOURCE_STATE, &state);
			if (state != AL_PLAYING)
				alSourcePlay(source.source);
			return kStreamResult_Playing;
		}
		alSourceUnqueueBuffers(source.source, num, m_rbufs);
		m_availBuf[1] = num;
		m_availBuf[0] = 0;
	}

	AddrSize bytesDecoded = 0;
	
	while ((m_availBuf[0] != m_availBuf[1]) && !m_eos && (bytesDecoded < kMaxBytesDecodedPerTick)) {
		// decode another block.
		
		while ((m_decodeOfs < kStreamingBufferSize) && (bytesDecoded < kMaxBytesDecodedPerTick)) {
			AddrSize x;

			AddrSize bytesToDecode = std::min(kStreamingBufferSize-m_decodeOfs, kMaxBytesDecodedPerTick-bytesDecoded);
			RAD_ASSERT(bytesToDecode > 0);
			
			m_vbd->Decode(
				((U8*)m_blockData)+m_decodeOfs,
				bytesToDecode,
				audio_codec::ogg_vorbis::EM_Little,
				audio_codec::ogg_vorbis::ST_16Bit,
				audio_codec::ogg_vorbis::DT_Signed,
				0,
				x
			);
			
			if (0 == x) {
				if (m_loop) {
					m_vbd->SeekBytes(0, false);
				} else {
					m_eos = true;
					break;
				}
			}
			
			m_decodeOfs += x;
			bytesDecoded += x;
		}
		
		if ((m_decodeOfs > 0) && (m_eos || (m_decodeOfs == kStreamingBufferSize))) {
			int idx = m_availBuf[0]++;
			RAD_ASSERT(idx < kNumStreamingBuffers);
			
			if (idx < kNumStreamingBuffers) {
				ALuint buf = m_rbufs[idx];
				
				alBufferData(
					buf,
					(m_bsi->channels==2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
					m_blockData,
					(ALsizei)m_decodeOfs,
					m_bsi->rate
				);
				
				if (source.source)
					alSourceQueueBuffers(source.source, 1, &buf);
			}

			m_decodeOfs = 0;
		}
	}

	if (source.mapped && source.source) {
		ALint state;
		alGetSourcei(source.source, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
			alSourcePlay(source.source);
	}

	return kStreamResult_Decoding;
}

void Sound::pause(SoundContext::Source &source, bool pause)
{
	RAD_ASSERT(source.source);
	RAD_ASSERT(source.paused != pause);

	source.paused = pause;
	if (pause) {
		++m_paused;
		if (source.source)
			m_alDriver->sourcePause(ALDRIVER_SIG source.source);
	} else {
		--m_paused;
		// will resume in SoundContext::Tick();
	}
}

void Sound::stop(SoundContext::Source &source)
{
	RAD_ASSERT(source.source);
	RAD_ASSERT(m_playing>0);
	if (source.paused) {
		--m_paused;
		source.paused = false;
	}
	
	if (source.source)
		m_alDriver->sourceStop(ALDRIVER_SIG source.source);

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

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source) {
			m_alDriver->sourcei(
				ALDRIVER_SIG 
				source.source, 
				AL_LOOPING, 
				m_loop ? AL_TRUE : AL_FALSE
			);
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

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source) {
			m_alDriver->sourcei(
				ALDRIVER_SIG
				source.source, 
				AL_SOURCE_RELATIVE, 
				value ? AL_TRUE : AL_FALSE
			);
			
			if (!value) // force muted until we tick
				m_alDriver->sourcef(ALDRIVER_SIG source.source, AL_GAIN, 0.f);
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(innerAngle)(float value) {
	if (m_angles[0] == value)
		return;
	m_angles[0] = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source) {
			m_alDriver->sourcef(
				ALDRIVER_SIG 
				source.source, 
				AL_CONE_INNER_ANGLE, 
				value
			);
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(outerAngle)(float value) {
	if (m_angles[1] == value)
		return;
	m_angles[1] = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source)
			m_alDriver->sourcef(ALDRIVER_SIG source.source, AL_CONE_OUTER_ANGLE, value);
	}
}

void Sound::RAD_IMPLEMENT_SET(outerVolume)(float value) {
	if (m_outerVolume == value)
		return;
	m_outerVolume = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source)
			m_alDriver->sourcef(ALDRIVER_SIG source.source, AL_CONE_OUTER_GAIN, value);
	}
}

void Sound::RAD_IMPLEMENT_SET(minVolume)(float value) {
	if (m_minMaxVolume[0] == value)
		return;
	m_minMaxVolume[0] = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source) {
			m_alDriver->sourcef(
				ALDRIVER_SIG 
				source.source, 
				AL_MIN_GAIN, 
				math::Clamp(value, 0.f, 1.f)
			);
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(maxVolume)(float value) {
	if (m_minMaxVolume[1] == value)
		return;
	m_minMaxVolume[1] = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source) {
			m_alDriver->sourcef(
				ALDRIVER_SIG
				source.source, 
				AL_MAX_GAIN, 
				math::Clamp(value, 0.f, 1.f)
			);
		}
	}
}

void Sound::RAD_IMPLEMENT_SET(pitch)(float value) {
	if (m_pitch == value)
		return;
	m_pitch = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source)
			m_alDriver->sourcef(ALDRIVER_SIG source.source, AL_PITCH, value);
	}
}

void Sound::RAD_IMPLEMENT_SET(rolloff)(float value) {
	if (m_rolloff == value)
		return;
	m_rolloff = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source)
			m_alDriver->sourcef(ALDRIVER_SIG source.source, AL_ROLLOFF_FACTOR, value);
	}
}

void Sound::RAD_IMPLEMENT_SET(refDistance)(float value) {
	if (m_refDistance == value)
		return;
	m_refDistance = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source)
			m_alDriver->sourcef(ALDRIVER_SIG source.source, AL_REFERENCE_DISTANCE, value);
	}
}

void Sound::RAD_IMPLEMENT_SET(maxDistance)(float value) {
	if (m_maxDistance == value)
		return;
	m_maxDistance = value;

	SoundContext::Ref ctx = m_ctx.lock();
	if (!ctx)
		return;

	for (SourceVec::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
		SoundContext::Source &source = *it;
		if (source.source)
			m_alDriver->sourcef(ALDRIVER_SIG source.source, AL_MAX_DISTANCE, value);
	}
}
