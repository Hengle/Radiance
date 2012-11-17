/*! \file BezierSpline.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup physics
*/

#include RADPCH
#include "BezierSpline.h"

namespace physics {

void CubicBZSpline::Eval(float t, Vec3 &pos, Vec3 *tangent) const {
	float t_sq = t*t;
	float t_cb = t_sq*t;

	pos = (m_cfs[0]*t_cb) + (m_cfs[1]*t_sq) + (m_cfs[2]*t) + m_cfs[3];
	if (tangent) { // 1st derrivative is tangent line
		*tangent = (3.f*m_cfs[0]*t_sq) + (2.f*m_cfs[1]*t) + m_cfs[2];
		tangent->Normalize();
	}
}

void CubicBZSpline::MakeCfs(const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3 &d) {
	m_cfs[0] = -a + (3.f*b) - (3.f*c) + d;
	m_cfs[1] = (3.f*a) - (6.f*b) + (3.f*c);
	m_cfs[2] = (-3.f*a) + (3.f*b);
	m_cfs[3] = a;
}

} // physics
