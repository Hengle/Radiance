// MathUtils.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "MathUtils.h"

RADENG_API Quat RADENG_CALL QuatFromAngles(const Vec3 &angles) {
	return Quat(Vec3(0, 0, 1), math::DegToRad(angles[2])) *
		Quat(Vec3(0, 1, 0), math::DegToRad(angles[1])) *
		Quat(Vec3(1, 0, 0), math::DegToRad(angles[0]));
}

RADENG_API Vec3 RADENG_CALL AnglesFromQuat(const Quat &quat) {
	Mat4 m = Mat4::Rotation(quat);
	Vec3 v = Vec3(1, 0, 0) * m;
	return LookAngles(v);
}

RADENG_API Vec3 RADENG_CALL LookAngles(const Vec3 &fwd) {
	Vec3 v;
	v[0] = 0.f;
	v[1] = math::RadToDeg(math::ArcSin(-fwd[2])); // this is works but won't let us pitch over (upside down)
	v[2] = math::RadToDeg(math::ArcTan2(fwd[1], fwd[0]));

	for (int i = 0 ; i < 3; ++i) {
		if (v[i] < 0)
			v[i] += 360.f;
		if (v[i] > 360)
			v[i] -= 360.f;
	}

	return v;
}

RADENG_API Vec3 RADENG_CALL RotateVector(const Vec3 &v, const Vec3 &angles) {
	return Mat4::Rotation(QuatFromAngles(angles)).Transform3X3(v);
}

RADENG_API Vec3 RADENG_CALL Unproject(const Mat4 &mvp, int viewport[4], const Vec3 &p) {
	Vec3 z(
		2*(p[0]-viewport[0])/viewport[2]-1,
		2*(viewport[3]-(p[1]-viewport[1]))/viewport[3]-1,
		p[2]
	);

	return mvp.Inverse().Transform(z);
}

RADENG_API bool RADENG_CALL Project(const Mat4 &mvp, int viewport[4], const Vec3 &p, Vec3 &out) {
	Vec4 x(p, 1.f);
	Vec4 z = mvp.Transform(x);
	
	if (z[3] != 0.f) {
		out[0] = z[0] / z[3];
		out[1] = z[1] / z[3];
		out[2] = z[2] / z[3];

		out[0] = (out[0]+1)*0.5f*viewport[2]+viewport[0];
		out[1] = viewport[3]-((out[1]+1)*0.5f*viewport[3]+viewport[1]);
	}
	
	return z[3] > 0.f;
}

RADENG_API float RADENG_CALL LerpSin(float t) {
	t *= math::Constants<float>::PI()*2.f;
	t -= math::Constants<float>::PI_OVER_2();
	return (math::Sin(t)+1.f)*0.5f;
}

RADENG_API float RADENG_CALL ArcLerpSin(float t) { 
	// do inverse of LerpSin
	t *= 2.f;
	t -= 1.f;
	t = math::ArcSin(t);
	t += math::Constants<float>::PI_OVER_2();
	t /= math::Constants<float>::PI()*2.f;
	return t;
}

RADENG_API float RADENG_CALL ClampedLerpSin(float t) {
	if (t <= 0.f)
		return 0.f;
	if (t >= 1.f)
		return 1.f;

	t *= math::Constants<float>::PI();
	t -= math::Constants<float>::PI_OVER_2();
	return (math::Sin(t)+1.f)*0.5f;
}

RADENG_API float RADENG_CALL ClampedArcLerpSin(float t) { 
	// do inverse of LerpSin
	if (t <= 0.f)
		return 0.f;
	if (t >= 1.f)
		return 1.f;
	t *= 2.f;
	t -= 1.f;
	t = math::ArcSin(t);
	t += math::Constants<float>::PI_OVER_2();
	t /= math::Constants<float>::PI();
	return t;
}

RADENG_API Vec3 RADENG_CALL WrapAngles(const Vec3 &angles) {
	Vec3 v;
	for (int i = 0; i < 3; ++i)
		v[i] = math::Mod(angles[i], 360.f);
	return v;
}

RADENG_API Vec3 RADENG_CALL DeltaAngles(const Vec3 &start, const Vec3 &end, int *flags)
{ // return shortest path.
	Vec3 v;
	Vec3 vs = WrapAngles(start);
	Vec3 es = WrapAngles(end);

	int f = flags ? *flags : 0;
	int of = 0;

	for (int i = 0; i < 3; ++i) {
		float s = vs[i];
		float e = es[i];

		int bit = i*4;

		if ((f&(1<<bit)) || (!f && s < 0.f)) {
			of |= 1<<bit;
			s += 360.f;
		}

		if ((f&(1<<(bit+1))) || (!f && e < 0.f)) {
			of |= 1<<(bit+1);
			e += 360.f;
		}

		if ((f&(1<<(bit+2))) || (!f && (e > s) && (e-s) > 180.f)) {
			of |= 1<<(bit+2);
			v[i] = s - (360 - e);
		} else if ((f&(1<<(bit+3))) || (!f && (s > e) && (s-e) > 180.f)) {
			of |= 1<<(bit+3);
			v[i] = e + (360 - s);
		} else {
			v[i] = e-s;
		}
	}

	of |= 0x10000000; // non-zero so we know we've recorded state

	if (flags && !*flags)
		*flags = of;

	return v;
}

RADENG_API Vec3 RADENG_CALL LerpAngles(const Vec3 &start, const Vec3 &end, float frac, int *flags) { 
	// lerp shortest path
	Vec3 v;
	Vec3 vs = WrapAngles(start);
	Vec3 es = WrapAngles(end);

	int f = flags ? *flags : 0;
	int of = 0;

	for (int i = 0; i < 3; ++i) {
		float s = vs[i];
		float e = es[i];

		int bit = i*4;

		if ((f&(1<<bit)) || (!f && s < 0.f)) {
			of |= 1<<bit;
			s += 360.f;
		}

		if ((f&(1<<(bit+1))) || (!f && e < 0.f)) {
			of |= 1<<(bit+1);
			e += 360.f;
		}

		if ((f&(1<<(bit+2))) || (!f && e > s && (e-s) > 180.f)) {
			of |= 1<<(bit+2);
			e -= 360.f;
		} else if ((f&(1<<(bit+3))) || (!f && s > e && (s-e) > 180.f)) {
			of |= 1<<(bit+3);
			s -= 360.f;
		}

		s = math::Lerp(s, e, frac);
		if (s < 0.f)
			s += 360.f;
		v[i] = s;
	}

	of |= 0x10000000; // non-zero so we know we've recorded state

	if (flags && !*flags)
		*flags = of;

	return v;
}

RADENG_API void RADENG_CALL FrameVecs(const Vec3 &fwd, Vec3 &up, Vec3 &left) {
	up = Vec3(0.f, 0.f, 1.f);

	if (math::Abs(fwd.Dot(up)) > 0.99999f)
		up = Vec3(0.f, 1.f, 0.f);
	
	left = up.Cross(fwd);
	up = fwd.Cross(left);
}
