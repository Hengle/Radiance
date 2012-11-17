// Vector.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Math.h"
#include "VectorDef.h"
#include "../PushPack.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector2<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Vector2
{

public:	

	// Type definitions

	typedef T ValueType;

	// Constants

	static const T UnitEpsilon;
	static const Vector2<T> Zero;
	
	// Constructors

	Vector2();
	explicit Vector2(const T *v);
	Vector2(T x, T y);
	Vector2(const Vector2<T> &v);

	// Destructor

	~Vector2();

	// Access

	T X() const;
	void SetX(T x);
	T Y() const;
	void SetY(T y);

	const T &operator[](int i) const;
	T &operator[](int i);

	// Operations

	Vector2<T> &Initialize();
	Vector2<T> &Initialize(const T *v);
	Vector2<T> &Initialize(T x, T y);
	Vector2<T> &Reverse();
	Vector2<T> Reflect(const Vector2<T> &v) const;

	// Returns length.
	T Normalize();

	// Unary operations

	Vector2<T> operator-() const;
	T MagnitudeSquared() const;
	T Magnitude() const;
	Vector2<T> Unit() const;
	bool IsUnit(const T &epsilon = UnitEpsilon) const;
		
	// Binary operations

	Vector2<T> operator+(const Vector2<T> &v) const;
	Vector2<T> operator-(const Vector2<T> &v) const;
	Vector2<T> operator*(const Vector2<T> &v) const;
	Vector2<T> operator/(const Vector2<T> &v) const;
	T Dot(const Vector2<T> &v) const;

	// Assignment operations

	Vector2<T> &operator=(const Vector2<T> &v);
	Vector2<T> &operator+=(const Vector2<T> &v);
	Vector2<T> &operator-=(const Vector2<T> &v);
	Vector2<T> &operator*=(const Vector2<T> &v);
	Vector2<T> &operator/=(const Vector2<T> &v);

	// Comparison operations
	bool NearlyEquals(const Vector2<T>& v, const T &epsilon = Epsilon<T>()) const;
	int Compare(const Vector2<T> &v) const;
	bool operator==(const Vector2<T> &v) const;
	bool operator!=(const Vector2<T> &v) const;
	bool operator <(const Vector2<T> &v) const;
	bool operator >(const Vector2<T> &v) const;
	bool operator <=(const Vector2<T> &v) const;
	bool operator >=(const Vector2<T> &v) const;

	// Scalar binary operations

	Vector2<T> operator+(T s) const;
	template <typename X>
	friend Vector2<X> operator+(X s, const Vector2<X> &v);
	Vector2<T> operator-(T s) const;
	template <typename X>
	friend Vector2<X> operator-(X s, const Vector2<X> &v);
	Vector2<T> operator*(T s) const;
	template <typename X>
	friend Vector2<X> operator*(X s, const Vector2<X> &v);
	Vector2<T> operator/(T s) const;
	
	// Scalar assignment operations

	Vector2<T> &operator+=(T s);
	Vector2<T> &operator-=(T s);
	Vector2<T> &operator*=(T s);
	Vector2<T> &operator/=(T s);

protected:

	T m_x;
	T m_y;

};
	
//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector2<T> friend functions
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Vector2<T> operator+(T s, const Vector2<T> &v);

template <typename T>
Vector2<T> operator-(T s, const Vector2<T> &v);

template <typename T>
Vector2<T> operator*(T s, const Vector2<T> &v);

//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector3<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Vector3 : public Vector2<T>
{

public:

	typedef Vector2<T> SuperType;
	
	// Constants

	static const Vector3<T> Zero;

	// Constructors

	Vector3();
	explicit Vector3(const T *v);	
	Vector3(T xx, T yy, T zz);
	Vector3(const Vector2<T> &v, T zz);
	Vector3(const Vector3<T> &v);

	// Destructor

	~Vector3();

	// Access

	T Z() const;
	void SetZ(T z);

	const T &operator[](int i) const;
	T &operator[](int i);

	// Operations

	Vector3<T> &Initialize();
	Vector3<T> &Initialize(const T *v);
	Vector3<T> &Initialize(T x, T y, T z);
	Vector3<T> &Initialize(const Vector2<T> &v, T z);
	Vector3<T> &Reverse();

	// Returns length.
	T Normalize();

	// Unary operations

	Vector3<T> operator-() const;
	T MagnitudeSquared() const;
	T Magnitude() const;
	T ApproximateMagnitude() const;
	Vector3<T> Unit() const;
	bool IsUnit(const T &epsilon = SuperType::UnitEpsilon) const;
	
	// Binary operations

	Vector3<T> operator+(const Vector3<T> &v) const;
	Vector3<T> operator-(const Vector3<T> &v) const;
	Vector3<T> operator*(const Vector3<T> &v) const;
	Vector3<T> operator/(const Vector3<T> &v) const;
	Vector3<T> Cross(const Vector3<T> &v) const;
	T Dot(const Vector3<T> &v) const;

	// Plane operations (this should be a unit plane normal vector)

	Vector3<T> Reflect(const Vector3<T> &v) const;
	Vector3<T> Project(const Vector3<T> &v) const;	

	// Assignment operations

	Vector3<T> &operator=(const Vector3<T> &v);
	Vector3<T> &operator+=(const Vector3<T> &v);
	Vector3<T> &operator-=(const Vector3<T> &v);
	Vector3<T> &operator*=(const Vector3<T> &v);
	Vector3<T> &operator/=(const Vector3<T> &v);

	// Comparison operations
	
	bool NearlyEquals(const Vector3<T>& v, const T& epsilon = Epsilon<T>()) const;
	int Compare(const Vector3<T> &v) const;
	bool operator==(const Vector3<T> &v) const;
	bool operator!=(const Vector3<T> &v) const;
	bool operator <(const Vector3<T> &v) const;
	bool operator >(const Vector3<T> &v) const;
	bool operator <=(const Vector3<T> &v) const;
	bool operator >=(const Vector3<T> &v) const;

	// Scalar binary operations

	Vector3<T> operator+(T s) const;
	template <typename X>
	friend Vector3<X> operator+(X s, const Vector3<X> &v); // friend operators of template classes are broken in .NET (they don't link).
	Vector3<T> operator-(T s) const;
	template <typename X>
	friend Vector3<X> operator-(X s, const Vector3<X> &v);
	Vector3<T> operator*(T s) const;
	template <typename X>
	friend Vector3<X> operator*(X s, const Vector3<X> &v);
	Vector3<T> operator/(T s) const;

	// Scalar assignment operations

	Vector3<T> &operator+=(T s);
	Vector3<T> &operator-=(T s);
	Vector3<T> &operator*=(T s);
	Vector3<T> &operator/=(T s);

	const Vector3<T> &FrameVecs(Vector3<T> &up, Vector3<T> &left) const;

protected:

	T m_z;

};
	
//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector3<T> friend functions
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Vector3<T> operator+(T s, const Vector3<T> &v);

template <typename T>
Vector3<T> operator-(T s, const Vector3<T> &v);

template <typename T>
Vector3<T> operator*(T s, const Vector3<T> &v);

template <typename T>
void ClearExtents(Vector3<T>* mins, Vector3<T>* maxs);

template <typename T>
void ExpandExtentsToFit(const Vector3<T>& i, Vector3<T>* mins, Vector3<T>* maxs);

//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector4<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Vector4 : public Vector3<T>
{

public:

	typedef Vector3<T> SuperType;
	
	// Constants

	static const Vector4<T> Zero;

	// Constructors

	Vector4();
	explicit Vector4(const T *v);
	Vector4(T x, T y, T z, T w);
	Vector4(const Vector2<T> &v, T z, T w);
	Vector4(const Vector3<T> &v, T w);
	Vector4(const Vector4<T> &v);

	// Destructor

	~Vector4();

	// Access

	T W() const;
	void SetW(T w);

	const T &operator[](int i) const;
	T &operator[](int i);

	// In place operations

	Vector4<T> &Initialize();
	Vector4<T> &Initialize(const T *v);
	Vector4<T> &Initialize(T x, T y, T z, T w);
	Vector4<T> &Initialize(const Vector2<T> &v, T zz, T ww);
	Vector4<T> &Initialize(const Vector3<T> &v, T ww);
	Vector4<T> &Reverse();

	// Returns length.
	T Normalize();

	// Unary operations

	Vector4<T> operator-() const;
	T MagnitudeSquared() const;
	T Magnitude() const;
	Vector4<T> Unit() const;
	bool IsUnit(const T &epsilon = SuperType::UnitEpsilon) const;
	
	// Binary operations

	Vector4<T> operator+(const Vector4<T> &v) const;
	Vector4<T> operator-(const Vector4<T> &v) const;
	Vector4<T> operator*(const Vector4<T> &v) const;
	Vector4<T> operator/(const Vector4<T> &v) const;
	T Dot(const Vector4<T> &v) const;

	// Assignment operations

	Vector4<T> &operator=(const Vector4<T> &v);
	Vector4<T> &operator+=(const Vector4<T> &v);
	Vector4<T> &operator-=(const Vector4<T> &v);
	Vector4<T> &operator*=(const Vector4<T> &v);
	Vector4<T> &operator/=(const Vector4<T> &v);

	// Comparison operations
	
	bool NearlyEquals(const Vector4<T>& v, const T& epsilon = Epsilon<T>()) const;
	int Compare(const Vector4<T> &v) const;
	bool operator==(const Vector4<T> &v) const;
	bool operator!=(const Vector4<T> &v) const;
	bool operator <(const Vector4<T> &v) const;
	bool operator >(const Vector4<T> &v) const;
	bool operator <=(const Vector4<T> &v) const;
	bool operator >=(const Vector4<T> &v) const;

	// Scalar binary operations

	Vector4<T> operator+(T s) const;
	template <typename X>
	friend Vector4<X> operator+(X s, const Vector4<X> &v);
	Vector4<T> operator-(T s) const;
	template <typename X>
	friend Vector4<X> operator-(X s, const Vector4<X> &v);
	Vector4<T> operator*(T s) const;
	template <typename X>
	friend Vector4<X> operator*(X s, const Vector4<X> &v);
	Vector4<T> operator/(T s) const;

	// Scalar assignment operations

	Vector4<T> &operator+=(T s);
	Vector4<T> &operator-=(T s);
	Vector4<T> &operator*=(T s);
	Vector4<T> &operator/=(T s);

protected:

	T m_w;

};
	
//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector4<T> friend functions
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Vector4<T> operator+(T s, const Vector4<T> &v);

template <typename T>
Vector4<T> operator-(T s, const Vector4<T> &v);

template <typename T>
Vector4<T> operator*(T s, const Vector4<T> &v);
	
} // math


#include "../PopPack.h"
#include "Vector.inl"
