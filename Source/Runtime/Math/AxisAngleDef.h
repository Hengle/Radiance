// AxisAngleDef.h
// Definitions for AxisAngle.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntMath.h"
#include "../ReflectDef.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math forward references
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T> class AxisAngle;
typedef AxisAngle<F32> AxisAngleF;
typedef AxisAngle<F64> AxisAngleD;

} // math


#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

RADREFLECT_DECLARE(RADRT_API, ::math::AxisAngleF)
RADREFLECT_DECLARE(RADRT_API, ::math::AxisAngleD)

#endif

