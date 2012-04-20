// Map.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "../World/Keys.h"
#include <Runtime/Math/Math.h>
#include <Runtime/Math/Vector.h>
#include <Runtime/Math/Plane.h>
#include <Runtime/Math/Quaternion.h>
#include <Runtime/Math/Matrix.h>
#include <Runtime/Math/ConvexPolygon.h>
#include <Runtime/Math/AABB.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/StringBase.h>
#include <string>

namespace tools {

RAD_ZONE_DEC(RADENG_API, Z3DX);

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

	typedef typename zone_vector<Vec3, Z3DXT>::type Vec3Vec;
	typedef typename zone_vector<Mat4, Z3DXT>::type Mat4Vec;

	struct Material
	{
		String name;
	};

	typedef typename zone_vector<Material, Z3DXT>::type MatVec;
	typedef typename zone_vector<String, Z3DXT>::type StringVec;

	struct BoneWeight
	{
		int bone;
		float weight;

		bool operator == (const BoneWeight &w) const
		{
			return bone == w.bone && weight == w.weight;
		}

		bool operator != (const BoneWeight &w) const
		{
			return !(*this == w);
		}
	};

	typedef typename zone_vector<BoneWeight, Z3DXT>::type BoneWeights;

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

		// NOTE: < > <= >= do not take into account the normal.

		bool operator < (const TriVert &v) const
		{
			if (pos[0] < v.pos[0] ||
				(pos[0] == v.pos[0] && pos[1] < v.pos[1]) ||
				(pos[0] == v.pos[0] && pos[1] == v.pos[1] && pos[2] < v.pos[2]))
			{
				return true;
			}

			if (pos == v.pos)
			{
				for (int i = 0; i < MaxUVChannels; ++i)
				{
					if (i > 0)
					{
						for (int k = 0; k < i; ++k)
						{
							if (st[k][0] != v.st[k][0] ||
								st[k][1] != v.st[k][1])
									return false;
						}
					}

					if (st[i][0] < v.st[i][0] ||
						(st[i][0] == v.st[i][0] && st[i][1] < v.st[i][1]))
					{
						return true;
					}
				}
			}
			return false;
		}

		bool operator <= (const TriVert &v) const
		{
			return (*this < v) || (*this == v);
		}

		bool operator > (const TriVert &v) const
		{
			if (pos[0] > v.pos[0] ||
				(pos[0] == v.pos[0] && pos[1] > v.pos[1]) ||
				(pos[0] == v.pos[0] && pos[1] == v.pos[1] && pos[2] > v.pos[2]))
			{
				return true;
			}

			if (pos == v.pos)
			{
				for (int i = 0; i < MaxUVChannels; ++i)
				{
					if (i > 0)
					{
						for (int k = 0; k < i; ++k)
						{
							if (st[k][0] != v.st[k][0] ||
								st[k][1] != v.st[k][1])
									return false;
						}
					}

					if (st[i][0] > v.st[i][0] ||
						(st[i][0] == v.st[i][0] && st[i][1] > v.st[i][1]))
					{
						return true;
					}
				}
			}
			return false;
		}

		bool operator >= (const TriVert &v) const
		{
			return (*this > v) || (*this == v);
		}

		bool operator == (const TriVert &v) const
		{
			for (int i = 0; i < 3; i++)
				if (pos[i] != v.pos[i])
					return false;

			for (int i = 0; i < MaxUVChannels; ++i)
				for (int j = 0; j < 2; ++j)
					if (st[i][j] != v.st[i][j])
						return false;

			return true;
		}

		bool operator != (const TriVert &v) const
		{
			return !(*this == v);
		}

		Vec3 pos; // rounded vertex position.
		Vec3 orgPos; // original vertex position.
		Vec2 st[MaxUVChannels];
		Vec3 normal;
		Vec3 color;
		BoneWeights weights;
	};

	// adds < > <= >= that test for normal.
	struct NormalTriVert : public TriVert
	{
		NormalTriVert() {}
		explicit NormalTriVert(const TriVert &v) : TriVert(v) {}

		bool operator < (const NormalTriVert &v) const
		{
			if (TriVert::operator < (v))
				return true;

			if (TriVert::operator == (v))
			{
				if (this->normal[0] < v.normal[0] ||
				(this->normal[0] == v.normal[0] && this->normal[1] < v.normal[1]) ||
				(this->normal[0] == v.normal[0] && this->normal[1] == v.normal[1] && this->normal[2] < v.normal[2]))
				{
					return true;
				}
			}

			return false;
		}

		bool operator <= (const NormalTriVert &v) const
		{
			return (*this < v) || (*this == v);
		}

		bool operator > (const NormalTriVert &v) const
		{
			if (TriVert::operator > (v))
				return true;

			if (TriVert::operator == (v))
			{
				if (this->normal[0] > v.normal[0] ||
				(this->normal[0] == v.normal[0] && this->normal[1] > v.normal[1]) ||
				(this->normal[0] == v.normal[0] && this->normal[1] == v.normal[1] && this->normal[2] > v.normal[2]))
				{
					return true;
				}
			}

			return false;
		}

		bool operator >= (const NormalTriVert &v) const
		{
			return (*this > v) || (*this == v);
		}

		bool operator == (const NormalTriVert &v) const
		{
			if (TriVert::operator != (v))
				return false;

			for (int i = 0; i < 3; i++)
				if (this->normal[i] != v.normal[i])
					return false;

			return true;
		}

		bool operator != (const NormalTriVert &v) const
		{
			return !(*this == v);
		}
	};

	// adds < > <= >= that test for bone weights.
	struct WeightedTriVert : public TriVert
	{
		WeightedTriVert() {}
		explicit WeightedTriVert(const TriVert &v) : TriVert(v) {}

		bool operator < (const WeightedTriVert &v) const
		{
			if (TriVert::operator < (v))
				return true;
			if (TriVert::operator == (v))
			{
				if (TriVert::weights.size() < v.weights.size())
					return true;
				if (TriVert::weights.size() == v.weights.size())
				{
					bool eq = true;
					for (size_t i = 0; i < TriVert::weights.size() && eq; ++i)
					{
						const BoneWeight &a = TriVert::weights[i];
						const BoneWeight &b = v.weights[i];
						if (a.bone < b.bone)
							return true;
						if (a.bone == b.bone && a.weight < b.weight)
							return true;
						eq = a == b;
					}
				}
			}
			return false;
		}

		bool operator <= (const WeightedTriVert &v) const
		{
			return (*this < v) || (*this == v);
		}

		bool operator > (const WeightedTriVert &v) const
		{
			if (TriVert::operator > (v))
				return true;
			if (TriVert::operator == (v))
			{
				if (TriVert::weights.size() > v.weights.size())
					return true;
				if (TriVert::weights.size() == v.weights.size())
				{
					bool eq = true;
					for (size_t i = 0; i < TriVert::weights.size() && eq; ++i)
					{
						const BoneWeight &a = TriVert::weights[i];
						const BoneWeight &b = v.weights[i];
						if (a.bone > b.bone)
							return true;
						if (a.bone == b.bone && a.weight > b.weight)
							return true;
						eq = a == b;
					}
				}
			}
			return false;
		}

		bool operator >= (const WeightedTriVert &v) const
		{
			return (*this > v) || (*this == v);
		}

		bool operator == (const WeightedTriVert &v) const
		{
			if (TriVert::operator == (v))
			{
				if (TriVert::weights.size() == v.weights.size())
				{
					for (size_t i = 0; i < TriVert::weights.size(); ++i)
					{
						const BoneWeight &a = TriVert::weights[i];
						const BoneWeight &b = v.weights[i];
						if (a != b)
							return false;
					}

					return true;
				}
			}

			return false;
		}

		bool operator != (const WeightedTriVert &v) const
		{
			return !(*this == v);
		}
	};

	struct WeightedNormalTriVert : public NormalTriVert
	{
		WeightedNormalTriVert() {}
		explicit WeightedNormalTriVert(const TriVert &v) : NormalTriVert(v) {}
		explicit WeightedNormalTriVert(const NormalTriVert &v) : NormalTriVert(v) {}

		bool operator < (const WeightedNormalTriVert &v) const
		{
			if (NormalTriVert::operator < (v))
				return true;
			if (NormalTriVert::operator == (v))
			{
				if (NormalTriVert::weights.size() < v.weights.size())
					return true;
				if (NormalTriVert::weights.size() == v.weights.size())
				{
					bool eq = true;
					for (size_t i = 0; i < NormalTriVert::weights.size() && eq; ++i)
					{
						const BoneWeight &a = NormalTriVert::weights[i];
						const BoneWeight &b = v.weights[i];
						if (a.bone < b.bone)
							return true;
						if (a.bone == b.bone && a.weight < b.weight)
							return true;
						eq = a == b;
					}
				}
			}
			return false;
		}

		bool operator <= (const WeightedNormalTriVert &v) const
		{
			return (*this < v) || (*this == v);
		}

		bool operator > (const WeightedNormalTriVert &v) const
		{
			if (NormalTriVert::operator > (v))
				return true;
			if (NormalTriVert::operator == (v))
			{
				if (NormalTriVert::weights.size() > v.weights.size())
					return true;
				if (NormalTriVert::weights.size() == v.weights.size())
				{
					bool eq = true;
					for (size_t i = 0; i < NormalTriVert::weights.size() && eq; ++i)
					{
						const BoneWeight &a = NormalTriVert::weights[i];
						const BoneWeight &b = v.weights[i];
						if (a.bone > b.bone)
							return true;
						if (a.bone == b.bone && a.weight > b.weight)
							return true;
						eq = a == b;
					}
				}
			}
			return false;
		}

		bool operator >= (const WeightedNormalTriVert &v) const
		{
			return (*this > v) || (*this == v);
		}

		bool operator == (const WeightedNormalTriVert &v) const
		{
			if (NormalTriVert::operator == (v))
			{
				if (NormalTriVert::weights.size() == v.weights.size())
				{
					for (size_t i = 0; i < NormalTriVert::weights.size(); ++i)
					{
						const BoneWeight &a = NormalTriVert::weights[i];
						const BoneWeight &b = v.weights[i];
						if (a != b)
							return false;
					}

					return true;
				}
			}

			return false;
		}

		bool operator != (const WeightedNormalTriVert &v) const
		{
			return !(*this == v);
		}
	};

	struct TriModel;
	typedef typename zone_vector<TriVert, Z3DXT>::type TriVertVec;
	typedef typename zone_vector<NormalTriVert, Z3DXT>::type NormalTriVertVec;

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
		zone_vector<int, Z3DXT>::type areas;
		TriModel *model;
	};

	typedef typename zone_vector<TriFace, Z3DXT>::type TriFaceVec;

	typedef typename zone_vector<BoneWeights, Z3DXT>::type Skin;
	typedef boost::shared_ptr<Skin> SkinRef;

	struct BoneTM
	{
		Quat r;
		Vec3 s;
		Vec3 t;
	};

	struct Bone
	{
		String name;
		int parent;
		Mat4 world;
	};

	struct BonePose
	{
		BoneTM m;
		float fov;
		String tag;
	};

	typedef typename zone_vector<Bone, Z3DXT>::type BoneVec;

	struct Skel
	{
		typedef boost::shared_ptr<Skel> Ref;
		BoneVec bones;
	};

	typedef typename zone_vector<typename Skel::Ref, Z3DXT>::type SkelVec;
	typedef typename zone_vector<BonePose, Z3DXT>::type BonePoseVec;
	typedef typename zone_vector<BonePoseVec, Z3DXT>::type BoneFrames;

	struct Anim
	{
		typedef boost::shared_ptr<Anim> Ref;
		String name;
		U32 frameRate;
		bool looping;
		int firstFrame;
		BoneFrames frames;
	};

	typedef typename zone_map<String, typename Anim::Ref, Z3DXT>::type AnimMap;

	struct TriModel
	{
		typedef boost::shared_ptr<TriModel> Ref;
		typedef typename zone_vector<Ref, Z3DXT>::type Vec;

		TriModel() : outside(true), cinematic(false), hideUntilRef(false), hideWhenDone(false) {}

		TriVertVec verts;
		TriFaceVec tris;
		SkinRef skin;
		AnimMap anims;
		BBox bounds;
		int contents;
		int id;
		int skel;
		int numChannels;
		bool outside;
		bool cinematic;
		bool hideUntilRef;
		bool hideWhenDone;
	};

	struct Entity
	{
		typedef boost::shared_ptr<Entity> Ref;
		typedef typename zone_vector<Ref, Z3DXT>::type Vec;

		Entity() : maxEnt(false), id(-1) {}
		String name;
		world::Keys keys;
		Vec3 origin;
		typename TriModel::Vec models;
		SkelVec skels;
		bool maxEnt;
		int id;
	};

	struct Camera
	{
		typedef boost::shared_ptr<Camera> Ref;
		typedef typename zone_vector<Ref, Z3DXT>::type Vec;
		String name;
		int firstFrame;
		AnimMap anims;
	};

	typename Entity::Vec ents;
	MatVec               mats;
	typename Entity::Ref worldspawn;
	typename Camera::Vec cameras;

	typename Entity::Ref EntForName(const char *name) const
	{
		for (typename Entity::Vec::const_iterator it = ents.begin(); it != ents.end(); ++it)
		{
			if ((*it)->name == name) return *it;
		}
		return typename Entity::Ref();
	}
};

template <typename T>
const T MapT<T>::MaxRange = T(16384);

template <typename T>
const T MapT<T>::HalfRange = T(16384/2);

typedef MapT<float> Map;

typedef boost::shared_ptr<Map> MapRef;
typedef zone_vector<MapRef, Z3DXT>::type MapVec;

} // tools
