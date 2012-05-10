// WorldCinematics.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "WorldDraw.h"
#include "BSPFile.h"
#include "../Renderer/SkMesh.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Container/ZoneSet.h>

namespace world {

class World;

class RADENG_CLASS WorldCinematics
{
public:
	typedef boost::shared_ptr<WorldCinematics> Ref;

	enum CinematicFlags
	{
		RAD_FLAG(CF_AnimateCamera),
		RAD_FLAG(CF_CanPlayForever),
		RAD_FLAG(CF_Loop)
	};

	class RADENG_CLASS Notify
	{
	public:
		typedef boost::shared_ptr<Notify> Ref;
		virtual ~Notify () {}
		virtual void OnTag(const char *str) = 0;
		virtual void OnComplete() = 0;
		virtual void OnSkip() = 0;
	};

	void Tick(int frame, float dt);
	
	//! Plays the specified cinematic with options.
	/*! \param notify Passing in a valid notify object will redirect all animation tag
		data to the specified notify object, and bypass the normal World->PostEvent() processing.
	*/
	bool PlayCinematic(
		const char *name, 
		int flags,
		float xfadeCamera,
		const Notify::Ref &notify
	);

	void StopCinematic(const char *name);
	float CinematicTime(const char *name);
	bool SetCinematicTime(const char *name, float time);
	void Skip();

	RAD_DECLARE_READONLY_PROPERTY(WorldCinematics, cameraActive, bool);

private:

	RAD_DECLARE_GET(cameraActive, bool) { return m_cameraActive; }

	int Spawn(const bsp_file::BSPFile::Ref &bsp, const xtime::TimeSlice &time, int flags);

	class Actor
	{
	public:
		typedef boost::shared_ptr<Actor> Ref;
		typedef zone_vector<Ref, ZWorldT>::type Vec;

		r::SkMesh::Ref m;
		int flags;
		int frame;
		bool visible;
		bool loop;
	};

	class SkActorBatch : public MBatchDraw
	{
	public:
		typedef boost::shared_ptr<SkActorBatch> Ref;

		SkActorBatch(const Actor::Ref &actor, int idx, int matId);

		virtual void Bind(r::Shader *shader);
		virtual void CompileArrayStates(r::Shader &shader);
		virtual void FlushArrayStates(r::Shader *shader);
		virtual void Draw();

		virtual RAD_DECLARE_GET(visible, bool) { return m_actor->visible; }
		virtual RAD_DECLARE_GET(rgba, const Vec4&) { return s_rgba; }
		virtual RAD_DECLARE_GET(scale, const Vec3&) { return s_scale; }
		virtual RAD_DECLARE_GET(xform, bool) { return true; }

	private:

		static Vec4 s_rgba;
		static Vec3 s_scale;

		Actor::Ref m_actor;
		int m_idx;
	};

	typedef zone_set<int, ZWorldT>::type IntSet;

	struct Cinematic
	{
		typedef boost::shared_ptr<Cinematic> Ref;
		typedef zone_list<Ref, ZWorldT>::type List;

		String name;
		Notify::Ref notify;
		const bsp_file::BSPCinematicTrigger *trigger;
		const bsp_file::BSPCameraTrack *track;
		const bsp_file::BSPCinematic *cinematic;
		IntSet actors;
		bool camera;
		float frame[2];
		float xfade[2];
		int emitFrame;
		int triggerNum;
		int updateFrame;
		int flags;
		int loopCount;
		bool done;
		bool suppressFinish;
	};

	friend struct SkaNotify;

	struct SkaNotify : public ska::Notify
	{
	public:

		SkaNotify(WorldCinematics &w, Cinematic &c) : m_w(&w), m_c(&c) {}

		virtual void OnTag(const ska::AnimTagEventData &data);
		virtual void OnEndFrame(const ska::AnimStateEventData &data) {}
		virtual void OnFinish(const ska::AnimStateEventData &data, bool masked) {}
	private:
		WorldCinematics *m_w;
		Cinematic *m_c;
	};

	friend class World;
	WorldCinematics(World *w);

	void EmitCameraTags(Cinematic &c);
	void BlendCameraFrame(Cinematic &c);

	bsp_file::BSPFile::Ref m_bspFile;
	Cinematic::List m_cinematics;
	Actor::Vec m_actors;
	bool m_cameraActive;
	bool m_cameraEnabled;
	World *m_world;
	int m_spawnOfs;
	bool m_inUpdate;
};

} // world