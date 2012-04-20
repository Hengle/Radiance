// Material.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace r {

inline void Material::SetColor(int num, int index, float *c)
{
	RAD_ASSERT(num >= Color0 && num < NumColors);
	RAD_ASSERT(index >= ColorA && index < NumColorIndices);
	m_colors[num][index][0] = c[0];
	m_colors[num][index][1] = c[1];
	m_colors[num][index][2] = c[2];
	m_colors[num][index][3] = c[3];
}

inline void Material::Color(int num, int index, float *out) const
{
	RAD_ASSERT(num >= Color0 && num < NumColors);
	RAD_ASSERT(index >= ColorA && index < NumColorIndices);
	out[0] = m_colors[num][index][0];
	out[1] = m_colors[num][index][1];
	out[2] = m_colors[num][index][2];
	out[3] = m_colors[num][index][3];
}

inline void Material::SampleColor(int num, float *out) const
{
	RAD_ASSERT(num >= Color0 && num < NumColors);
	if (m_animated)
	{	
		out[0] = m_sampledColor[num][0];
		out[1] = m_sampledColor[num][1];
		out[2] = m_sampledColor[num][2];
		out[3] = m_sampledColor[num][3];
	}
	else
	{
		out[0] = m_colors[num][ColorA][0];
		out[1] = m_colors[num][ColorA][1];
		out[2] = m_colors[num][ColorA][2];
		out[3] = m_colors[num][ColorA][3];
	}
}

inline const WaveAnim &Material::ColorWave(int num) const
{
	RAD_ASSERT(num >= Color0 && num < NumColors);
	return m_colorWaves[num];
}

inline WaveAnim &Material::ColorWave(int num)
{
	RAD_ASSERT(num >= Color0 && num < NumColors);
	return m_colorWaves[num];
}

inline int Material::TcGen(MTSource source, int index) const
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices);
	return m_tcGen[source][index];
}

inline void Material::SetTcGen(MTSource source, int index, int gen)
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices && gen < NumTcGens);
	m_tcGen[source][index] = gen;
}

inline WaveAnim &Material::Wave(MTSource source, int index, int tcMod, TexCoord tc)
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices && tcMod < NumTcMods && tc < NumCoords);
	return m_waves[source][index][tcMod][tc];
}

inline const WaveAnim &Material::Wave(MTSource source, int index, int tcMod, TexCoord tc) const
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices && tcMod < NumTcMods && tc < NumCoords);
	return m_waves[source][index][tcMod][tc];
}

inline void Material::Sample(MTSource source, int index, int tcMod, int &ops, float *out) const
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices && tcMod < NumTcMods);
	const float (*src)[NumCoords][3] = &m_waveSamples[source][index][tcMod];
	out[0] = (*src)[0][0];
	out[1] = (*src)[1][0];
	out[2] = (*src)[0][1];
	out[3] = (*src)[1][1];
	out[4] = (*src)[0][2];
	out[5] = (*src)[1][2];
	ops = m_waveOps[source][index][tcMod];
}

inline void Material::SetTextureId(MTSource source, int index, int id)
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices);
	m_ids[source][index] = id;
}

inline int Material::TextureId(MTSource source, int index) const
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices);
	return m_ids[source][index];
}

inline void Material::SetTextureFPS(MTSource source, int index, float fps)
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices);
	m_textureFPS[source][index] = fps;
}

inline float Material::TextureFPS(MTSource source, int index) const
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices);
	return m_textureFPS[source][index];
}

inline void Material::SetClampTextureFrames(MTSource source, int index, bool clamp)
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices);
	m_textureClamp[source][index] = clamp;
}

inline bool Material::ClampTextureFrames(MTSource source, int index) const
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices);
	return m_textureClamp[source][index];
}

inline void Material::ReleaseShader() 
{ 
	m_shader.reset(); 
}

} // r
