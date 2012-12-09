/*! \file SceneFile.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once

#include "../Types.h"
#include "../World/Keys.h"
#include "Progress.h"
#include <Runtime/Math/Math.h>
#include <Runtime/Math/Vector.h>
#include <Runtime/Math/Plane.h>
#include <Runtime/Math/Quaternion.h>
#include <Runtime/Math/Matrix.h>
#include <Runtime/Math/Winding.h>
#include <Runtime/Math/AABB.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/StringBase.h>
#include <string>

namespace tools {

RAD_ZONE_DEC(RADENG_API, Z3DX);

//! A structure which contains a scene loaded from a 3DX file.
/*! The 3DX format stores objects exported from 3DS Max, consisting of geometry,
	cinematics, and camera motion data. It's the data container used for importing
	skeletal animations, and map data.

	The MapBuilder class also loads certain fields in this structure that are not present
	in the 3DX file, like certain types of entities and waypoints.
*/
template <typename T>
class SceneFileT {
public:

	/*
	==============================================================================
	Basic types
	==============================================================================
	*/
	typedef T ValueType;

	enum {
		kInvalidMatId = -1,
		kMaxUVChannels = 2,
	};

	static const T kMaxRange;
	static const T kHalfRange;

	typedef math::Plane<T> Plane;
	typedef math::Vector2<T> Vec2;
	typedef math::Vector3<T> Vec3;
	typedef math::Vector4<T> Vec4;
	typedef math::Matrix4X4<T> Mat4;
	typedef math::Quaternion<T> Quat;
	typedef math::Winding<Vec3, Plane> Winding;
	typedef math::AABB3<T> BBox;

	typedef typename zone_vector<Vec3, Z3DXT>::type Vec3Vec;
	typedef typename zone_vector<Mat4, Z3DXT>::type Mat4Vec;

	/*
	==============================================================================
	   Materials
	==============================================================================
	*/
	struct Material {
		String name;
	};

	typedef typename zone_vector<Material, Z3DXT>::type MatVec;
	typedef typename zone_vector<String, Z3DXT>::type StringVec;

	/*
	==============================================================================
	Bone Weights
	==============================================================================
	*/
	struct BoneWeight {
		int bone;
		float weight;

		bool operator == (const BoneWeight &w) const {
			return bone == w.bone && weight == w.weight;
		}

		bool operator != (const BoneWeight &w) const {
			return !(*this == w);
		}
	};

	typedef typename zone_vector<BoneWeight, Z3DXT>::type BoneWeights;

	/*
	==============================================================================
	Vertices
	==============================================================================
	*/

	//! Vertex data is contained in several classes, each with operators to allow spliting rays and polygons specialized
	//! on a particular vertex type.
	/*! The basic vertex contains fields for everything: position, tangents, uvs, colors etc. Tangents are computed
		during scene loading. The vertex defines operators to allow specializations of a Winding to do proper
		splitting of polygons.

		Additionally the vertex defines operators for comparison < <= > >= == !=. This allows the object to be used
		in associative containers like set, map.

		Comparison operators on this class only take into account the position and uvs.
	*/ 
	struct TriVert {
		typedef T ValueType;

		TriVert() {}
		TriVert(const TriVert &v) {
			*this = v;
		}

		explicit TriVert(const Vec3 &v) : pos(v) {}
		operator Vec3 () const { return pos; }

		// operators that let us specialize Winding on this vertex type.

		TriVert operator + (const TriVert &v) const {
			TriVert x;

			x.pos = pos + v.pos;
			x.orgPos = orgPos + v.orgPos;
			for (int i = 0; i < kMaxUVChannels; ++i) {
				x.st[i] = st[i] + v.st[i];
				x.tangent[i] = Vec4(x.tangent[i] + v.tangent[i], x.tangent[i].W());
			}
			x.normal = normal + v.normal;
			x.color = color + v.color;

			return x;
		}

		TriVert operator - (const TriVert &v) const {
			TriVert x;

			x.pos = pos - v.pos;
			x.orgPos = orgPos - v.orgPos;
			for (int i = 0; i < kMaxUVChannels; ++i) {
				x.st[i] = st[i] - v.st[i];
				x.tangent[i] = Vec4(x.tangent[i] - v.tangent[i], x.tangent[i].W());
			}
			x.normal = normal - v.normal;
			x.color = color - v.color;

			return x;
		}

		TriVert operator * (const TriVert &v) const {
			TriVert x;

			x.pos = pos * v.pos;
			x.orgPos = orgPos * v.orgPos;
			for (int i = 0; i < kMaxUVChannels; ++i) {
				x.st[i] = st[i] * v.st[i];
				x.tangent[i] = Vec4(x.tangent[i] * v.tangent[i], x.tangent[i].W());
			}
			x.normal = normal * v.normal;
			x.color = color * v.color;

			return x;
		}

		TriVert operator / (const TriVert &v) const {
			TriVert x;

			x.pos = pos / v.pos;
			x.orgPos = orgPos / v.orgPos;
			for (int i = 0; i < kMaxUVChannels; ++i) {
				x.st[i] = st[i] / v.st[i];
				x.tangent[i] = Vec4(x.tangent[i] / v.tangent[i], x.tangent[i].W());
			}
			x.normal = normal / v.normal;
			x.color = color / v.color;

			return x;
		}

		TriVert &operator = (const TriVert &v) {
			pos = v.pos;
			orgPos = v.orgPos;
			for (int i = 0; i < kMaxUVChannels; ++i) {
				st[i] = v.st[i];
				tangent[i] = v.tangent[i];
			}

			normal = v.normal;
			color = v.color;
			weights = v.weights;
			return *this;
		}

		TriVert &operator += (const TriVert &v)
		{
			*this = *this + v;
			return *this;
		}

		TriVert &operator -= (const TriVert &v) {
			*this = *this - v;
			return *this;
		}

		TriVert &operator *= (const TriVert &v) {
			*this = *this * v;
			return *this;
		}

		TriVert &operator /= (const TriVert &v) {
			*this = *this / v;
			return *this;
		}

		TriVert operator * (ValueType s) const {
			TriVert x;

			x.pos = pos * s;
			x.orgPos = orgPos * s;
			for (int i = 0; i < kMaxUVChannels; ++i) {
				x.st[i] = st[i] * s;
				x.tangent[i] = Vec4(x.tangent[i] * s, x.tangent[i].W());
			}
			x.normal = normal * s;
			x.color = color * s;

			return x;
		}

		TriVert operator / (ValueType s) const {
			TriVert x;

			x.pos = pos / s;
			x.orgPos = orgPos / s;
			for (int i = 0; i < kMaxUVChannels; ++i) {
				x.st[i] = st[i] / s;
				x.tangent[i] = Vec4(x.tangent[i] / s, x.tangent[i].W());
			}
			x.normal = normal / s;
			x.color = color / s;

			return x;
		}

		TriVert &operator *= (const ValueType s) {
			*this = *this * s;
			return *this;
		}

		TriVert &operator /= (const ValueType s) {
			*this = *this / s;
			return *this;
		}

		// NOTE: < > <= >= do not take into account the normal.

		bool operator < (const TriVert &v) const {
			if (pos < v.pos)
				return true;

			if (pos == v.pos) {
				for (int i = 0; i < kMaxUVChannels; ++i) {
					if (i > 0) {
						for (int k = 0; k < i; ++k) {
							if (st[k] != v.st[k])
								return false;
						}
					}

					if (st[i] < v.st[i])
						return true;
				}
			}

			return false;
		}

		bool operator <= (const TriVert &v) const {
			return (*this < v) || (*this == v);
		}

		bool operator > (const TriVert &v) const {

			if (pos > v.pos)
				return true;

			if (pos == v.pos) {
				for (int i = 0; i < kMaxUVChannels; ++i) {
					if (i > 0) {
						for (int k = 0; k < i; ++k) {
							if (st[k] != v.st[k])
								return false;
						}
					}

					if (st[i] > v.st[i])
						return true;
				}
			}
			return false;
		}

		bool operator >= (const TriVert &v) const {
			return (*this > v) || (*this == v);
		}

		bool operator == (const TriVert &v) const {
			if (pos != v.pos)
				return false;

			for (int i = 0; i < kMaxUVChannels; ++i) {
				if(st[i] != v.st[i])
					return false;
			}

			return true;
		}

		bool operator != (const TriVert &v) const {
			return !(*this == v);
		}

		Vec3 pos; // rounded vertex position.
		Vec3 orgPos; // original vertex position.
		Vec2 st[kMaxUVChannels];
		// NOTE: from Eric Lengyel's book, tangent.w stores sign of bitangent
		// which can be found via: bitangent = tangent.w * (normal.Cross(tangent))
		Vec4 tangent[kMaxUVChannels];
		Vec3 normal;
		Vec3 color;
		BoneWeights weights;
	};

	//! A vertex class that modifies the comparison operators to include the vertex normals & tangents.
	struct NormalTriVert : public TriVert {
		NormalTriVert() {}
		explicit NormalTriVert(const TriVert &v) : TriVert(v) {}

		bool operator < (const NormalTriVert &v) const {
			if (TriVert::operator < (v))
				return true;

			if (TriVert::operator == (v)) {
				
				if (this->normal < v.normal)
					return true;
				
				if (this->normal == v.normal) {
					for (int i = 0; i < kMaxUVChannels; ++i) {
						if (i > 0) {
							for (int k = 0; k < i; ++k) {
								if (this->tangent[k] != v.tangent[k])
									return false;
							}
						}

						if (this->tangent[i] < v.tangent[i])
							return true;
					}
				}
			}

			return false;
		}

		bool operator <= (const NormalTriVert &v) const {
			return (*this < v) || (*this == v);
		}

		bool operator > (const NormalTriVert &v) const {
			if (TriVert::operator > (v))
				return true;

			if (TriVert::operator == (v)) {

				if (this->normal > v.normal)
					return true;

				if (this->normal == v.normal) {
					for (int i = 0; i < kMaxUVChannels; ++i) {
						if (i > 0) {
							for (int k = 0; k < i; ++k) {
								if (this->tangent[k] != v.tangent[k])
									return false;
							}
						}

						if (this->tangent[i] > v.tangent[i])
							return true;
					}
				}
			}

			return false;
		}

		bool operator >= (const NormalTriVert &v) const {
			return (*this > v) || (*this == v);
		}

		bool operator == (const NormalTriVert &v) const {
			if (TriVert::operator != (v))
				return false;

			if (this->normal != v.normal)
				return false;
			
			for (int i = 0; i < kMaxUVChannels; ++i) {
				if (this->tangent[i] != v.tangent[i])
					return false;
			}

			return true;
		}

		bool operator != (const NormalTriVert &v) const {
			return !(*this == v);
		}
	};

	//! A vertex class that modified the comparison operators to include bone weights, normals & tangents.
	struct WeightedTriVert : public TriVert {
		WeightedTriVert() {}
		explicit WeightedTriVert(const TriVert &v) : TriVert(v) {}

		bool operator < (const WeightedTriVert &v) const {
			if (TriVert::operator < (v))
				return true;
			if (TriVert::operator == (v)) {
				if (TriVert::weights.size() < v.weights.size())
					return true;
				if (TriVert::weights.size() == v.weights.size()) {
					bool eq = true;
					for (size_t i = 0; i < TriVert::weights.size() && eq; ++i) {
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
			if (TriVert::operator == (v)) {
				if (TriVert::weights.size() > v.weights.size())
					return true;
				if (TriVert::weights.size() == v.weights.size()) {
					bool eq = true;
					for (size_t i = 0; i < TriVert::weights.size() && eq; ++i) {
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

		bool operator == (const WeightedTriVert &v) const {
			if (TriVert::operator == (v)) {
				if (TriVert::weights.size() == v.weights.size()) {
					for (size_t i = 0; i < TriVert::weights.size(); ++i) {
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

		bool operator != (const WeightedTriVert &v) const {
			return !(*this == v);
		}
	};

	//! A vertx class that modified the comparison operators to include bone weights and vertex normals.
	struct WeightedNormalTriVert : public NormalTriVert {
		WeightedNormalTriVert() {}
		explicit WeightedNormalTriVert(const TriVert &v) : NormalTriVert(v) {}
		explicit WeightedNormalTriVert(const NormalTriVert &v) : NormalTriVert(v) {}

		bool operator < (const WeightedNormalTriVert &v) const {
			if (NormalTriVert::operator < (v))
				return true;
			if (NormalTriVert::operator == (v)) {
				if (NormalTriVert::weights.size() < v.weights.size())
					return true;
				if (NormalTriVert::weights.size() == v.weights.size()) {
					bool eq = true;
					for (size_t i = 0; i < NormalTriVert::weights.size() && eq; ++i) {
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

		bool operator <= (const WeightedNormalTriVert &v) const {
			return (*this < v) || (*this == v);
		}

		bool operator > (const WeightedNormalTriVert &v) const {
			if (NormalTriVert::operator > (v))
				return true;
			if (NormalTriVert::operator == (v)) {
				if (NormalTriVert::weights.size() > v.weights.size())
					return true;
				if (NormalTriVert::weights.size() == v.weights.size()) {
					bool eq = true;
					for (size_t i = 0; i < NormalTriVert::weights.size() && eq; ++i) {
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

		bool operator >= (const WeightedNormalTriVert &v) const {
			return (*this > v) || (*this == v);
		}

		bool operator == (const WeightedNormalTriVert &v) const {
			if (NormalTriVert::operator == (v)) {
				if (NormalTriVert::weights.size() == v.weights.size()) {
					for (size_t i = 0; i < NormalTriVert::weights.size(); ++i) {
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

		bool operator != (const WeightedNormalTriVert &v) const {
			return !(*this == v);
		}
	};

	/*
	==============================================================================
	Triangles & Models
	==============================================================================
	*/

	struct TriModel;
	typedef typename zone_vector<TriVert, Z3DXT>::type TriVertVec;
	typedef typename zone_vector<NormalTriVert, Z3DXT>::type NormalTriVertVec;
	typedef typename zone_set<int, Z3DXT>::type AreaNumSet;
	typedef typename zone_vector<int, Z3DXT>::type IntVec;

	//! Contains a single triangle in a model. Indices are indexes into the models vertex array.
	struct TriFace {
		TriFace() : outside(true), shared(-1), contents(0), surface(0) {
		}

		TriFace(unsigned int a,
			unsigned int b,
			unsigned int c,
			int mat,
			const Plane &plane,
			TriModel *model) : outside(true), shared(-1) {
			this->mat = mat;
			this->plane = plane;
			this->model = model;
			v[0] = a;
			v[1] = b;
			v[2] = c;
		}

		Plane plane;
		AreaNumSet areas;
		unsigned int v[3];
		int mat;
		int shared;
		int contents;
		int surface;
		bool outside;
		TriModel *model;
	};

	typedef typename zone_vector<TriFace, Z3DXT>::type TriFaceVec;

	/*
	==============================================================================
	Bones & Skeletons
	==============================================================================
	*/

	typedef typename zone_vector<BoneWeights, Z3DXT>::type Skin;
	typedef boost::shared_ptr<Skin> SkinRef;

	struct BoneTM {
		Quat r;
		Vec3 s;
		Vec3 t;
	};

	struct Bone {
		String name;
		int parent;
		Mat4 world;
	};

	struct BonePose {
		BoneTM m;
		float fov;
		String tag;
	};

	typedef typename zone_vector<Bone, Z3DXT>::type BoneVec;

	struct Skel {
		typedef boost::shared_ptr<Skel> Ref;
		BoneVec bones;
	};

	typedef typename zone_vector<typename Skel::Ref, Z3DXT>::type SkelVec;
	typedef typename zone_vector<BonePose, Z3DXT>::type BonePoseVec;
	typedef typename zone_vector<BonePoseVec, Z3DXT>::type BoneFrames;

	/*
	==============================================================================
	Animations
	==============================================================================
	*/

	struct Anim {
		typedef boost::shared_ptr<Anim> Ref;
		String name;
		U32 frameRate;
		bool looping;
		int firstFrame;
		BoneFrames frames;
	};

	typedef typename zone_map<String, typename Anim::Ref, Z3DXT>::type AnimMap;

	/*
	==============================================================================
	Models
	==============================================================================
	*/

	struct TriModel {
		typedef boost::shared_ptr<TriModel> Ref;
		typedef typename zone_vector<Ref, Z3DXT>::type Vec;

		TriModel() : 
			contents(0), 
			ignore(false),
			outside(true), 
			cinematic(false), 
			hideUntilRef(false), 
			hideWhenDone(false) {
			portalAreas[0] = -1;
			portalAreas[1] = -1;
		}

		TriVertVec verts;
		TriFaceVec tris;
		SkinRef skin;
		AnimMap anims;
		BBox bounds;
		String name;
		AreaNumSet areas;
		IntVec emitIds;
		IntVec portalIds;
		int id;
		int skel;
		int contents;
		int numChannels;
		int portalAreas[2];
		bool ignore;
		bool outside;
		bool cinematic;
		bool hideUntilRef;
		bool hideWhenDone;
	};

	/*
	==============================================================================
	Entities & Camera Motion
	==============================================================================
	*/

	struct Entity {
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

	struct Camera {
		typedef boost::shared_ptr<Camera> Ref;
		typedef typename zone_vector<Ref, Z3DXT>::type Vec;
		String name;
		int firstFrame;
		AnimMap anims;
	};

	/*
	==============================================================================
	Waypoints
	==============================================================================
	*/
	struct WaypointConnection;
	typedef boost::shared_ptr<WaypointConnection> WaypointConnectionRef;

	struct Waypoint;
	typedef boost::shared_ptr<Waypoint> WaypointRef;

	struct WaypointPair {
		Waypoint *head;
		Waypoint *tail;

		int headId;
		int tailId;

		int Compare(const WaypointPair &p) const {
			if (headId < p.headId)
				return -1;
			if (headId > p.headId)
				return 1;
			if (tailId < p.tailId)
				return -1;
			if (tailId > p.tailId)
				return 1;
			return 0;
		}

		bool operator == (const WaypointPair &p) const {
			return Compare(p) == 0;
		}

		bool operator != (const WaypointPair &p) const {
			return Compare(p) != 0;
		}

		bool operator < (const WaypointPair &p) const {
			return Compare(p) < 0;
		}

		bool operator > (const WaypointPair &p) const {
			return Compare(p) > 0;
		}

		bool operator <= (const WaypointPair &p) const {
			return Compare(p) <= 0;
		}

		bool operator >= (const WaypointPair &p) const {
			return Compare(p) >= 0;
		}
	};

	struct WaypointConnection {
		typedef boost::shared_ptr<WaypointConnection> Ref;
		typedef typename zone_map<WaypointPair, Ref, Z3DXT>::type Map;

		WaypointConnection() : emitId(-1) {
		}

		WaypointPair waypoints;
		Vec3 ctrls[2];
		String cmds[4];
		int flags;
		int emitId;
	};

	struct Waypoint {
		typedef boost::shared_ptr<Waypoint> Ref;
		typedef typename zone_map<int, Ref, Z3DXT>::type Map;

		Waypoint() : emitId(-1) {
		}

		String targetName;
		String userId;
		String floorName;
		int flags;
		typename WaypointConnection::Map connections;
		Vec3 pos;
		int uid;
		int emitId;
	};

	int version;
	typename Entity::Vec ents;
	MatVec               mats;
	typename Entity::Ref worldspawn;
	typename Camera::Vec cameras;
	typename WaypointConnection::Map waypointConnections;
	typename Waypoint::Map waypoints;

	typename Entity::Ref EntForName(const char *name) const {
		for (typename Entity::Vec::const_iterator it = ents.begin(); it != ents.end(); ++it) {
			if ((*it)->name == name) 
				return *it;
		}
		return typename Entity::Ref();
	}
};

template <typename T>
const T SceneFileT<T>::kMaxRange = T(16384);

template <typename T>
const T SceneFileT<T>::kHalfRange = T(16384/2);

typedef SceneFileT<float> SceneFile;
typedef SceneFileT<double> SceneFileD;

typedef boost::shared_ptr<SceneFile> SceneFileRef;
typedef zone_vector<SceneFileRef, Z3DXT>::type SceneFileVec;

bool LoadSceneFile(stream::InputStream &stream, SceneFile &map, bool smooth, UIProgress *ui = 0);

} // tools
