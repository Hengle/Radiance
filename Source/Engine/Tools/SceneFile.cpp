// SceneFile.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#include "SceneFile.h"
#include "../COut.h"
#include <Runtime/Runtime.h>
#include <Runtime/Container/ZoneHashMap.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Endian/EndianStream.h>
#include <string>
#include <vector>
#include <map>
#include <iterator>

namespace tools {

RAD_ZONE_DEF(RADENG_API, Z3DX, "3DX", ZTools);

using namespace stream;

namespace {
	enum {
		kId = RAD_FOURCC_LE('R', 'S', 'C', 'N'),
		kVersion2 = 2,
		kVersion3 = 3,
		kVersion4 = 4,
		kVersion5 = 5,
		kVersion6 = 6,
		kVersion = 7,

		kHasMaterialFlag = 0x80000000,
		kHasAnimsFlag = 0x00200000,
		kHasMeshFlag = 0x00100000,
		kCinematicObjectFlag = 0x00080000,
		kHideUntilRefedFlag = 0x00040000,
		kHideWhenDoneFlag = 0x00010000,
		kSetBBoxFlag = 0x00008000,
		kAffectedByWorldLightsFlag = 0x00004000,
		kAffectedByObjectLightsFlag = 0x00002000,
		kCastShadows = 0x00001000,
		kAnimType_Skeletal = 0,
		kAnimType_Vertex = 1
	};

	struct Material {
		String name;
		int flags;
		int emitId;

		struct Sub {
			String name;
			int id;
			int emitId;
		};

		typedef zone_map<UReg, Sub, ZToolsT>::type SubHash;
		SubHash subs;
	};

	typedef zone_vector<Material, Z3DXT>::type MatVec;
	
	String ReadString(InputStream &stream) {
		String s;
		for (;;) {
			S8 c;
			stream >> c;
			if (!c)
				break;
			s += (char)c;
		}

		return s;
	}

	SceneFile::Vec2 ReadVec2(InputStream &stream) {
		float v[2];
		stream >> v[0];
		stream >> v[1];
		return SceneFile::Vec2(SceneFile::ValueType(v[0]), SceneFile::ValueType(v[1]));
	}

	SceneFile::Vec3 ReadVec3(InputStream &stream) {
		float v[3];
		stream >> v[0];
		stream >> v[1];
		stream >> v[2];
		return SceneFile::Vec3(SceneFile::ValueType(v[0]), SceneFile::ValueType(v[1]), SceneFile::ValueType(v[2]));
	}

	SceneFile::Mat4 ReadMat3(InputStream &stream) {
		float v[12];
		for (int i = 0; i < 12; ++i)
			stream >> v[i];
		return SceneFile::Mat4(
			SceneFile::Vec4(v[0], v[1], v[2],  0.f),
			SceneFile::Vec4(v[3], v[4], v[5],  0.f),
			SceneFile::Vec4(v[6], v[7], v[8],  0.f),
			SceneFile::Vec4(v[9], v[10], v[11], 1.f)
		);
	}

	SceneFile::Quat ReadQuat(InputStream &stream) {
		float w, x, y, z;
		stream >> w >> x >> y >> z;
		return SceneFile::Quat(x, y, z, w);
	}

	
	void DecompAffine(SceneFile::Vec3 &s, SceneFile::Quat &q, SceneFile::Vec3 &t, const SceneFile::Mat4 &in) {
		int i;
		
		SceneFile::Mat4 m = in;

		for (i = 0; i < 3; ++i) {
			double d = sqrt(in[i][0] * in[i][0] + in[i][1] * in[i][1] + in[i][2] * in[i][2]);
			s[i] = (float)d;
			m[i][0] = (float)(in[i][0] / d);
			m[i][1] = (float)(in[i][1] / d);
			m[i][2] = (float)(in[i][2] / d);
		}
		
		q = in.Rotation();
		
		for (i = 0; i < 3; ++i) {
			t[i] = m[3][i];
		}
	}

	SceneFile::BoneTM ReadBoneTM(InputStream &stream) {
		SceneFile::BoneTM tm;
		SceneFile::Mat4 m = ReadMat3(stream);
		DecompAffine(tm.s, tm.r, tm.t, m);
		return tm;
	}

	typedef SceneFile::Vec2 UV;
	typedef zone_set<UReg, Z3DXT>::type TriFaceIdxSet;
	typedef zone_vector<UV, Z3DXT>::type UVVec;

	struct TriVert {
		SceneFile::Vec3 pos;
		SceneFile::Vec3 orgPos;
	};

	typedef zone_vector<TriVert, Z3DXT>::type TriVertVec;

	struct UVFace {
		int v[3];
	};

	typedef zone_vector<UVFace, Z3DXT>::type UVFaceVec;

	struct TriFace {
		int v[3];
		int sm[3];
		int smg;
		int mat;
		SceneFile::Plane plane;
	};

	typedef zone_vector<TriFace, Z3DXT>::type TriFaceVec;

	struct TriModel {
		UVVec      uvs[SceneFile::kMaxUVChannels];
		UVFaceVec  uvtris[SceneFile::kMaxUVChannels];
		TriVertVec verts;
		TriFaceVec tris;
		SceneFile::BBox bounds;
		String name;
		int id;
		int skel;
		int numChannels;
		SceneFile::SkinRef skin;
		SceneFile::AnimMap anims;
		bool cinematic;
		bool hideUntilRef;
		bool hideWhenDone;
		bool affectedByWorldLights;
		bool affectedByObjectLights;
		bool castShadows;
	};

	bool ReadTriModel(InputStream &stream, int version, TriModel &mdl, int flags, const SceneFile::SkelVec &skels) {
		U32 nv, nf, nc;

		if (flags&kHasMeshFlag) {
			stream >> nv;
			stream >> nf;
			stream >> nc;

			mdl.numChannels = nc;

			mdl.verts.reserve(nv);
			for (U32 i = 0; i < nv; ++i) {
				TriVert v;
				v.orgPos = ReadVec3(stream);
				v.pos = v.orgPos;
				mdl.verts.push_back(v);

				if (!(flags & kSetBBoxFlag))
					mdl.bounds.Insert(v.pos);
			}

			UVFaceVec  uvtris[SceneFile::kMaxUVChannels];

			for (U32 i = 0; i < nc; ++i) {
				U32 nuv;
				stream >> nuv;

				if (i < SceneFile::kMaxUVChannels) {
					mdl.uvs[i].reserve(nuv);
					mdl.uvtris[i].reserve(nf);
				}

				for (U32 j = 0; j < nuv; ++j) {
					SceneFile::Vec2 v = ReadVec2(stream); // ignore
					
					if (i < SceneFile::kMaxUVChannels) {
						mdl.uvs[i].push_back(v);
					}
				}

				for (U32 j = 0; j < nf; ++j) {
					UVFace f;
					stream >> f.v[0];
					stream >> f.v[1];
					stream >> f.v[2];
					
					if (i < SceneFile::kMaxUVChannels) {
						uvtris[i].push_back(f);
					}
				}
			}
			
			mdl.tris.reserve(nf);
			bool warn = false;
			for (U32 i = 0; i < nf; ++i) {

				TriFace f;
				stream >> f.v[0];
				stream >> f.v[1];
				stream >> f.v[2];
				stream >> f.smg;
				stream >> f.mat;

				int z;
				for (z = 0; z < 2; ++z) {
					if (mdl.verts[f.v[z]].pos == mdl.verts[f.v[z+1]].pos) 
						break;
					if (mdl.verts[f.v[z]].pos == mdl.verts[f.v[(z+2)%3]].pos) 
						break;
				}

				if (z == 2) {
					f.plane.Initialize(mdl.verts[f.v[0]].pos, mdl.verts[f.v[1]].pos, mdl.verts[f.v[2]].pos);
					mdl.tris.push_back(f);

					for (U32 k = 0; k < SceneFile::kMaxUVChannels && k < nc; ++k)
						mdl.uvtris[k].push_back(uvtris[k][i]);
				} else if (!warn) {
					warn = true;
					COut(C_Warn) << "WARNING: mesh id " << mdl.id << " has degenerate triangles." << std::endl;
				}
			}
		}

		S32 numAnims;
		U32 frameRate;

		stream >> numAnims;

		if (numAnims < 0) // EOS indicator.
			return true;

		stream >> frameRate;

		S32 animType = kAnimType_Skeletal;

		if (version > kVersion5)
			stream >> animType;

		if (animType == kAnimType_Skeletal) {
			if (flags&kHasMeshFlag) {
				mdl.skin.reset(new SceneFile::Skin());
				mdl.skin->reserve(nv);

				// skin
				for (U32 i = 0; i < nv; ++i) {
					SceneFile::BoneWeights weights;

					U32 numWeights;
					stream >> numWeights;
					weights.reserve(numWeights);

					for (U32 j = 0; j < numWeights; ++j) {
						SceneFile::BoneWeight w;
						stream >> w.weight;
						stream >> w.bone;
						if (w.weight > 0.0f)
							weights.push_back(w);
					}

					mdl.skin->push_back(weights);
				}
			}

			// anims
			for (int i = 0; i < numAnims; ++i) {
				const SceneFile::Skel::Ref &skel = skels[mdl.skel];

				SceneFile::Anim::Ref a(new SceneFile::Anim());
				a->name = ReadString(stream);
			
				U32 flags, numFrames, firstFrame;

				if (version > kVersion2) {
					stream >> flags >> firstFrame >> numFrames;
				} else {
					firstFrame = 0;
					stream >> flags >> numFrames;
				}

				a->looping = flags&1;
				a->frameRate = frameRate;
				a->firstFrame = (int)firstFrame;
				a->boneFrames.resize(numFrames);

				for (U32 j = 0; j < numFrames; ++j) {
					SceneFile::BonePoseVec &tms = a->boneFrames[j];
					tms.resize(skel->bones.size());
					for (SceneFile::BonePoseVec::iterator tm = tms.begin(); tm != tms.end(); ++tm) {
						(*tm).fov = 0.f;
						(*tm).m = ReadBoneTM(stream);
						(*tm).tag = ReadString(stream);
					}
				}

				mdl.anims.insert(SceneFile::AnimMap::value_type(a->name, a));
			}
		} else {
			RAD_ASSERT(animType == kAnimType_Vertex);
			if (!(flags&kHasMeshFlag))
				return false;

			for (int i = 0; i < numAnims; ++i) {
				SceneFile::Anim::Ref a(new SceneFile::Anim());
				a->name = ReadString(stream);
			
				U32 flags, numFrames, firstFrame;
				stream >> flags >> firstFrame >> numFrames;
			
				a->looping = flags&1;
				a->frameRate = frameRate;
				a->firstFrame = (int)firstFrame;
				
				U32 numVertFrames;
				stream >> numVertFrames;

				a->vertexFrames.resize(numVertFrames);

				for (U32 j = 0; j < numVertFrames; ++j) {
					SceneFile::VertexFrame &vframe = a->vertexFrames[j];
				
					U32 frame;
					stream >> frame;
					vframe.frame = frame;

					vframe.verts.resize(mdl.verts.size());

					U32 numVerts;
					stream >> numVerts;

					if (numVerts != (U32)mdl.verts.size())
						return false;

					for (U32 k = 0; k < numVerts; ++k)
						vframe.verts[k].pos = ReadVec3(stream);
				}

				mdl.anims.insert(SceneFile::AnimMap::value_type(a->name, a));
			}
		}

		return true;
	}

	struct SmoothVert : public SceneFile::WeightedNormalTriVert {
		TriFaceIdxSet faces;
		int sm;
		int id;
	};

	typedef zone_vector<SmoothVert, Z3DXT>::type SmoothVertVec;
	typedef zone_vector<int, Z3DXT>::type IntVec;
	typedef zone_map<int, IntVec, Z3DXT>::type SmoothVertIdxMap;
		
	void AddFaces(const SmoothVert &src, SmoothVert &dst) {
		for (TriFaceIdxSet::const_iterator it = src.faces.begin(); it != src.faces.end(); ++it)
			dst.faces.insert(*it);
	}

	void CombineFaces(SmoothVert &a, SmoothVert &b) {
		AddFaces(a, b);
		a.faces = b.faces;
	}

	int HashVert(const SmoothVert &v, SmoothVertVec &vec, SmoothVertIdxMap &idxMap) {
		SmoothVertIdxMap::iterator it = idxMap.find(v.id);

		int idx = -1;

		if (it != idxMap.end()) {
			IntVec &indices = it->second;
			for (IntVec::const_iterator it = indices.begin(); it != indices.end(); ++it) {
				SmoothVert &x = vec[*it];
				if ((x.sm&v.sm)||(x.sm==v.sm)) {
					// identical?
					if (x.sm == v.sm) {
						int i;
						for (i = 0; i < SceneFile::kMaxUVChannels; ++i) {
							if (x.st[i] != v.st[i])
								break;
						}
						if (i == SceneFile::kMaxUVChannels) {
							RAD_ASSERT(idx == -1);
							idx = *it;
						}
					}

					AddFaces(v, x);
				}
			}
		} else {
			it = idxMap.insert(SmoothVertIdxMap::value_type(v.id, IntVec())).first;
		}

		if (idx != -1)
			return idx;

		idx = (int)vec.size();
		vec.push_back(v);
		it->second.push_back(idx);
		return idx;
	}

	void MakeNormals(const TriModel &mdl, SmoothVertVec &vec) {
		
		for (SmoothVertVec::iterator it = vec.begin(); it != vec.end(); ++it) {
			SmoothVert &v = *it;
			v.normal = SceneFile::Vec3::Zero;
			
			for (int i = 0; i < SceneFile::kMaxUVChannels; ++i) {
				v.tangent[i] = SceneFile::Vec4::Zero;
			}
			
			for (TriFaceIdxSet::iterator it2 = v.faces.begin(); it2 != v.faces.end(); ++it2) {
				U32 idx = *it2;
				v.normal += mdl.tris[idx].plane.Normal();
				v.normal.Normalize();
			}
		}

		// calculate tangents.

		zone_vector<SceneFile::Vec3, Z3DXT>::type tan2[SceneFile::kMaxUVChannels];

		for (int i = 0; i < SceneFile::kMaxUVChannels; ++i)
			tan2[i].resize(vec.size(), SceneFile::Vec3::Zero);

		for (TriFaceVec::const_iterator it = mdl.tris.begin(); it != mdl.tris.end(); ++it) {
			const TriFace &tri = *it;

			SmoothVert &v1 = vec[tri.sm[0]];
			SmoothVert &v2 = vec[tri.sm[1]];
			SmoothVert &v3 = vec[tri.sm[2]];

			SceneFile::Vec3 x = v2.pos - v1.pos;
			SceneFile::Vec3 y = v3.pos - v1.pos;
				
			for (int i = 0; i < SceneFile::kMaxUVChannels; ++i) {

				SceneFile::Vec3 &m0 = tan2[i][tri.sm[0]];
				SceneFile::Vec3 &m1 = tan2[i][tri.sm[1]];
				SceneFile::Vec3 &m2 = tan2[i][tri.sm[2]];

				SceneFile::Vec2 s = v2.st[i] - v1.st[i];
				SceneFile::Vec2 t = v3.st[i] - v1.st[i];

				SceneFile::ValueType r = (s[0] * t[1] - s[1] * t[0]);
				if (r != 0.f) {
					r = SceneFile::ValueType(1) / r;
				} else {
					r = 1.f;
				}

				SceneFile::Vec4 udir(
					(t[1] * x[0] - s[1] * y[0]) * r,
					(t[1] * x[1] - s[1] * y[1]) * r,
					(t[1] * x[2] - s[1] * y[2]) * r,
					0.f
				);

				SceneFile::Vec3 vdir(
					(s[0] * y[0] - t[0] * x[0]) * r,
					(s[0] * y[1] - t[0] * x[1]) * r,
					(s[0] * y[2] - t[0] * x[2]) * r
				);

				v1.tangent[i] += udir;
				v2.tangent[i] += udir;
				v3.tangent[i] += udir;
				m0 += vdir;
				m1 += vdir;
				m2 += vdir;
			}
		}

		int mOfs = 0;

		for (SmoothVertVec::iterator it = vec.begin(); it != vec.end(); ++it, ++mOfs) {
			SmoothVert &v = *it;
			
			for (int i = 0; i < SceneFile::kMaxUVChannels; ++i) {
				const Vec3 &m = tan2[i][mOfs];

				SceneFile::Vec3 t = v.tangent[i];
				t = t - (v.normal * v.normal.Dot(t));
				t.Normalize();
				v.tangent[i] = Vec4(t, 0.f);
				// determinant
				v.tangent[i][3] = (v.normal.Cross(t).Dot(m) < 0.f) ? -1.f : 1.f;
			}
		}
	}

	// apply smoothing groups and build the real trimodel.
	SceneFile::TriModel::Ref Build(TriModel &mdl, int id, int count, bool useSmGroups) {
		COut(C_Debug) << "(3DX) processing mesh " << (id+1) << "/" << count << std::endl;
		
		SceneFile::TriModel::Ref mmdl(new SceneFile::TriModel());
		mmdl->id = mdl.id;
		mmdl->name = mdl.name;
		mmdl->skel = mdl.skel;
		mmdl->anims = mdl.anims;
		mmdl->numChannels = mdl.numChannels;
		mmdl->cinematic = mdl.cinematic;
		mmdl->hideUntilRef = mdl.hideUntilRef;
		mmdl->hideWhenDone = mdl.hideWhenDone;
		mmdl->affectedByObjectLights = mdl.affectedByObjectLights;
		mmdl->affectedByWorldLights = mdl.affectedByWorldLights;
		mmdl->castShadows = mdl.castShadows;

		SmoothVertVec smv;
		SmoothVertIdxMap smidxm;
		SmoothVert v;
		const UVVec     *uvv = mdl.uvs;
		const UVFaceVec *uvf = mdl.uvtris;

		smv.reserve(mdl.verts.size());
		mmdl->tris.reserve(mdl.tris.size());

		UReg idx = 0;
		for (TriFaceVec::iterator it = mdl.tris.begin(); it != mdl.tris.end(); ++it, ++idx) {
			TriFace &tri = *it;

			for (int i = 0; i < 3; ++i) {
				v.faces.clear();
				v.weights.clear();
				v.faces.insert(idx);
				v.pos = mdl.verts[tri.v[i]].pos;
				v.orgPos = mdl.verts[tri.v[i]].orgPos;
				if (mdl.skin)
					v.weights = (*mdl.skin)[tri.v[i]];
				v.sm  = useSmGroups ? tri.smg : 1;
				v.id = tri.v[i];

				for (int j = 0; j < SceneFile::kMaxUVChannels; ++j) {
					v.tangent[j] = SceneFile::Vec4::Zero;

					if (uvv[j].empty()) {
						v.st[j] = UV::Zero;
					} else {
						v.st[j] = uvv[j][uvf[j][idx].v[i]];
					}
				}

				tri.sm[i] = HashVert(v, smv, smidxm);
			}

			mmdl->tris.push_back(SceneFile::TriFace(tri.sm[0], tri.sm[1], tri.sm[2], tri.mat, tri.plane, mmdl.get()));
		}

		MakeNormals(mdl, smv);

		mmdl->verts.reserve(smv.size());
		mmdl->bounds = mdl.bounds;

		if (mdl.skin)
			mmdl->skin.reset(new SceneFile::Skin());
				
		for (SmoothVertVec::iterator it = smv.begin(); it != smv.end(); ++it) {
			mmdl->verts.push_back(*it);
			if (mmdl->skin)
				mmdl->skin->push_back((*it).weights);
		}

		// smooth vertex frames
		for (SceneFile::AnimMap::iterator it = mmdl->anims.begin(); it != mmdl->anims.end(); ++it) {
			const SceneFile::Anim::Ref &anim = it->second;
			
			for (SceneFile::VertexFrames::iterator it = anim->vertexFrames.begin(); it != anim->vertexFrames.end(); ++it) {
				SceneFile::VertexFrame &vframe = *it;

				// pull vert positions from originally indexed verts.
				for (SmoothVertVec::iterator it = smv.begin(); it != smv.end(); ++it) {
					SmoothVert &v = *it;
					v.pos = vframe.verts[v.id];
				}

				// replace frame with smoothed vertices
				// false because we don't need to regenerate sm groups (already done)
				MakeNormals(mdl, smv);

				vframe.verts.clear();

				for (SmoothVertVec::iterator it = smv.begin(); it != smv.end(); ++it) {
					vframe.verts.push_back(*it);
				}
			}
		}

		return mmdl;
	}

	int EmitMaterial(const String &name, SceneFile &map) {
		for (size_t i = 0; i < map.mats.size(); ++i) {
			if (map.mats[i].name == name)
				return (int)i;
		}

		SceneFile::Material mm;
		mm.name = name;
		map.mats.push_back(mm);
		return (int)(map.mats.size()-1);
	}
}

bool LoadSceneFile(InputStream &nakedstr, SceneFile &map, bool smooth, UIProgress *ui)
{
	LittleInputStream stream(nakedstr.Buffer());
	U32 id, version;

	stream >> id;
	stream >> version;

	map.version = version;

	if (id != kId || (version < kVersion2 || version > kVersion)) 
		return false;

	MatVec mats;
	U32 n;

	// materials.

	stream >> n;
	for (U32 i = 0; i < n; ++i) {
		Material m;
		m.name = ReadString(stream);
		stream >> m.flags;
		if (m.flags & 1) { // multisub
			U32 z;
			stream >> z;
			for (U32 j = 0; j < z; ++j) {
				Material::Sub s;
				stream >> s.id;
				s.name = ReadString(stream);
				s.emitId = -1;
				m.subs.insert(Material::SubHash::value_type(s.id, s));
			}
		} else {
			m.emitId = -1;
		}
		mats.push_back(m);
	}

	if (version > kVersion2) { // load cameras

		stream >> n;

		for (U32 i = 0; i < n; ++i) {
			SceneFile::Camera::Ref cam(new (Z3DX) SceneFile::Camera());

			cam->name = ReadString(stream);

			U32 flags;
			stream >> flags;

			S32 numAnims; // signed because may be -1
			U32 frameRate, z;
			
			if (version > kVersion5) {
				stream >> numAnims;
				if (numAnims < 0)
					continue;
				stream >> frameRate >> z;
				RAD_ASSERT(z == kAnimType_Skeletal);
			} else {
				stream >> z; // skip skel index
				stream >> numAnims >> frameRate;
			}
					
			for (S32 k = 0; k < numAnims; ++k) {

				SceneFile::Anim::Ref a(new SceneFile::Anim());
				a->name = ReadString(stream);
				
				S32 firstFrame;
				U32 numFrames;
				stream >> flags >> firstFrame >> numFrames;

				a->looping = flags&1;
				a->frameRate = frameRate;
				a->firstFrame = (int)firstFrame;
				a->boneFrames.resize(numFrames);

				for (U32 j = 0; j < numFrames; ++j) {

					SceneFile::BonePoseVec &tms = a->boneFrames[j];
					tms.resize(1);
					for (SceneFile::BonePoseVec::iterator tm = tms.begin(); tm != tms.end(); ++tm) {
						stream >> (*tm).fov;
						(*tm).m = ReadBoneTM(stream);
						(*tm).tag = ReadString(stream);
					}
				}

				cam->anims.insert(SceneFile::AnimMap::value_type(a->name, a));
				map.cameras.push_back(cam);
			}
		}
	}

	if (version > kVersion6) { // omni lights
		stream >> n;

		for (U32 i = 0; i < n; ++i) {
			SceneFile::OmniLight::Ref light(new (Z3DX) SceneFile::OmniLight());

			light->name = ReadString(stream);
			light->pos = ReadVec3(stream);

			S32 flags;
			stream >> flags;
			light->flags = (int)flags;

			light->color = ReadVec3(stream);
			light->shadowColor = ReadVec3(stream);
			stream >> light->brightness;
			stream >> light->radius;

			map.omniLights.push_back(light);
		}
	}

	// entities
	stream >> n;

	{
		TriModel mdl;

		for (U32 i = 0; i < n; ++i) {
			SceneFile::Entity::Ref ent(new SceneFile::Entity());
			ent->maxEnt = true;
			ent->name = ReadString(stream);
			stream >> ent->id;
			ent->origin = ReadVec3(stream);
			U32 z;
			
			stream >> z;
			for (U32 j = 0; j < z; ++j) { // skels.
		
				SceneFile::Skel::Ref sk(new SceneFile::Skel());
				U32 numBones;
				stream >> numBones;
				for (U32 b = 0; b < numBones; ++b) {
					SceneFile::Bone bone;
					bone.name = ReadString(stream);
					stream >> bone.parent;
					bone.world = ReadMat3(stream);
					sk->bones.push_back(bone);
				}

				ent->skels.push_back(sk);
			}

			stream >> z;

			if (ui) {
				ui->total = z;
				ui->totalProgress = 0;
				ui->Refresh();
			}

			for (U32 j = 0; j < z; ++j) { // trimodels

				Material *m = 0;
				U32 flags;

				stream >> mdl.id;
				if (version > kVersion3)
					mdl.name = ReadString(stream);
				stream >> flags;
				stream >> mdl.skel;

				if (flags & kHasMaterialFlag) { // has material
					U32 idx;
					stream >> idx;
					m = &mats[idx];
				}

				if ((version > kVersion4) && (flags & kSetBBoxFlag)) { // bbox provided
					String bboxString = ReadString(stream);
					float mins[3];
					float maxs[3];
					sscanf(bboxString.c_str, "%f %f %f %f %f %f",
						&mins[0], &mins[1], &mins[2],
						&maxs[0], &maxs[1], &maxs[2]
					);
					mdl.bounds.Initialize(
						(SceneFile::ValueType)mins[0], (SceneFile::ValueType)mins[1], (SceneFile::ValueType)mins[2],
						(SceneFile::ValueType)maxs[0], (SceneFile::ValueType)maxs[2], (SceneFile::ValueType)maxs[2]
					);
				} else {
					mdl.bounds.Initialize();
				}

				if (version > kVersion2) {
					mdl.cinematic = (flags&kCinematicObjectFlag) ? true : false;
					mdl.hideUntilRef = (flags&kHideUntilRefedFlag) ? true : false;
					mdl.hideWhenDone = (flags&kHideWhenDoneFlag) ? true : false;
				} else {
					mdl.cinematic = false;
					mdl.hideUntilRef = false;
					mdl.hideWhenDone = false;
				}

				if (version > kVersion6) {
					mdl.affectedByObjectLights = (flags&kAffectedByObjectLightsFlag) ? true : false;
					mdl.affectedByWorldLights = (flags&kAffectedByWorldLightsFlag) ? true : false;
					mdl.castShadows = (flags&kCastShadows) ? true : false;
				} else {
					mdl.affectedByObjectLights = false;
					mdl.affectedByWorldLights = false;
					mdl.castShadows = false;
				}

				mdl.verts.clear();
				mdl.tris.clear();
				mdl.anims.clear();
				mdl.skin.reset();
				
				for (int i = 0; i < SceneFile::kMaxUVChannels; ++i) {
					mdl.uvs[i].clear();
					mdl.uvtris[i].clear();
				}
				
				if (flags&(kHasMeshFlag|kHasAnimsFlag)) {
					if (!ReadTriModel(stream, version, mdl, flags, ent->skels))
						return false;
				}

				for (TriFaceVec::iterator it = mdl.tris.begin(); it != mdl.tris.end(); ++it) {
					if (m) {
						Material::SubHash::iterator sub = m->subs.find((U32)it->mat);
						if (sub != m->subs.end()) {
							if (sub->second.emitId == -1)
								sub->second.emitId = EmitMaterial(sub->second.name, map);
							it->mat = (int)sub->second.emitId;
						} else {
							if (m->emitId == -1)
								m->emitId = EmitMaterial(m->name, map);
							it->mat = (int)m->emitId;
						}
					} else {
						it->mat = -1;
					}
				}

				if ((flags&kHasAnimsFlag) || (!mdl.tris.empty()))
					ent->models.push_back(Build(mdl, j, z, smooth));

				if (ui) {
					ui->Step();
					ui->Refresh();
				}
			}

			if (ent->name == "worldspawn") {
				if (map.worldspawn) { // move to worldspawn
					std::copy(
						ent->models.begin(), 
						ent->models.end(), 
						std::back_inserter(map.worldspawn->models)
					);

					map.worldspawn->skels = ent->skels;
				} else {
					map.worldspawn = ent;
				}
			} else {
				map.ents.push_back(ent);
			}
		}
	}

	return true;
}

} // tools
