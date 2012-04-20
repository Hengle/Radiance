// MathVectorReflectMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

#include "../ReflectMap.h"
#include "Vector.h"

//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector2
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_REFLECT_VECTOR_COMMON(_name, _namespace, _type, _valueType)\
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)\
	RADREFLECT_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
	RADREFLECT_ARG("v", const _namespace::_type &)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_CONSTMETHOD_NAMED("Negate", _namespace::_type, operator-)\
	RADREFLECT_CONSTMETHOD(_valueType, MagnitudeSquared)\
	RADREFLECT_CONSTMETHOD(_valueType, Magnitude)\
	RADREFLECT_CONSTMETHOD(_namespace::_type, Unit)\
	RADREFLECT_BEGIN_CONSTMETHOD(bool)\
		RADREFLECT_DEFAULT_ARG("epsilon", const _valueType&, _namespace::_type##::UnitEpsilon)\
	RADREFLECT_END_METHOD(IsUnit)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("v", const _namespace::_type&)\
	RADREFLECT_END_METHOD_NAMED("Add", operator+)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("v", const _namespace::_type&)\
	RADREFLECT_END_METHOD_NAMED("Subtract", operator-)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("v", const _namespace::_type&)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("v", const _namespace::_type&)\
	RADREFLECT_END_METHOD_NAMED("Divide", operator/)\
	RADREFLECT_BEGIN_CONSTMETHOD(_valueType)\
		RADREFLECT_ARG("v", const _namespace::_type&)\
	RADREFLECT_END_METHOD(Dot)\
	RADREFLECT_BEGIN_CONSTMETHOD(bool)\
		RADREFLECT_ARG("v", const _namespace::_type&)\
		RADREFLECT_DEFAULT_ARG("epsilon", const _valueType&, math::Epsilon<_valueType>())\
	RADREFLECT_END_METHOD(NearlyEquals)\
	RADREFLECT_BEGIN_CONSTMETHOD(bool)\
		RADREFLECT_ARG("v", const _namespace::_type&)\
	RADREFLECT_END_METHOD_NAMED("ExactlyEquals", operator==)\
	RADREFLECT_BEGIN_CONSTMETHOD(bool)\
		RADREFLECT_ARG("v", const _namespace::_type&)\
	RADREFLECT_END_METHOD_NAMED("NotEqual", operator!=)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_METHOD_NAMED("ScalarAdd", operator+)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_METHOD_NAMED("ScalarSubtract", operator-)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_METHOD_NAMED("ScalarMultiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_METHOD_NAMED("ScalarDivide", operator/)

//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector2<T>
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_REFLECT_VECTOR2(_name, _namespace, _type, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, Vector2)\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("x", _valueType)\
		RADREFLECT_ARG("y", _valueType)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_PROPERTY_GET_METHOD("X", X, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("X", SetX, _valueType)\
	RADREFLECT_PROPERTY_GET_METHOD("Y", Y, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("Y", SetY, _valueType)\
	PRIVATE_REFLECT_VECTOR_COMMON(_name, _namespace, _type, _valueType)

PRIVATE_REFLECT_VECTOR2("math::Vector2F", math, Vector2F, float)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_MANAGEDTYPEINTEROP(
		"Microsoft::DirectX::Vector2",
		"%DST% = gcnew %TYPE%(%SRC%.X, %SRC%.Y);",
		"%DST% = %TYPE%(%SRC%->X, %SRC%->Y);"
	)
RADREFLECT_END_NAMESPACE(RADRT_API, math, Vector2F)
PRIVATE_REFLECT_VECTOR2("math::Vector2D", math, Vector2D, double)
RADREFLECT_END_NAMESPACE(RADRT_API, math, Vector2D)

//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector3<T>
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_REFLECT_VECTOR3(_name, _namespace, _type, _superType, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, Vector3)\
	RADREFLECT_SUPER(_namespace::_superType)\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("x", _valueType)\
		RADREFLECT_ARG("y", _valueType)\
		RADREFLECT_ARG("z", _valueType)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_PROPERTY_GET_METHOD("Z", Z, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("Z", SetZ, _valueType)\
	RADREFLECT_CONSTMETHOD(_valueType, ApproximateMagnitude)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::_type)\
		RADREFLECT_ARG("v", const _namespace::_type&)\
	RADREFLECT_END_METHOD(Cross)\
	PRIVATE_REFLECT_VECTOR_COMMON(_name, _namespace, _type, _valueType)

PRIVATE_REFLECT_VECTOR3("math::Vector3F", math, Vector3F, Vector2F, float)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_MANAGEDTYPEINTEROP(
		"Microsoft::DirectX::Vector3",
		"%DST% = gcnew %TYPE%(%SRC%.X, %SRC%.Y, %SRC%.Z);",
		"%DST% = %TYPE%(%SRC%->X, %SRC%->Y, %SRC%->Z);"
	)
RADREFLECT_END_NAMESPACE(RADRT_API, math, Vector3F)
PRIVATE_REFLECT_VECTOR3("math::Vector3D", math, Vector3D, Vector2D, double)
RADREFLECT_END_NAMESPACE(RADRT_API, math, Vector3D)

//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector4<T>
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_REFLECT_VECTOR4(_name, _namespace, _type, _superType, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, Vector4)\
	RADREFLECT_SUPER(_namespace::_superType)\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("x", _valueType)\
		RADREFLECT_ARG("y", _valueType)\
		RADREFLECT_ARG("z", _valueType)\
		RADREFLECT_ARG("w", _valueType)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_PROPERTY_GET_METHOD("W", W, _valueType)\
	RADREFLECT_PROPERTY_SET_METHOD("W", SetW, _valueType)\
	PRIVATE_REFLECT_VECTOR_COMMON(_name, _namespace, _type, _valueType)

PRIVATE_REFLECT_VECTOR4("math::Vector4F", math, Vector4F, Vector3F, float)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_MANAGEDTYPEINTEROP(
		"Microsoft::DirectX::Vector4",
		"%DST% = gcnew %TYPE%(%SRC%.X, %SRC%.Y, %SRC%.Z, %SRC%.W);",
		"%DST% = %TYPE%(%SRC%->X, %SRC%->Y, %SRC%->Z, %SRC%->W);"
	)
RADREFLECT_END_NAMESPACE(RADRT_API, math, Vector4F)
PRIVATE_REFLECT_VECTOR4("math::Vector4D", math, Vector4D, Vector3D, double)
RADREFLECT_END_NAMESPACE(RADRT_API, math, Vector4D)

#endif
