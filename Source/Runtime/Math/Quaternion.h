// Quaternion.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Vector.h"
#include "AxisAngle.h"
#include "QuaternionDef.h"
#include "../PushPack.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math::Quaternion<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Quaternion
{

public:

	// Constants

	static const Quaternion<T> Identity;

	// Constructors

	Quaternion();
	explicit Quaternion(const T *v);
	Quaternion(T x, T y, T z, T w);
	explicit Quaternion(const Vector4<T> &v);
	explicit Quaternion(const Vector3<T> &v, const T &rad);
	Quaternion(const Quaternion<T> &q);
	
	// Destructor

	~Quaternion();

	// Conversion

	AxisAngle<T> ToAxisAngle(const T &epsilon = T(0.0005)) const;

	// Access

	const T &operator[](int i) const;
	T &operator[](int i);
	
	T X() const;
	void SetX(T x);
	T Y() const;
	void SetY(T y);
	T Z() const;
	void SetZ(T z);
	T W() const;
	void SetW(T w);

	// In place operations

	Quaternion<T> &Initialize(const T *v);
	Quaternion<T> &Initialize(T xx, T yy, T zz, T ww);
	Quaternion<T> &Initialize(const Vector4<T> &v);
	Quaternion<T> &Initialize(const Vector3<T> &v, const T &rad);
	Quaternion<T> &MakeIdentity();
	Quaternion<T> &MakeConjugate();
	Quaternion<T> &Normalize();
	Quaternion<T> &Reverse();
	Quaternion<T> &Invert();
		
	// Unary operations

	Quaternion<T> operator-() const;
	T Magnitude() const;
	T MagnitudeSquared() const;
	Quaternion<T> Conjugate() const;
	Quaternion<T> Unit() const;
	Quaternion<T> Inverse() const;

	// Binary operations

	Quaternion<T> operator+(const Quaternion<T> &q) const;
	Quaternion<T> operator-(const Quaternion<T> &q) const;
	Quaternion<T> operator*(const Quaternion<T> &q) const;

	// Assignment operations

	Quaternion<T> &operator=(const Quaternion<T> &q);
	Quaternion<T> &operator+=(const Quaternion<T> &q);
	Quaternion<T> &operator-=(const Quaternion<T> &q);
	Quaternion<T> &operator*=(const Quaternion<T> &q);
	
	// Scalar binary operations

	Quaternion<T> operator*(T s) const;
	template <typename X>
	friend Quaternion<X> operator*(X s, const Quaternion<X> &q);
	Quaternion<T> operator/(T s) const;

	// Scalar assignment operations

	Quaternion<T> &operator*=(T s);
	Quaternion<T> &operator/=(T s);

	// Comparison operations

	bool operator==(const Quaternion<T> &v) const;
	bool operator!=(const Quaternion<T> &v) const;

protected:

	Vector4<T> m_v;

	template <typename X>
	friend Quaternion<X> QuaternionMid(const Quaternion<X> &p, const Quaternion<X> &q);

	template <typename X>
	friend Quaternion<X> Slerp(const Quaternion<X> &p, const Quaternion<X> &q, T t);
};
	
//////////////////////////////////////////////////////////////////////////////////////////
// math::Quaternion<T> friend functions
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Quaternion<T> operator*(T s, const Quaternion<T> &q);

template <typename T>
Quaternion<T> Slerp(const Quaternion<T> &from, const Quaternion<T> &to, T t);

} // math


#include "../PopPack.h"
#include "Quaternion.inl"
