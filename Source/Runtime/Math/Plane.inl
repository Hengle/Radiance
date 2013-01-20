// Plane.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Math.h"


namespace math {

template <typename T>
const Plane<T> Plane<T>::X = Plane<T>(Vector3<T>(T(1.0), T(0.0), T(0.0)), T(0.0));

template <typename T>
const Plane<T> Plane<T>::Y = Plane<T>(Vector3<T>(T(0.0), T(1.0), T(0.0)), T(0.0));

template <typename T>
const Plane<T> Plane<T>::Z = Plane<T>(Vector3<T>(T(0.0), T(0.0), T(1.0)), T(0.0));

template <typename T>
inline Plane<T>::Plane()
{
}
	
template <typename T>
inline Plane<T>::Plane(T a, T b, T c, T d) :
m_normal(a, b, c),
m_d(d)
{
}

template <typename T>
inline Plane<T>::Plane(const Vector3<T> &normal, T d)
{
	Initialize(normal, d);
}

template <typename T>
inline Plane<T>::Plane(const Vector3<T> &normal, const Vector3<T> &p)
{
	Initialize(normal, p);
}

template <typename T>
inline Plane<T>::Plane(const Vector3<T> &normal, const Vector3<T> &p, UnitHint u)
{
	Initialize(normal, p, u);
}
	
template <typename T>
inline Plane<T>::Plane(const Vector3<T> &p1, const Vector3<T> &p2, const Vector3<T> &p3)
{
	Initialize(p1, p2, p3);
}
	
template <typename T>
inline Plane<T>::Plane(const Plane<T> &p) :
m_normal(p.m_normal),
m_d(p.m_d)
{
}
	
template <typename T>
inline Plane<T>::~Plane()
{
}

template <typename T>
inline const Vector3<T> &Plane<T>::Normal() const
{
	return m_normal;
}

template <typename T>
inline T Plane<T>::A() const
{
	return m_normal.X();
}

template <typename T>
inline T Plane<T>::B() const
{
	return m_normal.Y();
}

template <typename T>
inline T Plane<T>::C() const
{
	return m_normal.Z();
}

template <typename T>
inline T Plane<T>::D() const
{
	return m_d;
}

template <typename T>
inline Plane<T> &Plane<T>::Initialize(T a, T b, T c, T d)
{
	m_normal.Initialize(a, b, c);
	m_d = d;
	return *this;
}

template <typename T>
inline Plane<T> &Plane<T>::Initialize(const Vector3<T> &normal, T d)
{
	m_normal = normal;
	m_d = d;
	return *this;
}

template <typename T>
inline Plane<T> &Plane<T>::Initialize(const Vector3<T> &normal, const Vector3<T> &p)
{
	m_normal = normal.Unit();
	m_d = m_normal.Dot(p); 
	return *this;
}

template <typename T>
inline Plane<T> &Plane<T>::Initialize(const Vector3<T> &normal, const Vector3<T> &p, UnitHint u)
{
	m_normal = normal;
	m_d = m_normal.Dot(p);
	return *this;
}

template <typename T>
inline Plane<T> &Plane<T>::Initialize(const Vector3<T> &p1, const Vector3<T> &p2, const Vector3<T> &p3)
{
	m_normal = p3 - p1;
	m_normal = m_normal.Cross(p2 - p1);
	m_normal.Normalize();
	m_d = m_normal.Dot(p1);
	return *this;
}
	
template <typename T>
inline Plane<T> &Plane<T>::Flip()
{
	m_normal = -m_normal;
	m_d = -m_d;
	return *this;
}

template <typename T>
inline Plane<T> &Plane<T>::Translate(T d)
{
	m_d += d;
	return *this;
}

template <typename T>
inline Plane<T> &Plane<T>::Translate(const Vector3<T> &t)
{
	m_d += m_normal.Dot(t);
	return *this;
}

template <typename T>
inline Plane<T> Plane<T>::operator-() const
{
	return Plane<T>(-m_normal, -m_d);
}

template <typename T>
template <typename TVector3>
inline TVector3 Plane<T>::Reflect(const TVector3 &v) const
{
	TVector3 c(m_normal);
	return c.Reflect(v);
}

template <typename T>
template <typename TVector3>
inline TVector3 Plane<T>::Project(const TVector3 &v) const
{
	TVector3 t = m_normal * -(m_normal.Dot(v) - m_d);
	return v + t;
}

template <typename T>
template <typename TVector3>
bool Plane<T>::IntersectLineSegment(TVector3 &result, const TVector3 &a, const TVector3 &b, const T &tolerance) const
{
	return IntersectLineSegment(result, a, Distance(a), b, Distance(b), tolerance);
}

template <typename T>
template <typename TVector3>
bool Plane<T>::IntersectLineSegment(TVector3 &result, const TVector3 &a, const T &distA, const TVector3 &b, const T &distB, const T &tolerance)
{
	if ((Abs(distA) <= tolerance) || (Abs(distB) <= tolerance)) 
		return false; // doesn't cross.
	if (SignBits(distA) == SignBits(distB)) 
		return false; // doesn't cross.
	result = IntersectLineSegment(a, distA, b, distB);
	return true;
}

template <typename T>
template <typename TVector3>
TVector3 Plane<T>::IntersectLineSegment(const TVector3 &a, const T &distA, const TVector3 &b, const T &distB)
{
	return Lerp(a, b, distA / (distA - distB));
}

template <typename T>
template <typename TVector3>
bool Plane<T>::IntersectRay(TVector3 &result, const TVector3 &rayDirection, const TVector3 &rayOrigin, const T &tolerance) const
{
    const T NdotV = m_normal.Dot(rayDirection);
    if (math::NearlyZero(NdotV, tolerance)) 
		return false;
    T t = (m_d - rayOrigin.Dot(m_normal)) / NdotV;
    if (t < T(0.0))
		return false;
    result = t * rayDirection + rayOrigin;
    return true;
}

template <typename T>
inline T Plane<T>::Distance(const Vector3<T> &v) const
{
	return (m_normal.Dot(v) - m_d);
}

template <typename T>
inline typename Plane<T>::SideType Plane<T>::Side(const  Vector3<T> &v, const T &tolerance) const
{
	T distance = Distance(v);
	if (distance > tolerance)
		return Front;
	if (distance < -tolerance)
		return Back;
	return On;
}

template <typename T>
inline typename Plane<T>::SideType Plane<T>::Side(const AABB3<T> &bounds, const T &tolerance) const
{
	Vector3<T> pts[2];

	// min/max
	if (m_normal[0] >= ValueType(0))
	{
		pts[0].SetX(bounds.Mins().X());
		pts[1].SetX(bounds.Maxs().X());
	}
	else
	{
		pts[0].SetX(bounds.Maxs().X());
		pts[1].SetX(bounds.Mins().X());
	}

	if (m_normal[1] >= ValueType(0))
	{
		pts[0].SetY(bounds.Mins().Y());
		pts[1].SetY(bounds.Maxs().Y());
	}
	else
	{
		pts[0].SetY(bounds.Maxs().Y());
		pts[1].SetY(bounds.Mins().Y());
	}

	if (m_normal[2] >= ValueType(0))
	{
		pts[0].SetZ(bounds.Mins().Z());
		pts[1].SetZ(bounds.Maxs().Z());
	}
	else
	{
		pts[0].SetZ(bounds.Maxs().Z());
		pts[1].SetZ(bounds.Mins().Z());
	}

	SideType a = Side(pts[0], tolerance);
	SideType b = Side(pts[1], tolerance);

	if (a == b) 
		return a;
	if (a == On) 
		return b;
	if (b == On) 
		return a;
	return Cross;
}

template <typename T>
inline bool Plane<T>::IsOn(const  Vector3<T> &v, const T &tolerance) const
{
	return (Abs(Distance(v)) <= tolerance);
}

template <typename T>
inline bool Plane<T>::NearlyEquals(const Plane<T> &p, const T &normalEpsilon, const T &distanceEpsilon) const
{
	return m_normal.NearlyEquals(p.m_normal, normalEpsilon) && math::NearlyEquals(m_d, p.m_d, distanceEpsilon);
}

template <typename T>
inline bool Plane<T>::operator==(const Plane<T> &p) const
{
	return (m_normal == p.m_normal) && (m_d == p.m_d);
}

template <typename T>
inline bool Plane<T>::operator!=(const Plane<T> &p) const
{
	return (m_normal != p.m_normal) || (m_d != p.m_d);
}

} // math
