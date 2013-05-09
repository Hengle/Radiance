/*! \file WorldDrawLight.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "World.h"
#include "../Game/GameCVars.h"
#include "Light.h"
#include "Entity.h"
#include "Occupant.h"
#include "../Renderer/Shader.h"

namespace world {

void WorldDraw::DrawUnshadowedLitBatch(const details::MBatch &batch) {

	if (!batch.head)
		return;

	r::Material *mat = batch.matRef->mat;

	bool first = true;

	mat->BindStates();
	mat->BindTextures(batch.matRef->loader);

	RAD_ASSERT(mat->shader->HasPass(r::Shader::kPass_Default));
	mat->shader->Begin(r::Shader::kPass_Default, *mat);

	// draw base pass
	for (details::MBatchDrawLink *link = batch.head; link; link = link->next) {
		MBatchDraw *draw = link->draw;

		if (first) {
			first = false;
			++m_counters.numMaterials;
		}

		DrawBatch(*draw, *mat);
	}

	mat->shader->End();

	if (!batch.matRef->mat->lit)
		return;

	for (details::MBatchDrawLink *link = batch.head; link; link = link->next) {
		MBatchDraw *draw = link->draw;
		DrawUnshadowedLitBatchLights(*draw, *mat);
	}
}

void WorldDraw::DrawBatch(
	MBatchDraw &draw,
	r::Material &mat
) {
	Vec3 pos;
	Vec3 angles;
	Mat4 invTx;

	bool tx = draw.GetTransform(pos, angles);
	if (tx) {
		m_rb->PushMatrix(pos, draw.scale, angles);
		invTx = Mat4::Translation(-pos) * (Mat4::Rotation(QuatFromAngles(angles)).Transpose());
	}

	draw.Bind(mat.shader.get().get());
	r::Shader::Uniforms u(draw.rgba.get());

	if (tx) {
		u.eyePos = invTx * m_world->camera->pos.get();
	} else {
		u.eyePos = m_world->camera->pos;
	}

	mat.shader->BindStates(u);
	m_rb->CommitStates();
	draw.CompileArrayStates(*mat.shader.get());
	draw.Draw();

	if (tx)
		m_rb->PopMatrix();
}

void WorldDraw::DrawUnshadowedLitBatchLights(MBatchDraw &draw, r::Material &mat) {

	Vec3 pos;
	Vec3 angles;
	Mat4 invTx;

	bool diffuse = mat.shader->HasPass(r::Shader::kPass_Diffuse1);
	bool specular = mat.shader->HasPass(r::Shader::kPass_Specular1);
	bool diffuseSpecular = mat.shader->HasPass(r::Shader::kPass_DiffuseSpecular1);

	RAD_ASSERT_MSG(!(diffuse&&specular) || diffuseSpecular, "Shaders with diffuse and specular passes must also have combined DiffuseSpecular pass"); // sanity check on shader

	bool didDiffuse = false;
	bool didDiffuseSpecular = false;

	const bool tx = draw.GetTransform(pos, angles);
	if (tx) {
		m_rb->PushMatrix(pos, draw.scale, angles);
		invTx = Mat4::Translation(-pos) * (Mat4::Rotation(QuatFromAngles(angles)).Transpose());
	}

	// draw all unshadowed lights
	r::Shader::Uniforms u(draw.rgba.get());
	BBox lightBounds;

	if (tx) {
		u.eyePos = invTx * m_world->camera->pos.get();
	} else {
		u.eyePos = m_world->camera->pos;
	}

	int curPass = -1;

	for (;;) {
		details::LightInteraction *interaction = draw.m_interactions;

		while (interaction) {

			// batch up to kNumLights
			u.lights.numLights = 0;
			lightBounds.Initialize();

			while (interaction && (u.lights.numLights < r::kMaxLights)) {
				if (interaction->light->m_visFrame != m_markFrame) {
					interaction = interaction->nextOnBatch;
					continue; // not visible this frame.
				}

				Light::LightStyle style = interaction->light->style;

				if (!(style & Light::kStyle_CastShadows)) {
					bool add = false;

					if ((style&Light::kStyle_DiffuseSpecular) == Light::kStyle_DiffuseSpecular) {
						add = diffuseSpecular || (!didDiffuseSpecular && (diffuse||specular));
					}
					
					if (style&Light::kStyle_Diffuse) {
						add = add || (!diffuseSpecular && !didDiffuseSpecular && diffuse);
					}

					if (style&Light::kStyle_Specular) {
						add = add || (!diffuseSpecular && !didDiffuseSpecular && specular);
					}

					if (add) {
						if (interaction->light->m_drawFrame != m_markFrame) {
							interaction->light->m_drawFrame = m_markFrame;
							++m_counters.drawnLights;
						}
						r::LightDef &lightDef = u.lights.lights[u.lights.numLights++];
						GenLightDef(*interaction->light, lightDef, tx ? &invTx : 0);
						BBox bounds(interaction->light->bounds);
						bounds.Translate(interaction->light->pos);
						lightBounds.Insert(bounds);
					}
				}

				interaction = interaction->nextOnBatch;
			}

			if (u.lights.numLights > 0) {

				// find the correct pass
				int pass;

				if (diffuseSpecular) {
					pass = r::Shader::kPass_DiffuseSpecular1 + u.lights.numLights - 1;
				} else if (diffuse) {
					pass = r::Shader::kPass_Diffuse1 + u.lights.numLights - 1;
				} else {
					RAD_ASSERT(specular);
					pass = r::Shader::kPass_Specular1 + u.lights.numLights - 1;
				}

				RAD_ASSERT(mat.shader->HasPass((r::Shader::Pass)pass));

				if (curPass != pass) {
					mat.shader->End();
					mat.shader->Begin((r::Shader::Pass)pass, mat);
					curPass = pass;
					draw.Bind(mat.shader.get().get());
				}

				Vec4 scissorRect;
				bool scissor = false;
				
				if (m_world->cvars->r_lightscissor.value) {
					scissor = m_rb->CalcBoundsScissor(lightBounds, scissorRect);
				}

				m_rb->BindLitMaterialStates(mat, scissor ? &scissorRect : 0);

				mat.shader->BindStates(u);
				m_rb->CommitStates();
				draw.CompileArrayStates(*mat.shader.get());
				draw.Draw();

				++m_counters.numBatches;
			}
		}

		if (diffuseSpecular) {
			diffuseSpecular = false;
			didDiffuseSpecular = true;
		} else if(diffuse) {
			diffuse = false;
			didDiffuse = true;
		} else {
			break;
		}
	}

	if (curPass != -1)
		mat.shader->End();

	if (tx)
		m_rb->PopMatrix();
}

void WorldDraw::GenLightDef(
	const Light &light,
	r::LightDef &lightDef,
	Mat4 *tx
) {
	lightDef.diffuse = light.diffuseColor;
	lightDef.specular = light.specularColor;
	lightDef.pos = tx ? ((*tx) * light.pos.get()) : light.pos;
	lightDef.brightness = light.brightness;
	lightDef.radius = light.radius;
	
	lightDef.flags = 0;
	if (light.style.get() & Light::kStyle_Diffuse)
		lightDef.flags |= r::LightDef::kFlag_Diffuse;
	if (light.style.get() & Light::kStyle_Specular)
		lightDef.flags |= r::LightDef::kFlag_Specular;
}

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
	if (entity.lightInteractionFlags == 0)
		return;
	
	// link potential light interactions
	for (LightPtrSet::const_iterator it = leaf.lights.begin(); it != leaf.lights.end(); ++it) {
		Light &light = **it;

		if (!(light.interactionFlags & entity.lightInteractionFlags))
			continue; // masked

		BBox lightBounds(light.m_bounds);
		lightBounds.Translate(light.m_pos);

		if (lightBounds.Touches(bounds)) {
			for (DrawModel::Map::const_iterator it = entity.models->begin(); it != entity.models->end(); ++it) {
				const DrawModel::Ref &model = it->second;
				for (MBatchDraw::RefVec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
					const MBatchDraw::Ref &batch = *it;
					BBox batchBounds(batch->bounds);
					batchBounds.Translate(entity.ps->worldPos);
					if (lightBounds.Touches(batchBounds)) {
						if (batch->lit && !FindInteraction(light, *batch)) {
							CreateInteraction(light, entity, *batch);
						}
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
	if (occupant.lightInteractionFlags == 0)
		return;

	// link potential light interactions
	for (LightPtrSet::const_iterator it = leaf.lights.begin(); it != leaf.lights.end(); ++it) {
		Light &light = **it;

		if (!(light.interactionFlags & occupant.lightInteractionFlags))
			continue; // masked

		BBox lightBounds(light.m_bounds);
		lightBounds.Translate(light.m_pos);

		if (lightBounds.Touches(bounds)) {
			for (MBatchDraw::RefVec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
				const MBatchDraw::Ref &batch = *it;
				if (lightBounds.Touches(batch->bounds)) {
					if (batch->lit && !FindInteraction(light, *batch)) {
						CreateInteraction(light, occupant, *batch);
					}
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

	if ((!light.interactionFlags & Light::kInteractionFlag_World))
		return; // not a world light

	// NOTE: models can span areas, this may check them multiple times.
	for (IntSet::const_iterator it = light.m_areas.begin(); it != light.m_areas.end(); ++it) {
		const dBSPArea &area = m_world->m_areas[*it];

		for (int i = 0; i < area.numModels; ++i) {
			U16 modelNum = *(m_world->m_bsp->ModelIndices() + i + area.firstModel);
			RAD_ASSERT(modelNum < (U16)m_worldModels.size());

			const MStaticWorldMeshBatch::Ref &m = m_worldModels[modelNum];

			if (m->lit && bounds.Touches(m->bounds)) {
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

		if (!(light.interactionFlags & entity.lightInteractionFlags))
			continue;

		BBox entBounds(entity.ps->bbox);
		entBounds.Translate(entity.ps->worldPos);

		if (!bounds.Touches(entBounds))
			continue;

		for (DrawModel::Map::const_iterator it = entity.models->begin(); it != entity.models->end(); ++it) {
			const DrawModel::Ref &model = it->second;
			for (MBatchDraw::RefVec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
				const MBatchDraw::Ref &batch = *it;
				if (batch->lit && !FindInteraction(light, *batch)) {
					CreateInteraction(light, entity, *batch);
				}
			}
		}
	}

	for (MBatchOccupantPtrSet::const_iterator it = leaf.occupants.begin(); it != leaf.occupants.end(); ++it) {
		MBatchOccupant &occupant = **it;

		if (!(light.interactionFlags & occupant.lightInteractionFlags))
			continue;

		for (MBatchDraw::RefVec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
			const MBatchDraw::Ref &batch = *it;
			if (batch->lit && !FindInteraction(light, *batch)) {
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

	if (headOnLight)
		headOnLight->prevOnLight = &interaction;

	headOnLight = &interaction;

	interaction.prevOnBatch = 0;
	interaction.nextOnBatch = headOnBatch;

	if (headOnBatch)
		headOnBatch->prevOnBatch = &interaction;

	headOnBatch = &interaction;
}

void WorldDraw::UnlinkInteraction(
	details::LightInteraction &interaction,
	details::LightInteraction *&headOnLight,
	details::LightInteraction *&headOnBatch
) {
	if (headOnLight == &interaction)
		headOnLight = interaction.nextOnLight;
	if (interaction.nextOnLight)
		interaction.nextOnLight->prevOnLight = interaction.prevOnLight;
	if (interaction.prevOnLight)
		interaction.prevOnLight->nextOnLight = interaction.nextOnLight;


	if (headOnBatch == &interaction)
		headOnBatch = interaction.nextOnBatch;
	if (interaction.nextOnBatch)
		interaction.nextOnBatch->prevOnBatch = interaction.prevOnBatch;
	if (interaction.prevOnBatch)
		interaction.prevOnBatch->nextOnBatch = interaction.nextOnBatch;
}

void WorldDraw::UpdateLightInteractions(Light &light) {
	
	BBox bounds(light.m_bounds);
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
