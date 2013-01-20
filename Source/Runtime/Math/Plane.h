// Plane.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "PlaneDef.h"
#include "Vector.h"
#include "AABB.h"
#include "../PushPack.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math::Plane<T>
//////////////////////////////////////////////////////////////////////////////////////////
//
// this defines a plane such that Ax + Bx + Cz - D = 0.
//
// NOTE that the sign of D is flipped from the standard plane equation.
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Plane
{

public:

	typedef T ValueType;

	static const Plane<T> X;
	static const Plane<T> Y;
	static const Plane<T> Z;

	// Hints

	enum UnitHint { Unit };

	// Plane side

	enum SideType
	{
		Front,
		Back,
		On,
		Cross,
		NumSides
	};

	// Constructors

	Plane();
	Plane(const Plane<T> &p);
	explicit Plane(T a, T b, T c, T d);
	explicit Plane(const Vector3<T> &normal, T d);
	explicit Plane(const Vector3<T> &normal, const Vector3<T> &p);
	explicit Plane(const Vector3<T> &normal, const Vector3<T> &p, UnitHint u);
	// expects a counter-clock-wise wound triangle.
	explicit Plane(const Vector3<T> &p1, const Vector3<T> &p2, const Vector3<T> &p3);
	
	// Destructor

	~Plane();

	// Access

	const Vector3<T> &Normal() const;
	T A() const;
	T B() const;
	T C() const;
	T D() const;

	// In place operations

	Plane<T> &Initialize(T a, T b, T c, T d);
	Plane<T> &Initialize(const Vector3<T> &normal, T d);
	Plane<T> &Initialize(const Vector3<T> &normal, const Vector3<T> &p);
	Plane<T> &Initialize(const Vector3<T> &normal, const Vector3<T> &p, UnitHint u);
	// expects a clock-wise wound triangle.
	Plane<T> &Initialize(const Vector3<T> &p1, const Vector3<T> &p2, const Vector3<T> &p3);
		
	// Unary operations

	Plane<T> operator-() const;

    // Operates on this

    Plane<T> &Flip();
	Plane<T> &Translate(T d);
	Plane<T> &Translate(const Vector3<T> &t);

	// Binary operations
	
	template <typename TVector3>
	TVector3 Reflect(const TVector3 &v) const;

	template <typename TVector3>
	TVector3 Project(const TVector3 &v) const;

	template <typename TVector3>
	bool IntersectLineSegment(TVector3 &result, const TVector3 &a, const TVector3 &b, const T &tolerance = Epsilon<T>()) const;

	template <typename TVector3>
	static bool IntersectLineSegment(TVector3 &result, const TVector3 &a, const T &distA, const TVector3 &b, const T &distB, const T &tolerance = Epsilon<T>());

	template <typename TVector3>
	static TVector3 IntersectLineSegment(const TVector3 &a, const T &distA, const TVector3 &b, const T &distB);

    template <typename TVector3>
    bool IntersectRay(TVector3 &result, const TVector3 &rayDirection, const TVector3 &rayOrigin, const T &tolerance = Epsilon<T>()) const;

	T Distance(const Vector3<T> &v) const;
	SideType Side(const Vector3<T> &v, const T &tolerance = Epsilon<T>()) const;
	SideType Side(const AABB3<T> &bounds, const T &tolerance = Epsilon<T>()) const;
	bool IsOn(const Vector3<T> &v, const T &tolerance = Epsilon<T>()) const;

	// Comparison operations
	
	bool NearlyEquals(const Plane<T> &p, const T &normalEpsilon = Epsilon<T>() , const T &distanceEpsilon = Epsilon<T>()) const;
	bool operator==(const Plane<T> &p) const;
	bool operator!=(const Plane<T> &p) const;
	
private:

	// Data

	Vector3<T> m_normal;
	T          m_d;

};

} // math


#include "../PopPack.h"
#include "Plane.inl"
