/*! \file Sprites.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#include RADPCH
#include "Sprites.h"
#include <Runtime/Base/SIMD.h>

namespace r {

namespace {
struct SpriteVertex {
	float pos[3];
	float rgba[4];
	float skin[4];
	float padd0; // 16 bytes
};
BOOST_STATIC_ASSERT(sizeof(SpriteVertex) == (sizeof(float)*12));
BOOST_STATIC_ASSERT(sizeof(Sprite) >= sizeof(SpriteVertex));

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

SpriteBatch::SpriteBatch(
	int spriteSize,
	int minSprites, 
	int maxSprites
) {
	RAD_DEBUG_ONLY(m_init = false);
	Init(spriteSize, minSprites, maxSprites);
}

void SpriteBatch::Init(int spriteSize, int minSprites, int maxSprites) {
	RAD_ASSERT(!m_init);
	RAD_ASSERT(spriteSize >= sizeof(Sprite));
	RAD_DEBUG_ONLY(m_init = true);

	m_head = 0;
	m_tail = 0;
	m_numSprites = 0;
	m_meshSprites = 0;
	m_vertStream = 0;
	m_minSprites = minSprites;

	int spritesInChunk;
	int maxChunks;

	if (maxSprites < 1) {
		// if they have no upper limit, chunkify sprites.
		spritesInChunk = 256;
		maxChunks = kMaxSprites / spritesInChunk;
		m_maxSprites = kMaxSprites;
	} else {
		// can't do more than kMaxSprites
		spritesInChunk = std::min(maxSprites, (int)kMaxSprites);
		maxChunks = 1;
		m_maxSprites = spritesInChunk;
	}

	m_p.Create(
		ZRender,
		"spritebatch",
		spriteSize,
		spritesInChunk,
		SIMDDriver::kAlignment,
		maxChunks
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

	m_m.SwapChain();

	Mesh::StreamPtr::Ref vb = m_m.Map(m_vertStream);
	SpriteVertex *v = (SpriteVertex*)vb->ptr.get();

	for (Sprite *sprite = m_head; sprite; sprite = sprite->next) {
		SIMD->MemRep16(v, sprite, sizeof(SpriteVertex), 4);
		v[0].skin[3] = 0.f;
		v[1].skin[3] = 1.f;
		v[2].skin[3] = 2.f;
		v[3].skin[3] = 3.f;	
		v += 4;
	}

	vb.reset();
}

void SpriteBatch::Draw() {
	m_m.Draw(0, m_numSprites * 2);
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
		0,
		3
	);

	m_m.MapSource(
		m_vertStream,
		kMaterialGeometrySource_VertexColor,
		0,
		sizeof(SpriteVertex),
		sizeof(float)*3,
		4
	);

	m_m.MapSource(
		m_vertStream,
		kMaterialGeometrySource_SpriteSkin,
		0,
		sizeof(SpriteVertex),
		sizeof(float)*7,
		4
	);

	int uvStream = m_m.AllocateStream(kStreamUsage_Static, sizeof(float)*2, m_meshSprites*4);
	m_m.MapSource(
		uvStream,
		kMaterialGeometrySource_TexCoords,
		0,
		0,
		0,
		2
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
