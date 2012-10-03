// ConvexPolygon.inl
// Convex Polygon class (inlines)
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Vector.h"
#include <algorithm>

#include "../PushSystemMacros.h"

namespace math {

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator>::ConvexPolygon()
{
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator>::ConvexPolygon(const SelfType &p)
{
	Initialize(p);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator>::ConvexPolygon(const VertexListType &v)
{
	Initialize(v);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator>::ConvexPolygon(const VertexListType &v, const PlaneType &p)
{
	Initialize(v, p);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator>::ConvexPolygon(const VertexType *v, UReg count)
{
	Initialize(v, count);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator>::ConvexPolygon(const VertexType *v, UReg count, const PlaneType &p)
{
	Initialize(v, count, p);
}

template<typename TVertex, typename TPlane, typename TAllocator>
ConvexPolygon<TVertex, TPlane, TAllocator>::ConvexPolygon(const PlaneType &p, const ValueType &size)
{
	Initialize(p, size);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator>::~ConvexPolygon()
{
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator> &ConvexPolygon<TVertex, TPlane, TAllocator>::Initialize(const SelfType &p)
{
	m_verts = p.m_verts;
	m_plane = p.m_plane;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator> &ConvexPolygon<TVertex, TPlane, TAllocator>::Initialize(const VertexListType &v)
{
	m_verts = v;
	CalcPlane();
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator> &ConvexPolygon<TVertex, TPlane, TAllocator>::Initialize(const VertexListType &v, const PlaneType &p)
{
	m_verts = v;
	m_plane = p;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
ConvexPolygon<TVertex, TPlane, TAllocator> &ConvexPolygon<TVertex, TPlane, TAllocator>::Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2)
{
	m_verts.reserve(3);
	m_verts.clear();
	m_verts.push_back(v0);
	m_verts.push_back(v1);
	m_verts.push_back(v2);
	CalcPlane();
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
ConvexPolygon<TVertex, TPlane, TAllocator> &ConvexPolygon<TVertex, TPlane, TAllocator>::Initialize(const VertexType &v0, const VertexType &v1, const VertexType &v2, const PlaneType &p)
{
	m_verts.reserve(3);
	m_verts.clear();
	m_verts.push_back(v0);
	m_verts.push_back(v1);
	m_verts.push_back(v2);
	m_plane = p;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
ConvexPolygon<TVertex, TPlane, TAllocator> &ConvexPolygon<TVertex, TPlane, TAllocator>::Initialize(const VertexType *p, UReg count)
{
	RAD_ASSERT(p);
	RAD_ASSERT(count);

	m_verts.clear();
	for (UReg i = 0; i < count; i++)
	{
		m_verts.push_back(p[i]);
	}

	CalcPlane();
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
ConvexPolygon<TVertex, TPlane, TAllocator> &ConvexPolygon<TVertex, TPlane, TAllocator>::Initialize(const VertexType *v, UReg count, const PlaneType &p)
{
	RAD_ASSERT(v);
	RAD_ASSERT(count);

	m_verts.clear();
	m_verts.reserve(count);

	for (UReg i = 0; i < count; i++)
	{
		m_verts.push_back(v[i]);
	}

	m_plane = p;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
ConvexPolygon<TVertex, TPlane, TAllocator> &ConvexPolygon<TVertex, TPlane, TAllocator>::Initialize(const PlaneType &p, const ValueType &size)
{
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
inline bool ConvexPolygon<TVertex, TPlane, TAllocator>::Empty() const
{
	return m_verts.empty();
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline typename ConvexPolygon<TVertex, TPlane, TAllocator>::VertexListType &ConvexPolygon<TVertex, TPlane, TAllocator>::Vertices()
{
	return m_verts;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline const typename ConvexPolygon<TVertex, TPlane, TAllocator>::VertexListType &ConvexPolygon<TVertex, TPlane, TAllocator>::Vertices() const
{
	return m_verts;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator> &ConvexPolygon<TVertex, TPlane, TAllocator>::Flip()
{
	std::reverse(m_verts.begin(), m_verts.end());
	m_plane = -m_plane;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline ConvexPolygon<TVertex, TPlane, TAllocator> &ConvexPolygon<TVertex, TPlane, TAllocator>::Translate(const Vector3<ValueType> &t)
{
	for (typename VertexListType::iterator v = m_verts.begin(); v != m_verts.end(); ++v)
	{
		(*v) += t;
	}

	m_plane.Translate(t);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline void ConvexPolygon<TVertex, TPlane, TAllocator>::CalcPlane()
{
	RAD_ASSERT(m_verts.size() >= 3);
	m_plane.Initialize( m_verts[0], m_verts[1], m_verts[2] );
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline const typename ConvexPolygon<TVertex, TPlane, TAllocator>::PlaneType &ConvexPolygon<TVertex, TPlane, TAllocator>::Plane() const
{
	return m_plane;
}

template<typename TVertex, typename TPlane, typename TAllocator>
typename ConvexPolygon<TVertex, TPlane, TAllocator>::SideType ConvexPolygon<TVertex, TPlane, TAllocator>::Side(const PlaneType &p, const ValueType &epsilon) const
{
	RAD_ASSERT(!m_verts.empty());
	UReg front = 0, back = 0, on = 0;
	
	for (typename VertexListType::const_iterator i = m_verts.begin(); i != m_verts.end(); ++i)
	{
		ValueType d = p.Distance(*i);
		if (d > epsilon)       { front++; }
		else if (d < -epsilon) { back++; }
		else                   { on++; }

		if (front && back) break;
	}

	RAD_ASSERT( front || back || on );

	if (front && !back)      { return PlaneType::Front; }
	else if (back && !front) { return PlaneType::Back; }
	else if (front && back)  { return PlaneType::Cross; }
	else                     { return PlaneType::On; }
}

template<typename TVertex, typename TPlane, typename TAllocator>
typename ConvexPolygon<TVertex, TPlane, TAllocator>::ValueType ConvexPolygon<TVertex, TPlane, TAllocator>::MaxPlaneDist(const PlaneType &p) const
{
	RAD_ASSERT(!m_verts.empty());
	ValueType max = -std::numeric_limits<ValueType>::max();
	ValueType min = std::numeric_limits<ValueType>::max();

	for (UReg i = 0; i < (UReg)m_verts.size(); i++)
	{
		ValueType d = p.Distance(m_verts[i]);
		if (d < min) min = d;
		if (d > max) max = d;
	}

	return (Abs(min) > Abs(max)) ? min : max;
}

template<typename TVertex, typename TPlane, typename TAllocator>
typename ConvexPolygon<TVertex, TPlane, TAllocator>::SideType ConvexPolygon<TVertex, TPlane, TAllocator>::MajorSide(const PlaneType &p, const ValueType &epsilon) const
{
	RAD_ASSERT(!m_verts.empty());
	ValueType side = MaxPlaneDist(p);

	return (Abs(side)>=epsilon) ? ((side > ValueType(0)) ? PlaneType::Front : ((side < ValueType(0)) ? PlaneType::Back : PlaneType::On)) : PlaneType::On;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline void ConvexPolygon<TVertex, TPlane, TAllocator>::Chop(const PlaneType &p, SideType side, SelfType &poly, const ValueType &epsilon) const
{
	RAD_ASSERT( side == PlaneType::Front || side == PlaneType::Back );
		
	if (side == PlaneType::Front)
	{
		Split(p, &poly, 0, epsilon);
	}
	else
	{
		Split(p, 0, &poly, epsilon);
	}
}

template<typename TVertex, typename TPlane, typename TAllocator>
void ConvexPolygon<TVertex, TPlane, TAllocator>::Split(const PlaneType &p, SelfType *front, SelfType *back, const ValueType &epsilon) const
{
	if (!front && !back) return;

	VertexListType *frontVerts = 0, *backVerts = 0;

	if (front) frontVerts = &(front->m_verts);
	if (back) backVerts  = &(back->m_verts);

	if (frontVerts) frontVerts->clear();
	if (backVerts) backVerts->clear();

	std::vector<SideType, TAllocator> sides(TAllocator::template rebind<SideType>::other(m_verts.get_allocator())); // rebind to same allocator.
	std::vector<ValueType, TAllocator> dots(TAllocator::template rebind<ValueType>::other(m_verts.get_allocator()));
	UReg counts[PlaneType::NumSides];

	memset(counts, 0, sizeof(counts));

	sides.reserve(m_verts.size() + 1);
	dots.reserve(m_verts.size() + 1);

	// classify.
	for (UReg i = 0; i < m_verts.size(); i++)
	{
		ValueType d = p.Distance(m_verts[i]);

		if (d > epsilon)
		{
			sides.push_back(PlaneType::Front);
		}
		else if (d < -epsilon)
		{
			sides.push_back(PlaneType::Back);
		}
		else
		{
			sides.push_back(PlaneType::On);
		}

		dots.push_back(d);
		counts[sides[i]]++;
	}

	if (counts[PlaneType::On] == m_verts.size()) return;

	sides.push_back(sides[0]); // eliminates modulus operator later.
	dots.push_back(dots[0]);

	RAD_ASSERT(counts[PlaneType::Front] || counts[PlaneType::Back]);

	if (!counts[PlaneType::Back]) // all on front
	{
		if (front) *front = *this;
		return;
	}
	else if (!counts[PlaneType::Front])
	{
		if (back) *back = *this;
		return;
	}

	// count.
	{
		int fc = 0, bc = 0;

		for (UReg i = 0; i < m_verts.size(); i++)
		{
			switch (sides[i])
			{
			case PlaneType::Front:
				fc++;
				break;
			case PlaneType::Back:
				bc++;
				break;
			case PlaneType::On:
				fc++; bc++;
				continue;
			}

			// does this edge cross the plane?
			if (sides[i + 1] != PlaneType::On && sides[i + 1] != sides[i]) // yes.
			{
				fc++; bc++;
			}
		}

		if (frontVerts) frontVerts->reserve(fc);
		if (backVerts) backVerts->reserve(bc);
	}

	// split
	for (UReg i = 0; i < m_verts.size(); i++)
	{
		switch (sides[i])
		{
		case PlaneType::Front:
			if (frontVerts) frontVerts->push_back(m_verts[i]);
			break;
		case PlaneType::Back:
			if (backVerts) backVerts->push_back(m_verts[i]);
			break;
		case PlaneType::On:
			if (frontVerts) frontVerts->push_back(m_verts[i]);
			if (backVerts) backVerts->push_back(m_verts[i]);
			continue;
		}

		// does this edge cross the plane?
		if (sides[i + 1] != PlaneType::On && sides[i + 1] != sides[i]) // yes.
		{
			UReg next = (i + 1) % m_verts.size();
				
			TVertex mid = PlaneType::LineSegmentIntersect(m_verts[i], dots[i], m_verts[next], dots[next]);

			if (frontVerts) frontVerts->push_back(mid);
			if (backVerts) backVerts->push_back(mid);
		}
	}

	if (front) front->m_plane = m_plane;
	if (back)  back->m_plane = m_plane;
}

template<typename TVertex, typename TPlane, typename TAllocator>
template<typename EqualsType>
bool ConvexPolygon<TVertex, TPlane, TAllocator>::RemoveBadVerts(const EqualsType &vertsEqual, const ValueType &polyPlaneEpsilon, const ValueType &edgeEpsilon)
{
	bool removed = false;

	// remove duplicate vertices.
	for (int i = 0; i < (int)m_verts.size(); ++i)
	{
		for (int k = i + 1; k < (int)m_verts.size(); ++k)
		{
			if( vertsEqual(m_verts[i], m_verts[k]) )
			{
				m_verts.erase(m_verts.begin() + k);
				k = i + 1;
				removed = true;
			}
		}
	}

	// remove verts not on the plane.
	for (typename VertexListType::iterator i = m_verts.begin(); i != m_verts.end();)
	{
		if (m_plane.On(*i, polyPlaneEpsilon))
		{
			++i;
		}
		else
		{
			i = m_verts.erase(i);
			removed = true;
		}
	}

	// remove colinear edge vertices.
	if (m_verts.size() > 3)
	{
		for (UReg i = 0; !m_verts.empty() && (i < m_verts.size()-1); ++i)
		{
			UReg k = i + 1;

			PlaneType edgePlane;
			Vector3<ValueType> edge = m_verts[i] - m_verts[k];
			{
				Vector3<ValueType> unitEdge = m_plane.Normal().Cross(edge).Unit();
				// put winding on front.
				edgePlane.Initialize(unitEdge, unitEdge.Dot(m_verts[i]));
			}

			for (UReg j = (k + 1) % m_verts.size(); j < m_verts.size();)
			{
				if (edgePlane.On(m_verts[j], edgeEpsilon))
				{
					// colinear.
					Vector3<ValueType> tempEdge = m_verts[j] - m_verts[i];
					if (edge.MagnitudeSquared() >= tempEdge.MagnitudeSquared())
					{
						m_verts.erase(m_verts.begin()+j);
						removed = true;
					}
					else
					{
						edge = tempEdge;
						m_verts.erase(m_verts.begin()+k);
						k = j; 
						j++;
						removed = true;
					}
				}
				else
				{
					break;
				}
			}
		}
	}

	return removed;
}

template<typename TVertex, typename TPlane, typename TAllocator>
bool ConvexPolygon<TVertex, TPlane, TAllocator>::IsConvex(const ValueType &epsilon) const
{
	if (m_verts.size() >= 3)
	{
		PlaneType edgePlane;

		for (UReg i = 0; i < m_verts.size(); i++)
		{
			UReg k = (i + 1) % m_verts.size();
			{
				Vector3<ValueType> edge = m_plane.Normal().Cross(m_verts[i] - m_verts[k]).Unit();
				edgePlane.Initialize(edge, edge.Dot(m_verts[i]));
			}
			// make sure all points are in front of this edge!
			for (k = 0; k < m_verts.size(); k++)
			{
				if (edgePlane.Side(m_verts[k], epsilon) == PlaneType::Back)
				{
					return false;
				}
			}
		}

		return true;
	}
	else
	{
		return false;
	}
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline void ConvexPolygon<TVertex, TPlane, TAllocator>::Swap(SelfType &p)
{
	PlaneType temp = m_plane;
	m_plane = p.m_plane;
	p.m_plane = temp;
	m_verts.swap(p.m_verts);
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline void ConvexPolygon<TVertex, TPlane, TAllocator>::Clear() {
	m_verts.clear();
}

template<typename TVertex, typename TPlane, typename TAllocator>
template<typename EqualsType>
inline bool ConvexPolygon<TVertex, TPlane, TAllocator>::Valid(const EqualsType &vertsEqual, const ValueType &polyPlaneEpsilon, const ValueType &edgeEpsilon, const ValueType &convexityEpsilon) const
{
	if( (m_verts.size() >= 3) && IsConvex(convexityEpsilon) )
	{
		ConvexPolygon copy = *this;
		return !copy.RemoveBadVerts(vertsEqual, polyPlaneEpsilon, edgeEpsilon);
	}
	else
	{
		return false;
	}
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline typename ConvexPolygon<TVertex, TPlane, TAllocator>::SelfType &ConvexPolygon<TVertex, TPlane, TAllocator>::operator = (const VertexListType &v)
{
	m_verts = v;
	CalcPlane();
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline typename ConvexPolygon<TVertex, TPlane, TAllocator>::SelfType &ConvexPolygon<TVertex, TPlane, TAllocator>::operator = (const SelfType & p)
{
	m_verts = p.m_verts;
	m_plane = p.m_plane;
	return *this;
}

template<typename TVertex, typename TPlane, typename TAllocator>
inline bool ConvexPolygon<TVertex, TPlane, TAllocator>::DefaultEquals::operator () (const TVertex &a, const TVertex &b) const
{
	return a.NearlyEquals(b, epsilon);
}

} // math

#include "../PopSystemMacros.h"
