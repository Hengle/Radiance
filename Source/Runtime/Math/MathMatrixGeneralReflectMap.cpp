// MathAxisAngleReflectionMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if !defined(RADMATH_OPT_NO_REFLECTION) && !defined(RAD_OPT_NO_REFLECTION)

#include "../ReflectMap.h"
#include "Matrix.h"

//////////////////////////////////////////////////////////////////////////////////////////
// math::Axis3
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_ENUM_NAMESPACE("math::Axis3", math, Axis3)
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)\
	RADREFLECT_ENUM_VALUE(math::AxisX)
	RADREFLECT_ENUM_VALUE(math::AxisY)
	RADREFLECT_ENUM_VALUE(math::AxisZ)
RADREFLECT_END_NAMESPACE(RADRT_API, math, Axis3)

//////////////////////////////////////////////////////////////////////////////////////////
// math::Scale3
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_REFLECT_SCALE3(_name, _namespace, _type, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, Scale3)\
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)\
	RADREFLECT_SUPER(_namespace::Vector3<_valueType>)\
	RADREFLECT_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("s", _valueType)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("x", _valueType)\
		RADREFLECT_ARG("y", _valueType)\
		RADREFLECT_ARG("z", _valueType)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("v", const _namespace::Vector2<_valueType>&)\
		RADREFLECT_ARG("z", _valueType)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("v", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector3<_valueType>)\
		RADREFLECT_ARG("v", const _namespace::Vector3<_valueType>&)\
	RADREFLECT_END_METHOD(Scale)\
	RADREFLECT_END_NAMESPACE(RADRT_API, _namespace, _type)

PRIVATE_REFLECT_SCALE3("math::Scale3F", math, Scale3F, float)
PRIVATE_REFLECT_SCALE3("math::Scale3D", math, Scale3D, double)

//////////////////////////////////////////////////////////////////////////////////////////
// math::Rows4X4
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_REFLECT_ROWS4X4(_name, _namespace, _type, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, Rows4X4)\
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)\
	RADREFLECT_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("row0", const _namespace::Vector4<_valueType>&)\
		RADREFLECT_ARG("row1", const _namespace::Vector4<_valueType>&)\
		RADREFLECT_ARG("row2", const _namespace::Vector4<_valueType>&)\
		RADREFLECT_ARG("row3", const _namespace::Vector4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("columns", const _namespace::Columns4X4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("rows", const _namespace::_type&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector4<_valueType>)\
		RADREFLECT_ARG("i", int)\
	RADREFLECT_END_METHOD(Column)\
	RADREFLECT_END_NAMESPACE(RADRT_API, _namespace, _type)

PRIVATE_REFLECT_ROWS4X4("math::Rows4X4F", math, Rows4X4F, float)
PRIVATE_REFLECT_ROWS4X4("math::Rows4X4D", math, Rows4X4D, double)

//////////////////////////////////////////////////////////////////////////////////////////
// math::Columns4X4
//////////////////////////////////////////////////////////////////////////////////////////

#define PRIVATE_REFLECT_COLUMNS4X4(_name, _namespace, _type, _valueType)\
	RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(_name, _namespace, _type, Columns4X4)\
	RADREFLECT_RTLI_CLASS_ATTRIBUTE_VISIBLE(true)\
	RADREFLECT_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("col0", const _namespace::Vector4<_valueType>&)\
		RADREFLECT_ARG("col1", const _namespace::Vector4<_valueType>&)\
		RADREFLECT_ARG("col2", const _namespace::Vector4<_valueType>&)\
		RADREFLECT_ARG("col3", const _namespace::Vector4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("rows", const _namespace::Rows4X4<_valueType>&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTRUCTOR\
		RADREFLECT_ARG("columns", const _namespace::_type&)\
	RADREFLECT_END_CONSTRUCTOR\
	RADREFLECT_BEGIN_CONSTMETHOD(_namespace::Vector4<_valueType>)\
		RADREFLECT_ARG("i", int)\
	RADREFLECT_END_METHOD(Row)\
	RADREFLECT_END_NAMESPACE(RADRT_API, _namespace, _type)

PRIVATE_REFLECT_COLUMNS4X4("math::Columns4X4F", math, Columns4X4F, float)
PRIVATE_REFLECT_COLUMNS4X4("math::Columns4X4D", math, Columns4X4D, double)

#endif
