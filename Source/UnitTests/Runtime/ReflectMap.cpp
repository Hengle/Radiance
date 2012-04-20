// ReflectMap.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#include <Runtime/ReflectMap.h>
#include "ReflectTest.h"

//////////////////////////////////////////////////////////////////////////////////////////
// ReflectedEnum
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_ENUM("ReflectedEnum", ReflectedEnum)
	RADREFLECT_ENUM_VALUE(VALUE0)
	RADREFLECT_ENUM_VALUE_NAMED("VALUE1", VALUE1)
RADREFLECT_END(RADNULL_API, ReflectedEnum)

//////////////////////////////////////////////////////////////////////////////////////////
// ReflectClassTestAttribute
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("ReflectClassTestAttribute", ReflectClassTestAttribute)

RADREFLECT_END(RADNULL_API, ReflectClassTestAttribute)

//////////////////////////////////////////////////////////////////////////////////////////
// ReflectClassTestBase
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("ReflectClassTestBase", ReflectClassTestBase)

	RADREFLECT_MEMBER_NAMED("num", int, m_num)
		RADREFLECT_RTLI_ATTRIBUTE_VISIBLE(false)

RADREFLECT_END(RADNULL_API, ReflectClassTestBase)

//////////////////////////////////////////////////////////////////////////////////////////
// ReflectClassTest
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_BEGIN_CLASS("ReflectClassTest", ReflectClassTest)

	RADREFLECT_CLASS_ATTRIBUTE_NAMED("bleh", ReflectClassTestAttribute, ())

	RADREFLECT_SUPER_NAMED("super", ReflectClassTestBase)
		RADREFLECT_ATTRIBUTE(ReflectClassTestAttribute, (69))

	RADREFLECT_CONSTRUCTOR
		RADREFLECT_ATTRIBUTE_NAMED("default", ReflectClassTestAttribute, (2))

	RADREFLECT_CLASS_ATTRIBUTE(ReflectClassTestAttribute, (37))

	RADREFLECT_BEGIN_CONSTRUCTOR
		RADREFLECT_ARG("x", float)
			RADREFLECT_ATTRIBUTE(ReflectClassTestAttribute, ())
			RADREFLECT_ATTRIBUTE_NAMED("in", ReflectClassTestAttribute, (1))
		RADREFLECT_ARG("y", float)
		RADREFLECT_ARG("z", float)
	RADREFLECT_END_CONSTRUCTOR
		RADREFLECT_ATTRIBUTE(ReflectClassTestAttribute, ())

	RADREFLECT_BEGIN_METHOD(ReflectClassTest)
		RADREFLECT_ARG("x", int)
	RADREFLECT_END_METHOD_NAMED("Add", operator+)

	RADREFLECT_CONSTMETHOD_NAMED("GetX", float, X)
		RADREFLECT_ATTRIBUTE(ReflectClassTestAttribute, ())

	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_DEFAULT_ARG("x", float, 69.0f)
			RADREFLECT_ATTRIBUTE(ReflectClassTestAttribute, (37))
	RADREFLECT_END_METHOD_NAMED("SetX", SetX)
		RADREFLECT_ATTRIBUTE(ReflectClassTestAttribute, (3))

	RADREFLECT_CONSTMETHOD_NAMED("GetY", float, Y)

	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_DEFAULT_ARG("y", float, 0.0f)
	RADREFLECT_END_METHOD_NAMED("SetY", Y)

	RADREFLECT_METHOD_NAMED("GetZ", float, Z)

	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_ARG("z", const float)
	RADREFLECT_END_METHOD_NAMED("SetZ", Z)

	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_ARG("z", float&)
		//RADREFLECT_ARG("x", float&)
	RADREFLECT_END_METHOD(RefTestGetZ)

	RADREFLECT_BEGIN_METHOD(ReflectedEnum)
		RADREFLECT_ARG("val", ReflectedEnum)
	RADREFLECT_END_METHOD(SetEnumValue)

	RADREFLECT_BEGIN_METHOD(void)
		RADREFLECT_ARG("val", int)
	RADREFLECT_END_METHOD(SetIntValue)

	RADREFLECT_STATICMETHOD_NAMED("GetX", float, StaticX)

	RADREFLECT_BEGIN_STATICMETHOD(void)
		RADREFLECT_DEFAULT_ARG("x", float, 37.0f)
	RADREFLECT_END_METHOD_NAMED("SetX", StaticX)

	RADREFLECT_MEMBER_NAMED("x", float, m_x)
		RADREFLECT_ATTRIBUTE(ReflectClassTestAttribute, (12))

	RADREFLECT_MEMBER_NAMED("y", float, m_y)
		RADREFLECT_ATTRIBUTE(ReflectClassTestAttribute, (6))
	RADREFLECT_MEMBER_NAMED("z", float, m_z)

	RADREFLECT_STATICMEMBER_NAMED("x", float, s_x)
		RADREFLECT_ATTRIBUTE(ReflectClassTestAttribute, (37))

	RADREFLECT_STATICMEMBER_NAMED("Y", const float, CONST_Y)

	RADREFLECT_PROPERTY(prop1, const char*, const char*)
	RADREFLECT_PROPERTY_GET(prop2, int)
	RADREFLECT_PROPERTY_SET(prop3, int)

RADREFLECT_END(RADNULL_API, ReflectClassTest)
