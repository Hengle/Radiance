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

typedef ::string::String String;
typedef math::AABB3<float> BBox;
typedef math::Matrix4X4F Mat4;
typedef math::Vector2F Vec2;
typedef math::Vector3F Vec3;
typedef math::Vector4F Vec4;
typedef math::QuaternionF Quat;
typedef math::Scale3F Scale3;
typedef math::EulerF Euler;
typedef math::AxisAngleF AxisAngle;
typedef math::PlaneF Plane;
typedef Vec4 Color4;
