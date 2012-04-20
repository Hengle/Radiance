// MathEulerReflectMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

#include "../ReflectMap.h"
#include "Euler.h"

//////////////////////////////////////////////////////////////////////////////////////////
// math::Euler
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_REFLECT_EULER(_name, _namespace, _type, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, Euler)\
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)\
	RADREFLECT_SUPER(_namespace::Vector3<_valueType>)\
	RADREFLECT_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("yaw", _valueType)\
		RADREFLECT_ARG("pitch", _valueType)\
		RADREFLECT_ARG("roll", _valueType)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("e", const _namespace::_type&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("v", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_PROPERTY_GET_METHOD("Pitch", Pitch, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("Pitch", Pitch, _valueType)\
	RADREFLECT_PROPERTY_GET_METHOD("Yaw", Yaw, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("Yaw", Yaw, _valueType)\
	RADREFLECT_PROPERTY_GET_METHOD("Roll", Roll, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("Roll", Roll, _valueType)\
	RADREFLECT_END_NAMESPACE(RADRT_API, _namespace, _type)

PRIVATE_REFLECT_EULER("math::EulerF", math, EulerF, float)
PRIVATE_REFLECT_EULER("math::EulerD", math, EulerD, double)

#endif
