// MathAffineMatrix4X4ReflectMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

#include "../ReflectMap.h"
#include "Matrix.h"

#define PRIVATE_REFLECT_MATRIX4X4(_name, _namespace, _type, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, AffineMatrix4X4)\
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)\
	RADREFLECT_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("m", const _namespace::_type&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("m", const _namespace::RigidMatrix4X4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("m", const _namespace::SolidMatrix4X4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("width", _valueType)\
		RADREFLECT_ARG("height", _valueType)\
		RADREFLECT_ARG("zNear", _valueType)\
		RADREFLECT_ARG("zFar", _valueType)\
	RADREFLECT_END_METHOD(OrthoLH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("width", _valueType)\
		RADREFLECT_ARG("height", _valueType)\
		RADREFLECT_ARG("zNear", _valueType)\
		RADREFLECT_ARG("zFar", _valueType)\
	RADREFLECT_END_METHOD(OrthoRH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("left", _valueType)\
		RADREFLECT_ARG("right", _valueType)\
		RADREFLECT_ARG("top", _valueType)\
		RADREFLECT_ARG("bottom", _valueType)\
		RADREFLECT_ARG("zNear", _valueType)\
		RADREFLECT_ARG("zFar", _valueType)\
	RADREFLECT_END_METHOD(OrthoOffCenterLH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("left", _valueType)\
		RADREFLECT_ARG("right", _valueType)\
		RADREFLECT_ARG("top", _valueType)\
		RADREFLECT_ARG("bottom", _valueType)\
		RADREFLECT_ARG("zNear", _valueType)\
		RADREFLECT_ARG("zFar", _valueType)\
	RADREFLECT_END_METHOD(OrthoOffCenterRH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("axis", _namespace::Axis3)\
		RADREFLECT_ARG("radians", _valueType)\
	RADREFLECT_END_METHOD(Rotation)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("axisAngle", const _namespace::AxisAngle<_valueType>&)\
	RADREFLECT_END_METHOD(Rotation)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("quaternion", const _namespace::Quaternion<_valueType>&)\
	RADREFLECT_END_METHOD(Rotation)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("euler", const _namespace::Euler<_valueType>&)\
	RADREFLECT_END_METHOD(Rotation)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("eye", const _namespace::Vector3<_valueType>&)\
		RADREFLECT_ARG("at", const _namespace::Vector3<_valueType>&)\
		RADREFLECT_ARG("up", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_METHOD(LookAtLH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("eye", const _namespace::Vector3<_valueType>&)\
		RADREFLECT_ARG("at", const _namespace::Vector3<_valueType>&)\
		RADREFLECT_ARG("up", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_METHOD(LookAtRH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("scale", const _namespace::Scale3<_valueType>&)\
	RADREFLECT_END_METHOD(Scaling)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("offset", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_METHOD(Translation)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector4<_valueType>)\
		RADREFLECT_ARG("i", int)\
	RADREFLECT_END_METHOD(Column)\
	RADREFLECT_CONSTMETHOD(_namespace::Columns4X4<_valueType>, Columns)\
	RADREFLECT_CONSTMETHOD(_namespace::Quaternion<_valueType>, Rotation)\
	RADREFLECT_CONSTMETHOD(_namespace::AffineMatrix4X4<_valueType>, Inverse)\
	RADREFLECT_CONSTMETHOD(_valueType, Determinant)\
	RADREFLECT_CONSTMETHOD(_valueType, Determinant3X3)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector3<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_METHOD(ScaleAndRotate)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector4<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector4<_valueType>&)\
	RADREFLECT_END_METHOD(ScaleAndRotate)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector3<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_METHOD(Translate)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector4<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector4<_valueType>&)\
	RADREFLECT_END_METHOD(Translate)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector3<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_METHOD(Transform)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector4<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector4<_valueType>&)\
	RADREFLECT_END_METHOD(Transform)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::Matrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Add", operator+)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::AffineMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Add", operator+)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::RigidMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Add", operator+)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::SolidMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Add", operator+)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::Matrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Subtract", operator-)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::AffineMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Subtract", operator-)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::RigidMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Subtract", operator-)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::SolidMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Subtract", operator-)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::Matrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::AffineMatrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::AffineMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::AffineMatrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::RigidMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::AffineMatrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::SolidMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_METHOD_NAMED("Divide", operator/)\
	RADREFLECT_END_NAMESPACE(RADRT_API, _namespace, _type)

PRIVATE_REFLECT_MATRIX4X4("math::AffineMatrix4X4F", math, AffineMatrix4X4F, float)
PRIVATE_REFLECT_MATRIX4X4("math::AffineMatrix4X4D", math, AffineMatrix4X4D, double)

#endif
