// VectorDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntMath.h"
#include "../ReflectDef.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math forward declarations
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T> class Vector2;
template<typename T> class Vector3;
template<typename T> class Vector4;

typedef Vector2<F32> Vector2F;
typedef Vector3<F32> Vector3F;
typedef Vector4<F32> Vector4F;

typedef Vector2<F64> Vector2D;
typedef Vector3<F64> Vector3D;
typedef Vector4<F64> Vector4D;

} // math


#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

RADREFLECT_DECLARE(RADRT_API, ::math::Vector2F)
RADREFLECT_DECLARE(RADRT_API, ::math::Vector2D)
RADREFLECT_DECLARE(RADRT_API, ::math::Vector3F)
RADREFLECT_DECLARE(RADRT_API, ::math::Vector3D)
RADREFLECT_DECLARE(RADRT_API, ::math::Vector4F)
RADREFLECT_DECLARE(RADRT_API, ::math::Vector4D)

#endif

