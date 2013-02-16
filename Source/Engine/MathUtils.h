// MathUtils.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"

RADENG_API Quat RADENG_CALL QuatFromAngles(const Vec3 &angles);
RADENG_API Vec3 RADENG_CALL AnglesFromQuat(const Quat &quat);
RADENG_API Vec3 RADENG_CALL LookAngles(const Vec3 &fwd);
RADENG_API Vec3 RADENG_CALL ForwardFromAngles(const Vec3 &angles);
RADENG_API Vec3 RADENG_CALL RotateVector(const Vec3 &v, const Vec3 &angles);
RADENG_API bool RADENG_CALL Project(const Mat4 &mvp, int viewport[4], const Vec3 &p, Vec3 &out);
RADENG_API Vec3 RADENG_CALL Unproject(const Mat4 &mvp, int viewport[4], const Vec3 &p);

// Both LerpAngles and DeltaAngles take the shortest path.
// flags is an optional argument that can be used to
// record the result of the Lerp() directions to let you
// modify the start and end positions over time but retain
// the lerping directions.

RADENG_API Vec3 RADENG_CALL LerpAngles(
	const Vec3 &start, 
	const Vec3 &end, 
	float frac, 
	int *flags=0
);

RADENG_API Vec3 RADENG_CALL DeltaAngles(
	const Vec3 &start, 
	const Vec3 &end,
	int *flags=0
);

RADENG_API Vec3 RADENG_CALL AbsAngles(const Vec3 &angles);
RADENG_API Vec3 RADENG_CALL WrapAngles(const Vec3 &angles);
RADENG_API void RADENG_CALL FrameVecs(const Vec3 &fwd, Vec3 &up, Vec3 &left);

// LerpSin will apply a sin over domain of [0, N] output range [0, 1]
// The function does a complete cycle every 1.0f cycle of t.
RADENG_API float RADENG_CALL LerpSin(float t);
RADENG_API float RADENG_CALL ArcLerpSin(float t);

// ClampedLerpSin will apply a sin over domain [0, 1] output range [0, 1]
// useful for blending using a sin instead of linear time
RADENG_API float RADENG_CALL ClampedLerpSin(float t);
RADENG_API float RADENG_CALL ClampedArcLerpSin(float t);

RADENG_API bool RADENG_CALL RayIntersectsBBox(const Vec3 &a, const Vec3 &b, const BBox &bounds);