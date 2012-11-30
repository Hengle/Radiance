/*! \file BezierSpline_inl.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup physics
*/

namespace physics {

///////////////////////////////////////////////////////////////////////////////

inline CubicBZSpline::CubicBZSpline() {
}

inline CubicBZSpline::CubicBZSpline(const CubicBZSpline &s) {
	operator = (s);
}

inline CubicBZSpline::CubicBZSpline(const Vec3 &a, const Vec3 &ctrlA, const Vec3 &ctrlB, const Vec3 &b) {
	Init(a, ctrlA, ctrlB, b);
}

inline CubicBZSpline::CubicBZSpline(const Vec3 *curve) { // curve[4]
	Init(curve);
}

inline void CubicBZSpline::Init(const Vec3 &a, const Vec3 &ctrlA, const Vec3 &ctrlB, const Vec3 &b) {
	MakeCfs(a, ctrlA, ctrlB, b);
}

inline void CubicBZSpline::Init(const Vec3 *curve) { // curve[4]
	Init(curve[0], curve[1], curve[2], curve[3]);
}

inline CubicBZSpline &CubicBZSpline::operator = (const CubicBZSpline &s) {
	for (int i = 0; i < 4; ++i)
		m_cfs[i] = s.m_cfs[i];
}

///////////////////////////////////////////////////////////////////////////////

template <int tNumPts>
inline CachedCubicBZSpline<tNumPts>::CachedCubicBZSpline() {
}

template <int tNumPts>
inline CachedCubicBZSpline<tNumPts>::CachedCubicBZSpline(const CachedCubicBZSpline &s) {
	operator = (s);
}
	
template <int tNumPts>
void CachedCubicBZSpline<tNumPts>::Load(const CubicBZSpline &s, bool reverse) {
	BOOST_STATIC_ASSERT(kNumPts > 1);
	const float kStep = 1.f / (kNumPts-1);

	m_length = 0.f;
	
	int i = 0;
	for (float t = 0; t <= 1.f; t += kStep) {

		float _t = (reverse) ? (1.f - t) : t;

		s.Eval(_t, m_pts[i].pos, &m_pts[i].tangent);
				
		if (i > 0) {
			m_pts[i-1].length = (m_pts[i].pos-m_pts[i-1].pos).Magnitude();
			m_length += m_pts[i-1].length;
		}

		m_pts[i].offset = m_length;

		++i;
	}
	RAD_ASSERT(i == kNumPts);
}

template <int tNumPts>
void CachedCubicBZSpline<tNumPts>::Eval(float t, Vec3 &pos, Vec3 *tangent) const {
	const float kStep = 1.f / (kNumPts-1);

	int a = (int)(t * (kNumPts - 1));
	RAD_ASSERT((a >= 0) && (a <= kNumPts));
	if (a < kNumPts-1) {
		float lerp = (t - (a*kStep)) / kStep;
		pos = math::Lerp(m_pts[a].pos, m_pts[a+1].pos, lerp);
		if (tangent)
			*tangent = math::Lerp(m_pts[a].tangent, m_pts[a+1].tangent, lerp);
	} else {
		pos = m_pts[kNumPts-1].pos;
		if (tangent)
			*tangent = m_pts[kNumPts-1].tangent;
	}
}

template <int tNumPts>
inline CachedCubicBZSpline<tNumPts> &CachedCubicBZSpline<tNumPts>::operator = (const CachedCubicBZSpline &s) {
	for (int i = 0; i < kNumPts; ++i)
		m_pts[i] = s.m_pts[i];
	return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <int tNumPts>
inline void CachedCubicBZSpline<tNumPts>::SmoothMotion::Init() {
	m_d = 0.f;
	m_idx = 0;
}

template <int tNumPts>
inline float CachedCubicBZSpline<tNumPts>::SmoothMotion::Eval(const CachedCubicBZSpline &spline, float distance, Vec3 &pos, Vec3 *tangent, float *t) {
	const float kStep = 1.f / (kNumPts-1);

	m_d += distance;
	float d = m_d;

	const Point *point = &spline.m_pts[m_idx];

	while (d >= (point->offset+point->length)) {
		d -= point->offset;
		++m_idx;
		if (m_idx == kNumPts) {
			pos = point->pos;
			if (tangent)
				*tangent = point->tangent;
			if (t)
				*t = 1.f;
			return (distance - d);
		}
		point = &spline.m_pts[m_idx];
	}

	if (m_idx < kNumPts-1) {
		d -= point->offset;
		float lerp = d / point->length;
		pos = math::Lerp(point->pos, spline.m_pts[m_idx+1].pos, lerp);
		if (tangent)
			*tangent = math::Lerp(point->tangent, spline.m_pts[m_idx+1].tangent, lerp);
		if (t)
			*t = (lerp*kStep) + (kStep*m_idx);
	} else {
		pos = spline.m_pts[kNumPts-1].pos;
		if (tangent)
			*tangent = spline.m_pts[kNumPts-1].tangent;
		if (t)
			*t = 1.f;
	}

	return distance;
}

} // physics
