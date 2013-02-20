/*! \file Sprites.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#include RADPCH
#include "Sprites.h"

namespace r {

namespace {
struct SpriteVertex {
	float pos[3];
	float rgba[4];
	float skin[4];
};
BOOST_STATIC_ASSERT(sizeof(SpriteVertex) == (sizeof(float)*11));
enum {
	kSkinFrames = 2
};
}

SpriteBatch::SpriteBatch() : 
m_head(0), 
m_tail(0), 
m_numSprites(0), 
m_minSprites(0), 
m_maxSprites(0),
m_meshSprites(0),
m_vertStream(0) {
	RAD_DEBUG_ONLY(m_init = false);
}

SpriteBatch::SpriteBatch(int minSprites, int maxSprites) {
	RAD_DEBUG_ONLY(m_init = false);
	Init(minSprites, maxSprites);
}

void SpriteBatch::Init(int minSprites, int maxSprites) {
	RAD_ASSERT(!m_init);
	RAD_DEBUG_ONLY(m_init = true);

	m_head = 0;
	m_tail = 0;
	m_numSprites = 0;
	m_meshSprites = 0;
	m_vertStream = 0;
	m_minSprites = minSprites;
	// cannot generate indices for more than this:
	m_maxSprites = std::min(maxSprites, (int)kMaxSprites);
	if (m_maxSprites < 1)
		m_maxSprites = kMaxSprites;

	UReg maxSize = std::numeric_limits<UReg>::max();
	if (maxSprites > 0)
		maxSize = maxSprites * sizeof(Sprite);
	
	m_p.Create(
		ZRender,
		"spritebatch",
		sizeof(Sprite),
		64,
		DefaultAlignment,
		maxSize
	);
}

Sprite *SpriteBatch::AllocateSprite() {
	RAD_ASSERT(m_init);
	Sprite *sprite = (Sprite*)m_p.GetChunk();
	if (sprite) {
		sprite->prev = m_tail;
		sprite->next = 0;

		if (m_tail)
			m_tail->next = sprite;
		m_tail = sprite;
		if (!m_head)
			m_head = sprite;

		++m_numSprites;
	}
	return sprite;
}

void SpriteBatch::FreeSprite(Sprite *sprite) {
	RAD_ASSERT(m_init);
	RAD_ASSERT(sprite);
	if (sprite->prev)
		sprite->prev->next = sprite->next;
	if (sprite->next)
		sprite->next->prev = sprite->prev;
	if (m_head == sprite)
		m_head = sprite->next;
	if (m_tail == sprite)
		m_tail = sprite->prev;
	m_p.ReturnChunk(sprite);
	--m_numSprites;
}

void SpriteBatch::Compact() {
	RAD_ASSERT(m_init);
	m_p.Compact();
}

void SpriteBatch::Skin() {
	RAD_ASSERT(m_init);

	AllocateMesh();

	Mesh::StreamPtr::Ref vb = m_m.Map(m_vertStream);
	SpriteVertex *v = (SpriteVertex*)vb->ptr.get();

	SpriteVertex z;

	for (Sprite *sprite = m_head; sprite; sprite = sprite->next) {
		
		z.pos[0] = sprite->pos[0];
		z.pos[1] = sprite->pos[1];
		z.pos[2] = sprite->pos[2];
		z.rgba[0] = sprite->rgba[0];
		z.rgba[1] = sprite->rgba[1];
		z.rgba[2] = sprite->rgba[2];
		z.rgba[3] = sprite->rgba[3];
		z.skin[0] = sprite->size[0];
		z.skin[1] = sprite->size[1];
		z.skin[2] = sprite->rot;

		float idx = 0.f;
		for (int i = 0; i < 4; ++i, idx += 1.f) {
			z.skin[3] = idx;
			*v = z;
			++v;
		}
		
	}

	vb.reset();
}

void SpriteBatch::Draw() {
	m_m.Draw(m_numSprites * 2);
}

void SpriteBatch::AllocateMesh() {
	RAD_ASSERT(m_init);
	if (m_meshSprites >= m_numSprites)
		return;
	m_meshSprites = std::max(m_minSprites, m_numSprites);

	m_m.Release();
	
	if (kSkinFrames > 1)
		m_m.AllocateSwapChains(kSkinFrames);

	m_vertStream = m_m.AllocateStream(kStreamUsage_Stream, sizeof(SpriteVertex), m_meshSprites*4, true);
	
	m_m.MapSource(
		m_vertStream,
		kMaterialGeometrySource_Vertices,
		0,
		sizeof(SpriteVertex),
		0
	);

	m_m.MapSource(
		m_vertStream,
		kMaterialGeometrySource_VertexColor,
		0,
		sizeof(SpriteVertex),
		sizeof(float)*3
	);

	m_m.MapSource(
		m_vertStream,
		kMaterialGeometrySource_SpriteSkin,
		0,
		sizeof(SpriteVertex),
		sizeof(float)*7
	);

	int uvStream = m_m.AllocateStream(kStreamUsage_Static, sizeof(float)*2, m_meshSprites*4);
	m_m.MapSource(
		uvStream,
		kMaterialGeometrySource_TexCoords,
		0,
		0,
		0
	);

	// generate UV's
	Mesh::StreamPtr::Ref vb = m_m.Map(uvStream);
	float *uvs = (float*)vb->ptr.get();
	for (int i = 0; i < m_meshSprites; ++i) {
		uvs[0] = 0.f;
		uvs[1] = 0.f;
		uvs[2] = 0.f;
		uvs[3] = 1.f;
		uvs[4] = 1.f;
		uvs[5] = 1.f;
		uvs[6] = 1.f;
		uvs[7] = 0.f;
		uvs += 8;
	}
	
	vb.reset();

	// generate indices
	vb = m_m.MapIndices(kStreamUsage_Static, sizeof(U16), m_meshSprites*2*3);
	U16 *indices = (U16*)vb->ptr.get();
	for (int i = 0; i < m_meshSprites; ++i) {
		const U16 base = (U16)(i * 4);
		indices[0] = base;
		indices[1] = base+1;
		indices[2] = base+2;
		indices[3] = base;
		indices[4] = base+2;
		indices[5] = base+3;
		indices += 6;
	}
	vb.reset();
}

}
