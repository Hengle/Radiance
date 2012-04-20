// MathMatrix4X4ReflectMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

#include "../ReflectMap.h"
#include "Matrix.h"

#define PRIVATE_REFLECT_MATRIX4X4(_name, _namespace, _type, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, Matrix4X4)\
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)\
	RADREFLECT_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("m", const _namespace::_type&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("m", const _namespace::AffineMatrix4X4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("m", const _namespace::RigidMatrix4X4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("m", const _namespace::SolidMatrix4X4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("rows", const _namespace::Rows4X4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("columns", const _namespace::Columns4X4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("row0", const _namespace::Vector4<_valueType>&)\
		RADREFLECT_ARG("row1", const _namespace::Vector4<_valueType>&)\
		RADREFLECT_ARG("row2", const _namespace::Vector4<_valueType>&)\
		RADREFLECT_ARG("row3", const _namespace::Vector4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("row0", const _namespace::Vector3<_valueType>&)\
		RADREFLECT_ARG("row1", const _namespace::Vector3<_valueType>&)\
		RADREFLECT_ARG("row2", const _namespace::Vector3<_valueType>&)\
		RADREFLECT_ARG("row3", const _namespace::Vector3<_valueType>&)\
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
		RADREFLECT_ARG("fovY", _valueType)\
		RADREFLECT_ARG("aspect", _valueType)\
		RADREFLECT_ARG("zNear", _valueType)\
		RADREFLECT_ARG("zFar", _valueType)\
	RADREFLECT_END_METHOD(PerspectiveFovLH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("fovY", _valueType)\
		RADREFLECT_ARG("aspect", _valueType)\
		RADREFLECT_ARG("zNear", _valueType)\
		RADREFLECT_ARG("zFar", _valueType)\
	RADREFLECT_END_METHOD(PerspectiveFovRH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("width", _valueType)\
		RADREFLECT_ARG("height", _valueType)\
		RADREFLECT_ARG("zNear", _valueType)\
		RADREFLECT_ARG("zFar", _valueType)\
	RADREFLECT_END_METHOD(PerspectiveLH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("width", _valueType)\
		RADREFLECT_ARG("height", _valueType)\
		RADREFLECT_ARG("zNear", _valueType)\
		RADREFLECT_ARG("zFar", _valueType)\
	RADREFLECT_END_METHOD(PerspectiveRH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("left", _valueType)\
		RADREFLECT_ARG("right", _valueType)\
		RADREFLECT_ARG("top", _valueType)\
		RADREFLECT_ARG("bottom", _valueType)\
		RADREFLECT_ARG("zNear", _valueType)\
		RADREFLECT_ARG("zFar", _valueType)\
	RADREFLECT_END_METHOD(PerspectiveOffCenterLH)\
	RADREFLECT_BEGIN_STATICMETHOD(_namespace::_type)\
		RADREFLECT_ARG("left", _valueType)\
		RADREFLECT_ARG("right", _valueType)\
		RADREFLECT_ARG("top", _valueType)\
		RADREFLECT_ARG("bottom", _valueType)\
		RADREFLECT_ARG("zNear", _valueType)\
		RADREFLECT_ARG("zFar", _valueType)\
	RADREFLECT_END_METHOD(PerspectiveOffCenterRH)\
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
	RADREFLECT_CONSTMETHOD(_namespace::Matrix4X4<_valueType>, Transpose)\
	RADREFLECT_CONSTMETHOD(_namespace::Matrix4X4<_valueType>, Inverse)\
	RADREFLECT_CONSTMETHOD(_valueType, Determinant)\
	RADREFLECT_CONSTMETHOD(_valueType, Determinant3X3)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector3<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_METHOD(Transform)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector4<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector4<_valueType>&)\
	RADREFLECT_END_METHOD(Transform)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector3<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_METHOD(Transform3X3)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector4<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector4<_valueType>&)\
	RADREFLECT_END_METHOD(Transform3X3)\
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
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::AffineMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::RigidMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("m", const _namespace::SolidMatrix4X4<_valueType>&)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_METHOD_NAMED("Multiply", operator*)\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Matrix4X4<_valueType>)\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_METHOD_NAMED("Divide", operator/)\

PRIVATE_REFLECT_MATRIX4X4("math::Matrix4X4F", math, Matrix4X4F, float)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_MANAGEDTYPEINTEROP(
		"Microsoft::DirectX::Matrix",
		// TO UNMANAGED TYPE
		"{ Microsoft::DirectX::Vector4 row0 = Microsoft::DirectX::Vector4(%SRC%.M11, %SRC%.M12, %SRC%.M13, %SRC%.M14);"
		"  Microsoft::DirectX::Vector4 row1 = Microsoft::DirectX::Vector4(%SRC%.M21, %SRC%.M22, %SRC%.M23, %SRC%.M24);"
		"  Microsoft::DirectX::Vector4 row2 = Microsoft::DirectX::Vector4(%SRC%.M31, %SRC%.M32, %SRC%.M33, %SRC%.M34);"
		"  Microsoft::DirectX::Vector4 row3 = Microsoft::DirectX::Vector4(%SRC%.M41, %SRC%.M42, %SRC%.M43, %SRC%.M44);"
		"  %DST% = gcnew %TYPE%(row0, row1, row2, row3); }"
		,
		// TO MANAGED TYPE
		"{ Microsoft::DirectX::Vector4 col0 = %SRC%->Column(0);"
		"  Microsoft::DirectX::Vector4 col1 = %SRC%->Column(1);"
		"  Microsoft::DirectX::Vector4 col2 = %SRC%->Column(2);"
		"  Microsoft::DirectX::Vector4 col3 = %SRC%->Column(3);"
		"  %DST%.M11 = col0.X; %DST%.M21 = col0.Y; %DST%.M31 = col0.Z; %DST%.M41 = col0.Z;"
		"  %DST%.M12 = col1.X; %DST%.M22 = col1.Y; %DST%.M32 = col1.Z; %DST%.M42 = col1.Z;"
		"  %DST%.M13 = col2.X; %DST%.M23 = col2.Y; %DST%.M33 = col2.Z; %DST%.M43 = col2.Z;"
		"  %DST%.M14 = col3.X; %DST%.M24 = col3.Y; %DST%.M34 = col3.Z; %DST%.M44 = col3.Z; }"
	)
RADREFLECT_END_NAMESPACE(RADRT_API, math, Matrix4X4F)
PRIVATE_REFLECT_MATRIX4X4("math::Matrix4X4D", math, Matrix4X4D, double)
RADREFLECT_END_NAMESPACE(RADRT_API, math, Matrix4X4D)

#endif
