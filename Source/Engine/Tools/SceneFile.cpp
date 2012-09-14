// SceneFile.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if defined(RAD_OPT_TOOLS)

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
		kVersion = 4,

		kHasMaterialFlag = 0x80000000,
		kHasAnimsFlag = 0x00200000,
		kHasMeshFlag = 0x00100000,
		kCinematicObjectFlag = 0x00080000,
		kHideUntilRefedFlag = 0x00040000,
		kHideWhenDoneFlag = 0x00010000
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
	};

	void ReadTriModel(InputStream &stream, int version, TriModel &mdl, int flags, const SceneFile::SkelVec &skels) {
		U32 nv, nf, nc;

		if (flags&kHasMeshFlag)
		{
			stream >> nv;
			stream >> nf;
			stream >> nc;

			mdl.numChannels = nc;

			mdl.verts.reserve(nv);
			for (U32 i = 0; i < nv; ++i) {
				TriVert v;
				v.orgPos = ReadVec3(stream);
				v.pos = v.orgPos;
				mdl.bounds.Insert(v.pos);
				mdl.verts.push_back(v);
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
					stream >> f.v[2];
					stream >> f.v[1];
					stream >> f.v[0];
					
					if (i < SceneFile::kMaxUVChannels) {
						uvtris[i].push_back(f);
					}
				}
			}
			
			mdl.tris.reserve(nf);
			bool warn = false;
			for (U32 i = 0; i < nf; ++i) {

				TriFace f;
				stream >> f.v[2];
				stream >> f.v[1];
				stream >> f.v[0];
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
					f.plane.Initialize(mdl.verts[f.v[2]].pos, mdl.verts[f.v[1]].pos, mdl.verts[f.v[0]].pos);
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
			return;

		stream >> frameRate;

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
			a->frames.resize(numFrames);

			for (U32 j = 0; j < numFrames; ++j) {
				SceneFile::BonePoseVec &tms = a->frames[j];
				tms.resize(skel->bones.size());
				for (SceneFile::BonePoseVec::iterator tm = tms.begin(); tm != tms.end(); ++tm) {
					(*tm).fov = 0.f;
					(*tm).m = ReadBoneTM(stream);
					(*tm).tag = ReadString(stream);
				}
			}

			mdl.anims.insert(SceneFile::AnimMap::value_type(a->name, a));
		}
	}

	struct SmoothVert : public SceneFile::WeightedTriVert {
		TriFaceIdxSet faces;
		int sm;
		int id;
	};

	typedef zone_multimap<SmoothVert, int, Z3DXT>::type SmoothVertIdxMap;
	typedef zone_vector<SmoothVert, Z3DXT>::type SmoothVertVec;

	void AddFaces(const SmoothVert &src, SmoothVert &dst) {
		for (TriFaceIdxSet::const_iterator it = src.faces.begin(); it != src.faces.end(); ++it)
			dst.faces.insert(*it);
	}

	void CombineFaces(SmoothVert &a, SmoothVert &b) {
		AddFaces(a, b);
		a.faces = b.faces;
	}

	int HashVert(const SmoothVert &v, SmoothVertVec &vec, SmoothVertIdxMap &idxMap, bool smooth) {
		std::pair<SmoothVertIdxMap::const_iterator,
		          SmoothVertIdxMap::const_iterator> pair = idxMap.equal_range(v);

		while (pair.first != pair.second) {
			if (smooth) {
				SmoothVert &x = vec[pair.first->second];
				if (v.sm & x.sm) {
					x.sm |= v.sm;
					AddFaces(v, x);
					return pair.first->second;
				}
				++pair.first;
				continue;
			}
			return pair.first->second;
		}

		vec.push_back(v);
		int ofs = (int)(vec.size()-1);
		idxMap.insert(SmoothVertIdxMap::value_type(v, ofs));
		return ofs;
	}

	void MakeNormals(const TriModel &mdl, SmoothVertVec &vec, bool smooth) {
		// some coincedent vertices may not have been combined
		// because their UV's are different. smooth over these as well.
		if (smooth) {
			for (SmoothVertVec::iterator it = vec.begin(); it != vec.end(); ++it) {
				SmoothVert &a = *it;
				for (SmoothVertVec::iterator it2 = vec.begin(); it2 != vec.end(); ++it2) {
					if (it == it2) 
						continue;

					SmoothVert &b = *it2;

					if (a.sm & b.sm) { // smooth?
						if (a.pos != b.pos) 
							continue;

						a.sm = b.sm = (a.sm|b.sm);
						CombineFaces(a, b);
					}
				}
			}
		}

		for (SmoothVertVec::iterator it = vec.begin(); it != vec.end(); ++it) {
			SmoothVert &v = *it;
			v.normal = SceneFile::Vec3::Zero;
			
			for (TriFaceIdxSet::iterator it2 = v.faces.begin(); it2 != v.faces.end(); ++it2) {
				U32 idx = *it2;
				v.normal += mdl.tris[idx].plane.Normal();
				v.normal.Normalize();
			}
		}
	}

	// apply smoothing groups and build the real trimodel.
	SceneFile::TriModel::Ref Build(TriModel &mdl, int id, int count, bool smooth) {
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
				v.sm  = tri.smg;
				v.id = tri.v[i];

				for (int j = 0; j < SceneFile::kMaxUVChannels; ++j) {
					if (uvv[j].empty()) {
						v.st[j] = UV::Zero;
					} else {
						v.st[j] = uvv[j][uvf[j][idx].v[i]];
					}
				}

				tri.sm[i] = HashVert(v, smv, smidxm, smooth);
			}

			mmdl->tris.push_back(SceneFile::TriFace(tri.sm[0], tri.sm[1], tri.sm[2], tri.mat, tri.plane, mmdl.get()));
		}

		MakeNormals(mdl, smv, smooth);

		mmdl->verts.reserve(smv.size());
		mmdl->bounds = mdl.bounds;

		if (mdl.skin)
			mmdl->skin.reset(new SceneFile::Skin());
				
		for (SmoothVertVec::iterator it = smv.begin(); it != smv.end(); ++it) {
			mmdl->verts.push_back(*it);
			if (mmdl->skin)
				mmdl->skin->push_back((*it).weights);
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
			SceneFile::Camera::Ref cam(new SceneFile::Camera());

			cam->name = ReadString(stream);

			U32 flags;
			stream >> flags;

			U32 z, frameRate;
			stream >> z; // skip skel index

			stream >> z >> frameRate; // num anims

			for (U32 k = 0; k < z; ++k) {

				SceneFile::Anim::Ref a(new SceneFile::Anim());
				a->name = ReadString(stream);
				
				S32 firstFrame;
				U32 numFrames;
				stream >> flags >> firstFrame >> numFrames;

				a->looping = flags&1;
				a->frameRate = frameRate;
				a->firstFrame = (int)firstFrame;
				a->frames.resize(numFrames);

				for (U32 j = 0; j < numFrames; ++j) {

					SceneFile::BonePoseVec &tms = a->frames[j];
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

				if (version > kVersion2) {
					mdl.cinematic = (flags&kCinematicObjectFlag) ? true : false;
					mdl.hideUntilRef = (flags&kHideUntilRefedFlag) ? true : false;
					mdl.hideWhenDone = (flags&kHideWhenDoneFlag) ? true : false;
				} else {
					mdl.cinematic = false;
					mdl.hideUntilRef = false;
					mdl.hideWhenDone = false;
				}

				mdl.verts.clear();
				mdl.tris.clear();
				mdl.bounds.Initialize();
				mdl.anims.clear();
				mdl.skin.reset();
				
				for (int i = 0; i < SceneFile::kMaxUVChannels; ++i) {
					mdl.uvs[i].clear();
					mdl.uvtris[i].clear();
				}
				
				if (flags&(kHasMeshFlag|kHasAnimsFlag))
					ReadTriModel(stream, version, mdl, flags, ent->skels);

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

				if (!mdl.tris.empty())
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

#endif // RAD_OPT_TOOLS