// Types.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <Runtime/Base.h>
#include <Runtime/String.h>
#include <Runtime/Math/AABB.h>
#include <Runtime/Math/Matrix.h>
#include <Runtime/Math/Vector.h>
#include <Runtime/Math/Quaternion.h>
#include <Runtime/Math/Euler.h>
#include <Runtime/Math/AxisAngle.h>
#include <Runtime/Math/Plane.h>
#include "Opts.h"
#include "Zones.h"

typedef math::AABB3<float> BBox;
typedef math::AABB3<float> BBoxD;
typedef math::Matrix4X4F Mat4;
typedef math::Matrix4X4D Mat4D;
typedef math::Vector2F Vec2;
typedef math::Vector2D Vec2D;
typedef math::Vector3F Vec3;
typedef math::Vector3D Vec3D;
typedef math::Vector4F Vec4;
typedef math::Vector4D Vec4D;
typedef math::QuaternionF Quat;
typedef math::QuaternionF QuatD;
typedef math::Scale3F Scale3;
typedef math::Scale3D Scale3D;
typedef math::EulerF Euler;
typedef math::EulerF EulerD;
typedef math::AxisAngleF AxisAngle;
typedef math::AxisAngleD AxisAngleD;
typedef math::PlaneF Plane;
typedef math::PlaneD PlaneD;
typedef Vec4 Color4;
typedef Vec4D Color4D;
