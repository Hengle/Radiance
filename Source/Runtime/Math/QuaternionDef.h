// QuaternionDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntMath.h"
#include "AxisAngleDef.h"
#include "../ReflectDef.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math forward references
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T> class Quaternion;
typedef Quaternion<F32> QuaternionF;
typedef Quaternion<F64> QuaternionD;

} // math


#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

RADREFLECT_DECLARE(RADRT_API, ::math::QuaternionF)
RADREFLECT_DECLARE(RADRT_API, ::math::QuaternionD)

#endif

