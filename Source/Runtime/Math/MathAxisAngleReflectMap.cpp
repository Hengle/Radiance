// MathAxisAngleReflectMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

#include "../ReflectMap.h"
#include "AxisAngle.h"

//////////////////////////////////////////////////////////////////////////////////////////
// math::AxisAngle
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_REFLECT_AXISANGLE(_name, _namespace, _type, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, AxisAngle)\
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)\
	RADREFLECT_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("axis", const _namespace::Vector3<_valueType>&)\
		RADREFLECT_ARG("radiance", _valueType)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("axisAngle", const _namespace::_type&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_PROPERTY_SET_METHOD("Axis", Axis, const _namespace::Vector3<_valueType>&)\
	RADREFLECT_PROPERTY_GET_METHOD("Angle", Angle, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("Angle", Angle, _valueType)\
	RADREFLECT_END_NAMESPACE(RADRT_API, _namespace, _type)

PRIVATE_REFLECT_AXISANGLE("math::AxisAngleF", math, AxisAngleF, float)
PRIVATE_REFLECT_AXISANGLE("math::AxisAngleD", math, AxisAngleD, double)

#endif
