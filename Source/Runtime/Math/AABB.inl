// AABB.inl
// Axis Aligned Bounding Box
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../PushSystemMacros.h"

namespace math {

template<typename T>
inline AABB3<T>::AABB3()
{
}

template<typename T>
inline AABB3<T>::AABB3(const AABB3& aabb) : m_mins(aabb.m_mins), m_maxs(aabb.m_maxs), m_org(aabb.m_org)
{
}

template<typename T>
inline AABB3<T>::AABB3(const Vector3<T>& mins, const Vector3<T>& maxs) : m_mins(mins), m_maxs(maxs)
{
	m_org = (m_maxs + m_mins) / T(2);
}

template<typename T>
inline AABB3<T>::AABB3(const T& minX, const T& minY, const T& minZ, const T& maxX, const T& maxY, const T& maxZ) :
m_mins(minX, minY, minZ), m_maxs(maxX, maxY, maxZ)
{
	m_org = (m_maxs + m_mins) / T(2);
}

template<typename T>
inline AABB3<T>::~AABB3()
{
}

template<typename T>
inline AABB3<T>& AABB3<T>::Initialize()
{
	m_mins.SetX(std::numeric_limits<T>::max());
	m_mins.SetY(std::numeric_limits<T>::max());
	m_mins.SetZ(std::numeric_limits<T>::max());
	m_maxs.SetX(-std::numeric_limits<T>::max());
	m_maxs.SetY(-std::numeric_limits<T>::max());
	m_maxs.SetZ(-std::numeric_limits<T>::max());
	m_org = Vector3<T>::Zero;

	return *this;
}

template<typename T>
inline AABB3<T>& AABB3<T>::Initialize(const AABB3& aabb)
{
	return Initialize(aabb.m_mins, aabb.m_maxs);
}

template<typename T>
inline AABB3<T>& AABB3<T>::Initialize(const Vector3<T>& mins, const Vector3<T>& maxs)
{
	m_mins = mins;
	m_maxs = maxs;
	m_org = (m_maxs + m_mins) / T(2);

	return *this;
}

template<typename T>
inline AABB3<T>& AABB3<T>::Initialize(const T& minX, const T& minY, const T& minZ, const T& maxX, const T& maxY, const T& maxZ)
{
	m_mins.SetX(minX);
	m_mins.SetY(minY);
	m_mins.SetZ(minZ);
	m_maxs.SetX(maxX);
	m_maxs.SetY(maxY);
	m_maxs.SetZ(maxZ);
	m_org = (m_maxs + m_mins) / T(2);

	return *this;
}

template<typename T>
inline AABB3<T>& AABB3<T>::Expand(const Vector3<T>& ex)
{
	Vector3<T> half = ex / 2.0f;
	m_mins -= half;
	m_maxs += half;
	m_org = (m_maxs + m_mins) / T(2);

	return *this;
}

template<typename T>
inline AABB3<T>& AABB3<T>::Expand(const T& expX, const T& expY, const T& expZ)
{
	return Expand(Vector3<T>(expX, expY, expZ));
}

template<typename T>
inline AABB3<T>& AABB3<T>::ExpandMin(const Vector3<T>& ex)
{
	m_mins -= ex;
	m_org = (m_maxs + m_mins) / T(2);
	return *this;
}

template<typename T>
inline AABB3<T>& AABB3<T>::ExpandMin(const T& expX, const T& expY, const T& expZ)
{
	return ExpandMin(Vector3<T>(expX, expY, expZ));
}

template<typename T>
inline AABB3<T>& AABB3<T>::ExpandMax(const Vector3<T>& ex)
{
	m_maxs += ex;
	m_org = (m_maxs + m_mins) / T(2);
	return *this;
}

template<typename T>
inline AABB3<T>& AABB3<T>::ExpandMax(const T& expX, const T& expY, const T& expZ)
{
	return ExpandMax(Vector3<T>(expX, expY, expZ));
}

template<typename T>
inline AABB3<T>& AABB3<T>::Insert(const AABB3<T>& aabb)
{
	m_mins.Initialize( (aabb.m_mins.X() < m_mins.X()) ? aabb.m_mins.X() : m_mins.X(),
		(aabb.m_mins.Y() < m_mins.Y()) ? aabb.m_mins.Y() : m_mins.Y(),
		(aabb.m_mins.Z() < m_mins.Z()) ? aabb.m_mins.Z() : m_mins.Z() );
	m_maxs.Initialize( (aabb.m_maxs.X() > m_maxs.X()) ? aabb.m_maxs.X() : m_maxs.X(),
		(aabb.m_maxs.Y() > m_maxs.Y()) ? aabb.m_maxs.Y() : m_maxs.Y(),
		(aabb.m_maxs.Z() > m_maxs.Z()) ? aabb.m_maxs.Z() : m_maxs.Z() );
	m_org = (m_maxs + m_mins) / T(2);

	return *this;
}

template<typename T>
inline AABB3<T>& AABB3<T>::Insert(const Vector3<T>& point)
{
	m_mins.Initialize( (point.X() < m_mins.X()) ? point.X() : m_mins.X(),
		(point.Y() < m_mins.Y()) ? point.Y() : m_mins.Y(),
		(point.Z() < m_mins.Z()) ? point.Z() : m_mins.Z() );
	m_maxs.Initialize( (point.X() > m_maxs.X()) ? point.X() : m_maxs.X(),
		(point.Y() > m_maxs.Y()) ? point.Y() : m_maxs.Y(),
		(point.Z() > m_maxs.Z()) ? point.Z() : m_maxs.Z() );
	m_org = (m_maxs + m_mins) / T(2);

	return *this;
}

template<typename T>
inline AABB3<T>& AABB3<T>::Insert(const T& x, const T& y, const T& z)
{
	return Insert(Vector3<T>(x, y, z));
}

template<typename T>
inline bool AABB3<T>::Contains(const Vector3<T>& point, const T epsilon) const
{
	return (((m_mins.X()-epsilon) <= point.X()) && ((m_maxs.X()+epsilon) >= point.X()) &&
		((m_mins.Y()-epsilon) <= point.Y()) && ((m_maxs.Y()+epsilon) >= point.Y()) &&
		((m_mins.Z()-epsilon) <= point.Z()) && ((m_maxs.Z()+epsilon) >= point.Z()));
}

template<typename T>
inline bool AABB3<T>::Touches(const AABB3& other, const T epsilon) const // returns true if they intersect OR the edges touch.
{
		// don't touch on X?
	if ((m_maxs.X() < other.m_mins.X()) || (other.m_maxs.X() < m_mins.X()) ||
		// don't touch on Y?
		(m_maxs.Y() < other.m_mins.Y()) || (other.m_maxs.Y() < m_mins.Y()) ||
		// don't touch on Z?
		(m_maxs.Z() < other.m_mins.Z()) || (other.m_maxs.Z() < m_mins.Z()))
	{
		return false;
	}

	return true;
}

template<typename T>
inline bool AABB3<T>::Instersects(const AABB3& other, const T epsilon ) const // returns true only if one piece is inside another.
{
		// don't touch on X?
	if ((m_maxs.X() <= other.m_mins.X()) || (other.m_maxs.X() <= m_mins.X()) ||
		// don't touch on Y?
		(m_maxs.Y() <= other.m_mins.Y()) || (other.m_maxs.Y() <= m_mins.Y()) ||
		// don't touch on Z?
		(m_maxs.Z() <= other.m_mins.Z()) || (other.m_maxs.Z() <= m_mins.Z()))
	{
		return false;
	}

	return true;
}

template<typename T>
inline AABB3<T>& AABB3<T>::Translate(const Vector3<T>& t)
{
	m_mins += t;
	m_maxs += t;
	m_org = (m_maxs + m_mins) / T(2);
	return *this;
}

template<typename T>
inline AABB3<T>& AABB3<T>::Translate(const T& x, const T& y, const T& z)
{
	return Translate(Vector3<T>(x, y, z));
}

template<typename T>
inline AABB3<T>& AABB3<T>::Scale(const Vector3<T>& s)
{
	m_mins = ((m_mins - m_org) * s) + m_org;
	m_maxs = ((m_maxs - m_org) * s) + m_org;
	return *this;
}

template<typename T>
inline AABB3<T>& AABB3<T>::Scale(const T& x, const T& y, const T& z)
{
	return Scale(Vector3<T>(x, y, z));
}

template<typename T>
inline bool AABB3<T>::operator == (const AABB3<T>& other) const
{
	return (m_mins == other.m_mins) && (m_maxs == other.m_maxs);
}

template<typename T>
inline bool AABB3<T>::operator != (const AABB3<T>& other) const
{
	return !(AABB3<T>::operator == (other));
}

template<typename T>
inline AABB3<T>& AABB3<T>::operator = (const AABB3<T>& other)
{
	return Initialize(other.m_mins, other.m_maxs);
}

template<typename T>
inline void AABB3<T>::SetMins(const Vector3<T>& v)
{
	m_mins = v;
	m_org = (m_maxs + m_mins) / T(2);
}

template<typename T>
inline void AABB3<T>::SetMaxs(const Vector3<T>& v)
{
	m_maxs = v;
	m_org = (m_maxs + m_mins) / T(2);
}

template<typename T>
inline const Vector3<T>& AABB3<T>::Mins() const
{
	return m_mins;
}

template<typename T>
inline const Vector3<T>& AABB3<T>::Maxs() const
{
	return m_maxs;
}

template<typename T>
const Vector3<T>& AABB3<T>::Origin() const
{
	return m_org;
}

template<typename T>
Vector3<T> AABB3<T>::Size() const
{
	return m_maxs - m_mins;
}

} // math

#include "../PopSystemMacros.h"
