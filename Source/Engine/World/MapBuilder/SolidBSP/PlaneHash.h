/*! \file PlaneHash.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#pragma once

#include <vector>
#include <Runtime/Container/ZoneVector.h>

namespace tools {
namespace solid_bsp {

class PlaneHash
{
public:
	typedef zone_vector< ::tools::solid_bsp::Plane, ZToolsT>::type PlaneVec;

	PlaneHash()
	{
		m_planes.reserve(4*Kilo);
		memset(m_hash, 0, sizeof(HashPlane*)*NumBuckets);
	}

	~PlaneHash()
	{
		for (int i = 0; i < NumBuckets; ++i)
		{
			HashPlane *head = m_hash[i];
			while (head)
			{
				HashPlane *next = head->next;
				delete head;
				head = next;
			}
		}
	}
	
	int FindPlaneNum(const ::tools::solid_bsp::Plane &p)
	{
		::tools::solid_bsp::Plane z = p;
		Clamp(z);
		
		int b = BucketNum(z.D());
		HashPlane *hp = m_hash[b];

		while (hp)
		{
			if (PlaneEqual(m_planes[hp->num], z))
			{
				break;
			}

			hp = hp->next;
		}

		if (hp)
			return hp->num;
		
		int x = (int)m_planes.size();
		m_planes.push_back(z);
		m_planes.push_back(-z);

		hp = new HashPlane();
		hp->num = x;
		hp->next = m_hash[b];
		m_hash[b] = hp;

		hp = new HashPlane();
		hp->num = x+1;
		hp->next = m_hash[b];
		m_hash[b] = hp;
		
		return x;
	}

	const ::tools::solid_bsp::Plane &Plane(int num) const
	{
		return m_planes[num];
	}

	const PlaneVec &Planes() const
	{
		return m_planes;
	}

private:

	void Clamp(::tools::solid_bsp::Plane &p)
	{
		if (p.Normal().X() > ValueType(0.995))
		{
			p.Initialize(Vec3(ValueType(1), ValueType(0), ValueType(0)), p.D());
		}
		else if (p.Normal().X() < ValueType(-0.995))
		{
			p.Initialize(Vec3(ValueType(-1), ValueType(0), ValueType(0)), p.D());
		}
		else if (p.Normal().Y() > ValueType(0.995))
		{
			p.Initialize(Vec3(ValueType(0), ValueType(1), ValueType(0)), p.D());
		}
		else if (p.Normal().Y() < ValueType(-0.995))
		{
			p.Initialize(Vec3(ValueType(0), ValueType(-1), ValueType(0)), p.D());
		}
		else if (p.Normal().Z() > ValueType(0.995))
		{
			p.Initialize(Vec3(ValueType(0), ValueType(0), ValueType(1)), p.D());
		}
		else if (p.Normal().Z() < ValueType(-0.995))
		{
			p.Initialize(Vec3(ValueType(0), ValueType(0), ValueType(-1)), p.D());
		}

		ValueType f = math::Floor(p.D()+ValueType(0.5));
		if (math::Abs(f-p.D()) < ValueType(0.0009999999999))
		{
			p.Initialize(p.Normal(), f);
		}
	}

	struct HashPlane
	{
		HashPlane *next;
		int num;
	};

	enum { NumBuckets = 4096 };
	
	int BucketNum(ValueType x)
	{
		int b = math::Abs((int)(x / ValueType(8)));
		return b & (NumBuckets-1);
	}

	bool PlaneEqual(const ::tools::solid_bsp::Plane &a, const ::tools::solid_bsp::Plane &b)
	{
		if (a.Normal().Dot(b.Normal()) > ValueType(0.95)) {
			if (math::Abs(a.D()-b.D()) < ValueType(0.1))
				return true;
		}

		return false;
	}

	HashPlane *m_hash[NumBuckets];
	PlaneVec m_planes;
};

} // box_bsp
} // tools
