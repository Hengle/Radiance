// ConvexPolygon.h
// Convex Polygon class
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Plane.h"
#include <vector>
#include "../PushPack.h"


namespace math {

//////////////////////////////////////////////////////////////////////////////////////////
// math::ConvexPolygon
//////////////////////////////////////////////////////////////////////////////////////////
//
// the end user is responsible to call CalcPlane() when vertices are moved or changed.
// Any functions called on the object will update the plane, but if you add/remove/change
// vertices you must recalc the plane via. CalcPlane().
//

template<typename TVertex, typename TPlane, typename TAllocator = std::allocator<TVertex> >
class ConvexPolygon
{
public:
	
	// Types

	typedef TVertex                                    VertexType;
	typedef TAllocator                                 AllocatorType;
	typedef std::vector<TVertex, TAllocator>           VertexListType;
	typedef typename TVertex::ValueType                ValueType;
	typedef TPlane                                     PlaneType;
	typedef typename PlaneType::SideType               SideType;
	typedef ConvexPolygon<TVertex, TPlane, TAllocator> SelfType;
	typedef boost::shared_ptr<SelfType>                Ref;

	// Constructors

	ConvexPolygon();
	ConvexPolygon(const SelfType &p);
	explicit ConvexPolygon(const VertexType &v0, const VertexType &v1, const VertexType &v2);
	explicit ConvexPolygon(const VertexType &v0, const VertexType &v1, const VertexType &v2, const PlaneType &p);
	explicit ConvexPolygon(const VertexListType &v);
	explicit ConvexPolygon(const VertexListType &v, const PlaneType &p);
	explicit ConvexPolygon(const VertexType *v, UReg count);
	explicit ConvexPolygon(const VertexType *v, UReg count, const PlaneType &p);
	explicit ConvexPolygon(const PlaneType &p, const ValueType &size);

	// Destructor

	~ConvexPolygon();

	// Creation

	SelfType &Initialize(const SelfType &p);
	SelfType &Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2);
	SelfType &Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2, const PlaneType &p);
	SelfType &Initialize(const VertexListType &v);
	SelfType &Initialize(const VertexListType &v, const PlaneType &p);
	SelfType &Initialize(const VertexType *v, UReg count);
	SelfType &Initialize(const VertexType *v, UReg count, const PlaneType &p);
	SelfType &Initialize(const PlaneType &p, const ValueType &size);

	// Access

	bool Empty() const;

	VertexListType &Vertices();
	const VertexListType &Vertices() const;

	// Operations

	SelfType &Flip();
	SelfType &Translate(const Vector3<ValueType> &t);

	void CalcPlane();
	const PlaneType &Plane() const;

	SideType Side(const PlaneType &p, const ValueType &epsilon) const;
	SideType MajorSide(const PlaneType &p, const ValueType &epsilon) const;
	ValueType MaxPlaneDist(const PlaneType &p) const;

	void Chop(const PlaneType &p, SideType side, SelfType &poly, const ValueType &epsilon) const;
	void Split(const PlaneType &p, SelfType *front, SelfType *back, const ValueType &epsilon) const;
	void Swap(SelfType &p);

	struct DefaultEquals
	{
		DefaultEquals(ValueType ep) { epsilon = ep; }
		bool operator() ( const TVertex &a, const TVertex &b ) const;
		ValueType epsilon;
	};

	// remove edge-colinear and duplicate vertices (returns true if verts were removed).
	template<typename EqualsType>
	bool RemoveBadVerts(const EqualsType &vertsEqual, const ValueType &polyPlaneEpsilon, const ValueType &edgeEpsilon); // requires update plane info.
	
	// requires update plane info.
	bool IsConvex(const ValueType &epsilon) const; 

	template<typename EqualsType>
	bool Valid
	(
		const EqualsType &vertsEqual, 
		const ValueType &polyPlaneEpsilon,
		const ValueType &edgeEpsilon, 
		const ValueType &convexityEpsilon 
	) const; // requires updated plane info.

	// operators

	SelfType &operator = (const VertexListType &v);
	SelfType &operator = (const SelfType & p);

private:

	PlaneType m_plane;
	VertexListType m_verts;

};

} // math


#include "../PopPack.h"
#include "ConvexPolygon.inl"
