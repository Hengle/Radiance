/*! \file Winding.inl
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup math
*/

#pragma once

#include "Vector.h"
#include <algorithm>

#include "../PushSystemMacros.h"

namespace math {

///////////////////////////////////////////////////////////////////////////////

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator>::Winding() {
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator>::Winding(const SelfType &p) {
	Initialize(p);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator>::Winding(const VertexListType &v) {
	Initialize(v);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator>::Winding(const VertexListType &v, const PlaneType &p) {
	Initialize(v, p);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator>::Winding(const VertexType *v, int count) {
	Initialize(v, count);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator>::Winding(const VertexType *v, int count, const PlaneType &p) {
	Initialize(v, count, p);
}

template<typename TVertex, typename TPlane, typename TAllocator>
Winding<TVertex, TPlane, TAllocator>::Winding(const PlaneType &p, const ValueType &size) {
	Initialize(p, size);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator>::~Winding() {
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator> &Winding<TVertex, TPlane, TAllocator>::Initialize(const SelfType &p) {
	m_verts = p.m_verts;
	m_plane = p.m_plane;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator> &Winding<TVertex, TPlane, TAllocator>::Initialize(const VertexListType &v) {
	m_verts = v;
	CalcPlane();
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator> &Winding<TVertex, TPlane, TAllocator>::Initialize(const VertexListType &v, const PlaneType &p) {
	m_verts = v;
	m_plane = p;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
Winding<TVertex, TPlane, TAllocator> &Winding<TVertex, TPlane, TAllocator>::Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2) {
	m_verts.reserve(3);
	m_verts.clear();
	m_verts.push_back(v0);
	m_verts.push_back(v1);
	m_verts.push_back(v2);
	CalcPlane();
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
Winding<TVertex, TPlane, TAllocator> &Winding<TVertex, TPlane, TAllocator>::Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2, const PlaneType &p) {
	m_verts.reserve(3);
	m_verts.clear();
	m_verts.push_back(v0);
	m_verts.push_back(v1);
	m_verts.push_back(v2);
	m_plane = p;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
Winding<TVertex, TPlane, TAllocator> &Winding<TVertex, TPlane, TAllocator>::Initialize(const VertexType *p, int count) {
	RAD_ASSERT(p);
	RAD_ASSERT(count>0);

	m_verts.clear();
	for (int i = 0; i < count; i++) {
		m_verts.push_back(p[i]);
	}

	CalcPlane();
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
Winding<TVertex, TPlane, TAllocator> &Winding<TVertex, TPlane, TAllocator>::Initialize(const VertexType *v, int count, const PlaneType &p) {
	RAD_ASSERT(v);
	RAD_ASSERT(count>0);

	m_verts.clear();
	m_verts.reserve(count);

	for (int i = 0; i < count; i++) {
		m_verts.push_back(v[i]);
	}

	m_plane = p;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
Winding<TVertex, TPlane, TAllocator> &Winding<TVertex, TPlane, TAllocator>::Initialize(const PlaneType &p, const ValueType &size) {
	TVertex up, left, org;
	org = p.Normal() * p.D();
	p.Normal().FrameVecs(up, left);

	TVertex v[4];

	ValueType halfSize = size / ValueType(2);
	v[0] = org + (left * halfSize) + (up * halfSize);
	v[1] = org + (left * halfSize) + (up * -halfSize);
	v[2] = org + (left * -halfSize) + (up * -halfSize);
	v[3] = org + (left * -halfSize) + (up * halfSize);

	Initialize(v, 4, p);
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline bool Winding<TVertex, TPlane, TAllocator>::Empty() const {
	return m_verts.empty();
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline typename Winding<TVertex, TPlane, TAllocator>::VertexListType &Winding<TVertex, TPlane, TAllocator>::Vertices() {
	return m_verts;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline const typename Winding<TVertex, TPlane, TAllocator>::VertexListType &Winding<TVertex, TPlane, TAllocator>::Vertices() const {
	return m_verts;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline int Winding<TVertex, TPlane, TAllocator>::NumVertices() const {
	return (int)m_verts.size();
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator> &Winding<TVertex, TPlane, TAllocator>::Flip() {
	std::reverse(m_verts.begin(), m_verts.end());
	m_plane = -m_plane;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline Winding<TVertex, TPlane, TAllocator> &Winding<TVertex, TPlane, TAllocator>::Translate(const Vector3<ValueType> &t) {
	for (typename VertexListType::iterator v = m_verts.begin(); v != m_verts.end(); ++v) {
		(*v) += t;
	}

	m_plane.Translate(t);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline void Winding<TVertex, TPlane, TAllocator>::CalcPlane() {
	RAD_ASSERT(m_verts.size() >= 3);
	m_plane.Initialize( m_verts[0], m_verts[1], m_verts[2] );
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline const typename Winding<TVertex, TPlane, TAllocator>::PlaneType &Winding<TVertex, TPlane, TAllocator>::Plane() const {
	return m_plane;
}

template<typename TVertex, typename TPlane, typename TAllocator>
typename Winding<TVertex, TPlane, TAllocator>::SideType Winding<TVertex, TPlane, TAllocator>::Side(const PlaneType &p, const ValueType &epsilon) const {
	RAD_ASSERT(!m_verts.empty());
	UReg front = 0, back = 0, on = 0;
	
	for (typename VertexListType::const_iterator i = m_verts.begin(); i != m_verts.end(); ++i) {
		ValueType d = p.Distance(*i);
		if (d > epsilon) { 
			front++; 
		} else if (d < -epsilon) { 
			back++; 
		} else { 
			on++; 
		}

		if (front && back) 
			break;
	}

	RAD_ASSERT( front || back || on );

	if (front && !back)
		return PlaneType::Front;
	if (back && !front)
		return PlaneType::Back;
	if (front && back)
		return PlaneType::Cross;
	return PlaneType::On;
}

template<typename TVertex, typename TPlane, typename TAllocator>
typename Winding<TVertex, TPlane, TAllocator>::ValueType Winding<TVertex, TPlane, TAllocator>::MaxPlaneDist(const PlaneType &p) const {
	RAD_ASSERT(!m_verts.empty());
	ValueType max = -std::numeric_limits<ValueType>::max();
	ValueType min = std::numeric_limits<ValueType>::max();

	for (UReg i = 0; i < (UReg)m_verts.size(); i++) {
		ValueType d = p.Distance(m_verts[i]);
		if (d < min) min = d;
		if (d > max) max = d;
	}

	return (Abs(min) > Abs(max)) ? min : max;
}

template<typename TVertex, typename TPlane, typename TAllocator>
typename Winding<TVertex, TPlane, TAllocator>::SideType Winding<TVertex, TPlane, TAllocator>::MajorSide(const PlaneType &p, const ValueType &epsilon) const {
	RAD_ASSERT(!m_verts.empty());
	ValueType side = MaxPlaneDist(p);

	if (Abs(side) >= epsilon) {
		if (side > ValueType(0))
			return PlaneType::Front;
		if (side < ValueType(0))
			return PlaneType::Back;
	}

	return PlaneType::On;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline void Winding<TVertex, TPlane, TAllocator>::Chop(const PlaneType &p, SideType side, SelfType &poly, const ValueType &epsilon) const {
	RAD_ASSERT(side == PlaneType::Front || side == PlaneType::Back);
		
	if (side == PlaneType::Front) {
		Split(p, &poly, 0, epsilon);
	} else {
		Split(p, 0, &poly, epsilon);
	}
}

template<typename TVertex, typename TPlane, typename TAllocator>
void Winding<TVertex, TPlane, TAllocator>::Split(const PlaneType &p, SelfType *front, SelfType *back, const ValueType &epsilon) const {
	
	if (!front && !back) 
		return;

	VertexListType *frontVerts = 0, *backVerts = 0;

	if (front) 
		frontVerts = &(front->m_verts);
	if (back) 
		backVerts  = &(back->m_verts);

	if (frontVerts) 
		frontVerts->clear();
	if (backVerts) 
		backVerts->clear();

	typedef typename TAllocator::template rebind<SideType>::other SideAllocator;
	typedef typename TAllocator::template rebind<ValueType>::other ValueAllocator;
	
	std::vector<SideType, SideAllocator> sides(SideAllocator(m_verts.get_allocator())); // rebind to same allocator.
	std::vector<ValueType, ValueAllocator> dots(ValueAllocator(m_verts.get_allocator()));
	size_t counts[PlaneType::NumSides];

	memset(counts, 0, sizeof(counts));

	sides.reserve(m_verts.size());
	dots.reserve(m_verts.size());

	// classify.
	for (size_t i = 0; i < m_verts.size(); i++) {
		ValueType d = p.Distance(m_verts[i]);

		if (d > epsilon) {
			sides.push_back(PlaneType::Front);
		} else if (d < -epsilon) {
			sides.push_back(PlaneType::Back);
		} else {
			sides.push_back(PlaneType::On);
		}

		dots.push_back(d);
		++counts[sides[i]];
	}

	if (counts[PlaneType::On] == m_verts.size()) 
		return;

	RAD_ASSERT(counts[PlaneType::Front] || counts[PlaneType::Back]);

	if (!counts[PlaneType::Back]) { 
		// all on front
		if (front) *front = *this;
		return;
	} else if (!counts[PlaneType::Front]) {
		if (back) *back = *this;
		return;
	}

	// count.
	{
		int fc = 0;
		int bc = 0;

		for (size_t i = 0; i < m_verts.size(); i++) {
			switch (sides[i]) {
			case PlaneType::Front:
				fc++;
				break;
			case PlaneType::Back:
				bc++;
				break;
			case PlaneType::On:
				fc++;
				bc++;
				continue;
			}

			size_t next = (i + 1);
			if (next >= m_verts.size())
				next = 0;

			// does this edge cross the plane?
			if (sides[next] != PlaneType::On && sides[next] != sides[i]) { 
				// yes.
				fc++; 
				bc++;
			}
		}

		if (frontVerts) 
			frontVerts->reserve(fc);
		if (backVerts) 
			backVerts->reserve(bc);
	}

	// split
	for (size_t i = 0; i < m_verts.size(); i++) {
		switch (sides[i]) {
		case PlaneType::Front:
			if (frontVerts) 
				frontVerts->push_back(m_verts[i]);
			break;
		case PlaneType::Back:
			if (backVerts) 
				backVerts->push_back(m_verts[i]);
			break;
		case PlaneType::On:
			if (frontVerts) 
				frontVerts->push_back(m_verts[i]);
			if (backVerts) 
				backVerts->push_back(m_verts[i]);
			continue;
		}

		size_t next = (i + 1);
		if (next >= m_verts.size())
			next = 0;

		// does this edge cross the plane?
		if (sides[next] != PlaneType::On && sides[next] != sides[i]) { // yes.
						
			TVertex mid = PlaneType::IntersectLineSegment(m_verts[i], dots[i], m_verts[next], dots[next]);

			if (frontVerts) 
				frontVerts->push_back(mid);
			if (backVerts) 
				backVerts->push_back(mid);
		}
	}

	if (front)
		front->m_plane = m_plane;
	if (back)
		back->m_plane = m_plane;
}

template<typename TVertex, typename TPlane, typename TAllocator>
template<typename EqualsType>
bool Winding<TVertex, TPlane, TAllocator>::RemoveBadVerts(
	const EqualsType &vertsEqual, 
	const ValueType &polyPlaneEpsilon, 
	const ValueType &edgeEpsilon
) {
	bool removed = false;

	// remove duplicate vertices.
	for (int i = 0; i < (int)m_verts.size(); ++i) {
		for (int k = i + 1; k < (int)m_verts.size(); ++k) {
			if (vertsEqual(m_verts[i], m_verts[k])) {
				m_verts.erase(m_verts.begin() + k);
				k = i + 1;
				removed = true;
			}
		}
	}

	// remove verts not on the plane.
	for (typename VertexListType::iterator i = m_verts.begin(); i != m_verts.end();) {
		if (m_plane.On(*i, polyPlaneEpsilon)) {
			++i;
		} else {
			i = m_verts.erase(i);
			removed = true;
		}
	}

	// remove colinear edge vertices.
	if (m_verts.size() > 3) {
		for (UReg i = 0; !m_verts.empty() && (i < m_verts.size()-1); ++i) {
			UReg k = i + 1;

			PlaneType edgePlane;
			Vector3<ValueType> edge = m_verts[i] - m_verts[k];
			{
				Vector3<ValueType> unitEdge = m_plane.Normal().Cross(edge).Unit();
				// put winding on front.
				edgePlane.Initialize(unitEdge, unitEdge.Dot(m_verts[i]));
			}

			for (UReg j = (k + 1) % m_verts.size(); j < m_verts.size();) {
				if (edgePlane.On(m_verts[j], edgeEpsilon)) {
					// colinear.
					Vector3<ValueType> tempEdge = m_verts[j] - m_verts[i];
					if (edge.MagnitudeSquared() >= tempEdge.MagnitudeSquared()) {
						m_verts.erase(m_verts.begin()+j);
						removed = true;
					} else {
						edge = tempEdge;
						m_verts.erase(m_verts.begin()+k);
						k = j; 
						j++;
						removed = true;
					}
				} else {
					break;
				}
			}
		}
	}

	return removed;
}

template<typename TVertex, typename TPlane, typename TAllocator>
bool Winding<TVertex, TPlane, TAllocator>::IsConvex(const ValueType &epsilon) const {
	if (m_verts.size() >= 3) {
		PlaneType edgePlane;

		for (UReg i = 0; i < m_verts.size(); i++) {
			UReg k = (i + 1);
			if (k >= m_verts.size())
				k = 0;
			{
				Vector3<ValueType> edge = m_plane.Normal().Cross(m_verts[i] - m_verts[k]).Unit();
				edgePlane.Initialize(edge, edge.Dot(m_verts[i]));
			}
			// make sure all points are in front of this edge!
			for (k = 0; k < m_verts.size(); k++) {
				if (edgePlane.Side(m_verts[k], epsilon) == PlaneType::Back) {
					return false;
				}
			}
		}

		return true;
	} else {
		return false;
	}
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline void Winding<TVertex, TPlane, TAllocator>::Swap(SelfType &p) {
	PlaneType temp = m_plane;
	m_plane = p.m_plane;
	p.m_plane = temp;
	m_verts.swap(p.m_verts);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline void Winding<TVertex, TPlane, TAllocator>::Clear() {
	m_verts.clear();
}

template<typename TVertex, typename TPlane, typename TAllocator>
template<typename EqualsType>
inline bool Winding<TVertex, TPlane, TAllocator>::Valid(const EqualsType &vertsEqual, const ValueType &polyPlaneEpsilon, const ValueType &edgeEpsilon, const ValueType &convexityEpsilon) const {
	if( (m_verts.size() >= 3) && IsConvex(convexityEpsilon) ) {
		Winding copy = *this;
		return !copy.RemoveBadVerts(vertsEqual, polyPlaneEpsilon, edgeEpsilon);
	} else {
		return false;
	}
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline typename Winding<TVertex, TPlane, TAllocator>::SelfType &Winding<TVertex, TPlane, TAllocator>::operator = (const VertexListType &v) {
	m_verts = v;
	CalcPlane();
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline typename Winding<TVertex, TPlane, TAllocator>::SelfType &Winding<TVertex, TPlane, TAllocator>::operator = (const SelfType & p) {
	m_verts = p.m_verts;
	m_plane = p.m_plane;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline bool Winding<TVertex, TPlane, TAllocator>::DefaultEquals::operator () (const TVertex &a, const TVertex &b) const {
	return a.NearlyEquals(b, epsilon);
}

///////////////////////////////////////////////////////////////////////////////

template<typename TVertex, typename TPlane, int TSize>
inline Winding<TVertex, TPlane, stack_tag<TSize> >::Winding() {
}

template<typename TVertex, typename TPlane, int TSize>
inline Winding<TVertex, TPlane, stack_tag<TSize> >::Winding(const SelfType &p) {
	Initialize(p);
}

template<typename TVertex, typename TPlane, int TSize>
inline Winding<TVertex, TPlane, stack_tag<TSize> >::Winding(const VertexType *v, int count) {
	Initialize(v, count);
}

template<typename TVertex, typename TPlane, int TSize>
inline Winding<TVertex, TPlane, stack_tag<TSize> >::Winding(const VertexType *v, int count, const PlaneType &p) {
	Initialize(v, count, p);
}

template<typename TVertex, typename TPlane, int TSize>
Winding<TVertex, TPlane, stack_tag<TSize> >::Winding(const PlaneType &p, const ValueType &size) {
	Initialize(p, size);
}

template<typename TVertex, typename TPlane, int TSize>
inline Winding<TVertex, TPlane, stack_tag<TSize> >::~Winding() {
}

template<typename TVertex, typename TPlane, int TSize>
inline Winding<TVertex, TPlane, stack_tag<TSize> > &Winding<TVertex, TPlane, stack_tag<TSize> >::Initialize(const SelfType &p) {
	for (int i = 0; i < p.m_numVerts; ++i)
		m_verts[i] = p.m_verts[i];
	m_numVerts = p.m_numVerts;
	m_plane = p.m_plane;
	return *this;
}

template<typename TVertex, typename TPlane, int TSize>
Winding<TVertex, TPlane, stack_tag<TSize> > &Winding<TVertex, TPlane, stack_tag<TSize> >::Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2) {
	m_numVerts = 0;
	push_back(v0);
	push_back(v1);
	push_back(v2);
	CalcPlane();
	return *this;
}

template<typename TVertex, typename TPlane, int TSize>
Winding<TVertex, TPlane, stack_tag<TSize> > &Winding<TVertex, TPlane, stack_tag<TSize> >::Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2, const PlaneType &p) {
	m_numVerts = 0;
	push_back(v0);
	push_back(v1);
	push_back(v2);
	m_plane = p;
	return *this;
}

template<typename TVertex, typename TPlane, int TSize>
Winding<TVertex, TPlane, stack_tag<TSize> > &Winding<TVertex, TPlane, stack_tag<TSize> >::Initialize(const VertexType *p, int count) {
	RAD_ASSERT(p);
	RAD_ASSERT(count>0);

	m_numVerts = 0;
	for (int i = 0; i < count; i++) {
		push_back(p[i]);
	}

	CalcPlane();
	return *this;
}

template<typename TVertex, typename TPlane, int TSize>
Winding<TVertex, TPlane, stack_tag<TSize> > &Winding<TVertex, TPlane, stack_tag<TSize> >::Initialize(const VertexType *v, int count, const PlaneType &p) {
	RAD_ASSERT(v);
	RAD_ASSERT(count>0);

	m_numVerts = 0;

	for (int i = 0; i < count; i++) {
		push_back(v[i]);
	}

	m_plane = p;
	return *this;
}

template<typename TVertex, typename TPlane, int TSize>
Winding<TVertex, TPlane, stack_tag<TSize> > &Winding<TVertex, TPlane, stack_tag<TSize> >::Initialize(const PlaneType &p, const ValueType &size) {
	TVertex up, left, org;
	org = p.Normal() * p.D();
	p.Normal().FrameVecs(up, left);

	TVertex v[4];

	ValueType halfSize = size / ValueType(2);
	v[0] = org + (left * halfSize) + (up * halfSize);
	v[1] = org + (left * halfSize) + (up * -halfSize);
	v[2] = org + (left * -halfSize) + (up * -halfSize);
	v[3] = org + (left * -halfSize) + (up * halfSize);

	Initialize(v, 4, p);
	return *this;
}

template<typename TVertex, typename TPlane, int TSize>
inline bool Winding<TVertex, TPlane, stack_tag<TSize> >::Empty() const {
	return m_numVerts == 0;
}

template<typename TVertex, typename TPlane, int TSize>
inline typename Winding<TVertex, TPlane, stack_tag<TSize> >::VertexType *Winding<TVertex, TPlane, stack_tag<TSize> >::Vertices() {
	return &m_verts[0];
}

template<typename TVertex, typename TPlane, int TSize>
inline const typename Winding<TVertex, TPlane, stack_tag<TSize> >::VertexType *Winding<TVertex, TPlane, stack_tag<TSize> >::Vertices() const {
	return &m_verts[0];
}

template<typename TVertex, typename TPlane, int TSize>
inline int Winding<TVertex, TPlane, stack_tag<TSize> >::NumVertices() const {
	return m_numVerts;
}

template<typename TVertex, typename TPlane, int TSize>
inline Winding<TVertex, TPlane, stack_tag<TSize> > &Winding<TVertex, TPlane, stack_tag<TSize> >::Flip() {
	int i = 0;
	int k = m_numVerts - 1;

	for (; i < k; ++i, --k) {
		std::swap(m_verts[i], m_verts[k]);
	}

	m_plane = -m_plane;
	return *this;
}

template<typename TVertex, typename TPlane, int TSize>
inline Winding<TVertex, TPlane, stack_tag<TSize> > &Winding<TVertex, TPlane, stack_tag<TSize> >::Translate(const Vector3<ValueType> &t) {
	for (int i = 0; i < m_numVerts; ++i) {
		m_verts[i] += t;
	}

	m_plane.Translate(t);
}

template<typename TVertex, typename TPlane, int TSize>
inline void Winding<TVertex, TPlane, stack_tag<TSize> >::CalcPlane() {
	RAD_ASSERT(m_numVerts >= 3);
	m_plane.Initialize(m_verts[0], m_verts[1], m_verts[2]);
}

template<typename TVertex, typename TPlane, int TSize>
inline const typename Winding<TVertex, TPlane, stack_tag<TSize> >::PlaneType &Winding<TVertex, TPlane, stack_tag<TSize> >::Plane() const {
	return m_plane;
}

template<typename TVertex, typename TPlane, int TSize>
typename Winding<TVertex, TPlane, stack_tag<TSize> >::SideType Winding<TVertex, TPlane, stack_tag<TSize> >::Side(const PlaneType &p, const ValueType &epsilon) const {
	RAD_ASSERT(m_numVerts);
	UReg front = 0, back = 0, on = 0;
	
	for (int i = 0; i < m_numVerts; ++i) {
		ValueType d = p.Distance(m_verts[i]);
		if (d > epsilon) { 
			front++; 
		} else if (d < -epsilon) { 
			back++; 
		} else { 
			on++; 
		}

		if (front && back) 
			break;
	}

	RAD_ASSERT( front || back || on );

	if (front && !back)
		return PlaneType::Front;
	if (back && !front)
		return PlaneType::Back;
	if (front && back)
		return PlaneType::Cross;
	return PlaneType::On;
}

template<typename TVertex, typename TPlane, int TSize>
typename Winding<TVertex, TPlane, stack_tag<TSize> >::ValueType Winding<TVertex, TPlane, stack_tag<TSize> >::MaxPlaneDist(const PlaneType &p) const {
	RAD_ASSERT(m_numVerts);
	ValueType max = -std::numeric_limits<ValueType>::max();
	ValueType min = std::numeric_limits<ValueType>::max();

	for (int i = 0; i < m_numVerts; ++i) {
		ValueType d = p.Distance(m_verts[i]);
		if (d < min) 
			min = d;
		if (d > max) 
			max = d;
	}

	return (Abs(min) > Abs(max)) ? min : max;
}

template<typename TVertex, typename TPlane, int TSize>
typename Winding<TVertex, TPlane, stack_tag<TSize> >::SideType Winding<TVertex, TPlane, stack_tag<TSize> >::MajorSide(const PlaneType &p, const ValueType &epsilon) const {
	RAD_ASSERT(m_numVerts);
	ValueType side = MaxPlaneDist(p);

	if (Abs(side) >= epsilon) {
		if (side > ValueType(0))
			return PlaneType::Front;
		if (side < ValueType(0))
			return PlaneType::Back;
	}

	return PlaneType::On;
}

template<typename TVertex, typename TPlane, int TSize>
inline void Winding<TVertex, TPlane, stack_tag<TSize> >::Chop(const PlaneType &p, SideType side, SelfType &poly, const ValueType &epsilon) const {
	RAD_ASSERT(side == PlaneType::Front || side == PlaneType::Back);
		
	if (side == PlaneType::Front) {
		Split(p, &poly, 0, epsilon);
	} else {
		Split(p, 0, &poly, epsilon);
	}
}

template<typename TVertex, typename TPlane, int TSize>
void Winding<TVertex, TPlane, stack_tag<TSize> >::Split(const PlaneType &p, SelfType *front, SelfType *back, const ValueType &epsilon) const {
	
	if (!front && !back) 
		return;

	if (front)
		front->m_numVerts = 0;

	if (back)
		back->m_numVerts = 0;
	
	boost::array<SideType, TSize> sides;
	boost::array<ValueType, TSize> dots;
	boost::array<size_t, PlaneType::NumSides> counts;

	memset(&counts[0], 0, sizeof(size_t) * PlaneType::NumSides);

	// classify.
	for (int i = 0; i < m_numVerts; ++i) {
		ValueType d = p.Distance(m_verts[i]);

		if (d > epsilon) {
			sides[i] = PlaneType::Front;
		} else if (d < -epsilon) {
			sides[i] = PlaneType::Back;
		} else {
			sides[i] = PlaneType::On;
		}

		dots[i] = d;
		++counts[sides[i]];
	}

	if (counts[PlaneType::On] == m_numVerts) 
		return;

	RAD_ASSERT(counts[PlaneType::Front] || counts[PlaneType::Back]);

	if (!counts[PlaneType::Back]) { 
		// all on front
		if (front) 
			*front = *this;
		return;
	} else if (!counts[PlaneType::Front]) {
		if (back) 
			*back = *this;
		return;
	}

	// split
	for (int i = 0; i < m_numVerts; i++) {
		switch (sides[i]) {
		case PlaneType::Front:
			if (front)
				front->push_back(m_verts[i]);
			break;
		case PlaneType::Back:
			if (back)
				back->push_back(m_verts[i]);
			break;
		case PlaneType::On:
			if (front)
				front->push_back(m_verts[i]);
			if (back)
				back->push_back(m_verts[i]);
			continue;
		}

		int next = i + 1;
		if (next >= m_numVerts)
			next = 0;

		// does this edge cross the plane?
		if (sides[next] != PlaneType::On && sides[next] != sides[i]) { // yes.
						
			TVertex mid = PlaneType::IntersectLineSegment(m_verts[i], dots[i], m_verts[next], dots[next]);

			if (front)
				front->push_back(mid);
			if (back)
				back->push_back(mid);
		}
	}

	if (front)
		front->m_plane = m_plane;
	if (back)
		back->m_plane = m_plane;
}

template<typename TVertex, typename TPlane, int TSize>
template<typename EqualsType>
bool Winding<TVertex, TPlane, stack_tag<TSize> >::RemoveBadVerts(
	const EqualsType &vertsEqual, 
	const ValueType &polyPlaneEpsilon, 
	const ValueType &edgeEpsilon
) {
	bool removed = false;

	// remove duplicate vertices.
	for (int i = 0; i < (int)m_numVerts; ++i) {
		for (int k = i + 1; k < (int)m_numVerts; ++k) {
			if (vertsEqual(m_verts[i], m_verts[k])) {
				erase(k);
				k = i + 1;
				removed = true;
			}
		}
	}

	// remove verts not on the plane.
	for (int i = 0; i < m_numVerts;) {
		if (m_plane.On(m_verts[i], polyPlaneEpsilon)) {
			++i;
		} else {
			erase(i);
			removed = true;
		}
	}

	// remove colinear edge vertices.
	if (m_numVerts > 3) {
		for (int i = 0; m_numVerts && (i < (m_numVerts-1)); ++i) {
			int k = i + 1;

			PlaneType edgePlane;
			Vector3<ValueType> edge = m_verts[i] - m_verts[k];
			{
				Vector3<ValueType> unitEdge = m_plane.Normal().Cross(edge).Unit();
				// put winding on front.
				edgePlane.Initialize(unitEdge, unitEdge.Dot(m_verts[i]));
			}

			for (int j = (k + 1) % m_numVerts; j < m_numVerts;) {
				if (edgePlane.On(m_verts[j], edgeEpsilon)) {
					// colinear.
					Vector3<ValueType> tempEdge = m_verts[j] - m_verts[i];
					if (edge.MagnitudeSquared() >= tempEdge.MagnitudeSquared()) {
						erase(j);
						removed = true;
					} else {
						edge = tempEdge;
						erase(k);
						k = j; 
						++j;
						removed = true;
					}
				} else {
					break;
				}
			}
		}
	}

	return removed;
}

template<typename TVertex, typename TPlane, int TSize>
bool Winding<TVertex, TPlane, stack_tag<TSize> >::IsConvex(const ValueType &epsilon) const {

	if (m_numVerts < 3)
		return false;

	PlaneType edgePlane;

	for (int i = 0; i < m_numVerts; ++i) {
		int k = (i + 1);
		if (k >= m_numVerts)
			k = 0;
		{
			Vector3<ValueType> edge = m_plane.Normal().Cross(m_verts[i] - m_verts[k]).Unit();
			edgePlane.Initialize(edge, edge.Dot(m_verts[i]));
		}
		// make sure all points are in front of this edge!
		for (k = 0; k < m_numVerts; k++) {
			if (edgePlane.Side(m_verts[k], epsilon) == PlaneType::Back) {
				return false;
			}
		}
	}

	return true;
}

template<typename TVertex, typename TPlane, int TSize>
inline void Winding<TVertex, TPlane, stack_tag<TSize> >::Swap(SelfType &p) {
	std::swap(m_plane, p.m_plane);

	VertArray x;
	for (int i = 0; i < m_numVerts; ++i)
		x[i] = m_verts[i];
	for (int i = 0; i < p.m_numVerts; ++i)
		m_verts[i] = p.m_verts[i];
	for (int i = 0; i < m_numVerts; ++i)
		p.m_verts[i] = x[i];

	std::swap(m_numVerts, p.m_numVerts);
}

template<typename TVertex, typename TPlane, int TSize>
inline void Winding<TVertex, TPlane, stack_tag<TSize> >::Clear() {
	m_numVerts = 0;
}

template<typename TVertex, typename TPlane, int TSize>
template<typename EqualsType>
inline bool Winding<TVertex, TPlane, stack_tag<TSize> >::Valid(const EqualsType &vertsEqual, const ValueType &polyPlaneEpsilon, const ValueType &edgeEpsilon, const ValueType &convexityEpsilon) const {
	if( (m_numVerts >= 3) && IsConvex(convexityEpsilon) ) {
		Winding copy(*this);
		return !copy.RemoveBadVerts(vertsEqual, polyPlaneEpsilon, edgeEpsilon);
	} else {
		return false;
	}
}

template<typename TVertex, typename TPlane, int TSize>
inline typename Winding<TVertex, TPlane, stack_tag<TSize> >::SelfType &Winding<TVertex, TPlane, stack_tag<TSize> >::operator = (const SelfType & p) {
	for (int i = 0; i < p.m_numVerts; ++i)
		m_verts[i] = p.m_verts[i];
	m_numVerts = p.m_numVerts;
	m_plane = p.m_plane;
	return *this;
}

template<typename TVertex, typename TPlane, int TSize>
inline bool Winding<TVertex, TPlane, stack_tag<TSize> >::DefaultEquals::operator () (const TVertex &a, const TVertex &b) const {
	return a.NearlyEquals(b, epsilon);
}

template<typename TVertex, typename TPlane, int TSize>
inline void Winding<TVertex, TPlane, stack_tag<TSize> >::push_back(const VertexType &v) {
	RAD_ASSERT(m_numVerts < TSize);
	if (m_numVerts < TSize) {
		m_verts[m_numVerts] = v;
		++m_numVerts;
	}
}

template<typename TVertex, typename TPlane, int TSize>
inline void Winding<TVertex, TPlane, stack_tag<TSize> >::erase(int i) {
	RAD_ASSERT(i <= m_numVerts);
	RAD_ASSERT(i >= 0);

	for (int k = i+1; k < m_numVerts; ++k) {
		m_verts[k-1] = m_verts[k];
	}

	--m_numVerts;
}

} // math

#include "../PopSystemMacros.h"
