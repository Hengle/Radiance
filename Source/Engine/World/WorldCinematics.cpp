// WorldCinematics.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "WorldCinematics.h"
#include "WorldDraw.h"
#include "World.h"
#include "Light.h"
#include "../Packages/Packages.h"
#include "../SkAnim/SkControllers.h"
#include "../App.h"
#include "../Engine.h"
#include "../Game/GameCVars.h"

using namespace pkg;
using namespace world::bsp_file;

namespace ska {
namespace details {

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
		actor->pos[0] = Vec3(bspActor->pos[0], bspActor->pos[1], bspActor->pos[2]);
		actor->pos[1] = actor->pos[0];
		actor->bounds[0] = BBox(bspActor->mins[0], bspActor->mins[1], bspActor->mins[2],
			bspActor->maxs[0], bspActor->maxs[1], bspActor->maxs[2]
		);
		actor->pos[2] = actor->pos[1];

		actor->bounds[1] = actor->bounds[0];
		actor->bounds[1].Translate(actor->pos[0]);

		LightingFlags lightingFlags = kLightingFlag_None;

		if (actor->flags&kCastShadows)
			lightingFlags |= kLightingFlag_CastShadows;

		int lightInteractionFlags = 0;

		if (actor->flags & kAffectedByObjectLights)
			lightInteractionFlags |= Light::kInteractionFlag_Objects;
		if (actor->flags & kAffectedByWorldLights)
			lightInteractionFlags |= Light::kInteractionFlag_World;

		if (bspActor->ska >= 0) {
			actor->skm = r::SkMesh::New(
				bsp->DSka(bspActor->ska),
				bsp->DSkm(bspActor->ska),
				ska::kSkinType_CPU
			);

			actor->occupant.reset(new ActorOccupant(*actor, lightingFlags, lightInteractionFlags, *m_world));

			// create material batches
			const ska::DSkm &dskm = bsp->DSkm(bspActor->ska);
			for (int i = 0; i < (int)dskm.meshes.size(); ++i) {
				int id = App::Get()->engine->sys->packages->ResolveId(
					dskm.meshes[i].material
				);

				if (id < 0)
					continue;

				MBatchDraw::Ref batch(new SkActorBatch(*m_world->draw, *actor, i, id));
				actor->occupant->AddMBatch(batch);
			}
		} else {
			actor->vtm = r::VtMesh::New(bsp->DVtm(-(bspActor->ska+1)));

			actor->occupant.reset(new ActorOccupant(*actor, lightingFlags, lightInteractionFlags, *m_world));

			// create material batches
			const ska::DVtm &vtm = bsp->DVtm(-(bspActor->ska+1));
			for (int i = 0; i < (int)vtm.meshes.size(); ++i) {
				int id = App::Get()->engine->sys->packages->ResolveId(
					vtm.meshes[i].material
				);

				if (id < 0)
					continue;

				MBatchDraw::Ref batch(new VtActorBatch(*m_world->draw, *actor, i, id));
				actor->occupant->AddMBatch(batch);
			}
		}

		actor->occupant->Link();
		m_actors.push_back(actor);
		++m_spawnOfs;
	}

	return (m_spawnOfs < (int)bsp->numActors.get()) ? SR_Pending : SR_Success;
}

void WorldCinematics::Tick(int frame, float dt) {

	m_cameraActive = false;
	m_inUpdate = true;

	bool active;

	for (Cinematic::List::iterator it = m_cinematics.begin(); it != m_cinematics.end(); ++it) {
		Cinematic &c = *(*it);
		c.updateCount = 0;
	}

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
			++c.updateCount;

			// tick actors
			for (IntSet::iterator it2 = c.actors.begin(); it2 != c.actors.end();) {
				const Actor::Ref &actor = m_actors[*it2];

				ska::AnimationSource::Ref source;

				if (actor->skm) {
					actor->skm->ska->Tick(
						dt,
						0.f,
						true,
						true,
						Mat4::Identity
					);

					source = boost::static_pointer_cast<ska::AnimationSource>(actor->skm->ska->root.get());
				} else {
					RAD_ASSERT(actor->vtm);

					actor->vtm->vtm->Tick(
						dt,
						true,
						true
					);

					source = boost::static_pointer_cast<ska::AnimationSource>(actor->vtm->vtm->root.get());
				}

				bool remove = false;

				if (!source || source->finished) {
#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematics(" << c.name << ") actor(" << *it2 << ") done." << std::endl;
#endif
					if (actor->flags&kHideWhenDone) {
						actor->visible = false;
						if (actor->skm) {
							actor->skm->ska->root = ska::Controller::Ref();
						} else {
							actor->vtm->vtm->root = ska::Controller::Ref();
						}
					}

					remove = true;
				}

				if (remove) {
					actor->frame = -1;
					IntSet::iterator next = it2; ++next;
					c.actors.erase(it2);
					it2 = next;
				} else {

					if (actor->skm) {
						const ska::BoneTM *motion = actor->skm->ska->absMotion;
						actor->pos[1] = actor->pos[0] + motion->t;
						actor->bounds[1] = actor->bounds[0];
						actor->bounds[1].Translate(actor->pos[1]);

						if (actor->pos[2] != actor->pos[1]) {
							actor->occupant->Link();
							actor->pos[2] = actor->pos[1];
						}
					}

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

				if (track && track != c.track && (c.flags&kCinematicFlag_AnimateCamera)) {
					if (c.track)
						EmitCameraTags(c);
					c.track = track;
					c.frame[1] = (float)c.trigger->frame;
					c.emitFrame = 0;
#if !defined(RAD_TARGET_GOLDEN)
					// don't be all spammy
					if (c.cinematic->numTriggers > 1) {
						COut(C_Debug) << "Cinematics(" << c.name << ") camera(" << c.trigger->camera << ") triggered." << std::endl;
					}
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
						if (actor->skm) {
							ska::Animation::Map::const_iterator animIt = actor->skm->ska->anims->find(c.name);

							if (animIt != actor->skm->ska->anims->end()) {
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
									ska::AnimState::kMoveType_None,
									*actor->skm->ska.get(),
									ska::Notify::Ref(new (ZWorld) SkaNotify(*this, c))
								);

								actor->skm->ska->root = boost::static_pointer_cast<ska::Controller>(source);
								actor->skm->ska->Tick( // advance delta
									tickDelta,
									0.f,
									true,
									true,
									Mat4::Identity
								);

								const ska::BoneTM *motion = actor->skm->ska->absMotion;
								actor->pos[1] = actor->pos[0] + motion->t;
								actor->bounds[1] = actor->bounds[0];
								actor->bounds[1].Translate(actor->pos[1]);

								if (actor->pos[2] != actor->pos[1]) {
									actor->occupant->Link();
									actor->pos[2] = actor->pos[1];
								}

								if (!actor->loop)
									++numActive;

	#if !defined(RAD_TARGET_GOLDEN)
								if (actor->loop)
									COut(C_Debug) << "Cinematics(" << c.name << ") looping actor(" << actorIdx << ") triggered." << std::endl;
								else
									COut(C_Debug) << "Cinematics(" << c.name << ") actor(" << actorIdx << ") triggered." << std::endl;
	#endif
							}
						} else {
							RAD_ASSERT(actor->vtm);
								ska::Animation::Map::const_iterator animIt = actor->vtm->vtm->anims->find(c.name);

							if (animIt != actor->vtm->vtm->anims->end()) {
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
									*actor->vtm->vtm.get(),
									ska::Notify::Ref(new (ZWorld) SkaNotify(*this, c))
								);

								actor->vtm->vtm->root = boost::static_pointer_cast<ska::Controller>(source);
								actor->vtm->vtm->Tick( // advance delta
									tickDelta,
									true,
									true
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
				if (c.flags&kCinematicFlag_Loop) {
#if !defined(RAD_TARGET_GOLDEN)
					// don't be all spammy
					if (c.cinematic->numTriggers > 1) {
						COut(C_Debug) << "Cinematic(" << c.name << ") looping." << std::endl;
					}
#endif
					c.frame[0] = c.frame[1] = 0;
					c.triggerNum = 0;
					c.track = 0;
					c.emitFrame = 0;
					c.trigger = m_bspFile->CinematicTriggers()+c.cinematic->firstTrigger;
					++c.loopCount;
					if (c.updateCount < 2) { // don't infinite loop on short cinematics
						c.updateFrame = -1; // this will force this cinematic to be ticked again in this loop.
						continue; // jump to start.
					}
				} else if (!(c.flags&kCinematicFlag_CanPlayForever) && !numActive) {
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

							if (actor->skm) {
								actor->skm->ska->root = ska::Controller::Ref();
							} else {
								RAD_ASSERT(actor->vtm);
								actor->vtm->vtm->root = ska::Controller::Ref();
							}
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
	const Entity::Ref &origin,
	const Notify::Ref &notify
) {
	for(Cinematic::List::const_iterator it = m_cinematics.begin(); it != m_cinematics.end(); ++it) {
		if (!strcmp(name, (*it)->name.c_str) && !(*it)->done)
			return true;
	}

	const BSPCinematic *bspC = FindCinematic(name);
	if (!bspC || (bspC->numTriggers < 1))
		return false;

	Cinematic::Ref c(new (ZWorld) Cinematic());
	c->name = name;
	c->flags = flags;
	c->origin = origin;
	c->notify = notify;
	c->frame[0] = c->frame[1] = 0.f;
	c->triggerNum = 0;
	c->cinematic = 0;
	c->track = 0;
	c->emitFrame = 0;
	c->loopCount = 0;
	c->updateFrame = -1;
	c->updateCount = 0;
	c->done = false;
	c->camera = false;
	c->xfade[0] = 0.f;
	c->xfade[1] = xfadeCamera;
	c->suppressFinish = m_inUpdate; // keep us from infinite looping.
	c->cinematic = bspC;
	
	c->trigger = m_bspFile->CinematicTriggers()+c->cinematic->firstTrigger;
	m_cinematics.push_back(c);

#if !defined(RAD_TARGET_GOLDEN)
	COut(C_Debug) << "Cinematic(" << c->name << ") started." << std::endl;
#endif

	return true;
}

void WorldCinematics::StopCinematic(const char *name, bool resetActors) {
	for (Cinematic::List::iterator it = m_cinematics.begin(); it != m_cinematics.end(); ++it) {
		Cinematic &c = *(*it);
		if (strcmp(name, c.name.c_str))
			continue;

		const world::bsp_file::BSPCinematicTrigger *trigger = m_bspFile->CinematicTriggers() + c.cinematic->firstTrigger;
		for (int i = 0; i < c.cinematic->numTriggers; ++i, ++trigger) {

			for (int i = 0; i < trigger->numActors; ++i) {
				int actorIdx = (int)(m_bspFile->ActorIndices()+trigger->firstActor)[i];
				actorIdx &= 0x00ffffff;

				RAD_ASSERT(actorIdx >= 0 && actorIdx < (int)m_actors.size());
				
				const Actor::Ref &a = m_actors[actorIdx];
				if (a->flags&(kHideWhenDone|kHideUntilRef)) {
					a->visible = false;
					a->frame = -1;
				}
			
				if ((a->flags&(kHideWhenDone|kHideUntilRef)) || resetActors) {
					a->pos[1] = a->pos[0];
					a->bounds[1] = a->bounds[0];
					a->bounds[1].Translate(a->pos[1]);

					a->occupant->Link();
					a->pos[2] = a->pos[1];
				
					a->frame = -1;

					if (a->skm) {
						a->skm->ska->root = ska::Controller::Ref();
					} else {
						RAD_ASSERT(a->vtm);
						a->vtm->vtm->root = ska::Controller::Ref();
					}
				}
			}
		}

#if !defined(RAD_TARGET_GOLDEN)
		COut(C_Debug) << "Cinematic(" << c.name << ") stopped." << std::endl;
#endif
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

				ska::AnimationSource::Ref source;

				if (actor->skm) {
					actor->skm->ska->Tick(
						timeDelta,
						0.f,
						true,
						false, // don't emit any events
						Mat4::Identity
					);

					source = boost::static_pointer_cast<ska::AnimationSource>(actor->skm->ska->root.get());
				} else {
					RAD_ASSERT(actor->vtm);
					actor->vtm->vtm->Tick(
						timeDelta,
						true,
						false
					);

					source = boost::static_pointer_cast<ska::AnimationSource>(actor->vtm->vtm->root.get());
				}
				

				bool remove = false;

				if (!source || source->finished) {
#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematics(" << c->name << ") actor(" << *it << ") done." << std::endl;
#endif
					if (actor->flags&kHideWhenDone) {
						actor->visible = false;
						if (actor->skm) {
							actor->skm->ska->root = ska::Controller::Ref();
						} else {
							RAD_ASSERT(actor->vtm);
							actor->vtm->vtm->root = ska::Controller::Ref();
						}
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

	// reset all active actors

	for (IntSet::iterator it = c->actors.begin(); it != c->actors.end(); ++it) {
		const Actor::Ref &actor = m_actors[*it];

#if !defined(RAD_TARGET_GOLDEN)
		COut(C_Debug) << "Cinematics(" << c->name << ") actor(" << *it << ") reset." << std::endl;
#endif
		actor->frame = -1;
		actor->visible = (actor->flags&kHideUntilRef) ? false : true;

		if (actor->skm) {
			actor->skm->ska->root = ska::Controller::Ref();
		} else {
			RAD_ASSERT(actor->vtm);
			actor->vtm->vtm->root = ska::Controller::Ref();
		}
	}
	
	c->actors.clear();
	c->updateFrame = -1;
	c->loopCount = 0;
	c->camera = false;
	c->emitFrame = intFrame+1;
	c->xfade[0] = c->xfade[1] = 0.f;
	c->frame[0] = c->frame[1] = frame;

	// build trigger state.

	c->trigger = m_bspFile->CinematicTriggers()+c->cinematic->firstTrigger;

	while (c->trigger && intFrame >= c->trigger->frame) {
		float triggerDelta = c->frame[0] - c->trigger->frame;

		const BSPCameraTrack *track = 0;
		if (c->trigger->camera > -1)
			track = m_bspFile->CameraTracks() + c->trigger->camera;

		if (track && (c->flags&kCinematicFlag_AnimateCamera)) {
			if (triggerDelta <= (float)(track->numTMs-1)) {
				c->track = track;
				c->camera = true;
				c->frame[1] = (float)c->trigger->frame;
#if !defined(RAD_TARGET_GOLDEN)
				// don't be all spammy
				if (c->cinematic->numTriggers > 1) {
					COut(C_Debug) << "Cinematics(" << c->name << ") camera(" << c->trigger->camera << ") triggered." << std::endl;
				}
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

			bool actorDone = true;

			// play cinematic animation.
			if (actor->skm) {
				ska::Animation::Map::const_iterator animIt = actor->skm->ska->anims->find(c->name);
				
				if (animIt != actor->skm->ska->anims->end()) {
					// only trigger this animation if it won't be complete after it's played or
					// the actor is not hidden when done.
					if (loop || (animIt->second->length >= tickDelta) || !(actor->flags&kHideWhenDone)) {
						actorDone = false;

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
							ska::AnimState::kMoveType_None,
							*actor->skm->ska.get(),
							ska::Notify::Ref(new (ZWorld) SkaNotify(*this, *c))
						);

						actor->skm->ska->root = boost::static_pointer_cast<ska::Controller>(source);
						actor->skm->ska->Tick( // advance delta
							tickDelta,
							0.f,
							true,
							false, // no animation events
							Mat4::Identity
						);

						const ska::BoneTM *motion = actor->skm->ska->absMotion;
						actor->pos[1] = actor->pos[0] + motion->t;
						actor->bounds[1] = actor->bounds[0];
						actor->bounds[1].Translate(actor->pos[1]);

						if (actor->pos[2] != actor->pos[1]) {
							actor->occupant->Link();
							actor->pos[2] = actor->pos[1];
						}

#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematics(" << c->name << ") actor(" << actorIdx << ") triggered." << std::endl;
#endif
					}
				}

			} else {
				RAD_ASSERT(actor->vtm);

					
				ska::Animation::Map::const_iterator animIt = actor->vtm->vtm->anims->find(c->name);

				if (animIt != actor->vtm->vtm->anims->end()) {
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
							*actor->vtm->vtm.get(),
							ska::Notify::Ref(new (ZWorld) SkaNotify(*this, *c))
						);

						actor->vtm->vtm->root = boost::static_pointer_cast<ska::Controller>(source);
						actor->vtm->vtm->Tick( // advance delta
							tickDelta,
							true,
							false // no animation events
						);

#if !defined(RAD_TARGET_GOLDEN)
					COut(C_Debug) << "Cinematics(" << c->name << ") actor(" << actorIdx << ") triggered." << std::endl;
#endif
					}
				}
			}

			if (actorDone) {
#if !defined(RAD_TARGET_GOLDEN)
				COut(C_Debug) << "Cinematics(" << c->name << ") actor(" << actorIdx << ") done." << std::endl;
#endif
				if (actor->flags&kHideWhenDone) {
					actor->visible = false;
					if (actor->skm) {
						actor->skm->ska->root = ska::Controller::Ref();
					} else {
						RAD_ASSERT(actor->vtm);
						actor->vtm->vtm->root = ska::Controller::Ref();
					}
				}

				actor->frame = -1;
				c->actors.erase(actorIdx);
			} else {
				c->actors.insert(actorIdx);
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

const bsp_file::BSPCinematic *WorldCinematics::FindCinematic(const char *name) {
	for (int i = 0; i < (int)m_bspFile->numCinematics.get(); ++i) {
		const BSPCinematic *bspC = m_bspFile->Cinematics()+i;
		const char *sz = m_bspFile->String(bspC->name);
		if (strcmp(name, sz))
			continue;
		return bspC;
	}

	return 0;
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

	Entity::Ref origin = c.origin.lock();
	if (origin) {
		Quat r = Quat(Vec3(0.f, 0.f, 1.f), math::DegToRad(origin->ps->worldAngles[2]));
		tmOut.r = r * tmOut.r;
		tmOut.t = Mat4::Rotation(r).Transform3X3(tmOut.t);
		tmOut.t = origin->ps->worldPos + tmOut.t;
	}

	if (m_cameraActive && c.xfade[0] < c.xfade[1]) {
		ska::BoneTM temp(tmOut);

		float blend = c.xfade[0] / c.xfade[1];
		tmOut.r = math::Slerp(m_world->camera->rot.get(), temp.r, blend);
		tmOut.t = math::Lerp(m_world->camera->pos.get(), temp.t, blend);
		fov = math::Lerp(m_world->camera->fov.get(), fov, blend);
	}

	if (!m_world->cvars->r_fly.value) {
		m_world->camera->rot = tmOut.r;
		m_world->camera->pos = tmOut.t;
		m_world->camera->fov = fov;
		m_world->camera->quatMode = true;
	}

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

WorldCinematics::SkActorBatch::SkActorBatch(WorldDraw &draw, const Actor &actor, int idx, int matId) :
MBatchDraw(draw, matId, actor.occupant.get()), m_idx(idx), m_actor(&actor) {
}

void WorldCinematics::SkActorBatch::Bind(r::Shader *shader) {
	m_actor->skm->Skin(m_idx);
	r::Mesh &m = m_actor->skm->Mesh(m_idx);
	m.BindAll(shader);
}

void WorldCinematics::SkActorBatch::CompileArrayStates(r::Shader &shader) {
	m_actor->skm->Mesh(m_idx).CompileArrayStates(shader);
}

void WorldCinematics::SkActorBatch::FlushArrayStates(r::Shader *shader) {
	m_actor->skm->Mesh(m_idx).FlushArrayStates(shader);
}

void WorldCinematics::SkActorBatch::Draw() {
	m_actor->skm->Mesh(m_idx).Draw();
}

Vec4 WorldCinematics::VtActorBatch::s_rgba(1, 1, 1, 1);
Vec3 WorldCinematics::VtActorBatch::s_scale(1, 1, 1);

WorldCinematics::VtActorBatch::VtActorBatch(WorldDraw &draw, const Actor &actor, int idx, int matId) :
MBatchDraw(draw, matId, actor.occupant.get()), m_idx(idx), m_actor(&actor) {
}

void WorldCinematics::VtActorBatch::Bind(r::Shader *shader) {
	m_actor->vtm->Skin(m_idx);
	r::Mesh &m = m_actor->vtm->Mesh(m_idx);
	m.BindAll(shader);
}

void WorldCinematics::VtActorBatch::CompileArrayStates(r::Shader &shader) {
	m_actor->vtm->Mesh(m_idx).CompileArrayStates(shader);
}

void WorldCinematics::VtActorBatch::FlushArrayStates(r::Shader *shader) {
	m_actor->vtm->Mesh(m_idx).FlushArrayStates(shader);
}

void WorldCinematics::VtActorBatch::Draw() {
	m_actor->vtm->Mesh(m_idx).Draw();
}

} // world
