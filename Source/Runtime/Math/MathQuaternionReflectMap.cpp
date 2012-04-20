// MathQuaternionReflectMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

#include "../ReflectMap.h"
#include "Quaternion.h"

//////////////////////////////////////////////////////////////////////////////////////////
// math::Quaternion
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_REFLECT_QUATERNION(_name, _namespace, _type, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, Quaternion)\
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)\
	RADREFLECT_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("x", _valueType)\
		RADREFLECT_ARG("y", _valueType)\
		RADREFLECT_ARG("z", _valueType)\
		RADREFLECT_ARG("w", _valueType)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("v", const _namespace::Vector4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("axis", const _namespace::Vector3<_valueType>&)\
		RADREFLECT_ARG("rad", _valueType)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("q", const _namespace::_type&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::AxisAngle<_valueType>)\
		RADREFLECT_DEFAULT_ARG("epsilon", const _valueType&, _valueType(0.0005))\
	RADREFLECT_END_METHOD(ToAxisAngle)\
	RADREFLECT_PROPERTY_GET_METHOD("X", X, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("X", SetX, _valueType)\
	RADREFLECT_PROPERTY_GET_METHOD("Y", Y, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("Y", SetY, _valueType)\
	RADREFLECT_PROPERTY_GET_METHOD("Z", Z, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("Z", SetZ, _valueType)\
	RADREFLECT_PROPERTY_GET_METHOD("W", W, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("W", SetW, _valueType)\
	RADREFLECT_CONSTMETHOD(_valueType, Magnitude)\
	RADREFLECT_CONSTMETHOD(_valueType, MagnitudeSquared)\
	RADREFLECT_CONSTMETHOD(_namespace::_type, Conjugate)\
	RADREFLECT_CONSTMETHOD(_namespace::_type, Unit)\
	RADREFLECT_CONSTMETHOD(_namespace::_type, Inverse)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("q", const _namespace::_type&)\
	RADREFLECT_END_METHOD_NAMED("Add", operator+)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("q", const _namespace::_type&)\
	RADREFLECT_END_METHOD_NAMED("Subtract", operator-)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("q", const _namespace::_type&)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_METHOD_NAMED("Divide", operator/)\

PRIVATE_REFLECT_QUATERNION("math::QuaternionF", math, QuaternionF, float)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_MANAGEDTYPEINTEROP(
		"Microsoft::DirectX::Quaternion",
		// TO UNMANAGED TYPE
		"%DST% = gcnew %TYPE%(%SRC%.X, %SRC%.Y, %SRC%.Z, %SRC%.W);"
		,
		// TO MANAGED TYPE
		"%DST%.X = %SRC%->X; %DST%.Y = %SRC%->Y; %DST%.Z = %SRC%->Z; %DST%.W = %SRC%->W;"
	)
RADREFLECT_END_NAMESPACE(RADRT_API, math, QuaternionF)

PRIVATE_REFLECT_QUATERNION("math::QuaternionD", math, QuaternionD, double)
RADREFLECT_END_NAMESPACE(RADRT_API, math, QuaternionD)

#endif
