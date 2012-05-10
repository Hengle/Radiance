// SkControllers.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Skeletal Animation
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "SkAnim.h"
#include <Runtime/Base/ObjectPool.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

namespace ska {

///////////////////////////////////////////////////////////////////////////////

struct Variant
{
	typedef zone_vector<Variant, ZSkaT>::type Vec;
	float timeScale[2];
	int loopCount[2];
	float weight;
	float in;
	float out;
	String name;
};

struct AnimState
{
	typedef zone_map<String, AnimState, ZSkaT>::type Map;
	String name;
	Variant::Vec variants;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS BlendTimer
{
public:
	BlendTimer();
	~BlendTimer();

	void Start(float time);
	void Tick(float dt);
	
	RAD_DECLARE_READONLY_PROPERTY(BlendTimer, frac, float);
	RAD_DECLARE_READONLY_PROPERTY(BlendTimer, finished, bool);

private:

	RAD_DECLARE_GET(frac, float);
	RAD_DECLARE_GET(finished, bool);

	float m_dt;
	float m_time;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Controller
{
public:
	typedef ControllerRef Ref;
	typedef ControllerWRef WRef;

	Controller(Ska &ska);
	virtual ~Controller();

	virtual bool Tick(
		float dt,
		BoneTM *out,
		int firstBone,
		int numBones,
		bool advance,
		bool emitTags
	) = 0;

	void Activate(bool active=true);

	RAD_DECLARE_READONLY_PROPERTY(Controller, rot, const Quat&);
	RAD_DECLARE_READONLY_PROPERTY(Controller, pos, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(Controller, deltaRot, const Quat&);
	RAD_DECLARE_READONLY_PROPERTY(Controller, deltaPos, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(Controller, ska, Ska*);
	RAD_DECLARE_READONLY_PROPERTY(Controller, in, float);
	RAD_DECLARE_READONLY_PROPERTY(Controller, out, float);

	// If set to true, then the animation motion will be
	// blended if using a BlendToController
	// Default: false
	RAD_DECLARE_PROPERTY(Controller, blendTM, bool, bool); 

	static void Blend(
		BoneTM *out,
		BoneTM *from,
		BoneTM *to,
		float weight,
		int firstBone,
		int numBones
	);

	static Quat Slerp(const Quat &from, const Quat &to, float t);

protected:

	virtual RAD_DECLARE_GET(in, float) = 0;
	virtual RAD_DECLARE_GET(out, float) = 0;

	virtual void OnActivate(bool active=true) {}

	BoneTM::Ref AllocBoneArray();
	void SetRot(const Quat &q) { m_q = q; }
	void SetPos(const Vec3 &p) { m_p = p; }
	void SetDeltaRot(const Quat &q) { m_dq = q; }
	void SetDeltaPos(const Vec3 &p) { m_dp = p; }

private:

	friend class Ska;

	RAD_DECLARE_GET(rot, const Quat&) { return m_q; }
	RAD_DECLARE_GET(pos, const Vec3&) { return m_p; }
	RAD_DECLARE_GET(deltaRot, const Quat&) { return m_dq; }
	RAD_DECLARE_GET(deltaPos, const Vec3&) { return m_dp; }
	RAD_DECLARE_GET(ska, Ska*) { return m_ska; }
	RAD_DECLARE_GET(blendTM, bool) { return m_blendTM; }
	RAD_DECLARE_SET(blendTM, bool) { m_blendTM = value; }

	Ska *m_ska;
	Quat m_q;
	Quat m_dq;
	Vec3 m_p;
	Vec3 m_dp;
	bool m_blendTM;
	bool m_active;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS AnimationSource : public Controller
{
public:

	typedef boost::shared_ptr<AnimationSource> Ref;
	typedef boost::weak_ptr<AnimationSource> WRef;

	~AnimationSource();

	static Ref New(
		Animation &anim,
		float in,
		float out,
		float timeScale,
		int loopCount, // 0 == loop forever
		bool loopPastEnd, // continue looping after loopCount?
		Ska &ska,
		const Notify::Ref &notify
	);

	virtual bool Tick(
		float dt,
		BoneTM *out,
		int firstBone,
		int numBones,
		bool advance,
		bool emitTags
	);

	void ResetLoopCount(int loopCount);
	void SetTime(float dt);

	// set to true when loopCount has been reached.
	RAD_DECLARE_READONLY_PROPERTY(AnimationSource, finished, bool);
	RAD_DECLARE_PROPERTY(AnimationSource, timeScale, float, float);

protected:

	virtual RAD_DECLARE_GET(in, float) { return m_in; }
	virtual RAD_DECLARE_GET(out, float) { return m_out; }

	virtual void OnActivate(bool active=true);

private:

	RAD_DECLARE_GET(finished, bool) { return m_loopCount > 0 ? (m_loopNum==m_loopCount) : false; }
	RAD_DECLARE_GET(timeScale, float) { return m_timeScale; }
	RAD_DECLARE_SET(timeScale, float) { m_timeScale = value; }

	AnimationSource(
		Animation &anim,
		float in,
		float out,
		float timeScale,
		int loopCount, // 0 == loop forever
		bool loopPastEnd,
		Ska &ska,
		const Notify::Ref &notify
	);

	void EmitTags(int firstBone, int numBones);
	void EmitTags(int frame, int numFrames, int firstBone, int numBones);

	static void Delete(AnimationSource *s);

	Notify::Ref m_notify;
	Animation *m_anim;
	float m_frame;
	float m_in;
	float m_out;
	float m_timeScale;
	int m_loopNum;
	int m_loopCount;
	int m_emitFrame;
	bool m_loopPastEnd;
	bool m_emitEndFrame;
	BoneTM m_tm;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS BlendToController : public Controller
{
public:

	typedef boost::shared_ptr<BlendToController> Ref;
	typedef boost::weak_ptr<BlendToController> WRef;

	static Ref New(
		Ska &ska,
		const Notify::Ref &notify
	);

	virtual ~BlendToController();

	void BlendTo(const Controller::Ref &to);

	virtual bool Tick(
		float dt,
		BoneTM *out,
		int firstBone,
		int numBones,
		bool advance,
		bool emitTags
	);

	RAD_DECLARE_READONLY_PROPERTY(BlendToController, validRootAnim, bool);

protected:

	virtual void OnActivate(bool active=true);

private:

	BlendToController(
		Ska &ska,
		const Notify::Ref &notify
	);

	virtual RAD_DECLARE_GET(in, float);
	virtual RAD_DECLARE_GET(out, float);
	RAD_DECLARE_GET(validRootAnim, bool) { return m_root; }

	class Blend;
	typedef boost::shared_ptr<Blend> BlendRef;
	struct Union
	{
		BlendRef blend;
		Controller::Ref controller;
	};

	class Blend
	{
	public:
		typedef boost::shared_ptr<Blend> Ref;
		static void Delete(Blend *b);
		static Ref New();

		void Activate(bool activate);

		bool Tick(
			float dt, 
			BoneTM &tm,
			BoneTM &delta,
			BoneTM *out,
			int firstBone,
			int numBones,
			bool advance,
			bool emitTags
		);

		RAD_DECLARE_READONLY_PROPERTY(Blend, out, float);
		RAD_DECLARE_READONLY_PROPERTY(Blend, in, float);

		Union a;
		Union b;
		BlendTimer blend;
		BoneTM::Ref bones;

	private:

		RAD_DECLARE_GET(out, float);
		RAD_DECLARE_GET(in, float);
	};

	static ThreadSafeObjectPool<Blend> s_blendPool;
	Blend::Ref m_root;
	Notify::Ref m_notify;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS AnimationVariantsSource : public Controller
{
public:

	typedef boost::shared_ptr<AnimationVariantsSource> Ref;
	typedef boost::weak_ptr<AnimationVariantsSource> WRef;

	static Ref New(
		const Variant::Vec &anims,
		Ska &ska,
		const Notify::Ref &notify,
		const char *blendTarget = 0
	);

	virtual ~AnimationVariantsSource();

	virtual bool Tick(
		float dt,
		BoneTM *out,
		int firstBone,
		int numBones,
		bool advance,
		bool emitTags
	);

	Ref Clone(const Notify::Ref &notify);

	void SetTime(float dt);

protected:

	virtual void OnActivate(bool active=true);

private:

	AnimationVariantsSource(
		const Variant::Vec &anims, 
		Ska &ska,
		const Notify::Ref &notify,
		const char *blendTarget
	);

	AnimationVariantsSource(const AnimationVariantsSource &s);

	virtual RAD_DECLARE_GET(in, float) { return m_blend ? m_blend->in : 0.f; }
	virtual RAD_DECLARE_GET(out, float) { return m_blend ? m_blend->out : 0.f; }

	struct Node
	{
		typedef zone_map<int, Node, ZSkaT>::type Map;
		Animation *anim;
		float in;
		float out;
		float timeScale[2];
		int loopCount[2];
	};

	void Init(const Variant::Vec &anims);
	void ChooseAnim();

	static void Delete(AnimationVariantsSource *s);

	Notify::Ref m_notify;
	Node::Map m_map;
	BoneTM m_tm;
	Node *m_node;
	Node *m_blendTarget;
	AnimationSource::Ref m_source;
	BlendToController::Ref m_blend;
};

} // ska

#include <Runtime/PopPack.h>