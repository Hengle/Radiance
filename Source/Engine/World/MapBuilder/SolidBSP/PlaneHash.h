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
		Snap(z);
		
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

	void Snap(::tools::solid_bsp::Plane &p) {
		for (int i = 0; i < 3; ++i) {
			ValueType d = math::Abs(p.Normal()[i] - ValueType(1));
			if (d < ValueType(0.00001)) {
				Vec3 normal(Vec3::Zero);
				normal[i] = ValueType(1);
				p.Initialize(normal, p.D());
				break;
			}

			d = math::Abs(p.Normal()[i] + ValueType(1));
			if (d < ValueType(0.00001)) {
				Vec3 normal(Vec3::Zero);
				normal[i] = ValueType(-1);
				p.Initialize(normal, p.D());
				break;
			}
		}

		ValueType f = math::Floor(p.D()+ValueType(0.5));
		if (math::Abs(p.D()-f) < ValueType(0.0001)) {
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
		if (math::Abs(a.Normal()[0] - b.Normal()[0]) < ValueType(0.000001)
			&& math::Abs(a.Normal()[1] - b.Normal()[1]) < ValueType(0.000001)
			&& math::Abs(a.Normal()[2] - b.Normal()[2]) < ValueType(0.000001)
			&& math::Abs(a.D() - b.D()) < ValueType(0.01) 
		) {
			return true;
		}

		return false;
	}

	HashPlane *m_hash[NumBuckets];
	PlaneVec m_planes;
};

} // box_bsp
} // tools
