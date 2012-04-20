// MatrixDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntMath.h"
#include "VectorDef.h"
#include "QuaternionDef.h"
#include "AxisAngleDef.h"
#include "EulerDef.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math forward declarations
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T> class Matrix4X4;
template<typename T> class AffineMatrix4X4;
template<typename T> class RigidMatrix4X4;
template<typename T> class SolidMatrix4X4;
template<typename T> class Scale3;
template<typename T> class Rows4X4;
template<typename T> class Columns4X4;

typedef Matrix4X4<F32> Matrix4X4F;
typedef Matrix4X4<F64> Matrix4X4D;
typedef AffineMatrix4X4<F32> AffineMatrix4X4F;
typedef AffineMatrix4X4<F64> AffineMatrix4X4D;
typedef RigidMatrix4X4<F32> RigidMatrix4X4F;
typedef RigidMatrix4X4<F64> RigidMatrix4X4D;
typedef SolidMatrix4X4<F32> SolidMatrix4X4F;
typedef SolidMatrix4X4<F64> SolidMatrix4X4D;
typedef Scale3<F32> Scale3F;
typedef Scale3<F64> Scale3D;
typedef Rows4X4<F32> Rows4X4F;
typedef Rows4X4<F64> Rows4X4D;
typedef Columns4X4<F32> Columns4X4F;
typedef Columns4X4<F64> Columns4X4D;

//////////////////////////////////////////////////////////////////////////////////////////
// math::Axis3
//////////////////////////////////////////////////////////////////////////////////////////

enum Axis3
{
	AxisX,
	AxisY,
	AxisZ
};

//////////////////////////////////////////////////////////////////////////////////////////
// helpers
//////////////////////////////////////////////////////////////////////////////////////////

#define RADMATH_MATRIX_FRIEND_DECLARATIONS \
	friend class Matrix4X4<T>; \
	friend class AffineMatrix4X4<T>; \
	friend class RigidMatrix4X4<T>; \
	friend class SolidMatrix4X4<T>;

} // math


#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

RADREFLECT_DECLARE(RADRT_API, ::math::Axis3)
RADREFLECT_DECLARE(RADRT_API, ::math::Matrix4X4F)
RADREFLECT_DECLARE(RADRT_API, ::math::Matrix4X4D)
RADREFLECT_DECLARE(RADRT_API, ::math::AffineMatrix4X4F)
RADREFLECT_DECLARE(RADRT_API, ::math::AffineMatrix4X4D)
RADREFLECT_DECLARE(RADRT_API, ::math::RigidMatrix4X4F)
RADREFLECT_DECLARE(RADRT_API, ::math::RigidMatrix4X4D)
RADREFLECT_DECLARE(RADRT_API, ::math::SolidMatrix4X4F)
RADREFLECT_DECLARE(RADRT_API, ::math::SolidMatrix4X4D)
RADREFLECT_DECLARE(RADRT_API, ::math::Scale3F)
RADREFLECT_DECLARE(RADRT_API, ::math::Scale3D)
RADREFLECT_DECLARE(RADRT_API, ::math::Rows4X4F)
RADREFLECT_DECLARE(RADRT_API, ::math::Rows4X4D)
RADREFLECT_DECLARE(RADRT_API, ::math::Columns4X4F)
RADREFLECT_DECLARE(RADRT_API, ::math::Columns4X4D)

#endif
