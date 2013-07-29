/*! \file Particles.inl
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

namespace r {

inline SpriteBatch &ParticleEmitter::Batch(int idx) {
	return *m_batches[idx].get();
}

} // r
