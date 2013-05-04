/*! \file Material.inl
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

namespace r {

inline void Material::SetColor(int num, int index, const Vec4 &color) {
	m_colors[num][index] = color;
}

inline Vec4 Material::Color(int num, int index) const {
	return m_colors[num][index];
}

inline WaveAnim &Material::ColorWave(int num) {
	return m_colorWaves[num];
}

inline Vec4 Material::SampleColor(int num) const {
	if (m_animated)
		return m_sampledColor[num];
	return m_colors[num][kColorA];
}

inline void Material::SetSpecularColor(int index, const Vec3 &color) {
	m_specularColors[index] = color;
}

inline Vec3 Material::SpecularColor(int index) const {
	return m_specularColors[index];
}

inline Vec3 Material::SampleSpecularColor() const {
	if (m_animated)
		return m_sampledSpecularColor;
	return m_specularColors[kColorA];
}

inline int Material::TCUVIndex(int index) const {
	return m_tcUVIndex[index];
}

inline void Material::SetTCUVIndex(int index, int uvIndex) {
	m_tcUVIndex[index] = (U8)uvIndex;
}

inline int Material::TCGen(int index) const {
	return (int)m_tcGen[index];
}

inline void Material::SetTCGen(int index, int gen) {
	m_tcGen[index] = (U8)gen;
}

inline WaveAnim &Material::Wave(int index, int tcMod, TexCoord tc) {
	return m_waves[index][tcMod][tc];
}

inline const WaveAnim &Material::Wave(int index, int tcMod, TexCoord tc) const {
	return m_waves[index][tcMod][tc];
}

inline void Material::Sample(int index, int tcMod, int &ops, float *out) const {
	const boost::array<boost::array<float, 3>, kNumTexCoordDimensions> *src = &m_waveSamples[index][tcMod];
	out[0] = (*src)[0][0];
	out[1] = (*src)[1][0];
	out[2] = (*src)[0][1];
	out[3] = (*src)[1][1];
	out[4] = (*src)[0][2];
	out[5] = (*src)[1][2];
	ops = m_waveOps[index][tcMod];
}

inline void Material::SetTextureId(int index, int id) {
	m_ids[index] = id;
}

inline int Material::TextureId(int index) const {
	return m_ids[index];
}

inline void Material::SetTextureFPS(int index, float fps) {
	m_textureFPS[index] = fps;
}

inline float Material::TextureFPS(int index) const {
	return m_textureFPS[index];
}

inline void Material::SetClampTextureFrames(int index, bool clamp) {
	m_textureClamp[index] = clamp ? 1 : 0;
}

inline bool Material::ClampTextureFrames(int index) const {
	return m_textureClamp[index] ? true : false;
}

inline void Material::ReleaseShader() { 
	m_shader.reset(); 
}

} // r
