// WaveAnim.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <Runtime/Math.h>
#include "WaveAnim.h"
#include <cstdlib>

WaveAnim::WaveAnim(
	Type type, 
	float base, 
	float phase, 
	float amplitude, 
	float freq
) : 
m_type(type), 
m_base(base), 
m_phase(phase), 
m_amplitude(amplitude), 
m_freq(freq)
{
}

WaveAnim::WaveAnim(const WaveAnim &wave)
{
	*this = wave;
}

WaveAnim &WaveAnim::operator = (const WaveAnim &wave)
{
	m_type = wave.m_type;
	m_base = wave.m_base;
	m_phase = wave.m_phase;
	m_amplitude = wave.m_amplitude;
	m_freq = wave.m_freq;
	return *this;
}

bool WaveAnim::operator == (const WaveAnim &anim) const
{
	return m_type == anim.m_type &&
		m_base == anim.m_base && 
		m_phase == anim.m_phase &&
		m_amplitude == anim.m_amplitude &&
		m_freq == anim.m_freq;
}

bool WaveAnim::operator != (const WaveAnim &anim) const
{
	return !(*this == anim);
}

float WaveAnim::Sample(float time) const
{
	if (m_type == T_Identity)
		return 1.0f;
	if (m_type == T_Constant)
		return m_base;
	if (m_freq <= 0.0f)
		return 1.0f;

	float freq = 1.0f/m_freq;

//	time *= m_freq;
	time += m_phase;

	switch (m_type)
	{
	case T_Square:// _|-|_|-|_
		{
			UReg z = (UReg)(time/freq);
			float a = (z&1) ? -m_amplitude : m_amplitude;
			return m_base + a;
		}
	case T_Sawtooth:// /|/|/|/|/|
		{
			float z = math::Mod(time, freq);
			z /= freq; // 0->1
			z = (z-0.5f)*2.0f; // -1->1
			return m_base + z*m_amplitude;
		}
	case T_Triangle:// /\/\/\/
		{
			float z = math::Mod(time, freq);
			z /= freq; // 0->1
			z *= 2.0f; // 0 -> 2
			if (z > 1.0f) // bend the line to make a wave
				z = 2.0f-z;
			z = (z-0.5f)*2.0f; // -1 -> 1
			return m_base + z*m_amplitude;
		}
	}

// T_Noise
	float z = (((float)rand()) / ((float)RAND_MAX) * 2.0f) - 1.0f;
	return m_base + z*m_amplitude;
}
