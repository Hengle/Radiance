// Vector.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#include "Math.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector2
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
const T Vector2<T>::UnitEpsilon = T(0.0005); 

template <typename T>
const Vector2<T> Vector2<T>::Zero = Vector2<T>(T(0.0), T(0.0));

template <typename T>
const Vector3<T> Vector3<T>::Zero = Vector3<T>(T(0.0), T(0.0), T(0.0));

template <typename T>
const Vector4<T> Vector4<T>::Zero = Vector4<T>(T(0.0), T(0.0), T(0.0), T(0.0));

template <typename T>
inline Vector2<T> operator+(T s, const Vector2<T> &v)
{
	return (v + s);
}

template <typename T>
inline Vector2<T> operator-(T s, const Vector2<T> &v)
{
	return Vector2<T>(s - v.m_x, s - v.m_y);
}

template <typename T>
inline Vector2<T> operator*(T s, const Vector2<T> &v)
{
	return (v * s);
}

template <typename T>
inline Vector2<T>::Vector2()
{
}

template <typename T>
inline Vector2<T>::Vector2(const T *v) :
m_x(v[0]),
m_y(v[1])
{
}

template <typename T>
inline Vector2<T>::Vector2(T x, T y) :
m_x(x),
m_y(y)
{
}

template <typename T>
inline Vector2<T>::Vector2(const Vector2<T> &v) :
m_x(v.m_x),
m_y(v.m_y)
{
}

template <typename T>
inline Vector2<T>::~Vector2()
{
}

template <typename T>
inline T Vector2<T>::X() const
{
	return m_x;
}

template <typename T>
inline void Vector2<T>::SetX(T x)
{
	m_x = x;
}

template <typename T>
inline T Vector2<T>::Y() const
{
	return m_y;
}

template <typename T>
inline void Vector2<T>::SetY(T y)
{
	m_y = y;
}

template <typename T>
inline const T &Vector2<T>::operator[](int i) const
{
	RAD_ASSERT(i >= 0 && i < 2);
	return (&m_x)[i];
}

template <typename T>
inline T &Vector2<T>::operator[](int i)
{
	RAD_ASSERT(i >= 0 && i < 2);
	return (&m_x)[i];
}

template <typename T>
inline Vector2<T> &Vector2<T>::Initialize()
{
	m_x = m_y = T(0.0);
	return *this;
}

template <typename T>
inline Vector2<T> &Vector2<T>::Initialize(const T *v)
{
	m_x = v[0];
	m_y = v[1];
	return *this;
}

template <typename T>
inline Vector2<T> &Vector2<T>::Initialize(T x, T y)
{
	m_x = x;
	m_y = y;
	return *this;
}

template <typename T>
inline T Vector2<T>::Normalize()
{
	T m = Magnitude();
	*this /= m;
	return m;
}

template <typename T>
inline Vector2<T> &Vector2<T>::Reverse()
{
	m_x = -m_x;
	m_y = -m_y;
	return *this;
}

template <typename T>
Vector2<T> Vector2<T>::Reflect(const Vector2<T> &v) const
{
	return (v - *this * 2 * Dot(v));
}

template <typename T>
inline Vector2<T> Vector2<T>::operator-() const
{
	return Vector2<T>(-m_x, -m_y);
}

template <typename T>
inline T Vector2<T>::MagnitudeSquared() const
{
	return Dot(*this);
}

template <typename T>
inline T Vector2<T>::Magnitude() const
{
	return SquareRoot(MagnitudeSquared());
}

template <typename T>
inline Vector2<T> Vector2<T>::Unit() const
{
	return (*this / Magnitude());
}

template <typename T>
inline bool Vector2<T>::IsUnit(const T &epsilon) const
{
	return math::NearlyEquals(MagnitudeSquared(), T(1), epsilon);
}

template <typename T>
inline Vector2<T> Vector2<T>::operator+(const Vector2<T> &v) const
{
	return Vector2<T>(m_x + v.m_x, m_y + v.m_y);
}

template <typename T>
inline Vector2<T> Vector2<T>::operator-(const Vector2<T> &v) const
{
	return Vector2<T>(m_x - v.m_x,	m_y - v.m_y);
}

template <typename T>
inline Vector2<T> Vector2<T>::operator*(const Vector2<T> &v) const
{
	return Vector2<T>(m_x * v.m_x,	m_y * v.m_y);
}

template <typename T>
inline Vector2<T> Vector2<T>::operator/(const Vector2<T> &v) const
{
	return Vector2<T>(m_x / v.m_x,	m_y / v.m_y);
}

template <typename T>
inline T Vector2<T>::Dot(const Vector2<T> &v) const
{
	return (m_x * v.m_x + m_y * v.m_y);
}
template <typename T>
inline Vector2<T> &Vector2<T>::operator=(const Vector2<T> &v)
{
	m_x = v.m_x;
	m_y = v.m_y;
	return *this;
}

template <typename T>
inline Vector2<T> &Vector2<T>::operator+=(const Vector2<T> &v)
{
	m_x += v.m_x;
	m_y += v.m_y;
	return *this;
}
template <typename T>
inline Vector2<T> &Vector2<T>::operator-=(const Vector2<T> &v)
{
	m_x -= v.m_x;
	m_y -= v.m_y;
	return *this;
}

template <typename T>
inline Vector2<T> &Vector2<T>::operator*=(const Vector2<T> &v)
{
	m_x *= v.m_x;
	m_y *= v.m_y;
	return *this;
}

template <typename T>
inline Vector2<T> &Vector2<T>::operator/=(const Vector2<T> &v)
{
	m_x /= v.m_x;
	m_y /= v.m_y;
	return *this;
}

template <typename T>
inline bool Vector2<T>::NearlyEquals(const Vector2<T>& v, const T& epsilon) const
{
	return ((v - *this).MagnitudeSquared() < Square(epsilon));
}

template <typename T>
inline int Vector2<T>::Compare(const Vector2<T> &v) const {
	if (m_x < v.m_x)
		return -1;
	if (m_x > v.m_x)
		return 1;
	if (m_y < v.m_y)
		return -1;
	if (m_y > v.m_y)
		return 1;
	return 0;
}

template <typename T>
inline bool Vector2<T>::operator==(const Vector2<T> &v) const
{
	return Compare(v) == 0;
}

template <typename T>
inline bool Vector2<T>::operator!=(const Vector2<T> &v) const
{
	return Compare(v) != 0;
}

template <typename T>
bool Vector2<T>::operator <(const Vector2<T> &v) const {
	return Compare(v) < 0;
}

template <typename T>
bool Vector2<T>::operator >(const Vector2<T> &v) const {
	return Compare(v) > 0;
}

template <typename T>
bool Vector2<T>::operator <=(const Vector2<T> &v) const {
	return Compare(v) <= 0;
}

template <typename T>
bool Vector2<T>::operator >=(const Vector2<T> &v) const {
	return Compare(v) >= 0;
}

template <typename T>
inline Vector2<T> Vector2<T>::operator+(T s) const
{
	return Vector2<T>(m_x + s, m_y + s);
}

template <typename T>
inline Vector2<T> Vector2<T>::operator-(T s) const
{
	return Vector2<T>(m_x - s, m_y - s);
}

template <typename T>
inline Vector2<T> Vector2<T>::operator*(T s) const
{
	return Vector2<T>(m_x * s, m_y * s);
}

template <typename T>
inline Vector2<T> Vector2<T>::operator/(T s) const
{
	return operator*(Reciprocal(s));
}

template <typename T>
inline Vector2<T> &Vector2<T>::operator+=(T s)
{
	m_x += s;
	m_y += s;
	return *this;	 
}

template <typename T>
inline Vector2<T> &Vector2<T>::operator-=(T s)
{
	m_x -= s;
	m_y -= s;
	return *this;	
}


template <typename T>
inline Vector2<T> &Vector2<T>::operator*=(T s)
{
	m_x *= s;
	m_y *= s;
	return *this;
}

template <typename T>
inline Vector2<T> &Vector2<T>::operator/=(T s)
{
	return operator*=(Reciprocal(s));
}
template <typename T>
inline Vector3<T> operator+(T s, const Vector3<T> &v)
{
	return (v + s);
}

template <typename T>
inline Vector3<T> operator-(T s, const Vector3<T> &v)
{
	return Vector3<T>(s - v.m_x, s - v.m_y, s - v.m_z);
}

template <typename T>
inline Vector3<T> operator*(T s, const Vector3<T> &v)
{
	return (v * s);
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector3
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector3<T>::Vector3()
{
}

template <typename T>
inline Vector3<T>::Vector3(const T *v) :
Vector2<T>(v),
m_z(v[2])
{
}

template <typename T>
inline Vector3<T>::Vector3(T x, T y, T z) :
Vector2<T>(x, y),
m_z(z)
{
}

template <typename T>
inline Vector3<T>::Vector3(const Vector2<T> &v, T z) :
Vector2<T>(v),
m_z(z)
{
}

template <typename T>
inline Vector3<T>::Vector3(const Vector3<T> &v) :
Vector2<T>(v),
m_z(v.m_z)
{
}

template <typename T>
inline Vector3<T>::~Vector3()
{
}

template <typename T>
inline T Vector3<T>::Z() const
{
	return m_z;
}

template <typename T>
inline void Vector3<T>::SetZ(T z)
{
	m_z = z;
}

template <typename T>
inline const T &Vector3<T>::operator[](int i) const
{
	RAD_ASSERT(i >= 0 && i < 3);
	return (&this->m_x)[i];
}

template <typename T>
inline T &Vector3<T>::operator[](int i)
{
	RAD_ASSERT(i >= 0 && i < 3);
	return (&this->m_x)[i];
}

template <typename T>
inline Vector3<T> &Vector3<T>::Initialize()
{
	Vector2<T>::Initialize();
	m_z = T(0.0);
	return *this;
}

template <typename T>
inline Vector3<T> &Vector3<T>::Initialize(const T *v)
{
	Vector2<T>::Initialize(v);
	m_z = v[2];
	return *this;
}

template <typename T>
inline Vector3<T> &Vector3<T>::Initialize(T x, T y, T z)
{
	Vector2<T>::Initialize(x, y);
	m_z = z;
	return *this;
}

template <typename T>
inline Vector3<T> &Vector3<T>::Initialize(const Vector2<T> &v, T z)
{
	Vector2<T>::operator=(v);
	m_z = z;
	return *this;
}

template <typename T>
inline T Vector3<T>::Normalize()
{
	T m = Magnitude();
	*this /= m;
	return m;
}

template <typename T>
inline Vector3<T> &Vector3<T>::Reverse()
{
	Vector2<T>::Reverse();
	m_z = -m_z;
	return *this;
}

template <typename T>
inline Vector3<T> Vector3<T>::operator-() const
{
	return Vector3<T>(Vector2<T>::operator-(), -m_z);
}

template <typename T>
inline T Vector3<T>::MagnitudeSquared() const
{
	return Dot(*this);
}

template <typename T>
inline T Vector3<T>::Magnitude() const
{
	return SquareRoot(MagnitudeSquared());
}

template <typename T>
inline T Vector3<T>::ApproximateMagnitude() const
{
	T tmp;	
	T max = abs(this->m_x);
	T mid = abs(this->m_y);
	T min = abs(this->m_z);	
	if (max < mid)
	{
		tmp = max;
		max = mid;
		mid = tmp;
	}	
	if (max < min)
	{
		tmp = max;
		max = min;
		min = tmp;
	}	
	return (max + ((mid + min) / T(4.0)));
}

template <typename T>
inline Vector3<T> Vector3<T>::Unit() const
{
	return (*this / Magnitude());
}

template <typename T>
inline bool Vector3<T>::IsUnit(const T &epsilon) const
{
	return math::NearlyEquals(MagnitudeSquared(), T(1), epsilon);
}

template <typename T>
inline Vector3<T> Vector3<T>::operator+(const Vector3<T> &v) const
{
	return Vector3<T>(Vector2<T>::operator+(v), m_z + v.m_z);
}

template <typename T>
inline Vector3<T> Vector3<T>::operator-(const Vector3<T> &v) const
{
	return Vector3<T>(Vector2<T>::operator-(v),	m_z - v.m_z);
}

template <typename T>
inline Vector3<T> Vector3<T>::operator*(const Vector3<T> &v) const
{
	return Vector3<T>(Vector2<T>::operator*(v),	m_z * v.m_z);
}

template <typename T>
inline Vector3<T> Vector3<T>::operator/(const Vector3<T> &v) const
{
	return Vector3<T>(Vector2<T>::operator/(v),	m_z / v.m_z);
}

template <typename T>
inline Vector3<T> Vector3<T>::Cross(const Vector3<T> &v) const
{
	return Vector3<T>(
		this->m_y * v.m_z - this->m_z * v.m_y,
		this->m_z * v.m_x - this->m_x * v.m_z,
		this->m_x * v.m_y - this->m_y * v.m_x
	);
}

template <typename T>
inline T Vector3<T>::Dot(const Vector3<T> &v) const
{
	return (Vector2<T>::Dot(v) + m_z * v.m_z);
}

template <typename T>
inline Vector3<T> Vector3<T>::Reflect(const Vector3<T> &v) const
{
	return (v - *this * 2 * Dot(v));	
}

template <typename T>
inline Vector3<T> Vector3<T>::Project(const Vector3<T> &v) const
{
	Vector3<T> proj = Cross(v.Cross(*this));
	return (proj * (v.Dot(proj.Unit())));
}

template <typename T>
inline Vector3<T> &Vector3<T>::operator=(const Vector3<T> &v)
{
	Vector2<T>::operator=(v);
	m_z = v.m_z;
	return *this;
}

template <typename T>
inline Vector3<T> &Vector3<T>::operator+=(const Vector3<T> &v)
{
	Vector2<T>::operator+=(v);
	m_z += v.m_z;
	return *this;
}

template <typename T>
inline Vector3<T> &Vector3<T>::operator-=(const Vector3<T> &v)
{
	Vector2<T>::operator-=(v);
	m_z -= v.m_z;
	return *this;
}

template <typename T>
inline Vector3<T> &Vector3<T>::operator*=(const Vector3<T> &v)
{
	Vector2<T>::operator*=(v);
	m_z *= v.m_z;
	return *this;
}

template <typename T>
inline Vector3<T> &Vector3<T>::operator/=(const Vector3<T> &v)
{
	Vector2<T>::operator/=(v);
	m_z /= v.m_z;
	return *this;
}

template <typename T>
inline bool Vector3<T>::NearlyEquals(const Vector3<T>& v, const T& epsilon) const
{
	return ((v - *this).MagnitudeSquared() < Square(epsilon));
}

template <typename T>
inline int Vector3<T>::Compare(const Vector3<T> &v) const {
	if (this->m_x < v.m_x)
		return -1;
	if (this->m_x > v.m_x)
		return 1;
	if (this->m_y < v.m_y)
		return -1;
	if (this->m_y > v.m_y)
		return 1;
	if (this->m_z < v.m_z)
		return -1;
	if (this->m_z > v.m_z)
		return 1;
	return 0;
}

template <typename T>
inline bool Vector3<T>::operator==(const Vector3<T> &v) const
{
	return Compare(v) == 0;
}

template <typename T>
inline bool Vector3<T>::operator!=(const Vector3<T> &v) const
{
	return Compare(v) != 0;
}

template <typename T>
bool Vector3<T>::operator <(const Vector3<T> &v) const {
	return Compare(v) < 0;
}

template <typename T>
bool Vector3<T>::operator >(const Vector3<T> &v) const {
	return Compare(v) > 0;
}

template <typename T>
bool Vector3<T>::operator <=(const Vector3<T> &v) const {
	return Compare(v) <= 0;
}

template <typename T>
bool Vector3<T>::operator >=(const Vector3<T> &v) const {
	return Compare(v) >= 0;
}

template <typename T>
inline Vector3<T> Vector3<T>::operator+(T s) const
{
	return Vector3<T>(this->m_x + s, this->m_y + s, this->m_z + s);
}

template <typename T>
inline Vector3<T> Vector3<T>::operator-(T s) const
{
	return Vector3<T>(this->m_x - s, this->m_y - s, this->m_z - s);
}

template <typename T>
inline Vector3<T> Vector3<T>::operator*(T s) const
{
	return Vector3<T>(this->m_x * s, this->m_y * s, this->m_z * s);
}

template <typename T>
inline Vector3<T> Vector3<T>::operator/(T s) const
{
	return operator*(Reciprocal(s));
}

template <typename T>
inline Vector3<T> &Vector3<T>::operator+=(T s)
{
	Vector2<T>::operator+=(s);
	m_z += s;
	return *this;
}

template <typename T>
inline Vector3<T> &Vector3<T>::operator-=(T s)
{
	Vector2<T>::operator-=(s);
	m_z -= s;
	return *this;
}

template <typename T>
inline Vector3<T> &Vector3<T>::operator*=(T s)
{
	Vector2<T>::operator*=(s);
	m_z *= s;
	return *this;
}

template <typename T>
inline Vector3<T> &Vector3<T>::operator/=(T s)
{
	return operator*=(Reciprocal(s));
}

template <typename T>
const Vector3<T> &Vector3<T>::FrameVecs(Vector3<T> &up, Vector3<T> &left) const
{
	int axis = 0;
	T max = Abs(this->m_x);
	T z = Abs(this->m_y);
	
	if (z > max)
	{
		max = z;
		axis = 1;
	}
	
	z = Abs(m_z);
	if (z > max)
	{
		max = z;
		axis = 2;
	}

	switch (axis)
	{
	case 0:
	case 1:
		up.Initialize(T(0), T(0), T(1));
		break;
	case 2:
		up.Initialize(T(0), T(1), T(0));
	}

	// push 'up' into plane.
	z = up.Dot(*this);
	up = up + ((*this) * -z);
	up.Normalize();
	left = up.Cross(*this);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// math::Vector4
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Vector4<T> operator+(T s, const Vector4<T> &v)
{
	return (v + s);
}

template <typename T>
inline Vector4<T> operator-(T s, const Vector4<T> &v)
{
	return Vector4<T>(s - v.m_x, s - v.m_y, s - v.m_z, s - v.m_w);
}

template <typename T>
inline Vector4<T> operator*(T s, const Vector4<T> &v)
{
	return (v * s);
}

template <typename T>
inline Vector4<T>::Vector4()
{
}

template <typename T>
inline Vector4<T>::Vector4(const T *v) :
Vector3<T>(v),
m_w(v[3])
{
}

template <typename T>
inline Vector4<T>::Vector4(T x, T y, T z, T w) :
Vector3<T>(x, y, z),
m_w(w)
{
}

template <typename T>
inline Vector4<T>::Vector4(const Vector2<T> &v, T z, T w) :
Vector3<T>(v, z),
m_w(w)
{
}

template <typename T>
inline Vector4<T>::Vector4(const Vector3<T> &v, T w) :
Vector3<T>(v),
m_w(w)
{
}

template <typename T>
inline Vector4<T>::Vector4(const Vector4<T> &v) :
Vector3<T>(v),
m_w(v.m_w)
{
}

template <typename T>
inline Vector4<T>::~Vector4()
{
}

template <typename T>
inline T Vector4<T>::W() const
{
	return m_w;
}

template <typename T>
inline void Vector4<T>::SetW(T w)
{
	m_w = w;
}

template <typename T>
inline const T &Vector4<T>::operator[](int i) const
{
	RAD_ASSERT(i >= 0 && i < 4);
	return (&this->m_x)[i];
}

template <typename T>
inline T &Vector4<T>::operator[](int i)
{
	RAD_ASSERT(i >= 0 && i < 4);
	return (&this->m_x)[i];
}

template <typename T>
inline Vector4<T> &Vector4<T>::Initialize()
{
	Vector3<T>::Initialize();
	m_w = T(0.0);
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::Initialize(const T *v)
{
	Vector3<T>::Initialize(v);
	m_w = v[3];
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::Initialize(T x, T y, T z, T w)
{
	Vector3<T>::Initialize(x, y, z);
	m_w = w;
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::Initialize(const Vector2<T> &v, T z, T w)
{
	Vector3<T>::Initialize(v, z);
	m_w = w;
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::Initialize(const Vector3<T> &v, T w)
{
	Vector3<T>::operator=(v);
	m_w = w;
	return *this;
}

template <typename T>
inline T Vector4<T>::Normalize()
{
	T m = Magnitude();
	*this /= m;
	return m;
}

template <typename T>
inline Vector4<T> &Vector4<T>::Reverse()
{
	Vector3<T>::Reverse();
	m_w = -m_w;
	return *this;
}

template <typename T>
inline Vector4<T> Vector4<T>::operator-() const
{
	return Vector4<T>(Vector3<T>::operator-(), -m_w);
}

template <typename T>
inline T Vector4<T>::MagnitudeSquared() const
{
	return Dot(*this);
}

template <typename T>
inline T Vector4<T>::Magnitude() const
{
	return (SquareRoot(MagnitudeSquared()));
}

template <typename T>
inline Vector4<T> Vector4<T>::Unit() const
{
	return (*this / Magnitude());
}

template <typename T>
inline bool Vector4<T>::IsUnit(const T &epsilon) const
{
	return math::NearlyEquals(MagnitudeSquared(), T(1), epsilon);
}

template <typename T>
inline Vector4<T> Vector4<T>::operator+(const Vector4<T> &v) const
{
	return Vector4<T>(Vector3<T>::operator+(v), m_w + v.m_w);
}

template <typename T>
inline Vector4<T> Vector4<T>::operator-(const Vector4<T> &v) const
{
	return Vector4<T>(Vector3<T>::operator-(v),	m_w - v.m_w);
}

template <typename T>
inline Vector4<T> Vector4<T>::operator*(const Vector4<T> &v) const
{
	return Vector4<T>(Vector3<T>::operator*(v),	m_w * v.m_w);
}

template <typename T>
inline Vector4<T> Vector4<T>::operator/(const Vector4<T> &v) const
{
	return Vector4<T>(Vector3<T>::operator/(v),	m_w / v.m_w);
}

template <typename T>
inline T Vector4<T>::Dot(const Vector4<T> &v) const
{
	return (Vector3<T>::Dot(v) + m_w * v.m_w);
}

template <typename T>
inline Vector4<T> &Vector4<T>::operator=(const Vector4<T> &v)
{
	Vector3<T>::operator=(v);
	m_w = v.m_w;
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::operator+=(const Vector4<T> &v)
{
	Vector3<T>::operator+=(v);
	m_w += v.m_w;
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::operator-=(const Vector4<T> &v)
{
	Vector3<T>::operator-=(v);
	m_w -= v.m_w;
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::operator*=(const Vector4<T> &v)
{
	Vector3<T>::operator*=(v);
	m_w *= v.m_w;
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::operator/=(const Vector4<T> &v)
{
	Vector3<T>::operator/=(v);
	m_w /= v.m_w;
	return *this;
}

template <typename T>
inline bool Vector4<T>::NearlyEquals(const Vector4<T>& v, const T& epsilon) const
{
	return ((v - *this).MagnitudeSquared() < Square(epsilon));
}

template <typename T>
inline int Vector4<T>::Compare(const Vector4<T> &v) const {
	if (this->m_x < v.m_x)
		return -1;
	if (this->m_x > v.m_x)
		return 1;
	if (this->m_y < v.m_y)
		return -1;
	if (this->m_y > v.m_y)
		return 1;
	if (this->m_z < v.m_z)
		return -1;
	if (this->m_z > v.m_z)
		return 1;
	if (this->m_w < v.m_w)
		return -1;
	if (this->m_w > v.m_w)
		return 1;
	return 0;
}

template <typename T>
inline bool Vector4<T>::operator==(const Vector4<T> &v) const
{
	return Compare(v) == 0;
}

template <typename T>
inline bool Vector4<T>::operator!=(const Vector4<T> &v) const
{
	return Compare(v) != 0;
}

template <typename T>
bool Vector4<T>::operator <(const Vector4<T> &v) const {
	return Compare(v) < 0;
}

template <typename T>
bool Vector4<T>::operator >(const Vector4<T> &v) const {
	return Compare(v) > 0;
}

template <typename T>
bool Vector4<T>::operator <=(const Vector4<T> &v) const {
	return Compare(v) <= 0;
}

template <typename T>
bool Vector4<T>::operator >=(const Vector4<T> &v) const {
	return Compare(v) >= 0;
}

template <typename T>
inline Vector4<T> Vector4<T>::operator+(T s) const
{
	return Vector4<T>(this->m_x + s, this->m_y + s, this->m_z + s, this->m_w + s);
}

template <typename T>
inline Vector4<T> Vector4<T>::operator-(T s) const
{
	return Vector4<T>(this->m_x - s, this->m_y - s, this->m_z - s, this->m_w - s);
}

template <typename T>
inline Vector4<T> Vector4<T>::operator*(T s) const
{
	return Vector4<T>(this->m_x * s, this->m_y * s, this->m_z * s, this->m_w * s);
}

template <typename T>
inline Vector4<T> Vector4<T>::operator/(T s) const
{
	return operator*(Reciprocal(s));
}

template <typename T>
inline Vector4<T> &Vector4<T>::operator+=(T s)
{
	Vector3<T>::operator+=(s);
	this->m_w += s;
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::operator-=(T s)
{
	Vector3<T>::operator-=(s);
	this->m_w -= s;
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::operator*=(T s)
{
	Vector3<T>::operator*=(s);
	this->m_w *= s;
	return *this;
}

template <typename T>
inline Vector4<T> &Vector4<T>::operator/=(T s)
{
	return operator*=(Reciprocal(s));
}

} // math

