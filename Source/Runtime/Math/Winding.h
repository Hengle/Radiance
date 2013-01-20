/*! \file Winding.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup math
*/

#pragma once

#include "Types.h"
#include "Plane.h"
#include <boost/array.hpp>
#include <vector>
#include "../PushPack.h"

namespace math {

template<typename TVertex, typename TPlane, typename TAllocator = std::allocator<TVertex> >
class Winding {
public:
	
	// Types

	typedef TVertex                                    VertexType;
	typedef TAllocator                                 AllocatorType;
	typedef std::vector<TVertex, TAllocator>           VertexListType;
	typedef typename TVertex::ValueType                ValueType;
	typedef TPlane                                     PlaneType;
	typedef typename PlaneType::SideType               SideType;
	typedef Winding<TVertex, TPlane, TAllocator>       SelfType;
	typedef boost::shared_ptr<SelfType>                Ref;

	// Constructors

	Winding();
	Winding(const SelfType &p);
	explicit Winding(const VertexType &v0, const VertexType &v1, const VertexType &v2);
	explicit Winding(const VertexType &v0, const VertexType &v1, const VertexType &v2, const PlaneType &p);
	explicit Winding(const VertexListType &v);
	explicit Winding(const VertexListType &v, const PlaneType &p);
	explicit Winding(const VertexType *v, int count);
	explicit Winding(const VertexType *v, int count, const PlaneType &p);
	explicit Winding(const PlaneType &p, const ValueType &size);

	// Destructor

	~Winding();

	// Creation

	SelfType &Initialize(const SelfType &p);
	SelfType &Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2);
	SelfType &Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2, const PlaneType &p);
	SelfType &Initialize(const VertexListType &v);
	SelfType &Initialize(const VertexListType &v, const PlaneType &p);
	SelfType &Initialize(const VertexType *v, int count);
	SelfType &Initialize(const VertexType *v, int count, const PlaneType &p);
	SelfType &Initialize(const PlaneType &p, const ValueType &size);

	// Access

	bool Empty() const;

	VertexListType &Vertices();
	const VertexListType &Vertices() const;

	int NumVertices() const;

	// Operations

	SelfType &Flip();
	SelfType &Translate(const Vector3<ValueType> &t);

	void CalcPlane();
	const PlaneType &Plane() const;

	SideType Side(const PlaneType &p, const ValueType &epsilon) const;
	SideType MajorSide(const PlaneType &p, const ValueType &epsilon) const;
	ValueType MaxPlaneDist(const PlaneType &p) const;

	void Chop(const PlaneType &p, SideType side, SelfType &poly, const ValueType &epsilon) const;
	void ChopInPlace(const PlaneType &p, SideType side, const ValueType &epsilon);
	void Split(const PlaneType &p, SelfType *front, SelfType *back, const ValueType &epsilon) const;
	void Swap(SelfType &p);
	void Clear();

	struct DefaultEquals {
		DefaultEquals(ValueType ep) { epsilon = ep; }
		bool operator() ( const TVertex &a, const TVertex &b ) const;
		ValueType epsilon;
	};

	// remove edge-colinear and duplicate vertices (returns true if verts were removed).
	template<typename EqualsType>
	bool RemoveBadVerts(
		const EqualsType &vertsEqual, 
		const ValueType &polyPlaneEpsilon, 
		const ValueType &edgeEpsilon
	); // requires update plane info.
	
	// requires update plane info.
	bool IsConvex(const ValueType &epsilon) const; 

	template<typename EqualsType>
	bool Valid(const EqualsType &vertsEqual, 
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

// Stack based Winding class, verts are stored in fixed size array.

template<typename TVertex, typename TPlane, int TSize >
class Winding<TVertex, TPlane, stack_tag<TSize> > {
public:

	enum {
		kMaxVerts = TSize
	};
	
	// Types

	typedef TVertex                                    VertexType;
	typedef typename TVertex::ValueType                ValueType;
	typedef TPlane                                     PlaneType;
	typedef typename PlaneType::SideType               SideType;
	typedef Winding<TVertex, TPlane, stack_tag<TSize> > SelfType;
	typedef boost::shared_ptr<SelfType>                Ref;

	// Constructors

	Winding();
	Winding(const SelfType &p);
	explicit Winding(const VertexType &v0, const VertexType &v1, const VertexType &v2);
	explicit Winding(const VertexType &v0, const VertexType &v1, const VertexType &v2, const PlaneType &p);
	explicit Winding(const VertexType *v, int count);
	explicit Winding(const VertexType *v, int count, const PlaneType &p);
	explicit Winding(const PlaneType &p, const ValueType &size);

	// Destructor

	~Winding();

	// Creation

	SelfType &Initialize(const SelfType &p);
	SelfType &Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2);
	SelfType &Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2, const PlaneType &p);
	SelfType &Initialize(const VertexType *v, int count);
	SelfType &Initialize(const VertexType *v, int count, const PlaneType &p);
	SelfType &Initialize(const PlaneType &p, const ValueType &size);

	// Access

	bool Empty() const;

	VertexType *Vertices();
	const VertexType *Vertices() const;

	int NumVertices() const;

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
	void Clear();

	struct DefaultEquals {
		DefaultEquals(ValueType ep) { epsilon = ep; }
		bool operator() ( const TVertex &a, const TVertex &b ) const;
		ValueType epsilon;
	};

	// remove edge-colinear and duplicate vertices (returns true if verts were removed).
	template<typename EqualsType>
	bool RemoveBadVerts(
		const EqualsType &vertsEqual, 
		const ValueType &polyPlaneEpsilon, 
		const ValueType &edgeEpsilon
	); // requires update plane info.
	
	// requires update plane info.
	bool IsConvex(const ValueType &epsilon) const; 

	template<typename EqualsType>
	bool Valid(
		const EqualsType &vertsEqual, 
		const ValueType &polyPlaneEpsilon,
		const ValueType &edgeEpsilon, 
		const ValueType &convexityEpsilon 
	) const; // requires updated plane info.

	// operators

	SelfType &operator = (const SelfType & p);

private:

	typedef boost::array<VertexType, TSize> VertArray;

	PlaneType m_plane;
	VertArray m_verts;
	int m_numVerts;

	void push_back(const VertexType &v);
	void erase(int index);
};

} // math


#include "../PopPack.h"
#include "Winding.inl"
