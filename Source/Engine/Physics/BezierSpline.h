/*! \file BezierSpline.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup physics
*/

#pragma once
#include "../Types.h"

namespace physics {

///////////////////////////////////////////////////////////////////////////////

template <int tNumPts> class CachedCubicBZSpline;

class RADENG_CLASS CubicBZSpline {
public:

	CubicBZSpline();
	CubicBZSpline(const CubicBZSpline &s);

	CubicBZSpline(const Vec3 &a, const Vec3 &ctrlA, const Vec3 &ctrlB, const Vec3 &b);
	CubicBZSpline(const Vec3 *curve); // curve[4] (a, handle, handle, b)

	void Init(const Vec3 &a, const Vec3 &ctrlA, const Vec3 &ctrlB, const Vec3 &b);
	void Init(const Vec3 *curve); // curve[4] (a, handle, handle, b)

	void Eval(float t, Vec3 &pos, Vec3 *tangent = 0) const;

	CubicBZSpline &operator = (const CubicBZSpline &s);

private:

	// a = a pos
	// b = a handle
	// c = b handle
	// d = b pos
	void MakeCfs(const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3 &d);

	boost::array<Vec3, 4> m_cfs;
};

///////////////////////////////////////////////////////////////////////////////

template <int tNumPts>
class RADENG_CLASS CachedCubicBZSpline {
public:
	enum {
		kNumPts = tNumPts
	};

	struct Point {
		Vec3 pos;
		Vec3 tangent;
		float length;
		float offset;
	};

	CachedCubicBZSpline();
	CachedCubicBZSpline(const CachedCubicBZSpline &s);
	
	void Load(const CubicBZSpline &s, bool reverse = false);
	void Eval(float t, Vec3 &pos, Vec3 *tangent = 0) const;

	CachedCubicBZSpline &operator = (const CachedCubicBZSpline &s);
	
	RAD_DECLARE_READONLY_PROPERTY(CachedCubicBZSpline, length, float);
	RAD_DECLARE_READONLY_PROPERTY(CachedCubicBZSpline, points, const Point*);

	// Smooth velocity functions:
	// The motion through a bezier spline with respect to T is
	// not constant. This function allows smooth motion through
	// a bezier spline with respect to velocity.

	class SmoothMotion {
	public:
		
		// Sets T to zero.
		void Init();

		//! Returns distance moved, if less than requested then end of spline was hit.
		//! Optional arguments returns tangent vector, and the T of the new position.
		float Eval(const CachedCubicBZSpline &spline, float distance, Vec3 &pos, Vec3 *tangent = 0, float *t = 0);

	private:

		float m_d;
		int m_idx;
	};

private:

	friend class SmoothMotion;

	RAD_DECLARE_GET(length, float) {
		return m_length;
	}

	RAD_DECLARE_GET(points, const Point*) {
		return &m_pts[0];
	}

	boost::array<Point, kNumPts> m_pts;
	float m_length;
};

} // physics

#include "BezierSpline_inl.h"
