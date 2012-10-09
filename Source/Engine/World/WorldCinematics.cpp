// WorldCinematics.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "WorldCinematics.h"
#include "WorldDraw.h"
#include "World.h"
#include "../Packages/Packages.h"
#include "../SkAnim/SkControllers.h"
#include "../App.h"
#include "../Engine.h"

using namespace pkg;
using namespace world::bsp_file;


namespace ska {
namespace details {

Quat Slerp(const Quat &from, const Quat &to, float t);
void BlendBones(BoneTM *out, const BoneTM *src, const BoneTM *dst, float weight, int first, int num);

} // details
} // ska

namespace world {

WorldCinematics::WorldCinematics(World *w) : 
m_world(w), 
m_spawnOfs(0),
m_cameraActive(false),
m_cameraEnabled(false),
m_inUpdate(false) {
}

int WorldCinematics::Spawn(
	const BSPFile::Ref &bsp, 
	const xtime::TimeSlice &time, 
	int flags
)
{
	m_bspFile = bsp;

	while ((m_spawnOfs < (int)bsp->numActors.get()) && time.remaining) {
		const BSPActor *bspActor = bsp->Actors()+m_spawnOfs;

		Actor::Ref actor(new (ZWorld) Actor());
		actor->flags = bspActor->flags;
		actor->visible = (actor->flags&kHideUntilRef) ? false : true;
		actor->loop = false;
		actor->frame = -1;

		actor->m = r::SkMesh::New(
			bsp->DSka(bspActor->ska),
			bsp->DSkm(bspActor->ska),
			ska::SkinCpu
		);

		m_actors.push_back(actor);

		// create material batches
		const ska::DSkm &dskm = bsp->DSkm(bspActor->ska);
		for (int i = 0; i < (int)dskm.meshes.size(); ++i) {
			int id = App::Get()->engine->sys->packages->ResolveId(
				dskm.meshes[i].material
			);

			if (id < 0)
				continue;

			MBatchDraw::Ref batch(new SkActorBatch(actor, i, id));
			m_world->draw->AddMaterial(id);
		}

		++m_spawnOfs;
	}

	return (m_spawnOfs < (int)bsp->numActors.get()) ? SR_Pending : SR_Success;
}

void WorldCinematics::Tick(int frame, float dt) {
	ska::BoneTM tm;

	m_cameraActive = false;
	m_inUpdate = true;

	bool active;

	do {
		// Tick any cinematics that were started by events to make sure we 
		// get an updated camera position before cuts occur
		active = false;

		for (Cinematic::List::iterator it = m_cinematics.begin(); it != m_cinematics.end();) {
			Cinematic &c = *(*it);

			if (c.updateFrame == frame) {
				++it;
				continue;
			}

			int numActive = 0;
			c.camera = false;
			active = true;
			c.updateFrame = frame;

			// tick actors
			for (IntSet::iterator it2 = c.actors.begin(); it2 != c.actors.end();) {
				const Actor::Ref &actor = m_actors[*it2];

				RAD_ASSERT(actor->m);
				actor->m->ska->Tick(
					dt,
					true,
					true,
					Mat4::Identity,
					ska::Ska::MT_None,
					tm
				);

				ska::AnimationSource::Ref source = 
					boost::static_pointer_cast<ska::AnimationSource>(actor->m->ska->root.get());

				bool remove = false;

				if (!source || source->finished) {
#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematics(" << c.name << ") actor(" << *it2 << ") done." << std::endl;
#endif
					if (actor->flags&kHideWhenDone) {
						actor->visible = false;
						actor->m->ska->root = ska::Controller::Ref();
					}

					remove = true;
				}

				if (remove) {
					actor->frame = -1;
					IntSet::iterator next = it2; ++next;
					c.actors.erase(it2);
					it2 = next;
				} else {
					if (!actor->loop)
						++numActive;

					++it2;
				}
			}

			c.frame[0] += dt*c.cinematic->fps;
			
			int rootFrame = (int)c.frame[0];

			while (c.trigger && rootFrame >= c.trigger->frame) {
				const BSPCameraTrack *track = 0;
				if (c.trigger->camera > -1)
					track = m_bspFile->CameraTracks() + c.trigger->camera;

				if (track && track != c.track && (c.flags&CF_AnimateCamera)) {
					if (c.track)
						EmitCameraTags(c);
					c.track = track;
					c.frame[1] = (float)c.trigger->frame;
					c.emitFrame = 0;
#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematics(" << c.name << ") camera(" << c.trigger->camera << ") triggered." << std::endl;
#endif
				}

				float tickDelta = (c.frame[0] - c.trigger->frame) / c.cinematic->fps;
				if (tickDelta < 0.001f)
					tickDelta = 0.001f;

				for (int i = 0; i < c.trigger->numActors; ++i) {
					int actorIdx = (int)(m_bspFile->ActorIndices()+c.trigger->firstActor)[i];
					bool loop = actorIdx & 0x10000000 ? true : false;
					actorIdx &= 0x00ffffff;

					RAD_ASSERT(actorIdx >= 0 && actorIdx < (int)m_actors.size());
					const Actor::Ref actor = m_actors[actorIdx];

					// always direct an actor on first play of a cinematic, or if they are
					// not doing anything.

					if (c.loopCount == 0 || actor->frame == -1) {
						// first, pull this actor out of any executing cinematic, we are taking him over!
						for (Cinematic::List::iterator it2 = m_cinematics.begin(); it2 != m_cinematics.end(); ++it2) {
							if (it == it2)
								continue;
							Cinematic &c2 = *(*it2);
#if !defined(RAD_TARGET_GOLDEN)
							if (c2.actors.find(actorIdx) != c2.actors.end()) {
								COut(C_Debug) << "Cinematics(" << c.name << ") is taking over directing actor (" << actorIdx << ") from cinematic (" << c2.name << ")." << std::endl;
							}
#endif
							c2.actors.erase(actorIdx);
						}

						// play cinematic animation.
						ska::Animation::Map::const_iterator animIt = actor->m->ska->anims->find(c.name);

						if (animIt != actor->m->ska->anims->end()) {
							actor->visible = true;
							actor->loop = loop;
							actor->frame = c.trigger->frame;
							c.actors.insert(actorIdx);

							ska::AnimationSource::Ref source = ska::AnimationSource::New(
								*(animIt->second),
								0.f,
								0.f,
								1.f,
								loop ? 0 : 1,
								false,
								*actor->m->ska.get(),
								ska::Notify::Ref(new (ZWorld) SkaNotify(*this, c))
							);

							actor->m->ska->root = boost::static_pointer_cast<ska::Controller>(source);
							actor->m->ska->Tick( // advance delta
								tickDelta,
								true,
								true,
								Mat4::Identity,
								ska::Ska::MT_None,
								tm
							);

							if (!actor->loop)
								++numActive;

#if !defined(RAD_TARGET_GOLDEN)
							if (actor->loop)
								COut(C_Debug) << "Cinematics(" << c.name << ") looping actor(" << actorIdx << ") triggered." << std::endl;
							else
								COut(C_Debug) << "Cinematics(" << c.name << ") actor(" << actorIdx << ") triggered." << std::endl;
#endif
						}
					}
				}

				if (++c.triggerNum < c.cinematic->numTriggers) {
					++c.trigger;
				} else {
					c.trigger = 0; // no more triggers
				}
			}

			// update camera
			if (c.track) {
				c.xfade[0] += dt;
				EmitCameraTags(c);
				BlendCameraFrame(c);
				m_cameraActive = true;

				if (!c.camera)
					c.track = 0;
			}

			bool suppress = c.suppressFinish;
			c.suppressFinish = false;

			if (!c.trigger && !c.camera && (c.actors.empty() || !numActive) && !suppress) {
				// Cinematic is completed (no upcoming triggers).
				// Some looping actors may remain.

				// If we are a looping cinematic, then all we want to do is reset our
				// time and start over.
				if (c.flags&CF_Loop) {
#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematic(" << c.name << ") looping." << std::endl;
#endif
					c.frame[0] = c.frame[1] = 0;
					c.triggerNum = 0;
					c.track = 0;
					c.emitFrame = 0;
					c.updateFrame = -1; // this will force this cinematic to be ticked again in this loop.
					c.trigger = m_bspFile->CinematicTriggers()+c.cinematic->firstTrigger;
					++c.loopCount;

					Cinematic::Ref r = *it; // keep us around in case OnComplete stops us.

					if (c.notify)
						c.notify->OnComplete();

					// NOTE: it may now be invalid since OnComplete() may have stopped us.
					it = m_cinematics.begin();
					continue; // jump to start.
				} else if (!(c.flags&CF_CanPlayForever) && !numActive) {
#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematic(" << c.name << ") finished." << std::endl;
#endif
					// no non-looping actors are playing and we are not a play forever cinematic
					// so clean up and stop.

					for (IntSet::iterator it2 = c.actors.begin(); it2 != c.actors.end(); ++it2) {
						const Actor::Ref &actor = m_actors[*it2];
						RAD_ASSERT(actor->loop);

						if (actor->flags&kHideWhenDone) {
							actor->visible = false;
							actor->frame = -1;
							actor->m->ska->root = ska::Controller::Ref();
#if !defined(RAD_TARGET_GOLDEN)
							COut(C_Debug) << "Cinematics(" << c.name << ") actor(" << *it2 << ") done." << std::endl;
#endif
						}
					}

					c.done = true;

					Cinematic::Ref r = *it; // keep us around after we erase(it)

					// this cinematic is complete.
					it = m_cinematics.erase(it);

					if (c.notify)
						c.notify->OnComplete();

					continue; // jump to start.
				}
			}
			
			++it;
		}

	} while(active);

	m_inUpdate = false;
}

bool WorldCinematics::PlayCinematic(
	const char *name, 
	int flags,
	float xfadeCamera,
	const Notify::Ref &notify
) {
	for(Cinematic::List::const_iterator it = m_cinematics.begin(); it != m_cinematics.end(); ++it) {
		if (!strcmp(name, (*it)->name.c_str) && !(*it)->done)
			return true;
	}

	Cinematic::Ref c(new (ZWorld) Cinematic());
	c->name = name;
	c->flags = flags;
	c->notify = notify;
	c->frame[0] = c->frame[1] = 0.f;
	c->triggerNum = 0;
	c->cinematic = 0;
	c->track = 0;
	c->emitFrame = 0;
	c->loopCount = 0;
	c->updateFrame = -1;
	c->done = false;
	c->camera = false;
	c->xfade[0] = 0.f;
	c->xfade[1] = xfadeCamera;
	c->suppressFinish = m_inUpdate; // keep us from infinite looping.

	for (int i = 0; i < (int)m_bspFile->numCinematics.get(); ++i) {
		const BSPCinematic *bspC = m_bspFile->Cinematics()+i;
		const char *sz = m_bspFile->String(bspC->name);
		if (strcmp(name, sz))
			continue;

		c->cinematic = bspC;
		break;
	}

	if (!c->cinematic || c->cinematic->numTriggers < 1)
		return false;

	c->trigger = m_bspFile->CinematicTriggers()+c->cinematic->firstTrigger;
	m_cinematics.push_back(c);

#if !defined(RAD_TARGET_GOLDEN)
	COut(C_Debug) << "Cinematic(" << c->name << ") started." << std::endl;
#endif

	return true;
}

void WorldCinematics::StopCinematic(const char *name) {
	for (Cinematic::List::iterator it = m_cinematics.begin(); it != m_cinematics.end(); ++it) {
		Cinematic &c = *(*it);
		if (strcmp(name, c.name.c_str))
			continue;

#if !defined(RAD_TARGET_GOLDEN)
		COut(C_Debug) << "Cinematic(" << c.name << ") stopped." << std::endl;
#endif

		// reset actors?
		for (IntSet::iterator it2 = c.actors.begin(); it2 != c.actors.end(); ++it2) {
			const Actor::Ref &a = m_actors[*it2];
			if (a->flags&kHideWhenDone) {
				a->visible = false;
				a->frame = -1;
				a->m->ska->root = ska::Controller::Ref();
			}
#if !defined(RAD_TARGET_GOLDEN)
			COut(C_Debug) << "Cinematics(" << c.name << ") actor(" << *it2 << ") done." << std::endl;
#endif
		}

		m_cinematics.erase(it);
		break;
	}
}

float WorldCinematics::CinematicTime(const char *name) {
	for (Cinematic::List::iterator it = m_cinematics.begin(); it != m_cinematics.end(); ++it) {
		Cinematic &c = *(*it);
		if (strcmp(name, c.name.c_str))
			continue;
		return c.frame[0] / c.cinematic->fps;
	}

	return -1.f;
}

bool WorldCinematics::SetCinematicTime(const char *name, float time) {
	Cinematic *c = 0;
	for (Cinematic::List::iterator it = m_cinematics.begin(); it != m_cinematics.end(); ++it) {
		Cinematic &x = *(*it);
		if (!strcmp(name, x.name.c_str)) {
			c = &x;
			break;
		}
	}

	if (!c)
		return false;

	ska::BoneTM tm;

	// Stop all actors.
	float frame = time * c->cinematic->fps;
	int intFrame = (int)frame;

	if ((intFrame == (int)c->frame[0]) && (frame >= c->frame[0])) { 
		// advance fractional frames

		float timeDelta = (frame - c->frame[0]);
		if (timeDelta > 0) {
			// no new triggers but we need to advance fractional frames
			for (IntSet::iterator it = c->actors.begin(); it != c->actors.end();) {
				const Actor::Ref &actor = m_actors[*it];

				RAD_ASSERT(actor->m);
				actor->m->ska->Tick(
					timeDelta,
					true,
					false, // don't emit any events
					Mat4::Identity,
					ska::Ska::MT_None,
					tm
				);

				ska::AnimationSource::Ref source = 
					boost::static_pointer_cast<ska::AnimationSource>(actor->m->ska->root.get());

				bool remove = false;

				if (!source || source->finished) {
#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematics(" << c->name << ") actor(" << *it << ") done." << std::endl;
#endif
					if (actor->flags&kHideWhenDone) {
						actor->visible = false;
						actor->m->ska->root = ska::Controller::Ref();
					}

					remove = true;
				}

				if (remove) {
					actor->frame = -1;
					IntSet::iterator next = it; ++next;
					c->actors.erase(it);
					it = next;
				} else {
					++it;
				}
			}
		}

		c->frame[0] = frame;
		return true;
	}

	// any actors triggered at or after our target frame need to be removed.
	// any other actors will have their time set when we build our trigger state

	for (IntSet::iterator it = c->actors.begin(); it != c->actors.end();) {
		const Actor::Ref &actor = m_actors[*it];

		if (actor->frame >= intFrame) {
#if !defined(RAD_TARGET_GOLDEN)
			COut(C_Debug) << "Cinematics(" << c->name << ") actor(" << *it << ") done." << std::endl;
#endif
			actor->frame = -1;
			actor->visible = false;
			actor->m->ska->root = ska::Controller::Ref();

			IntSet::iterator next = it; ++next;
			c->actors.erase(it);
			it = next;
		} else {
			++it;
		}
	}

	c->updateFrame = -1;
	c->loopCount = 0;
	c->camera = false;
	c->emitFrame = intFrame+1;
	c->xfade[0] = c->xfade[1] = 0.f;
	c->frame[0] = c->frame[1] = time;

	// build trigger state.

	c->trigger = m_bspFile->CinematicTriggers()+c->cinematic->firstTrigger;

	while (c->trigger && intFrame >= c->trigger->frame)
	{
		float triggerDelta = c->frame[0] - c->trigger->frame;

		const BSPCameraTrack *track = 0;
		if (c->trigger->camera > -1)
			track = m_bspFile->CameraTracks() + c->trigger->camera;

		if (track && (c->flags&CF_AnimateCamera)) {
			if (triggerDelta <= (float)(track->numTMs-1)) {
				c->track = track;
				c->camera = true;
				c->frame[1] = (float)c->trigger->frame;
#if !defined(RAD_TARGET_GOLDEN)
				COut(C_Debug) << "Cinematics(" << c->name << ") camera(" << c->trigger->camera << ") triggered." << std::endl;
#endif
			}
		}

		float tickDelta = (c->frame[0] - c->trigger->frame) / c->cinematic->fps;
		if (tickDelta < 0.001f)
			tickDelta = 0.001f;

		for (int i = 0; i < c->trigger->numActors; ++i) {
			int actorIdx = (int)(m_bspFile->ActorIndices()+c->trigger->firstActor)[i];
			bool loop = actorIdx & 0x10000000 ? true : false;
			actorIdx &= 0x00ffffff;

			RAD_ASSERT(actorIdx >= 0 && actorIdx < (int)m_actors.size());
			const Actor::Ref actor = m_actors[actorIdx];

			// is this actor just standing around?

			if (actor->frame == -1) {
				// play cinematic animation.
				ska::Animation::Map::const_iterator animIt = actor->m->ska->anims->find(c->name);

				if (animIt != actor->m->ska->anims->end()) {
					// only trigger this animation if it won't be complete after it's played or
					// the actor is not hidden when done.
					if (loop || (animIt->second->length >= tickDelta) || !(actor->flags&kHideWhenDone)) {
						actor->loop = loop;
						actor->visible = true;
						actor->frame = c->trigger->frame;
						c->actors.insert(actorIdx);

						ska::AnimationSource::Ref source = ska::AnimationSource::New(
							*(animIt->second),
							0.f,
							0.f,
							1.f,
							loop ? 0 : 1,
							false,
							*actor->m->ska.get(),
							ska::Notify::Ref(new (ZWorld) SkaNotify(*this, *c))
						);

						actor->m->ska->root = boost::static_pointer_cast<ska::Controller>(source);
						actor->m->ska->Tick( // advance delta
							tickDelta,
							true,
							false, // no animation events
							Mat4::Identity,
							ska::Ska::MT_None,
							tm
						);

#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematics(" << c->name << ") actor(" << actorIdx << ") triggered." << std::endl;
#endif
					}
				}
			} else {
				RAD_ASSERT(actor->m);
				ska::AnimationSource::Ref source = 
					boost::static_pointer_cast<ska::AnimationSource>(actor->m->ska->root.get());

				if (source) {
#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematics(" << c->name << ") actor(" << actorIdx << ") time-seek." << std::endl;
#endif
					source->SetTime(tickDelta);
				} else {
#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematics(" << c->name << ") actor(" << actorIdx << ") invalidated." << std::endl;
#endif
					c->actors.erase(actorIdx);
				}
			}
		}

		if (++c->triggerNum < c->cinematic->numTriggers) {
			++c->trigger;
		} else {
			c->trigger = 0; // no more triggers
		}
	}

	return true;
}

void WorldCinematics::Skip() {
	for (Cinematic::List::const_iterator it = m_cinematics.begin(); it != m_cinematics.end(); ++it) {
		const Cinematic &c = *(*it);
		if (c.notify)
			c.notify->OnSkip();
	}
}

void WorldCinematics::EmitCameraTags(Cinematic &c) {
	if (!c.track || !c.notify)
		return;

	int frame = (int)(c.frame[0]-c.frame[1]);
	if (frame > c.track->numTMs-1)
		frame = c.track->numTMs-1;

	for (; c.emitFrame <= frame; ++c.emitFrame) {
		const BSPCameraTM *tm = m_bspFile->CameraTMs() + c.track->firstTM + c.emitFrame;
		if (tm->tag > -1) {
			const char *tag = m_bspFile->String(tm->tag);

			// emit string, splitup multiples.
			String x;

			while (*tag) {
				if (*tag < 32 || *tag > 126) {
					if (!x.empty)
						c.notify->OnTag(x.c_str);
					x.Clear();
				} else {
					x += *tag;
				}

				++tag;
			}

			if (!x.empty)
				c.notify->OnTag(x.c_str);
		}
	}
}

void WorldCinematics::BlendCameraFrame(Cinematic &c) {
	if (!c.track)
		return;

	float frame = c.frame[0]-c.frame[1];
	float baseFrame, lerp;
	int src, dst;
	bool end = false;

	if (frame >= (float)(c.track->numTMs)) {
		frame = (float)(c.track->numTMs);
		src = c.track->numTMs-1;
		dst = src;
		lerp = 0.f;
		end = true;
	} else {
		math::ModF(baseFrame, lerp, frame);
		src = (int)baseFrame;
		dst = src+1;

		if (dst > c.track->numTMs-1)
		{
			dst = src;
			lerp = 0.f;
		}
	}

	ska::BoneTM tmSrc;
	ska::BoneTM tmDst;
	ska::BoneTM tmOut;

	const BSPCameraTM *bspTMSrc = m_bspFile->CameraTMs() + c.track->firstTM + src;
	const BSPCameraTM *bspTMDst = m_bspFile->CameraTMs() + c.track->firstTM + dst;

	tmSrc.r = bspTMSrc->r;
	tmSrc.t = bspTMSrc->t;
	tmSrc.s = Vec3(1, 1, 1);
	tmDst.r = bspTMDst->r;
	tmDst.t = bspTMDst->t;
	tmDst.s = Vec3(1, 1, 1);

	ska::details::BlendBones(&tmOut, &tmSrc, &tmDst, lerp, 0, 1);
	float fov = math::Lerp(bspTMSrc->fov, bspTMDst->fov, lerp);

	if (c.xfade[0] > c.xfade[1])
		c.xfade[0] = c.xfade[1];

	if (m_cameraActive && c.xfade[0] < c.xfade[1]) {
		ska::BoneTM temp(tmOut);

		float blend = c.xfade[0] / c.xfade[1];
		tmOut.r = ska::details::Slerp(m_world->camera->rot.get(), temp.r, blend);
		tmOut.t = math::Lerp(m_world->camera->pos.get(), temp.t, blend);
		fov = math::Lerp(m_world->camera->fov.get(), fov, blend);
	}

	m_world->camera->rot = tmOut.r;
	m_world->camera->pos = tmOut.t;
	m_world->camera->fov = fov;
	m_world->camera->quatMode = true;

	c.camera = !end;
}

void WorldCinematics::SkaNotify::OnTag(const ska::AnimTagEventData &data) {
	if (m_c->notify)
		m_c->notify->OnTag(data.tag.c_str);
	else
		m_w->m_world->PostEvent(data.tag.c_str);
}

Vec4 WorldCinematics::SkActorBatch::s_rgba(1, 1, 1, 1);
Vec3 WorldCinematics::SkActorBatch::s_scale(1, 1, 1);
BBox WorldCinematics::SkActorBatch::s_bounds(Vec3::Zero, Vec3::Zero);

WorldCinematics::SkActorBatch::SkActorBatch(const Actor::Ref &actor, int idx, int matId) :
MBatchDraw(matId), m_idx(idx), m_actor(actor) {
}

void WorldCinematics::SkActorBatch::Bind(r::Shader *shader) {
	m_actor->m->Skin(m_idx);
	r::Mesh &m = m_actor->m->Mesh(m_idx);
	m.BindAll(shader);
}

void WorldCinematics::SkActorBatch::CompileArrayStates(r::Shader &shader) {
	m_actor->m->Mesh(m_idx).CompileArrayStates(shader);
}

void WorldCinematics::SkActorBatch::FlushArrayStates(r::Shader *shader) {
	m_actor->m->Mesh(m_idx).FlushArrayStates(shader);
}

void WorldCinematics::SkActorBatch::Draw() {
	m_actor->m->Mesh(m_idx).Draw();
}

} // world
