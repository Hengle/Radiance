// EulerDef.h
// Definitions for Euler.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy

#pragma once

#include "IntMath.h"
#include "../ReflectDef.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math forward references
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T> class Euler;
typedef Euler<F32> EulerF;
typedef Euler<F64> EulerD;

} // math


#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

RADREFLECT_DECLARE(RADRT_API, ::math::EulerF)
RADREFLECT_DECLARE(RADRT_API, ::math::EulerD)

#endif
