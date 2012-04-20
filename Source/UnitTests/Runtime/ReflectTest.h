// ReflectTest.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy

#include <Runtime/ReflectDef.h>

//////////////////////////////////////////////////////////////////////////////////////////
// ReflectedEnum
//////////////////////////////////////////////////////////////////////////////////////////

enum ReflectedEnum
{
	VALUE0,
	VALUE1,
	VALUE2,
	VALUE3
};

RADREFLECT_DECLARE(RADNULL_API, ReflectedEnum)

//////////////////////////////////////////////////////////////////////////////////////////
// ReflectClassTestAttribute
//////////////////////////////////////////////////////////////////////////////////////////

class ReflectClassTestAttribute
{
	RADREFLECT_EXPOSE_PRIVATES(ReflectClassTestAttribute)

public:

	ReflectClassTestAttribute() : m_num(666) {}
	ReflectClassTestAttribute(int num) : m_num(num) {}

private:

	int m_num;
};

RADREFLECT_DECLARE(RADNULL_API, ReflectClassTestAttribute)

//////////////////////////////////////////////////////////////////////////////////////////
// ReflectClassTestBase
//////////////////////////////////////////////////////////////////////////////////////////

class ReflectClassTestBase
{
	RADREFLECT_EXPOSE_PRIVATES(ReflectClassTestBase)

public:

	ReflectClassTestBase() : m_num(0) {}
	~ReflectClassTestBase() {}

private:

	int m_num;
};

RADREFLECT_DECLARE(RADNULL_API, ReflectClassTestBase)

//////////////////////////////////////////////////////////////////////////////////////////
// ReflectClassTest
//////////////////////////////////////////////////////////////////////////////////////////

class ReflectClassTest :
public ReflectClassTestBase
{
	RADREFLECT_EXPOSE_PRIVATES(ReflectClassTest)

public:

	ReflectClassTest() : ReflectClassTestBase(), m_x(0.0f), m_y(0.0f), m_z(0.0f) {}
	ReflectClassTest(float x, float y, float z) : ReflectClassTestBase(), m_x(x), m_y(y), m_z(z) {}

	float X() const { return m_x; }
	void SetX(float x = 69.0f) { m_x = x; }

	float Y() const { return m_y; }
	void Y(float y = 0.0f) { m_y = y; }

	float Z() { return m_z; }
	void Z(const float z) { m_z = z; }

	ReflectClassTest operator + (int x) { return *this; }

	//void RefTestGetZ(float &z, float &x) { z = m_z; x = m_z; }
	void RefTestGetZ(float &z) { z = m_z; }
	ReflectedEnum SetEnumValue(ReflectedEnum val) { return val; }
	void SetIntValue(int val) {}

	static float StaticX() { return s_x; }
	static void StaticX(float x = 37.0f) { s_x = x; }

	RAD_DECLARE_PROPERTY(ReflectClassTest, prop1, const char*, const char*);
	RAD_DECLARE_READONLY_PROPERTY(ReflectClassTest, prop2, int);
	RAD_DECLARE_WRITEONLY_PROPERTY(ReflectClassTest, prop3, int);

private:

	const char* RAD_IMPLEMENT_GET(prop1) { return "This is a test string"; }
	void RAD_IMPLEMENT_SET(prop1) (const char *) {}
	int RAD_IMPLEMENT_GET(prop2) { return 555; }
	void RAD_IMPLEMENT_SET(prop3) (int x) {}


	float m_x, m_y, m_z;

	static float s_x;

	static const float CONST_Y;
};

RADREFLECT_DECLARE(RADNULL_API, ReflectClassTest)
