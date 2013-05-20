/*! \file WorldDrawLight.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "World.h"
#include "../Game/Game.h"
#include "../Game/GameCVars.h"
#include "Light.h"
#include "Entity.h"
#include "Occupant.h"
#include "../Renderer/Shader.h"
#include <algorithm>

namespace world {

void WorldDraw::DrawUnshadowedLitBatch(
	ViewDef &view,
	const details::MBatch &batch
) {

	if (!batch.head)
		return;

	r::Material *mat = batch.matRef->mat;

	RAD_ASSERT(mat->maxLights > 0);

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

		DrawBatch(view, *draw, *mat);
	}

	mat->shader->End();

	for (details::MBatchDrawLink *link = batch.head; link; link = link->next) {
		MBatchDraw *draw = link->draw;
		DrawUnshadowedLitBatchLights(view, *draw, *mat);
	}
}

void WorldDraw::DrawBatch(
	ViewDef &view,
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

void WorldDraw::DrawUnshadowedLitBatchLights(
	ViewDef &view,
	MBatchDraw &draw, 
	r::Material &mat
) {

	RAD_ASSERT(mat.maxLights > 0);

	Vec3 pos;
	Vec3 angles;
	Mat4 invTx;

	bool diffuse = mat.shader->HasPass(r::Shader::kPass_Diffuse1);
	bool diffuseSpecular = mat.shader->HasPass(r::Shader::kPass_DiffuseSpecular1);

	RAD_ASSERT_MSG(diffuse, "All lighting shaders must have diffuse pass!");
	
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
	
	const int kMaxLights = std::min(mat.maxLights.get(), m_world->cvars->r_maxLightsPerPass.value.get());

	bool didDiffuse = false;
	bool didDiffuseSpecular = false;

	for (;;) {
		details::LightInteraction *interaction = draw.m_interactions;

		while (interaction) {

			// batch up to kNumLights
			u.lights.numLights = 0;
			lightBounds.Initialize();
	
			while (interaction && (u.lights.numLights < kMaxLights)) {
				if (interaction->light->m_visFrame != m_markFrame) {
					interaction = interaction->nextOnBatch;
					continue; // not visible this frame.
				}

				const Light::LightStyle kStyle = interaction->light->style;

				// TODO: non-unified shadows need to only render unshadowed passes
				// in this function
				
				bool add = false;

				if ((kStyle&Light::kStyle_DiffuseSpecular) == Light::kStyle_DiffuseSpecular) {
					add = !didDiffuseSpecular;
				} else if (kStyle&Light::kStyle_Diffuse) {
					add = diffuse && (!diffuseSpecular || didDiffuseSpecular);
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

				interaction = interaction->nextOnBatch;
			}

			if (u.lights.numLights > 0) {

				// find the correct pass
				int pass;

				if (diffuseSpecular && !didDiffuseSpecular) {
					pass = r::Shader::kPass_DiffuseSpecular1 + u.lights.numLights - 1;
				} else {
					RAD_ASSERT(diffuse);
					pass = r::Shader::kPass_Diffuse1 + u.lights.numLights - 1;
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
					scissor = CalcScissorBounds(view, lightBounds, scissorRect);
				}

				m_rb->BindLitMaterialStates(mat, scissor ? &scissorRect : 0);

				mat.shader->BindStates(u);
				m_rb->CommitStates();
				draw.CompileArrayStates(*mat.shader.get());
				draw.Draw();

				++m_counters.numBatches;
			}
		}

		if (diffuseSpecular && !didDiffuseSpecular) {
			didDiffuseSpecular = true;
		} else {
			break;
		}
	}

	if (tx)
		m_rb->PopMatrix();

	if (curPass != -1) {
		mat.shader->End();
	}
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

bool WorldDraw::CalcScissorBounds(
	const ViewDef &view,
	const BBox &bounds,
	Vec4 &rect
) {
	if (bounds.Contains(view.camera.pos))
		return false;

	Vec4 rect;
	
	rect[0] = std::numeric_limits<float>::max();
	rect[1] = std::numeric_limits<float>::max();
	rect[2] = std::numeric_limits<float>::min();
	rect[3] = std::numeric_limits<float>::min();

	for (int x = 0; x < 2; ++x) {
		float fx = x ? bounds.Maxs()[0] : bounds.Mins()[0];

		for (int y = 0; y < 2; ++y) {

			float fy = y ? bounds.Maxs()[1] : bounds.Mins()[1];

			for (int z = 0; z < 2; ++z) {

				float fz = z ? bounds.Maxs()[2] : bounds.Mins()[2];

				Vec3 p;
				::Project(
					view.mvp,
					&view.viewport[0],
					Vec3(fx, fy, fz),
					p
				);

				// bounds
				rect[0] = math::Min(rect[0], p[0]);
				rect[1] = math::Min(rect[1], p[1]);
				rect[2] = math::Max(rect[2], p[0]);
				rect[3] = math::Max(rect[3], p[1]);
			}
		}
	}

	// clamp
	rect[0] = math::Clamp(rect[0], (float)view.viewport[0], (float)view.viewport[0]+view.viewport[2]);
	rect[1] = math::Clamp(rect[1], (float)view.viewport[1], (float)view.viewport[1]+view.viewport[3]);
	rect[2] = math::Clamp(rect[2], (float)view.viewport[0], (float)view.viewport[0]+view.viewport[2]);
	rect[3] = math::Clamp(rect[3], (float)view.viewport[1], (float)view.viewport[1]+view.viewport[3]);

	if ((rect[0] == (float)view.viewport[0]) && (rect[1] == (float)view.viewport[1]) &&
		(rect[2] == ((float)view.viewport[0]+view.viewport[2])) && (rect[3] == ((float)view.viewport[1]+view.viewport[3]))) {
		return false;
	}

	return true;
}

void WorldDraw::SetBoundingOrthoMatrix(
	const ViewDef &view,
	const BBox &bounds
) {
	Vec4 viewPlaneBounds;
	Vec2 zBounds;

	viewPlaneBounds[0] = std::numeric_limits<float>::max();
	viewPlaneBounds[1] = std::numeric_limits<float>::max();
	viewPlaneBounds[2] = std::numeric_limits<float>::min();
	viewPlaneBounds[3] = std::numeric_limits<float>::min();
	zBounds[0] = std::numeric_limits<float>::min();
	zBounds[1] = std::numeric_limits<float>::max();

	for (int x = 0; x < 2; ++x) {
		float fx = x ? bounds.Maxs()[0] : bounds.Mins()[0];

		for (int y = 0; y < 2; ++y) {

			float fy = y ? bounds.Maxs()[1] : bounds.Mins()[1];

			for (int z = 0; z < 2; ++z) {

				float fz = z ? bounds.Maxs()[2] : bounds.Mins()[2];

				Vec3 p = Vec3(fx, fy, fz) * view.mv;
				
				// bounds
				viewPlaneBounds[0] = math::Min(viewPlaneBounds[0], p[0]);
				viewPlaneBounds[1] = math::Min(viewPlaneBounds[1], p[1]);
				viewPlaneBounds[2] = math::Max(viewPlaneBounds[2], p[0]);
				viewPlaneBounds[3] = math::Max(viewPlaneBounds[3], p[1]);
				zBounds[0] = math::Max(zBounds[0], p[2]);
				zBounds[1] = math::Min(zBounds[1], p[2]);
			}
		}
	}

	m_rb->SetOrthoMatrix(
		viewPlaneBounds[0] - 16.f,
		viewPlaneBounds[2] + 16.f,
		viewPlaneBounds[1] - 16.f,
		viewPlaneBounds[3] + 16.f,
		zBounds[1] - 16.f,
		zBounds[0] + 16.f
	);
}

void WorldDraw::DrawViewUnifiedShadows(ViewDef &view) {
	for (EntityPtrSet::const_iterator it = view.shadowEntities.begin(); it != view.shadowEntities.end(); ++it) {
		DrawUnifiedEntityShadow(view, **it);
	}
	for (MBatchOccupantPtrSet::const_iterator it = view.shadowOccupants.begin(); it != view.shadowOccupants.end(); ++it) {
		DrawUnifiedOccupantShadow(view, **it);
	}
}

void WorldDraw::DrawUnifiedEntityShadow(ViewDef &view, const Entity &e) {
	Vec3 unifiedPos;
	float unifiedRadius;

	BBox bounds(e.ps->bbox);
	bounds.Translate(e.ps->worldPos);

	CalcUnifiedShadowPosAndSize(
		bounds,
		e.m_lightInteractions,
		unifiedPos,
		unifiedRadius
	);

	DrawUnifiedShadow(
		view,
		*e.models,
		unifiedPos,
		unifiedRadius
	);
}

void WorldDraw::DrawUnifiedOccupantShadow(ViewDef &view, const MBatchOccupant &o) {
	Vec3 unifiedPos;
	float unifiedRadius;

	CalcUnifiedShadowPosAndSize(
		o.bounds,
		o.m_lightInteractions,
		unifiedPos,
		unifiedRadius
	);

	DrawUnifiedShadow(
		view,
		*o.batches,
		unifiedPos,
		unifiedRadius
	);
}

void WorldDraw::DrawUnifiedShadow(
	ViewDef &view,
	const DrawModel::Map &models,
	const Vec3 &unifiedPos,
	float unifiedRadius
) {
}

void WorldDraw::DrawUnifiedShadow(
	ViewDef &view,
	const MBatchDraw::RefVec &batches,
	const Vec3 &unifiedPos,
	float unifiedRadius
) {
}

void WorldDraw::DrawUnifiedShadowBatches(
	const MBatchDraw::RefVec &batches
) {
}

void WorldDraw::CalcUnifiedShadowPosAndSize(
	const BBox &bounds,
	const details::LightInteraction *head,
	Vec3 &pos,
	float &radius
) {
	float totalDist = 0.f;
	const Vec3 &origin = bounds.Origin();
	
	for (const details::LightInteraction *i = head; i; i = i->nextOnBatch) {
		Vec3 z = i->light->pos.get() - origin;
		totalDist += z.Magnitude();
	}

	pos = head->light->pos;

	if (totalDist > 0.f) {
		
		for (const details::LightInteraction *i = head->nextOnBatch; i; i = i->nextOnBatch) {
			Vec3 z = i->light->pos.get() - origin;
			float w = z.Magnitude() / totalDist;
			if (w >= 1.f) {
				pos = i->light->pos;
			} else {
				pos = math::Lerp(i->light->pos.get(), pos, w);
			}
		}

		radius = 0.f;

		// calculate the radius that encloses all the effecting lights

		for (const details::LightInteraction *i = head->nextOnBatch; i; i = i->nextOnBatch) {
			Vec3 z = i->light->pos.get() - pos;
			float m = z.Magnitude();
			radius += i->light->radius + m;
		}

	} else {
		radius = head->light->radius;
	}
}

void WorldDraw::InvalidateInteractions(Light &light) {
	for (details::LightInteraction *i = light.m_interactionHead; i; i = i->nextOnLight) {
		i->dirty = true;
	}

	for (details::MatInteractionChain::const_iterator it = light.m_matInteractionChain.begin(); it != light.m_matInteractionChain.end(); ++it) {
		for (details::LightInteraction *i = it->second; i; i = i->nextOnLight) {
			i->dirty = true;
		}
	}
}

void WorldDraw::InvalidateInteractions(Entity &entity) {
	for (details::LightInteraction *i = entity.m_lightInteractions; i; i = i->nextOnBatch) {
		i->dirty = true;
	}

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
	for (details::LightInteraction *i = occupant.m_lightInteractions; i; i = i->nextOnBatch) {
		i->dirty = true;
	}

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
						if ((batch->maxLights > 0) && !FindInteraction(light, *batch)) {
							CreateInteraction(light, entity, *batch);
						}
					}
				}
			}
		}
	}
}

void WorldDraw::UnlinkEntity(Entity &entity) {
	while (entity.m_lightInteractions) {
		details::LightInteraction *i = entity.m_lightInteractions;
		UnlinkInteraction(*i, i->light->m_interactionHead, entity.m_lightInteractions);
		m_interactionPool.ReturnChunk(i);
	}

	for (DrawModel::Map::const_iterator it = entity.models->begin(); it != entity.models->end(); ++it) {
		const DrawModel::Ref &model = it->second;
		for (MBatchDraw::RefVec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
			const MBatchDraw::Ref &batch = *it;
			while (batch->m_interactions) {
				details::LightInteraction *i = batch->m_interactions;
				details::LightInteraction **lightHead = batch->m_interactions->light->ChainHead(batch->matId);
				UnlinkInteraction(*i, *lightHead, batch->m_interactions);
				m_interactionPool.ReturnChunk(i);
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

			if (occupant.lightingFlags.get()&kLightingFlag_CastShadows) {
				if (light.style.get()&Light::kStyle_CastShadows) {
					if (!FindInteraction(light, occupant)) {
						CreateInteraction(light, occupant);
					}
				}
			}

			for (MBatchDraw::RefVec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
				const MBatchDraw::Ref &batch = *it;
				if (lightBounds.Touches(batch->bounds)) {
					if ((batch->maxLights > 0) && !FindInteraction(light, *batch)) {
						CreateInteraction(light, occupant, *batch);
					}
				}
			}
		}
	}
}

void WorldDraw::UnlinkOccupant(MBatchOccupant &occupant) {

	while (occupant.m_lightInteractions) {
		details::LightInteraction *i = occupant.m_lightInteractions;
		UnlinkInteraction(*i, i->light->m_interactionHead, occupant.m_lightInteractions);
		m_interactionPool.ReturnChunk(i);
	}

	for (MBatchDraw::RefVec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
		const MBatchDraw::Ref &batch = *it;
		while (batch->m_interactions) {
			details::LightInteraction *i = batch->m_interactions;
			details::LightInteraction **lightHead = batch->m_interactions->light->ChainHead(batch->matId);
			UnlinkInteraction(*i, *lightHead, batch->m_interactions);
			m_interactionPool.ReturnChunk(i);
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

			if ((m->maxLights > 0) && bounds.Touches(m->bounds)) {
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

		if (light.style.get()&Light::kStyle_CastShadows) {
			if (entity.lightingFlags.get()&kLightingFlag_CastShadows) {
				if (!FindInteraction(light, entity)) {
					CreateInteraction(light, entity);
				}
			}
		}
		
		for (DrawModel::Map::const_iterator it = entity.models->begin(); it != entity.models->end(); ++it) {
			const DrawModel::Ref &model = it->second;
			for (MBatchDraw::RefVec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
				const MBatchDraw::Ref &batch = *it;
				if ((batch->maxLights > 0) && !FindInteraction(light, *batch)) {
					CreateInteraction(light, entity, *batch);
				}
			}
		}
	}

	for (MBatchOccupantPtrSet::const_iterator it = leaf.occupants.begin(); it != leaf.occupants.end(); ++it) {
		MBatchOccupant &occupant = **it;

		if (!(light.interactionFlags & occupant.lightInteractionFlags))
			continue;

		if (!bounds.Touches(occupant.bounds))
			continue;

		if (light.style.get()&Light::kStyle_CastShadows) {
			if (occupant.lightingFlags.get()&kLightingFlag_CastShadows) {
				if (!FindInteraction(light, occupant)) {
					CreateInteraction(light, occupant);
				}
			}
		}

		for (MBatchDraw::RefVec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
			const MBatchDraw::Ref &batch = *it;
			if ((batch->maxLights > 0) && !FindInteraction(light, *batch)) {
				CreateInteraction(light, occupant, *batch);
			}
		}
	}
}

void WorldDraw::UnlinkLight(Light &light) {

	for (details::MatInteractionChain::iterator it = light.m_matInteractionChain.begin(); it != light.m_matInteractionChain.end(); ++it) {
		details::LightInteraction **lightHead = &it->second;

		while (*lightHead) {
			details::LightInteraction *i = *lightHead;
			UnlinkInteraction(*i, *lightHead, i->draw->m_interactions);
			m_interactionPool.ReturnChunk(i);
		}
	}

	while (light.m_interactionHead) {
		details::LightInteraction *i = light.m_interactionHead;
		RAD_ASSERT(i->entity||i->occupant);
		RAD_ASSERT(i->draw == 0);
		UnlinkInteraction(*i, light.m_interactionHead, i->entity ? i->entity->m_lightInteractions : i->occupant->m_lightInteractions);
		m_interactionPool.ReturnChunk(i);
	}
}

details::LightInteraction *WorldDraw::FindInteraction(
	Light &light,
	const MBatchDraw &batch
) {
	for (details::LightInteraction *i = batch.m_interactions; i; i = i->nextOnBatch) {
		if (i->light == &light)
			return i;
	}

	return 0;
}

details::LightInteraction *WorldDraw::FindInteraction(
	Light &light,
	const Entity &entity
) {
	for (details::LightInteraction *i = entity.m_lightInteractions; i; i = i->nextOnBatch) {
		if (i->light == &light)
			return i;
	}

	return 0;
}

details::LightInteraction *WorldDraw::FindInteraction(
	Light &light,
	const MBatchOccupant &occupant
) {
	for (details::LightInteraction *i = occupant.m_lightInteractions; i; i = i->nextOnBatch) {
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

details::LightInteraction *WorldDraw::CreateInteraction(
	Light &light,
	Entity &entity
) {
	details::LightInteraction *i = reinterpret_cast<details::LightInteraction*>(m_interactionPool.SafeGetChunk());
	i->entity = &entity;
	i->light = &light;
	i->occupant = 0;
	i->dirty = true;
	i->draw = 0;

	LinkInteraction(*i, light.m_interactionHead, entity.m_lightInteractions);

	return i;
}

details::LightInteraction *WorldDraw::CreateInteraction(
	Light &light,
	MBatchOccupant &occupant
) {
	details::LightInteraction *i = reinterpret_cast<details::LightInteraction*>(m_interactionPool.SafeGetChunk());
	i->light = &light;
	i->occupant = &occupant;
	i->entity = 0;
	i->dirty = true;
	i->draw = 0;

	LinkInteraction(*i, light.m_interactionHead, occupant.m_lightInteractions);
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

	for (details::MatInteractionChain::iterator it = light.m_matInteractionChain.begin(); it != light.m_matInteractionChain.end(); ++it) {
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

	details::LightInteraction *next = 0;
	for (details::LightInteraction *i = light.m_interactionHead; i; i = next) {
		next = i->nextOnLight;
		RAD_ASSERT(i->draw == 0);

		if (i->dirty) {
			i->dirty = false;
			if (i->entity) {
				BBox entBounds(i->entity->ps->bbox);
				entBounds.Translate(i->entity->ps->worldPos);
				if (!bounds.Touches(entBounds)) {
					UnlinkInteraction(*i, light.m_interactionHead, i->entity->m_lightInteractions);
					m_interactionPool.ReturnChunk(i);
				}
			} else {
				RAD_ASSERT(i->occupant);
				if (!bounds.Touches(i->occupant->bounds)) {
					UnlinkInteraction(*i, light.m_interactionHead, i->occupant->m_lightInteractions);
					m_interactionPool.ReturnChunk(i);
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
