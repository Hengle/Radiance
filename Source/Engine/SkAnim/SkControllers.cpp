// SkControllers.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Skeletal Animation
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#include "SkControllers.h"
#include <cstdlib>
#include <algorithm>
#undef max
#undef emit

namespace ska {

namespace details {

void BlendBones(BoneTM *out, const BoneTM *src, const BoneTM *dst, float weight, int first, int num);
Quat Slerp(const Quat &from, const Quat &to, float t);
void IdentBones(BoneTM *out, int first, int num);

} // details

///////////////////////////////////////////////////////////////////////////////

BlendTimer::BlendTimer() : m_time(0.f), m_dt(0.f) {
}

BlendTimer::~BlendTimer() {
}

void BlendTimer::Start(float time) {
	m_dt = 0.f;
	m_time = time;
}

void BlendTimer::Tick(float dt) {
	m_dt += dt;
}

bool BlendTimer::RAD_IMPLEMENT_GET(finished) {
	return m_dt >= m_time;
}

float BlendTimer::RAD_IMPLEMENT_GET(frac) {
	return finished ? 1.f : m_dt / m_time;
}

///////////////////////////////////////////////////////////////////////////////

Controller::Controller(Ska &ska) : 
m_ska(&ska), 
m_q(Quat::Identity), 
m_p(Vec3::Zero), 
m_dq(Quat::Identity),
m_dp(Vec3::Zero),
m_blendTM(false),
m_active(false) {
}

Controller::~Controller() {
}

void Controller::Activate(bool active) {
	if (active == m_active)
		return;
	m_active = active;
	OnActivate(active);
}

BoneTM::Ref Controller::AllocBoneArray() {
	return BoneTM::Ref(new (ZSka) BoneTM[m_ska->numBones.get()]);
}

void Controller::Blend(
	BoneTM *out,
	BoneTM *from,
	BoneTM *to,
	float weight,
	int firstBone,
	int numBones
) {
	details::BlendBones(out, from, to, weight, firstBone, numBones);
}

Quat Controller::Slerp(const Quat &from, const Quat &to, float t) {
	return details::Slerp(from, to, t);
}

///////////////////////////////////////////////////////////////////////////////

ThreadSafeMemoryPool s_animationSourcePool(ZSka, "ska-anim-source", sizeof(AnimationSource), 8);

AnimationSource::Ref AnimationSource::New(
	const Animation &anim,
	float in,
	float out,
	float timeScale,
	int loopCount, // 0 == loop forever
	AnimState::MoveType moveType,
	Ska &ska,
	const Notify::Ref &notify
) {
	return Ref(
		new (s_animationSourcePool.SafeGetChunk()) AnimationSource(
			anim, 
			in, 
			out, 
			timeScale, 
			loopCount,
			moveType,
			ska,
			notify
		), &Delete
	);
}

void AnimationSource::Delete(AnimationSource *s) {
	s->~AnimationSource();
	s_animationSourcePool.ReturnChunk(s);
}

AnimationSource::AnimationSource(
	const Animation &anim,
	float in,
	float out,
	float timeScale,
	int loopCount, // 0 == loop forever
	AnimState::MoveType moveType,
	Ska &ska,
	const Notify::Ref &notify
) : 
Controller(ska),
m_anim(&anim),
m_in(in), 
m_out(out), 
m_timeScale(timeScale), 
m_frame(0.f),
m_notify(notify),
m_emitEndFrame(true),
m_moveType(moveType),
m_emitFrame(0) {
	m_loopCount[0] = 0;
	m_loopCount[1] = loopCount;
}

AnimationSource::~AnimationSource() {
}

void AnimationSource::OnActivate(bool active) {
	if (active) {
		m_frame = 0.f;
		m_loopCount[0] = 0;

		// get root tm
		m_anim->BlendFrames(
			0,
			0,
			0.f,
			&m_bipZero,
			0,
			1
		);

		m_tm = m_bipZero;
		SetPos(m_tm.t);
		SetRot(m_tm.r);
		SetDeltaRot(Quat::Identity);
		SetDeltaPos(Vec3::Zero);
	} else {
		if (m_notify) {
			AnimStateEventData d;
			d.ska = ska;
			d.anim = m_anim;
			m_notify->EmitFinish(d, true);
		}
	}
}

void AnimationSource::EmitTags(int firstBone, int numBones) {
	int current = FloatToInt(m_frame)+1;

	if (m_emitFrame > current) { // wrapped
		if (m_emitFrame < m_anim->numFrames)
			EmitTags(m_emitFrame, (m_anim->numFrames-m_emitFrame), firstBone, numBones);
		m_emitFrame = 0;
	}

	if (m_emitFrame < current) {
		EmitTags(m_emitFrame, (current-m_emitFrame), firstBone, numBones);
		m_emitFrame = current;
	}
}

void AnimationSource::EmitTags(int frame, int numFrames, int firstBone, int numBones) {
	m_anim->EmitTags(
		frame,
		numFrames,
		firstBone,
		numBones,
		m_notify
	);
}

void AnimationSource::ResetLoopCount(int loopCount) {
	m_loopCount[0] = 0;
	m_loopCount[1] = loopCount;
}

void AnimationSource::SetTime(float dt) {
	m_frame = (dt*m_timeScale) * m_anim->fps;

	if (m_frame > m_anim->numFrames.get()-1) {
		if (!m_loopCount) {
			if (m_anim->numFrames > 1) {
				// wrap
				m_frame = math::Mod(m_frame, (float)m_anim->numFrames.get()-1);
			} else {
				m_frame = 0;
			}
		} else {
			m_frame = (float)m_anim->numFrames.get()-1; // clamp to last frame.
		}
	}
}

bool AnimationSource::Tick(
	float dt,
	float distance,
	BoneTM *out,
	int firstBone,
	int numBones,
	bool advance,
	bool emitTags
) {
	emitTags = emitTags && advance;
	if (!advance) {
		dt = 0.f;
		distance = 0.f;
	}

	bool useDistance = false;

	if ((m_moveType == AnimState::kMoveType_Distance) && (m_anim->distance > 0.1f) && (distance >= 0.f)) {
		useDistance = true;
		m_frame += distance * ((float)m_anim->numFrames.get() / (float)m_anim->distance.get());
	} else {
		m_frame += (dt*m_timeScale) * m_anim->fps;
	}

	if (m_frame > m_anim->numFrames.get()-1) {
		bool emit = false;

		if (!m_loopCount[1] || (m_loopCount[0]+1 < m_loopCount[1])) {
			++m_loopCount[0];
						
			if (m_anim->numFrames > 1) { // wrap?
				m_frame = math::Mod(m_frame, (float)m_anim->numFrames.get()-1);
			} else {
				m_frame = 0;
			}

			// reset delta motion
			m_tm = m_bipZero;

		} else {
			m_loopCount[0] = m_loopCount[1];
			m_frame = (float)m_anim->numFrames.get()-1; // clamp to last frame.
			emit = true;
		}

		if (m_notify && m_emitEndFrame && emit) {
			AnimStateEventData d;
			d.ska = ska;
			d.anim = m_anim;
			m_notify->EmitEndFrame(d);
			m_emitEndFrame = false; // done
		}
	}

	if (emitTags)
		EmitTags(firstBone, numBones);

	int src;
	int dst;
	float frame;
	float lerp;

	math::ModF(frame, lerp, m_frame);
	src = FloatToInt(frame);
	dst = src+1;

	if (dst > m_anim->numFrames-1) {
		dst = src;
		lerp = 0.f;
	}

	if (out) {
		int start = firstBone;
		int count = numBones;
		int outOfs = 0;
		
		m_anim->BlendFrames(
			src,
			dst,
			lerp,
			out + outOfs,
			start,
			count
		);

		if (useDistance && (start == 0)) {
			out[0].t[0] = 0.f; // null out X axis motion
		} else if((m_moveType == AnimState::kMoveType_RemoveMotion) && (start == 0)) {
			out[0].t = m_bipZero.t; // remove all motion
		}
	}

	if (dt > 0.f || distance > 0.f) {
		BoneTM root;

		// get root motion.
		m_anim->BlendFrames(
			src,
			dst,
			lerp,
			&root,
			0,
			1
		);
		
		SetDeltaRot(m_tm.r.Inverse() * root.r);
		SetDeltaPos(root.t - m_tm.t);
		
		m_tm = root;

		SetPos(m_tm.t);
		SetRot(m_tm.r);

	} else {
		SetDeltaRot(Quat::Identity);
		SetDeltaPos(Vec3::Zero);
	}

	return true;
}

void AnimationSource::ResetMotion() {
	m_tm.s = Vec3(1, 1, 1);
	m_tm.t = Vec3::Zero;
	m_tm.r = Quat::Identity;
}

///////////////////////////////////////////////////////////////////////////////

ThreadSafeObjectPool<BlendToController::Blend> BlendToController::s_blendPool(ZSka, "ska-blend-pool", 8);

BlendToController::Ref BlendToController::New(Ska &ska, const Notify::Ref &notify) {
	return Ref(new (ZSka) BlendToController(ska, notify));
}

BlendToController::BlendToController(Ska &ska, const Notify::Ref &notify)
: Controller(ska), m_notify(notify) {
}

BlendToController::~BlendToController() {
}

bool BlendToController::Tick(
	float dt,
	float distance,
	BoneTM *out,
	int firstBone,
	int numBones,
	bool advance,
	bool emitTags
) {
	if (!m_root)
		return false;

	emitTags = emitTags && advance;

	BoneTM tm;
	BoneTM delta;

	if (!m_root->Tick(
		dt,
		distance,
		tm,
		delta,
		out,
		firstBone,
		numBones,
		advance,
		emitTags
	)) return false;
	
	SetRot(tm.r);
	SetPos(tm.t);
	SetDeltaRot(delta.r);
	SetDeltaPos(delta.t);
	return true;
}

void BlendToController::ResetMotion() {
	if (m_root)
		m_root->ResetMotion();
}

void BlendToController::OnActivate(bool active) {
	m_root.reset();
	SetDeltaRot(Quat::Identity);
	SetDeltaPos(Vec3::Zero);

	if (!active && m_notify) {
		AnimStateEventData d;
		d.ska = ska;
		d.anim = 0;
		m_notify->EmitFinish(d, true);
	}
}

void BlendToController::Blend::Delete(BlendToController::Blend *b) {
	s_blendPool.Destroy(b);
}

BlendToController::Blend::Ref BlendToController::Blend::New() {
	return Ref(s_blendPool.Construct(), &Delete);
}

void BlendToController::Blend::Activate(bool activate) {
	if (b.blend)
		b.blend->Activate(activate);
	if (b.controller)
		b.controller->Activate(activate);

	if (a.blend)
		a.blend->Activate(activate);
	if (a.controller)
		a.controller->Activate(activate);
}

bool BlendToController::Blend::Tick(
	float dt, 
	float distance,
	BoneTM &tm,
	BoneTM &delta,
	BoneTM *out,
	int firstBone,
	int numBones,
	bool advance,
	bool emitTags
) {
	if (!a.blend && !a.controller) {
		RAD_ASSERT(!b.blend);
		RAD_ASSERT(!b.controller);
		return false;
	}

	if (advance)
		blend.Tick(dt);

	bool isBlending = false;

	if (b.blend || b.controller) {
		if (blend.finished) {
			Union temp = a;

			a = b;
			b.blend.reset();
			b.controller.reset();
			bones.reset();

			if (temp.blend)
				temp.blend->Activate(false);
			if (temp.controller)
				temp.controller->Activate(false);

			// Activate messages created new blends?

			isBlending = b.blend || b.controller;
			if (isBlending) {
				if (b.blend)
					b.blend->Activate(true);
				if (b.controller)
					b.controller->Activate(true);

				if (advance)
					blend.Tick(dt);
			}
		} else {
			if (b.blend)
				b.blend->Activate(true);
			if (b.controller)
				b.controller->Activate(true);

			isBlending = true;
		}
	}

	// Blend B side.
	BoneTM bDelta;
	BoneTM btm;
	bDelta.s = Vec3(1, 1, 1); // avoid unitialized floats (potential NAN's).
	btm.s = Vec3(1, 1, 1);
	tm.s = Vec3(1, 1, 1);
	delta.s = Vec3(1, 1, 1);

	bool bValid = false;

	if (b.blend)
		bValid = b.blend->Tick(dt, distance, btm, bDelta, bones.get(), firstBone, numBones, advance, emitTags);
	if (b.controller) {
		bValid = b.controller->Tick(dt, distance, bones.get(), firstBone, numBones, advance, emitTags);
		btm.r = b.controller->rot;
		btm.t = b.controller->pos;
		bDelta.r = b.controller->deltaRot;
		bDelta.t = b.controller->deltaPos;
	}

	// Blend A side.
	bool aValid = false;

	if (a.blend) {
		a.blend->Activate(true);
		aValid = a.blend->Tick(
			dt,
			distance,
			tm,
			delta, 
			out, 
			firstBone, 
			numBones, 
			advance && (!isBlending || (a.blend->out>0.f)),
			emitTags && !isBlending
		);
	}

	if (a.controller) {
		a.controller->Activate(true);
		aValid = a.controller->Tick(
			dt,
			distance,
			out, 
			firstBone, 
			numBones, 
			advance && (!isBlending || (a.controller->out>0.f)),
			emitTags && !isBlending
		);
		tm.r = a.controller->rot;
		tm.t = a.controller->pos;
		delta.r = a.controller->deltaRot;
		delta.t = a.controller->deltaPos;
	}

	if (bValid && (b.blend || b.controller)) {
		Controller::Blend(out, out, bones.get(), blend.frac, firstBone, numBones);

		if (b.controller->blendTM) {
			Controller::Blend(&delta, &delta, &bDelta, blend.frac, 0, 1);
			Controller::Blend(&tm, &tm, &btm, blend.frac, 0, 1);
		} else {
			delta = bDelta;
			tm = btm;
		}
	}

	return aValid && (bValid || !(b.blend&&b.controller));
}

void BlendToController::Blend::ResetMotion() {
	if (a.blend) {
		a.blend->ResetMotion();
	} else if (a.controller) {
		a.controller->ResetMotion();
	}

	if (b.blend) {
		b.blend->ResetMotion();
	} else if(b.controller) {
		b.controller->ResetMotion();
	}
}

float BlendToController::Blend::RAD_IMPLEMENT_GET(in) {
	if (b.blend)
		return b.blend->in;
	if (b.controller)
		return b.controller->in;

	if (a.blend)
		return a.blend->in;

	RAD_ASSERT(a.controller);
	return a.controller->in;
}

float BlendToController::Blend::RAD_IMPLEMENT_GET(out) {
	if (b.blend)
		return b.blend->out;
	if (b.controller)
		return b.controller->out;

	if (a.blend)
		return a.blend->out;

	RAD_ASSERT(a.controller);
	return a.controller->out;
}

void BlendToController::BlendTo(const Controller::Ref &to) {
	if (!m_root) {
		m_root = Blend::New();
		m_root->a.controller = to;
		return;
	}

	if (m_root->b.blend || m_root->b.controller) { 
		// startup another blend.
		Blend::Ref b = Blend::New();
		b->a.blend = m_root;
		b->b.controller = to;
		b->bones = AllocBoneArray();
		to->Activate();
		float xfade = to->in.get();
		if (xfade >= 0.f)
			xfade = std::max(b->a.blend->out.get(), to->in.get());
		else
			xfade = -xfade;
		b->blend.Start(xfade);
		m_root = b;
	} else {
		RAD_ASSERT(m_root->a.controller);
		m_root->b.controller = to;
		m_root->bones = AllocBoneArray();

		to->Activate();
		float xfade = to->in.get();
		if (xfade >= 0.f)
			xfade = std::max(m_root->a.controller->out.get(), to->in.get());
		else
			xfade = -xfade;
		m_root->blend.Start(xfade);
	}
}

void BlendToController::BlendImmediate(const Controller::Ref &to) {
	m_root.reset();
	BlendTo(to);
}

float BlendToController::RAD_IMPLEMENT_GET(in) {
	return m_root ? m_root->in : 0.f;
}

float BlendToController::RAD_IMPLEMENT_GET(out) {
	return m_root ? m_root->out : 0.f;
}

///////////////////////////////////////////////////////////////////////////////

ThreadSafeMemoryPool s_animationVariantsSourcePool(ZSka, "ska-variants-pool", sizeof(AnimationVariantsSource), 8);

AnimationVariantsSource::Ref AnimationVariantsSource::New(
	const AnimState &variants,
	Ska &ska,
	const Notify::Ref &notify,
	const char *blendTarget
) {
	return Ref(new (s_animationVariantsSourcePool.SafeGetChunk()) AnimationVariantsSource(variants, ska, notify, blendTarget), &Delete);
}

AnimationVariantsSource::Ref AnimationVariantsSource::Clone(const Notify::Ref &notify) {
	Ref r(new (s_animationVariantsSourcePool.SafeGetChunk()) AnimationVariantsSource(*this), &Delete);
	r->m_notify = notify;
	return r;
}

void AnimationVariantsSource::Delete(AnimationVariantsSource *s) {
	s->~AnimationVariantsSource();
	s_animationVariantsSourcePool.ReturnChunk(s);
}

AnimationVariantsSource::AnimationVariantsSource(
	const AnimState &variants,
	Ska &ska,
	const Notify::Ref &notify,
	const char *blendTarget
) : Controller(ska), m_node(0), m_blendTarget(0), m_notify(notify), m_emitEndFrame(true) {

	m_moveType = variants.moveType;

	m_loopCount[0] = 0;
	m_loopCount[1] = variants.loopCount[0];
	
	if (variants.loopCount[0] != variants.loopCount[1])
		m_loopCount[1] += (rand() % (variants.loopCount[1]-variants.loopCount[0]));

	if (m_loopCount[1] < 0)
		m_loopCount[1] = 0;

	Init(variants.variants);

	if (blendTarget) {
		for (Node::Map::iterator it = m_map.begin(); it != m_map.end(); ++it) {
			Node *node = &it->second;
			if (!string::cmp(blendTarget, node->anim->name.get())) {
				m_blendTarget = node;
				break;
			}
		}
	}
}

AnimationVariantsSource::AnimationVariantsSource(const AnimationVariantsSource &s)
: Controller(*s.ska.get()), m_map(s.m_map), m_node(0) {
}

AnimationVariantsSource::~AnimationVariantsSource() {
}

void AnimationVariantsSource::Init(const Variant::Vec &anims) {
	float total = 0.f;
	for (Variant::Vec::const_iterator it = anims.begin(); it != anims.end(); ++it) {
		if (ska->anims->find((*it).name) == ska->anims->end())
			continue;
		total += (*it).weight;
	}

	int ofs = 0;
	for (Variant::Vec::const_iterator it = anims.begin(); it != anims.end(); ++it) {
		if ((*it).weight <= 0.f)
			continue;
		if (ska->anims->find((*it).name) == ska->anims->end())
			continue;

		Node n;
		n.anim = ska->anims->find((*it).name)->second;
		n.timeScale[0] = (*it).timeScale[0];
		n.timeScale[1] = (*it).timeScale[1];
		n.loopCount[0] = (*it).loopCount[0];
		n.loopCount[1] = (*it).loopCount[1];
		n.in = (*it).in;
		n.out = (*it).out;
		m_map.insert(Node::Map::value_type(ofs, n));
		ofs += (int)(((*it).weight/total) * RAND_MAX);
	}
}

void AnimationVariantsSource::OnActivate(bool active) {
	if (m_source) {
		RAD_ASSERT(!active);
		m_source->Activate(false);
		m_source.reset();
	}

	if (active) {
		m_node = 0;
		m_blend = BlendToController::New(*ska.get(), Notify::Ref());
		SetDeltaRot(Quat::Identity);
		SetDeltaPos(Vec3::Zero);
		ChooseAnim();
	}
}

void AnimationVariantsSource::ChooseAnim() {
	if (m_map.empty())
		return;

	if (m_loopCount[1] && (m_loopCount[0] == m_loopCount[1])) {
		if (m_emitEndFrame) {
			m_emitEndFrame = false;
			if (m_notify && m_source) {
				AnimStateEventData d;
				d.ska = ska;
				d.anim = m_source->animation;
				m_notify->EmitEndFrame(d);
			}
		}
		return;
	}

	for (;;) {
		Node::Map::iterator it = m_map.upper_bound(rand());
		if (it != m_map.begin())
			--it;
		Node *node = &it->second;

		if (m_blendTarget) {
			node = m_blendTarget;
			m_blendTarget = 0;
		}
		
		float timeScale = rand() / ((float)RAND_MAX) * (node->timeScale[1]-node->timeScale[0]);
		timeScale += node->timeScale[0];

		if (timeScale <= 0.f)
			timeScale = 1.f;

		int loopCount = node->loopCount[0];

		if (node->loopCount[0] != node->loopCount[1])
			loopCount += (rand() % (node->loopCount[1]-node->loopCount[0]));

		if (loopCount < 0)
			loopCount = 0;

		if (node == m_node) { // picked self?
			if (m_map.size() > 1)
				continue; // pick another animation
			RAD_ASSERT(m_source);
			m_source->ResetLoopCount(loopCount);
			m_source->timeScale = timeScale;
		} else {
			m_source = AnimationSource::New(
				*node->anim,
				node->in,
				node->out,
				timeScale,
				loopCount,
				m_moveType,
				*ska.get(),
				m_notify
			);

			m_blend->BlendTo(m_source);
		}

		m_node = node;
		break;
	}

	++m_loopCount[0];
}

bool AnimationVariantsSource::Tick(
	float dt,
	float distance,
	BoneTM *out,
	int firstBone,
	int numBones,
	bool advance,
	bool emitTags
) {
	if (m_map.empty())
		return false;

	if (!m_source || m_source->finished)
		ChooseAnim();

	int oldMask = 0;
	if (m_notify) {
		oldMask = m_notify->masked;
		m_notify->masked = oldMask | Notify::kMaskFlag_EndFrame; // don't emit endFrame in blend Tick
	}

	bool r = m_blend->Tick(
		dt,
		distance,
		out,
		firstBone,
		numBones,
		advance,
		emitTags
	);

	if (m_notify)
		m_notify->masked = oldMask;

	if (!r)
		return false;

	SetRot(m_blend->rot);
	SetPos(m_blend->pos);
	SetDeltaRot(m_blend->deltaRot);
	SetDeltaPos(m_blend->deltaPos);
	return true;
}

void AnimationVariantsSource::ResetMotion() {
	if (m_blend)
		m_blend->ResetMotion();
}

void AnimationVariantsSource::SetTime(float dt) {
	if (m_source)
		m_source->SetTime(dt);
}

} // ska
