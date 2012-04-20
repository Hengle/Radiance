// Map.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Runtime/Base.h"
#include "../../Runtime/Math/Math.h"
#include "../../Runtime/Math/Vector.h"
#include "../../Runtime/Math/Plane.h"
#include "../../Runtime/Math/Quaternion.h"
#include "../../Runtime/Math/Matrix.h"
#include "../../Runtime/Math/ConvexPolygon.h"
#include "../../Runtime/Math/AABB.h"
#include "../../Runtime/Container/HashMap.h"
#include "../../Runtime/StringBase.h"
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

template <typename T>
class MapT
{
public:

	typedef T ValueType;

	enum
	{
		InvalidMatId = -1,
		MaxUVChannels = 2,

		RAD_FLAG_BIT(ContentsAreaportal, 0),
		RAD_FLAG(ContentsSolid),
		RAD_FLAG(ContentsDetail),
		RAD_FLAG(ContentsNoClip),
		RAD_FLAG(ContentsNoDraw),
		FirstVisibleContents = ContentsAreaportal,
		LastVisibleContents = ContentsSolid,
		VisibleContents = ContentsSolid|ContentsAreaportal,
		StructuralContents = ContentsSolid|ContentsAreaportal,
		SolidContents = ContentsSolid
	};

	static const T MaxRange;
	static const T HalfRange;

	typedef math::Plane<T> Plane;
	typedef math::Vector2<T> Vec2;
	typedef math::Vector3<T> Vec3;
	typedef math::Vector4<T> Vec4;
	typedef math::Matrix4X4<T> Mat4;
	typedef math::Quaternion<T> Quat;
	typedef math::ConvexPolygon<Vec3, Plane> Winding;
	typedef math::AABB3<T> BBox;

	typedef std::vector<Vec3> Vec3Vec;

	struct Material
	{
		std::string name;
	};

	typedef std::vector<Material> MatVec;

	struct TriVert
	{
		typedef T ValueType;

		TriVert() {}
		explicit TriVert(const Vec3 &v) : pos(v) {}
		operator Vec3 () const { return pos; }

		// operators that let us specialize ConvexPolygon on this vertex type.

		TriVert operator + (const TriVert &v) const
		{
			TriVert x;

			x.pos = pos + v.pos;
			x.orgPos = orgPos + v.orgPos;
			for (int i = 0; i < MaxUVChannels; ++i)
			{
				x.st[i] = st[i] + v.st[i];
			}
			x.normal = normal + v.normal;
			x.color = color + v.color;

			return x;
		}

		TriVert operator - (const TriVert &v) const
		{
			TriVert x;

			x.pos = pos - v.pos;
			x.orgPos = orgPos - v.orgPos;
			for (int i = 0; i < MaxUVChannels; ++i)
			{
				x.st[i] = st[i] - v.st[i];
			}
			x.normal = normal - v.normal;
			x.color = color - v.color;

			return x;
		}

		TriVert operator * (const TriVert &v) const
		{
			TriVert x;

			x.pos = pos * v.pos;
			x.orgPos = orgPos * v.orgPos;
			for (int i = 0; i < MaxUVChannels; ++i)
			{
				x.st[i] = st[i] * v.st[i];
			}
			x.normal = normal * v.normal;
			x.color = color * v.color;

			return x;
		}

		TriVert operator / (const TriVert &v) const
		{
			TriVert x;

			x.pos = pos / v.pos;
			x.orgPos = orgPos / v.orgPos;
			for (int i = 0; i < MaxUVChannels; ++i)
			{
				x.st[i] = st[i] / v.st[i];
			}
			x.normal = normal / v.normal;
			x.color = color / v.color;

			return x;
		}

		TriVert &operator = (const TriVert &v)
		{
			pos = v.pos;
			orgPos = v.orgPos;
			for (int i = 0; i < MaxUVChannels; ++i)
			{
				st[i] = v.st[i];
			}

			normal = v.normal;
			color = v.color;
			return *this;
		}

		TriVert &operator += (const TriVert &v)
		{
			*this = *this + v;
			return *this;
		}

		TriVert &operator -= (const TriVert &v)
		{
			*this = *this - v;
			return *this;
		}

		TriVert &operator *= (const TriVert &v)
		{
			*this = *this * v;
			return *this;
		}

		TriVert &operator /= (const TriVert &v)
		{
			*this = *this / v;
			return *this;
		}

		TriVert operator * (ValueType s) const
		{
			TriVert x;

			x.pos = pos * s;
			x.orgPos = orgPos * s;
			for (int i = 0; i < MaxUVChannels; ++i)
			{
				x.st[i] = st[i] * s;
			}
			x.normal = normal * s;
			x.color = color * s;

			return x;
		}

		TriVert operator / (ValueType s) const
		{
			TriVert x;

			x.pos = pos / s;
			x.orgPos = orgPos / s;
			for (int i = 0; i < MaxUVChannels; ++i)
			{
				x.st[i] = st[i] / s;
			}
			x.normal = normal / s;
			x.color = color / s;

			return x;
		}

		TriVert &operator *= (const ValueType s)
		{
			*this = *this * s;
			return *this;
		}

		TriVert &operator /= (const ValueType s)
		{
			*this = *this / s;
			return *this;
		}

		Vec3 pos; // rounded vertex position.
		Vec3 orgPos; // original vertex position.
		Vec2 st[MaxUVChannels];
		Vec3 normal;
		Vec3 color;
	};

	struct TriModel;
	typedef std::vector<TriVert> TriVertVec;

	struct TriFace
	{
		TriFace() : outside(true), shared(-1)
		{
		}

		TriFace(unsigned int a,
			unsigned int b,
			unsigned int c,
			int mat,
			const Plane &plane,
			TriModel *model) : outside(true), shared(-1)
		{
			this->mat = mat;
			this->plane = plane;
			this->model = model;
			v[0] = a;
			v[1] = b;
			v[2] = c;
		}

		Plane plane;
		unsigned int v[3];
		int mat;
		int shared;
		bool outside;
		std::vector<int> areas;
		TriModel *model;
	};

	typedef std::vector<TriFace> TriFaceVec;

	struct TriModel
	{
		TriModel() : outside(true) {}

		TriVertVec verts;
		TriFaceVec tris;
		BBox bounds;
		int contents;
		int id;
		bool outside;
	};

	typedef boost::shared_ptr<TriModel> TriModelRef;

	typedef std::vector<TriModelRef> TriModelVec;

	struct KeyPair
	{
		KeyPair();
		KeyPair(const char *name, const char *value)
		{
			this->name = name;
			this->value = value;
		}

		std::string name;
		std::string value;

		int Int() const { return string::atoi(value.c_str()); }
		float Float() const { return string::atof(value.c_str()); }
		double Double() const { return (double)Float(); }
		Vec3 Vec() const
		{
			float v[3];
			string::sscanf("%f %f %f", value.c_str(), &v[0], &v[1], &v[2]);
			return Vec3((T)v[0], (T)v[1], (T)v[2]);
		}

	};

	typedef typename container::hash_map<std::string, KeyPair>::type KeyPairMap;

	struct Entity
	{
		std::string name;
		KeyPairMap  keys;
		Vec3        origin;
		TriModelVec models;
		int id;
	};

	typedef boost::shared_ptr<Entity> EntityRef;
	typedef std::vector<EntityRef> EntityVec;

	EntityVec ents;
	MatVec    mats;
	EntityRef worldspawn;

	EntityRef EntForName(const char *name) const
	{
		for (EntityVec::const_iterator it = ents.begin(); it != ents.end(); ++it)
		{
			if ((*it)->name == name) return *it;
		}
		return EntityRef();
	}

	void Load(const char *filename);
};

template <typename T>
const T MapT<T>::MaxRange = T(16384);

template <typename T>
const T MapT<T>::HalfRange = T(16384/2);

typedef MapT<double> Map;
