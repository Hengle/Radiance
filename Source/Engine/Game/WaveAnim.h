// WaveAnim.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"

class RADENG_CLASS WaveAnim
{
public:
	typedef boost::shared_ptr<WaveAnim> Ref;

	enum Type
	{
		T_Identity,
		T_Constant,
		T_Square,
		T_Sawtooth,
		T_Triangle,
		T_Noise
	};

	WaveAnim(
		Type type = T_Identity, 
		float base=0.0f, 
		float phase=1.0f, 
		float amplitude=0.0f, 
		float freq=0.0f // in cycles per second
	);

	WaveAnim(const WaveAnim &wave);

	RAD_DECLARE_PROPERTY(WaveAnim, type, Type, Type);
	RAD_DECLARE_PROPERTY(WaveAnim, base, float, float);
	RAD_DECLARE_PROPERTY(WaveAnim, phase, float, float);
	RAD_DECLARE_PROPERTY(WaveAnim, amplitude, float, float);
	RAD_DECLARE_PROPERTY(WaveAnim, freq, float, float);

	WaveAnim &operator = (const WaveAnim &wave);
	bool operator == (const WaveAnim &anim) const;
	bool operator != (const WaveAnim &anim) const;

	float Sample(float time) const;

private:

	RAD_DECLARE_GET(type, Type) { return m_type; }
	RAD_DECLARE_SET(type, Type) { m_type = value; }
	RAD_DECLARE_GET(base, float) { return m_base; }
	RAD_DECLARE_SET(base, float) { m_base = value; }
	RAD_DECLARE_GET(phase, float) { return m_phase; }
	RAD_DECLARE_SET(phase, float) { m_phase = value; }
	RAD_DECLARE_GET(amplitude, float) { return m_amplitude; }
	RAD_DECLARE_SET(amplitude, float) { m_amplitude = value; }
	RAD_DECLARE_GET(freq, float) { return m_freq; }
	RAD_DECLARE_SET(freq, float) { m_freq = value; }

	Type m_type;
	float m_base;
	float m_phase;
	float m_amplitude;
	float m_freq;
};
