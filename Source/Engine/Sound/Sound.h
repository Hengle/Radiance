// Sound.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "../Packages/PackagesDef.h"
#include "SoundDef.h"
#include "ALDriver.h"
#include <Runtime/File.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/StreamDef.h>
#include <Runtime/AudioCodec/VorbisDef.h>
#include <Runtime/Thread/Locks.h>
#include <Runtime/Thread/Thread.h>
#include <Runtime/PushPack.h>

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS SoundContext : public boost::enable_shared_from_this<SoundContext> {
public:
	typedef SoundContextRef Ref;
	typedef SoundContextWRef WRef;

	static Ref New(const ALDriver::Ref &driver);

	~SoundContext();

	void Tick(float dt, bool positional);

	void FadeMasterVolume(float volume, float time);

	void FadeChannelVolume(SoundChannel c, float volume, float time);
	float ChannelVolume(SoundChannel c) { return m_channels[c].volume[0]; }

	void PauseChannel(SoundChannel c, bool pause=true);
	bool ChannelIsPaused(SoundChannel c) { return m_channels[c].paused; }

	void PauseAll(bool pause=true);
	void StopAll();

	void SetDoppler(float dopplerFactor, float speedOfSound);

	SoundRef NewSound(ALuint buffer, int maxInstances=1);
	SoundRef NewSound(const pkg::AssetRef &sound, int maxInstances=1);

	RAD_DECLARE_PROPERTY(SoundContext, pos, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(SoundContext, vel, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(SoundContext, rot, const Quat&, const Quat&);
	RAD_DECLARE_PROPERTY(SoundContext, distanceModel, ALenum, ALenum);
	RAD_DECLARE_READONLY_PROPERTY(SoundContext, masterVolume, float);

private:

	RAD_DECLARE_GET(pos, const Vec3&) { 
		return m_pos[0]; 
	}

	RAD_DECLARE_SET(pos, const Vec3&) { 
		m_pos[0] = value; 
	}

	RAD_DECLARE_GET(vel, const Vec3&) { 
		return m_vel[0]; 
	}

	RAD_DECLARE_SET(vel, const Vec3&) { 
		m_vel[0] = value; 
	}

	RAD_DECLARE_GET(rot, const Quat&) { 
		return m_rot[0]; 
	}

	RAD_DECLARE_SET(rot, const Quat&) { 
		m_rot[0] = value; 
	}

	RAD_DECLARE_GET(distanceModel, ALenum) { 
		return m_distanceModel; 
	}

	RAD_DECLARE_SET(distanceModel, ALenum);

	RAD_DECLARE_GET(masterVolume, float) { 
		return m_volume[0]; 
	}

	SoundContext(const ALDriver::Ref &driver);

	struct Source {
		typedef zone_list<Source*, ZSoundT>::type List;
		typedef zone_map<int, List, ZSoundT>::type Map;
		
		Source() : 
			source(0), 
			idx(-1), 
			sound(0), 
			mapped(false), 
			play(false), 
			paused(false), 
			stream(false), 
			status(AL_INITIAL),
			channel(SC_Max) {}

		ALuint source;
		SoundChannel channel;
		int idx;
		int priority;
		Sound *sound;
		List::iterator it[3];
		bool play;
		bool mapped;
		bool paused;
		bool stream;
		volatile ALint status;
	};

	class Channel {
	public:
		Channel() : paused(false) {
			volume[0] = volume[1] = volume[2] = 1.f;
			time[0] = time[1] = 0.f;
		}

		Source::Map sources;
		float volume[3];
		float time[2];
		bool paused;
	};

	class StreamCallback : public ALDriver::Callback {
	public:
		StreamCallback(SoundContext &ctx);

		virtual void Tick(ALDriver &alDriver);

	private:

		SoundContext &m_ctx;
	};

	friend class Sound;
	friend class StreamCallback;

	bool Evict(int priority);
	bool MapSource(Source &source);
	void UnmapSource(Source &source);
	int TickStreams(ALDriver &alDriver);

	typedef boost::shared_mutex Mutex;
	typedef boost::lock_guard<Mutex> WriteLock;
	typedef boost::shared_lock<Mutex> ReadLock;
	typedef thread::upgrade_to_exclusive_lock<Mutex> UpgradeToWriteLock;

	Mutex m_m;
	Source::Map m_sources;
	Source::List m_streams;
	boost::array<Channel, SC_Max> m_channels;
	Vec3 m_pos[2];
	Vec3 m_vel[2];
	Quat m_rot[2];
	float m_volume[4];
	float m_time[2];
	int m_numSources;
	ALenum m_distanceModel;
	ALDriver::Ref m_alDriver;
	ALDriver::Callback::Ref m_streamCallback;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Sound
{
public:
	~Sound();
	typedef SoundRef Ref;
	typedef SoundWRef WRef;

	RAD_DECLARE_PROPERTY(Sound, pos, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(Sound, vel, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(Sound, rot, const Quat&, const Quat&);
	RAD_DECLARE_PROPERTY(Sound, loop, bool, bool);
	RAD_DECLARE_PROPERTY(Sound, sourceRelative, bool, bool); 
	RAD_DECLARE_PROPERTY(Sound, innerAngle, float, float);
	RAD_DECLARE_PROPERTY(Sound, outerAngle, float, float);
	RAD_DECLARE_PROPERTY(Sound, outerVolume, float, float);
	RAD_DECLARE_PROPERTY(Sound, minVolume, float, float);
	RAD_DECLARE_PROPERTY(Sound, maxVolume, float, float);
	RAD_DECLARE_PROPERTY(Sound, pitch, float, float);
	RAD_DECLARE_PROPERTY(Sound, rolloff, float, float);
	RAD_DECLARE_PROPERTY(Sound, refDistance, float, float);
	RAD_DECLARE_PROPERTY(Sound, maxDistance, float, float);
	RAD_DECLARE_READONLY_PROPERTY(Sound, volume, float);
	RAD_DECLARE_READONLY_PROPERTY(Sound, playing, bool);
	RAD_DECLARE_READONLY_PROPERTY(Sound, paused, bool);
	RAD_DECLARE_READONLY_PROPERTY(Sound, context, SoundContext::Ref);
	RAD_DECLARE_READONLY_PROPERTY(Sound, asset, const pkg::AssetRef&);

	void FadeVolume(
		float volume, 
		float time, 
		bool fadeOutAndStop = true /* only applies when fading to zero volume */
	);

	bool Play(SoundChannel c, int priority);
	void Pause(bool pause = true);
	void Rewind();
	void Stop();

private:

	friend class SoundContext;

	RAD_DECLARE_GET(pos, const Vec3&) { 
		return m_pos[0]; }

	RAD_DECLARE_SET(pos, const Vec3&) { 
		m_pos[0] = value; 
	}

	RAD_DECLARE_GET(vel, const Vec3&) { 
		return m_vel[0]; 
	}

	RAD_DECLARE_SET(vel, const Vec3&) { 
		m_vel[0] = value; 
	}

	RAD_DECLARE_GET(rot, const Quat&) { 
		return m_rot[0]; 
	}

	RAD_DECLARE_SET(rot, const Quat&) { 
		m_rot[0] = value; 
	}

	RAD_DECLARE_GET(volume, float) { 
		return m_volume[0]; 
	}

	RAD_DECLARE_GET(loop, bool) { 
		return m_loop; 
	}

	RAD_DECLARE_SET(loop, bool);

	RAD_DECLARE_GET(sourceRelative, bool) { 
		return m_relative; 
	}

	RAD_DECLARE_SET(sourceRelative, bool);

	RAD_DECLARE_GET(innerAngle, float) { 
		return m_angles[0]; 
	}

	RAD_DECLARE_SET(innerAngle, float);

	RAD_DECLARE_GET(outerAngle, float) { 
		return m_angles[0]; 
	}

	RAD_DECLARE_SET(outerAngle, float);

	RAD_DECLARE_GET(outerVolume, float) { 
		return m_outerVolume; 
	}

	RAD_DECLARE_SET(outerVolume, float);

	RAD_DECLARE_GET(minVolume, float) { 
		return m_minMaxVolume[0]; 
	}

	RAD_DECLARE_SET(minVolume, float);

	RAD_DECLARE_GET(maxVolume, float) { 
		return m_minMaxVolume[1]; 
	}

	RAD_DECLARE_SET(maxVolume, float);

	RAD_DECLARE_GET(pitch, float) { 
		return m_pitch; 
	}

	RAD_DECLARE_SET(pitch, float);

	RAD_DECLARE_GET(rolloff, float) { 
		return m_rolloff; 
	}

	RAD_DECLARE_SET(rolloff, float);

	RAD_DECLARE_GET(refDistance, float) {
		return m_refDistance; 
	}

	RAD_DECLARE_SET(refDistance, float);

	RAD_DECLARE_GET(maxDistance, float) { 
		return m_maxDistance; 
	}

	RAD_DECLARE_SET(maxDistance, float);

	RAD_DECLARE_GET(playing, bool) { 
		return m_playing>0; 
	}

	RAD_DECLARE_GET(paused, bool) { 
		return m_paused>0; 
	}

	RAD_DECLARE_GET(context, SoundContext::Ref) { 
		return m_ctx.lock(); 
	}

	RAD_DECLARE_GET(asset, const pkg::AssetRef&) { 
		return m_asset; 
	}

	Sound();

	enum {
		kNumStreamingBuffers = 2
	};

	enum StreamResult {
		kStreamResult_Playing,
		kStreamResult_Decoding,
		kStreamResult_Finished
	};

	bool Init(
		const SoundContext::Ref &ctx,
		ALuint buffer,
		int maxInstances
	);

	bool Init(
		const SoundContext::Ref &ctx,
		const pkg::AssetRef &asset,
		int maxInstances
	);

	bool InitStreaming(
		const SoundContext::Ref &ctx,
		const pkg::AssetRef &asset
	);

	bool Tick(
		float dt, 
		const SoundContext::Channel &channel,
		SoundContext::Source &source,
		bool positional
	);

	StreamResult TickStream(SoundContext::Source &source);

	void Pause(SoundContext::Source &source, bool pause);
	void Stop(SoundContext::Source &source);
	static void fn_GetSourceStatus(ALDriver &driver, ALDriver::Command *cmd);

	typedef zone_vector<SoundContext::Source, ZSoundT>::type SourceVec;

	Vec3 m_pos[2];
	Vec3 m_vel[2];
	Quat m_rot[2];
	ALuint m_sbufs[kNumStreamingBuffers];
	ALuint m_rbufs[kNumStreamingBuffers];
	int m_availBuf[2];
	stream::InputStream *m_is;
	audio_codec::ogg_vorbis::Decoder *m_vbd;
	audio_codec::ogg_vorbis::BSI *m_bsi;
	file::MMFileInputBuffer::Ref m_ib;
	void *m_blockData;
	AddrSize m_decodeOfs;
	bool m_eos;
	SourceVec m_sources;
	SoundContext::WRef m_ctx;
	ALDriver::Ref m_alDriver;
	pkg::AssetRef m_asset;
	bool m_loop;
	bool m_relative;
	bool m_fadeOutAndStop;
	float m_volume[4];
	float m_time[2];
	float m_angles[2];
	float m_outerVolume;
	float m_minMaxVolume[2];
	float m_pitch;
	float m_rolloff;
	float m_maxDistance;
	float m_refDistance;
	int m_playing;
	int m_paused;
};

#include <Runtime/PopPack.h>
