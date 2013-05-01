/*! \file WorldDrawLight.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "World.h"
#include "Light.h"
#include "Entity.h"
#include "Occupant.h"

namespace world {

void WorldDraw::InvalidateInteractions(Light &light) {
	for (details::MatInteractionChain::const_iterator it = light.m_interactions.begin(); it != light.m_interactions.end(); ++it) {
		for (details::LightInteraction *i = it->second; i; i = i->nextOnLight) {
			i->dirty = true;
		}
	}
}

void WorldDraw::InvalidateInteractions(Entity &entity) {
	for (DrawModel::Map::const_iterator it = entity.models->begin(); it != entity.models->end(); ++it) {
		const DrawModel::Ref &model = it->second;
		for (MBatchDraw::RefVec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
			const MBatchDraw::Ref &batch = *it;
			for (details::LightInteraction *i = batch->m_interactions; i; i = i->nextOnBatch) {
				i->dirty = true;
			}
		}
	}
}

void WorldDraw::InvalidateInteractions(MBatchOccupant &occupant) {
	for (MBatchDraw::RefVec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
		const MBatchDraw::Ref &batch = *it;
		for (details::LightInteraction *i = batch->m_interactions; i; i = i->nextOnBatch) {
			i->dirty = true;
		}
	}
}

void WorldDraw::LinkEntity(
	Entity &entity, 
	const BBox &bounds, 
	int nodeNum,
	dBSPLeaf &leaf,
	dBSPArea &area
) {
	if (entity.lightingFlags.get() == kLightingFlag_Unlit)
		return;

	// link potential light interactions
	for (LightPtrSet::const_iterator it = leaf.lights.begin(); it != leaf.lights.end(); ++it) {
		Light &light = **it;
		BBox bounds(light.m_size);
		bounds.Translate(light.m_pos);

		if (bounds.Touches(bounds)) {
			for (DrawModel::Map::const_iterator it = entity.models->begin(); it != entity.models->end(); ++it) {
				const DrawModel::Ref &model = it->second;
				for (MBatchDraw::RefVec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
					const MBatchDraw::Ref &batch = *it;
					if (!FindInteraction(light, *batch)) {
						CreateInteraction(light, entity, *batch);
					}
				}
			}
		}
	}
}

void WorldDraw::UnlinkEntity(Entity &entity) {
	for (DrawModel::Map::const_iterator it = entity.models->begin(); it != entity.models->end(); ++it) {
		const DrawModel::Ref &model = it->second;
		for (MBatchDraw::RefVec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
			const MBatchDraw::Ref &batch = *it;
			while (batch->m_interactions) {
				details::LightInteraction **lightHead = batch->m_interactions->light->ChainHead(batch->matId);
				UnlinkInteraction(*batch->m_interactions, *lightHead, batch->m_interactions);
			}
		}
	}
}

void WorldDraw::LinkOccupant(
	MBatchOccupant &occupant, 
	const BBox &bounds, 
	int nodeNum,
	dBSPLeaf &leaf,
	dBSPArea &area
) {
	if (occupant.lightingFlags.get() == kLightingFlag_Unlit)
		return;

	// link potential light interactions
	for (LightPtrSet::const_iterator it = leaf.lights.begin(); it != leaf.lights.end(); ++it) {
		Light &light = **it;
		BBox bounds(light.m_size);
		bounds.Translate(light.m_pos);

		if (bounds.Touches(bounds)) {
			for (MBatchDraw::RefVec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
				const MBatchDraw::Ref &batch = *it;
				if (!FindInteraction(light, *batch)) {
					CreateInteraction(light, occupant, *batch);
				}
			}
		}
	}
}

void WorldDraw::UnlinkOccupant(MBatchOccupant &occupant) {
	for (MBatchDraw::RefVec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
		const MBatchDraw::Ref &batch = *it;
		while (batch->m_interactions) {
			details::LightInteraction **lightHead = batch->m_interactions->light->ChainHead(batch->matId);
			UnlinkInteraction(*batch->m_interactions, *lightHead, batch->m_interactions);
		}
	}
}

void WorldDraw::LinkLight(Light &light, const BBox &bounds) {
	// link to static world models

	// NOTE: models can span areas, this may check them multiple times.
	for (IntSet::const_iterator it = light.m_areas.begin(); it != light.m_areas.end(); ++it) {
		const dBSPArea &area = m_world->m_areas[*it];

		for (int i = 0; i < area.numModels; ++i) {
			U16 modelNum = *(m_world->m_bsp->ModelIndices() + i + area.firstModel);
			RAD_ASSERT(modelNum < (U16)m_worldModels.size());

			const MStaticWorldMeshBatch::Ref &m = m_worldModels[modelNum];

			if (bounds.Touches(m->bounds)) {
				if (!FindInteraction(light, *m)) {
					CreateInteraction(light, *m);
				}
			}
		}
	}
}

void WorldDraw::LinkLight(
	Light &light, 
	const BBox &bounds, 
	int nodeNum,
	dBSPLeaf &leaf,
	dBSPArea &area
) {
	for (EntityPtrSet::const_iterator it = leaf.entities.begin(); it != leaf.entities.begin(); it++) {
		Entity &entity = **it;
		for (DrawModel::Map::const_iterator it = entity.models->begin(); it != entity.models->end(); ++it) {
			const DrawModel::Ref &model = it->second;
			for (MBatchDraw::RefVec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
				const MBatchDraw::Ref &batch = *it;
				if (!FindInteraction(light, *batch)) {
					CreateInteraction(light, entity, *batch);
				}
			}
		}
	}

	for (MBatchOccupantPtrSet::const_iterator it = leaf.occupants.begin(); it != leaf.occupants.end(); ++it) {
		MBatchOccupant &occupant = **it;
		for (MBatchDraw::RefVec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
			const MBatchDraw::Ref &batch = *it;
			if (!FindInteraction(light, *batch)) {
				CreateInteraction(light, occupant, *batch);
			}
		}
	}
}

void WorldDraw::UnlinkLight(Light &light) {
	for (details::MatInteractionChain::iterator it = light.m_interactions.begin(); it != light.m_interactions.end(); ++it) {
		details::LightInteraction **lightHead = &it->second;

		details::LightInteraction *next = 0;
		for (details::LightInteraction *i = *lightHead; i; i = next) {
			next = i->nextOnLight;
			UnlinkInteraction(*i, *lightHead, i->draw->m_interactions);
			m_interactionPool.ReturnChunk(i);
		}
	}
}

details::LightInteraction *WorldDraw::FindInteraction(
	Light &light,
	MBatchDraw &batch
) {
	for (details::LightInteraction *i = batch.m_interactions; i; i = i->nextOnBatch) {
		if (i->light == &light)
			return i;
	}

	return 0;
}

details::LightInteraction *WorldDraw::CreateInteraction(
	Light &light,
	MBatchDraw &batch
) {
	details::LightInteraction *i = reinterpret_cast<details::LightInteraction*>(m_interactionPool.SafeGetChunk());
	i->draw = &batch;
	i->light = &light;
	i->occupant = 0;
	i->entity = 0;
	i->dirty = true;
	
	details::LightInteraction **lightHead = light.ChainHead(batch.matId);
	RAD_ASSERT(lightHead);
	LinkInteraction(*i, *lightHead, batch.m_interactions);

	return i;
}

details::LightInteraction *WorldDraw::CreateInteraction(
	Light &light,
	Entity &entity,
	MBatchDraw &batch
) {
	details::LightInteraction *i = reinterpret_cast<details::LightInteraction*>(m_interactionPool.SafeGetChunk());
	i->draw = &batch;
	i->entity = &entity;
	i->light = &light;
	i->occupant = 0;
	i->dirty = true;
	
	details::LightInteraction **lightHead = light.ChainHead(batch.matId);
	RAD_ASSERT(lightHead);
	LinkInteraction(*i, *lightHead, batch.m_interactions);

	return i;
}

details::LightInteraction *WorldDraw::CreateInteraction(
	Light &light,
	MBatchOccupant &occupant,
	MBatchDraw &batch
) {
	details::LightInteraction *i = reinterpret_cast<details::LightInteraction*>(m_interactionPool.SafeGetChunk());
	i->draw = &batch;
	i->occupant = &occupant;
	i->light = &light;
	i->entity = 0;
	i->dirty = true;

	details::LightInteraction **lightHead = light.ChainHead(batch.matId);
	RAD_ASSERT(lightHead);
	LinkInteraction(*i, *lightHead, batch.m_interactions);

	return i;
}

void WorldDraw::LinkInteraction(
	details::LightInteraction &interaction,
	details::LightInteraction *&headOnLight,
	details::LightInteraction *&headOnBatch
) {
	interaction.prevOnLight = 0;
	interaction.nextOnLight = headOnLight;
	headOnLight = &interaction;

	interaction.prevOnBatch = 0;
	interaction.nextOnBatch = headOnBatch;
	headOnBatch = &interaction;
}

void WorldDraw::UnlinkInteraction(
	details::LightInteraction &interaction,
	details::LightInteraction *&headOnLight,
	details::LightInteraction *&headOnBatch
) {
	if (headOnLight = &interaction)
		headOnLight = interaction.nextOnLight;
	if (interaction.nextOnLight)
		interaction.nextOnLight->prevOnLight = interaction.prevOnLight;
	if (interaction.prevOnLight)
		interaction.prevOnLight->nextOnLight = interaction.nextOnLight;


	if (headOnBatch = &interaction)
		headOnBatch = interaction.nextOnBatch;
	if (interaction.nextOnBatch)
		interaction.nextOnBatch->prevOnBatch = interaction.prevOnBatch;
	if (interaction.prevOnBatch)
		interaction.prevOnBatch->nextOnBatch = interaction.nextOnBatch;
}

void WorldDraw::UpdateLightInteractions(Light &light) {
	
	BBox bounds(light.m_size);
	bounds.Translate(light.m_pos);

	for (details::MatInteractionChain::iterator it = light.m_interactions.begin(); it != light.m_interactions.end(); ++it) {
		details::LightInteraction **lightHead = &it->second;
		details::LightInteraction *next = 0;
		for (details::LightInteraction *i = *lightHead; i; i = next) {
			next = i->nextOnLight;

			if (i->dirty) {
				i->dirty = false;
				BBox batchBounds(i->draw->bounds);

				if (i->entity) {
					batchBounds.Translate(i->entity->ps->worldPos);
				}

				if (!bounds.Touches(batchBounds)) {
					UnlinkInteraction(*i, *lightHead, i->draw->m_interactions);
					m_interactionPool.ReturnChunk(i);
				}
			}
		}
	}
}

Light::Ref WorldDraw::CreateLight() {
	Light::Ref l(new Light(this->m_world), &DeleteLight);
	l->m_prev = 0;
	l->m_next = m_lights[0];
	if (m_lights[0])
		m_lights[0]->m_prev = l.get();
	if (!m_lights[1])
		m_lights[1] = l.get();
	m_lights[0] = l.get();
	return l;
}

void WorldDraw::CleanupLights() {
	for (Light *l = m_lights[0]; l; l = l->m_next) {
		UnlinkLight(*l);
	}
	m_lights[0] = m_lights[1] = 0;
}

void WorldDraw::DeleteLight(Light *light) {
	WorldDraw *self = light->m_world->m_draw.get();
	if (self->m_lights[0]) {
		self->UnlinkLight(*light);
		if (light->m_prev)
			light->m_prev->m_next = light->m_next;
		if (light->m_next)
			light->m_next->m_prev = light->m_prev;
		if (self->m_lights[0] == light)
			self->m_lights[0] = light->m_next;
		if (self->m_lights[1] == light)
			self->m_lights[1] = light->m_prev;
	}
	delete light;
}

} // world
