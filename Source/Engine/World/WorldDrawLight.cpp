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
		DrawUnshadowedLitBatchLights(view, *draw, batch);
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
		u.eyePos = invTx * view.camera.pos.get();
	} else {
		u.eyePos = view.camera.pos;
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
	const details::MBatch &batch
) {
	r::Material &mat = *batch.matRef->mat;
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
	
	if (tx) {
		u.eyePos = invTx * view.camera.pos.get();
	} else {
		u.eyePos = view.camera.pos;
	}

	const int kMaxLights = std::min(mat.maxLights.get(), m_world->cvars->r_maxLightsPerPass.value.get());
	
	const Vec4**scissorRects = (const Vec4**)stack_alloc(sizeof(Vec4*)*kMaxLights);

	// true if a light takes up the entire screen and we can't stencil anything
	// in the lighting pass
	bool fullscreenLight = false;

	// set when we've already done diffuseSpecular pass and are now interested
	// in rendering diffuse only lights
	bool didDiffuseSpecular = false;

	for (;;) {
		details::LightInteraction *interaction = draw.m_interactions;

		while (interaction) {

			// batch up to kNumLights
			u.lights.numLights = 0;
				
			while (interaction && (u.lights.numLights < kMaxLights)) {
				if ((interaction->light->m_visFrame != m_markFrame) ||
					(interaction->light->intensity < 0.01f)) {
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
#if defined(WORLD_DEBUG_DRAW)
						if (m_world->cvars->r_showlights.value) {
							m_dbgVars.lights.push_back(interaction->light->pos);
						}
#endif
					}
					r::LightDef &lightDef = u.lights.lights[u.lights.numLights++];
					GenLightDef(*interaction->light, lightDef, tx ? &invTx : 0);
					BBox bounds(interaction->light->bounds);
					bounds.Translate(interaction->light->pos);
					
					if (!fullscreenLight) {
						// no fullscreen lights in this pass, add stencil rects
						const Vec4 &stencilRect = interaction->light->m_scissor;
						if ((stencilRect[2] < 1.f) && (stencilRect[3] < 1.f)) {
							fullscreenLight = true;
						} else {
							scissorRects[u.lights.numLights-1] = &stencilRect;
						}
					}
				}

				interaction = interaction->nextOnBatch;
			}

			if (u.lights.numLights > 0) {

				if (!fullscreenLight)
					m_rb->RenderLightStencil(view, scissorRects, u.lights.numLights);

				// find the correct pass
				int pass;

				if (diffuseSpecular && !didDiffuseSpecular) {
					pass = r::Shader::kPass_DiffuseSpecular1 + u.lights.numLights - 1;
				} else {
					RAD_ASSERT(diffuse);
					pass = r::Shader::kPass_Diffuse1 + u.lights.numLights - 1;
				}

				RAD_ASSERT(mat.shader->HasPass((r::Shader::Pass)pass));

				m_rb->BindLitMaterialStates(mat, !fullscreenLight);
				mat.BindTextures(batch.matRef->loader);

				mat.shader->Begin((r::Shader::Pass)pass, mat);
				draw.Bind(mat.shader.get().get());
				
				mat.shader->BindStates(u);
				m_rb->CommitStates();
				draw.CompileArrayStates(*mat.shader.get());
				draw.Draw();
				mat.shader->End();

				++m_counters.numLightPasses;
				++m_counters.numBatches;
				++m_counters.numLightPassLights += u.lights.numLights;

				if (!fullscreenLight)
					++m_counters.numStencilLightPasses;
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
}

void WorldDraw::GenLightDef(
	const Light &light,
	r::LightDef &lightDef,
	Mat4 *tx
) {
	lightDef.diffuse = light.diffuseColor;
	lightDef.specular = light.specularColor;
	lightDef.pos = tx ? ((*tx) * light.pos.get()) : light.pos;
	lightDef.intensity = light.intensity;
	lightDef.radius = light.radius;
	
	lightDef.flags = 0;
	if (light.style.get() & Light::kStyle_Diffuse)
		lightDef.flags |= r::LightDef::kFlag_Diffuse;
	if (light.style.get() & Light::kStyle_Specular)
		lightDef.flags |= r::LightDef::kFlag_Specular;
}

bool WorldDraw::GenerateUnifiedEntityShadow(const ViewDef &view, const Entity &e, UnifiedShadow &projector) {
	
	BBox bounds(e.ps->bbox);
	bounds.Translate(e.ps->worldPos);

	CalcUnifiedLightPosAndSize(
		bounds,
		e.m_lightInteractions,
		projector.unifiedPos,
		projector.unifiedRadius
	);

#if defined(WORLD_DEBUG_DRAW)
	if (m_world->cvars->r_showunifiedlights.value) {
		m_dbgVars.unifiedLights.push_back(projector.unifiedPos);
	}
#endif

	Camera lightCam;
	lightCam.pos = projector.unifiedPos;
	lightCam.fov = 0.f;
	lightCam.farClip = 0.f;
	lightCam.LookAt(bounds.Origin());

	m_rb->RotateForCamera(lightCam);
	projector.mv = m_rb->GetModelViewMatrix();

	Vec3 radial(lightCam.pos.get() + (lightCam.fwd.get() * projector.unifiedRadius));

	CalcViewplaneBounds(0, projector.mv, bounds, &radial, projector.viewplanes, projector.zplanes);

	Mat4 prj = MakePerspectiveMatrix(projector.viewplanes, projector.zplanes, false);
	m_rb->SetPerspectiveMatrix(prj);
	
	if (!m_rb->BindUnifiedShadowRenderTarget(*m_shadow_M.material))
		return false;

	m_shadow_M.material->BindTextures(m_shadow_M.loader);
	m_shadow_M.material->shader->Begin(r::Shader::kPass_Default, *m_shadow_M.material);

	DrawUnifiedShadowTexture(
		view,
		*e.models,
		projector.unifiedPos,
		projector.unifiedRadius
	);

	m_shadow_M.material->shader->End();

	SetupPerspectiveFrustumPlanes(
		projector.frustum,
		lightCam,
		projector.viewplanes,
		projector.zplanes
	);

	projector.occluder = &e;

	return true;
}

bool WorldDraw::GenerateUnifiedOccupantShadow(const ViewDef &view, const MBatchOccupant &o, UnifiedShadow &projector) {
	const BBox &bounds = o.bounds;

	CalcUnifiedLightPosAndSize(
		bounds,
		o.m_lightInteractions,
		projector.unifiedPos,
		projector.unifiedRadius
	);

#if defined(WORLD_DEBUG_DRAW)
	if (m_world->cvars->r_showunifiedlights.value) {
		m_dbgVars.unifiedLights.push_back(projector.unifiedPos);
	}
#endif

	Camera lightCam;
	lightCam.pos = projector.unifiedPos;
	lightCam.fov = 0.f;
	lightCam.farClip = 0.f;
	lightCam.LookAt(bounds.Origin());

	m_rb->RotateForCamera(lightCam);
	projector.mv = m_rb->GetModelViewMatrix();

	Vec3 radial(lightCam.pos.get() + (lightCam.fwd.get() * projector.unifiedRadius));

	CalcViewplaneBounds(0, projector.mv, bounds, &radial, projector.viewplanes, projector.zplanes);

	Mat4 prj = MakePerspectiveMatrix(projector.viewplanes, projector.zplanes, false);
	m_rb->SetPerspectiveMatrix(prj);
	
	if (!m_rb->BindUnifiedShadowRenderTarget(*m_shadow_M.material))
		return false;

	m_shadow_M.material->BindTextures(m_shadow_M.loader);
	m_shadow_M.material->shader->Begin(r::Shader::kPass_Default, *m_shadow_M.material);

	DrawUnifiedShadowTexture(
		view,
		*o.batches,
		projector.unifiedPos,
		projector.unifiedRadius
	);

	m_shadow_M.material->shader->End();

	SetupPerspectiveFrustumPlanes(
		projector.frustum,
		lightCam,
		projector.viewplanes,
		projector.zplanes
	);

	projector.occluder = &o;

	return true;
}

#if defined(PRERENDER_SHADOWS)
void WorldDraw::GenerateViewUnifiedShadows(const ViewDef &view) {
	m_unifiedShadows->clear();
#else
void WorldDraw::DrawViewUnifiedShadows(const ViewDef &view) {
#endif
	bool stop = false;

	m_rb->BeginUnifiedShadows();

	UnifiedShadow shadow;

	for (EntityPtrVec::const_iterator it = view.shadowEntities.begin(); it != view.shadowEntities.end(); ++it) {
		const Entity &e = **it;
		if (FindShadowMaterials(e.m_models)) {
			if (GenerateUnifiedEntityShadow(view, e, shadow)) {
#if defined(PRERENDER_SHADOWS)
				m_unifiedShadows->push_back(shadow);
#else
				DrawViewWithUnifiedShadow(view, shadow);
#endif
			} else {
				stop = true;
				break; // out of shadow render targets!
			}
		}
	}

	if (!stop) {
		for (MBatchOccupantPtrVec::const_iterator it = view.shadowOccupants.begin(); it != view.shadowOccupants.end(); ++it) {
			const MBatchOccupant &o = **it;
		
			if (FindShadowMaterials(*o.batches)) {
				if (GenerateUnifiedOccupantShadow(view, o, shadow)) {
#if defined(PRERENDER_SHADOWS)
					m_unifiedShadows->push_back(shadow);
#else
					DrawViewWithUnifiedShadow(view, shadow);
#endif
				} else {
					break; // out of shadow render targets!
				}
			}
		}
	}

	m_rb->EndUnifiedShadows();

#if defined(PRERENDER_SHADOWS)
	m_rb->SetPerspectiveMatrix(view.camera, view.viewport);
	m_rb->RotateForCamera(view.camera);
#endif
}

#if defined(PRERENDER_SHADOWS)
void WorldDraw::DrawViewUnifiedShadows(const ViewDef &view) {
	m_rb->BeginUnifiedShadows();

	for (UnifiedShadow::StackVec::const_iterator it = m_unifiedShadows->begin(); it != m_unifiedShadows->end(); ++it) {
		DrawViewWithUnifiedShadow(view, *it);
	}

	m_rb->EndUnifiedShadows();
}
#endif

void WorldDraw::DrawUnifiedShadowTexture(
	const ViewDef &view,
	const DrawModel::Map &models,
	const Vec3 &unifiedPos,
	float unifiedRadius
) {
	for (DrawModel::Map::const_iterator it = models.begin(); it != models.end(); ++it) {
		const DrawModel::Ref &m = it->second;
		if (!m->visible)
			continue;
		DrawUnifiedShadowTexture(view, m->m_batches, unifiedPos, unifiedRadius);
	}
}

void WorldDraw::DrawUnifiedShadowTexture(
	const ViewDef &view,
	const MBatchDraw::Vec &batches,
	const Vec3 &unifiedPos,
	float unifiedRadius
) {
	Vec3 pos;
	Vec3 angles;
	r::Shader::Uniforms u;

	u.lights.numLights = 0;
	u.blendColor = Vec4(0.f, 0.f, 0.f, 1.f);

	// material->BindStates() was issued by caller
	for (MBatchDraw::Vec::const_iterator it = batches.begin(); it != batches.end(); ++it) {
		const MBatchDraw::Ref &draw = *it;

		details::MatRef *mat = draw->m_matRef;
		if (mat) {
			if (mat->mat->castShadows) {
				bool tx = draw->GetTransform(pos, angles);
				if (tx) {
					m_rb->PushMatrix(pos, draw->scale, angles);
				}

				draw->Bind(m_shadow_M.material->shader.get().get());
				m_shadow_M.material->shader->BindStates(u);
				m_rb->CommitStates();
				draw->CompileArrayStates(*m_shadow_M.material->shader.get());
				draw->Draw();

				if (tx)
					m_rb->PopMatrix();
			}
		}
	}
}

bool WorldDraw::FindShadowMaterials(
	const DrawModel::Map &models
) {
	for (DrawModel::Map::const_iterator it = models.begin(); it != models.end(); ++it) {
		const DrawModel::Ref &m = it->second;
		if (!m->visible)
			continue;
		if (FindShadowMaterials(m->m_batches))
			return true;
	}
	return false;
}

bool WorldDraw::FindShadowMaterials(
	const MBatchDraw::Vec &batches
) {
	// material->BindStates() was issued by caller
	for (MBatchDraw::Vec::const_iterator it = batches.begin(); it != batches.end(); ++it) {
		const MBatchDraw::Ref &draw = *it;

		details::MatRef *mat = draw->m_matRef;
		if (mat) {
			if (mat->mat->castShadows) {
				return true;
			}
		}
	}

	return false;
}

void WorldDraw::DrawViewWithUnifiedShadow(
	const ViewDef &view,
	const UnifiedShadow &shadow
) {
	StackWindingStackVec frustumVolume;
	BBox frustumBounds;

	World::MakeVolume(&shadow.frustum[0], (int)shadow.frustum->size(), frustumVolume, frustumBounds);

#if !defined(PRERENDER_SHADOWS)
	m_rb->SetPerspectiveMatrix(view.camera, view.viewport);
	m_rb->RotateForCamera(view.camera);
#endif

	r::Shader::Uniforms u;
	u.blendColor = Vec4(1.f, 1.f, 1.f, 1.f);
	u.lights.numLights = 1;
	memset(&u.lights.lights[0], 0, sizeof(r::LightDef));
	u.lights.lights[0].pos = shadow.unifiedPos;
	u.lights.lights[0].radius = shadow.unifiedRadius;
	u.tcGen = shadow.mv * MakePerspectiveMatrix(shadow.viewplanes, shadow.zplanes, true);

	m_projected_M.material->BindTextures(m_projected_M.loader);
	bool r = m_rb->BindUnifiedShadowTexture(*m_projected_M.material); // does mat->BindStates()
	RAD_ASSERT_MSG(r, "BindUnifiedShadowTexture() failed!");
	
	m_projected_M.material->shader->Begin(r::Shader::kPass_Default, *m_projected_M.material);

	Vec3 pos, angles;

	for (details::MBatchIdMap::const_iterator it = view.batches.begin(); it != view.batches.end(); ++it) {
		const details::MBatch &batch = *it->second;
		if ((batch.matRef->mat->receiveShadows == false) ||
			(batch.matRef->mat->sort != r::Material::kSort_Solid)) {
			continue; // this material doesn't want shadows.
		}

		for (details::MBatchDrawLink *link = batch.head; link; link = link->next) {
			MBatchDraw *draw = link->draw;
			if (draw->m_uid == shadow.occluder)
				continue; // don't render occluder with their own shadow

			if (m_world->cvars->r_frustumcull.value && !ClipBounds(frustumVolume, frustumBounds, draw->TransformedBounds())) {
				continue; // not in light frustum
			}

			bool tx = draw->GetTransform(pos, angles);
			if (tx)
				m_rb->PushMatrix(pos, draw->scale, angles);

			draw->Bind(m_projected_M.material->shader.get().get());
			m_projected_M.material->shader->BindStates(u, false);
			m_rb->CommitStates();
			draw->CompileArrayStates(*m_projected_M.material->shader.get());
			draw->Draw();

			if (tx)
				m_rb->PopMatrix();
		}
	}

	m_projected_M.material->shader->End();
}

void WorldDraw::CalcUnifiedLightPosAndSize(
	const BBox &bounds,
	const details::LightInteraction *head,
	Vec3 &pos,
	float &radius
) {
	float totalDist = 0.f;
	float totalIntensity = 0.f;
	const Vec3 &origin = bounds.Origin();
	
	for (const details::LightInteraction *i = head; i; i = i->nextOnBatch) {
		totalIntensity += math::Abs(i->light->shadowWeight.get());
	}

	if (totalIntensity == 0.f)
		return;

	float totalWeight = 0.f;

	for (const details::LightInteraction *i = head; i; i = i->nextOnBatch) {

		if (i->light->radius == 0.f)
			continue;

		Vec3 z = i->light->pos.get() - origin;
		float dist = z.Magnitude();
		float w = 1.f - math::Min(1.f, dist / i->light->radius);
		w = w * w * (math::Abs(i->light->shadowWeight.get()) / totalIntensity);

		totalWeight += w;

	}

	// blend lights
	pos = Vec3::Zero;
	radius = 0.f;

	for (const details::LightInteraction *i = head; i; i = i->nextOnBatch) {

		if (i->light->radius == 0.f)
			continue;

		Vec3 z = i->light->pos.get() - origin;
		float dist = z.Magnitude();
		float w = 1.f - math::Min(1.f, dist / i->light->radius);
		w = w * w * (math::Abs(i->light->shadowWeight.get()) / totalIntensity);

		w = w / totalWeight;
		pos += i->light->pos.get() * w;
		radius += i->light->radius * w;
	}

	Vec3 boundSize = bounds.Size();
	float boundRadius = math::Max(math::Max(boundSize[0], boundSize[1]), boundSize[2]);
	Vec3 v = pos - origin;
	float dist = v.Normalize();

	if (dist < boundRadius) { // keep outside of bounding radius
		pos += v*(boundRadius-dist);
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
		for (MBatchDraw::Vec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
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

	for (MBatchDraw::Vec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
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

			if (entity.lightingFlags.get()&kLightingFlag_CastShadows) {
				if (light.style.get()&Light::kStyle_CastShadows) {
					if (!FindInteraction(light, entity)) {
						CreateInteraction(light, entity);
					}
				}
			}

			for (DrawModel::Map::const_iterator it = entity.models->begin(); it != entity.models->end(); ++it) {
				const DrawModel::Ref &model = it->second;
				for (MBatchDraw::Vec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
					const MBatchDraw::Ref &batch = *it;
					if (lightBounds.Touches(batch->TransformedBounds())) {
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
		for (MBatchDraw::Vec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
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

			for (MBatchDraw::Vec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
				const MBatchDraw::Ref &batch = *it;
				if (lightBounds.Touches(batch->TransformedBounds())) {
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

	for (MBatchDraw::Vec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
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

	if (!(light.interactionFlags & Light::kInteractionFlag_World))
		return; // not a world light

	// NOTE: models can span areas, this may check them multiple times.
	for (IntSet::const_iterator it = light.m_areas.begin(); it != light.m_areas.end(); ++it) {
		const dBSPArea &area = m_world->m_areas[*it];

		for (int i = 0; i < area.numModels; ++i) {
			U16 modelNum = *(m_world->m_bsp->ModelIndices() + i + area.firstModel);
			RAD_ASSERT(modelNum < (U16)m_worldModels.size());

			const MStaticWorldMeshBatch::Ref &m = m_worldModels[modelNum];

			if ((m->maxLights > 0) && bounds.Touches(m->TransformedBounds())) {
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
			for (MBatchDraw::Vec::const_iterator it = model->batches->begin(); it != model->batches->end(); ++it) {
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

		for (MBatchDraw::Vec::const_iterator it = occupant.batches->begin(); it != occupant.batches->end(); ++it) {
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

	for (details::MatInteractionChain::iterator it = light.m_matInteractionChain.begin(); it != light.m_matInteractionChain.end(); ++it) {
		details::LightInteraction **lightHead = &it->second;
		details::LightInteraction *next = 0;
		for (details::LightInteraction *i = *lightHead; i; i = next) {
			next = i->nextOnLight;

			if (i->dirty) {
				i->dirty = false;
				
				if (!bounds.Touches(i->draw->TransformedBounds())) {
					UnlinkInteraction(*i, *lightHead, i->draw->m_interactions);
					m_interactionPool.ReturnChunk(i);
				}
			}
		}
	}

	// shadow
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

void WorldDraw::TickLights(float dt) {
	for (Light *l = m_lights[0]; l; l = l->m_next) {
		l->Tick(dt);
	}
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
