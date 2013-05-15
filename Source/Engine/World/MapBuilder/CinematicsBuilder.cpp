/*! \file CinematicsBuilder.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#include RADPCH
#include "CinematicsBuilder.h"

namespace tools {

bool CinematicsBuilder::Compile(
	const SceneFile &map,
	const CinematicActorCompressionMap &caMap,
	const world::bsp_file::BSPFileBuilder::Ref &bspFile
) {
	m_bspFile = bspFile;
	m_skaSize = 0;

	// create actor list
	m_actors.reserve(64);

	const SceneFile::Entity::Ref &world = map.worldspawn;
	for (SceneFile::TriModel::Vec::const_iterator it = world->models.begin(); it != world->models.end(); ++it) {
		const SceneFile::TriModel::Ref &model = *it;
		if (!model->cinematic)
			continue;
		if (model->anims.empty())
			continue;
		if (model->anims.begin()->second->boneFrames.empty() &&
			model->anims.begin()->second->vertexFrames.empty()) {
			continue;
		}
		if (!model->anims.begin()->second->boneFrames.empty() && (model->skel < 0)) {
			continue; // missing skeleton.
		}

		Actor a;
		a.emitId = -1;
		a.index = (int)(it-world->models.begin());
		a.flags = world::bsp_file::kCinematicObj;
		if (model->hideUntilRef)
			a.flags |= world::bsp_file::kHideUntilRef;
		if (model->hideWhenDone)
			a.flags |= world::bsp_file::kHideWhenDone;
		if (model->affectedByObjectLights)
			a.flags |= world::bsp_file::kAffectedByObjectLights;
		if (model->affectedByWorldLights)
			a.flags |= world::bsp_file::kAffectedByWorldLights;
		if (model->castShadows)
			a.flags |= world::bsp_file::kCastShadows;
		if (model->sky)
			a.flags |= world::bsp_file::kSkyActor;
		
		a.bounds = model->bounds;
		a.pos = a.bounds.Origin();
		a.bounds.Translate(-a.bounds.Origin());
		m_actors.push_back(a);
	}

	// pull names from cameras first...

	for (SceneFile::Camera::Vec::const_iterator it = map.cameras.begin(); it != map.cameras.end(); ++it) {
		const SceneFile::Camera::Ref &camera = *it;
		for (SceneFile::AnimMap::const_iterator it = camera->anims.begin(); it != camera->anims.end(); ++it) {
			if (!EmitCinematic(map, caMap, it->second->name))
				return false;
		}
	}

	// find cinematic actors.

	for (Actor::Vec::const_iterator it = m_actors.begin(); it != m_actors.end(); ++it) {
		const Actor &actor = *it;
		const SceneFile::TriModel::Ref &model = world->models[actor.index];
		for (SceneFile::AnimMap::const_iterator it = model->anims.begin(); it != model->anims.end(); ++it) {
			if (!EmitCinematic(map, caMap, it->second->name))
				return false;
		}
	}

	return true;
}

bool CinematicsBuilder::EmitCinematic(const SceneFile &map, const CinematicActorCompressionMap &caMap, const String &name) {
	if (m_cinematics.find(name) != m_cinematics.end())
		return true;

	COut(C_Info) << "Compiling cinematic '" << name << "'..." << std::endl;
	m_cinematics.insert(name);

	int fps = 30;
	Trigger::Map triggers;

	const SceneFile::Entity::Ref &world = map.worldspawn;

	// gather camera motion first.
	for (SceneFile::Camera::Vec::const_iterator it = map.cameras.begin(); it != map.cameras.end(); ++it) {
		const SceneFile::Camera::Ref &camera = *it;
		SceneFile::AnimMap::const_iterator animIt = camera->anims.find(name);
		if (animIt == camera->anims.end())
			continue;

		const SceneFile::Anim::Ref &anim = animIt->second;
		fps = (int)anim->frameRate;
		Trigger::Map::iterator triggerIt = triggers.find(anim->firstFrame);

		if (triggerIt != triggers.end()) {
			triggerIt->second.camera = (int)(it-map.cameras.begin());
		} else {
			Trigger t;
			t.frame = anim->firstFrame;
			t.camera = (int)(it-map.cameras.begin());
			triggers[t.frame] = t;
		}
	}

	// gather actor triggers
	for (Actor::Vec::iterator it = m_actors.begin(); it != m_actors.end(); ++it) {
		Actor &actor = *it;
		const SceneFile::TriModel::Ref &model = world->models[actor.index];
		SceneFile::AnimMap::const_iterator animIt = model->anims.find(name);
		if (animIt == model->anims.end())
			continue;
		
		const SkaCompressionMap *compressionMap = 0;
		CinematicActorCompressionMap::const_iterator caMapIt = caMap.find(model->name);
		if (caMapIt != caMap.end())
			compressionMap = &caMapIt->second;

		if (!EmitActor(map, compressionMap, actor))
			return false;

		const SceneFile::Anim::Ref &anim = animIt->second;
		fps = (int)anim->frameRate;
		Trigger::Map::iterator triggerIt = triggers.find(anim->firstFrame);

		int actorIdx = (int)(it-m_actors.begin());
		if (anim->looping)
			actorIdx |= 0x10000000;

		if (triggerIt != triggers.end()) {
			triggerIt->second.actors.push_back(actorIdx);
		} else {
			Trigger t;
			t.frame = anim->firstFrame;
			t.camera = -1;
			t.actors.push_back(actorIdx);
			triggers[t.frame] = t;
		}
	}

	if (triggers.empty())
		return true;

	int firstFrame = triggers.begin()->first;

	world::bsp_file::BSPCinematic *cinematic = m_bspFile->AddCinematic();
	cinematic->name = (int)m_bspFile->numStrings.get();
	*m_bspFile->AddString() = name.c_str;
	cinematic->firstTrigger = (int)m_bspFile->numCinematicTriggers.get();
	cinematic->fps = fps;

	for (Trigger::Map::const_iterator it = triggers.begin(); it != triggers.end(); ++it) {
		const Trigger &trigger = it->second;
		int frame = trigger.frame - firstFrame;

		world::bsp_file::BSPCinematicTrigger *bspTrigger = m_bspFile->AddCinematicTrigger();
		bspTrigger->frame = frame;
		bspTrigger->camera = -1;
		bspTrigger->firstActor = (int)m_bspFile->numActorIndices.get();
		bspTrigger->numActors = 0;

		if (trigger.camera > -1) { // emit camera track
			const SceneFile::Camera::Ref &camera = map.cameras[trigger.camera];
			SceneFile::AnimMap::const_iterator animIt = camera->anims.find(name);
			RAD_VERIFY(animIt != camera->anims.end());
			const SceneFile::Anim::Ref &anim = animIt->second;

			bspTrigger->camera = (int)m_bspFile->numCameraTracks.get();
			world::bsp_file::BSPCameraTrack *track = m_bspFile->AddCameraTrack();

			track->firstTM = (int)m_bspFile->numCameraTMs.get();
			track->name = (int)m_bspFile->numStrings.get();
			*m_bspFile->AddString() = camera->name.c_str;

			m_bspFile->ReserveCameraTMs((int)anim->boneFrames.size());

			for (SceneFile::BoneFrames::const_iterator it = anim->boneFrames.begin(); it != anim->boneFrames.end(); ++it) {
				const SceneFile::BonePoseVec &pose = *it;
				RAD_VERIFY(pose.size() == 1);
				world::bsp_file::BSPCameraTM *tm = m_bspFile->AddCameraTM();

				tm->r = pose[0].m.r;
				tm->t = pose[0].m.t;
				tm->fov = pose[0].fov;

				if (pose[0].tag.empty) {
					tm->tag = -1;
				} else {
					tm->tag = (int)m_bspFile->numStrings.get();
					*m_bspFile->AddString() = pose[0].tag.c_str;
				}

				++track->numTMs;
			}
		}

		for (IntVec::const_iterator it = trigger.actors.begin(); it != trigger.actors.end(); ++it) {
			int index = *it;
			bool loop = index&0x10000000 ? true : false;
			index &= 0x00ffffff;

			Actor &actor = m_actors[index];

			index = actor.emitId;
			if (loop)
				index |= 0x10000000;

			*m_bspFile->AddActorIndex() = (U32)index;
			++bspTrigger->numActors;
		}

		++cinematic->numTriggers;
	}

	return true;
}

bool CinematicsBuilder::EmitActor(const SceneFile &map, const SkaCompressionMap *compression, Actor &actor) {
	if (actor.emitId > -1)
		return true;

	actor.emitId = (int)m_bspFile->numActors.get();
	world::bsp_file::BSPActor *bspActor = m_bspFile->AddActor();
	
	bspActor->flags = actor.flags;
	bspActor->initial = -1;
	
	for (int i = 0; i < 3; ++i) {
		bspActor->pos[i] = actor.pos[i];
		bspActor->mins[i] = actor.bounds.Mins()[i];
		bspActor->maxs[i] = actor.bounds.Maxs()[i];
	}

	tools::SceneFile::TriModel::Ref m = map.worldspawn->models[actor.index];

	if (!m->anims.begin()->second->boneFrames.empty()) {

		// emit ska
		tools::SkaData::Ref ska = tools::CompileSkaData(
			"BSPBuilder",
			map,
			actor.index,
			compression
		);

		if (!ska)
			return false;

		tools::SkmData::Ref skm = tools::CompileSkmData(
			"BSPBuilder",
			map,
			actor.index,
			ska::kSkinType_CPU,
			ska->dska
		);

		if (!skm)
			return false;

		bspActor->ska = m_bspFile->AddSka(ska, skm);

		m_skaSize += ska->skaSize;
		m_skaSize += skm->skmSize[0] + skm->skmSize[1];
	} else {
		RAD_ASSERT(!m->anims.begin()->second->vertexFrames.empty());

		tools::VtmData::Ref vtm = tools::CompileVtmData("BSPBuilder", map, map, actor.index);
		if (!vtm)
			return false;

		bspActor->ska = -(m_bspFile->AddVtm(vtm) + 1);

		m_skaSize += vtm->vtmSize[0] + vtm->vtmSize[1];
	}

	return true;
}

} // tools
