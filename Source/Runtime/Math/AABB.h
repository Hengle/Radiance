// AABB.h
// Axis Aligned Bounding Box
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AABBDef.h"
#include "Vector.h"
#include "../PushPack.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// Axis Aligned Bounding Box (3D)
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
class AABB3
{
public:
	AABB3();
	AABB3(const AABB3& aabb);
	explicit AABB3(const Vector3<T>& mins, const Vector3<T>& maxs);
	explicit AABB3(const T& minX, const T& minY, const T& minZ, const T& maxX, const T& maxY, const T& maxZ);
	~AABB3();

	AABB3<T>& Initialize();
	AABB3<T>& Initialize(const AABB3& aabb);
	AABB3<T>& Initialize(const Vector3<T>& mins, const Vector3<T>& maxs);
	AABB3<T>& Initialize(const T& minX, const T& minY, const T& minZ, const T& maxX, const T& maxY, const T& maxZ);

	AABB3<T>& Expand(const Vector3<T>& ex);
	AABB3<T>& Expand(const T& expX, const T& expY, const T& expZ);
	AABB3<T>& ExpandMin(const Vector3<T>& ex);
	AABB3<T>& ExpandMin(const T& expX, const T& expY, const T& expZ);
	AABB3<T>& ExpandMax(const Vector3<T>& ex);
	AABB3<T>& ExpandMax(const T& expX, const T& expY, const T& expZ);

	AABB3<T>& Insert(const AABB3<T>& aabb);
	AABB3<T>& Insert(const Vector3<T>& point);
	AABB3<T>& Insert(const T& x, const T& y, const T& z);

	bool Contains(const Vector3<T>& point, const T epsilon = T(0)) const;

	bool Touches(const AABB3& other, const T epsilon = T(0)) const; // returns true if they intersect OR the edges touch.
	bool Instersects(const AABB3& other, const T epsilon = T(0)) const; // returns true only if one piece is inside another.
	Vector3<T> Size() const;
	const Vector3<T>& Origin() const;

	AABB3<T>& Translate(const Vector3<T>& t);
	AABB3<T>& Translate(const T& x, const T& y, const T& z);
	AABB3<T>& Scale(const Vector3<T>& s);
	AABB3<T>& Scale(const T& x, const T& y, const T& z);

	bool operator == (const AABB3<T>& other) const;
	bool operator != (const AABB3<T>& other) const;
	AABB3<T>& operator = (const AABB3<T>& other);

	void SetMins(const Vector3<T>& v);
	void SetMaxs(const Vector3<T>& v);
	const Vector3<T>& Mins() const;
	const Vector3<T>& Maxs() const;

private:

	Vector3<T> m_mins, m_maxs, m_org;
};

} // math


#include "../PopPack.h"
#include "AABB.inl"
