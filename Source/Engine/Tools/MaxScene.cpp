// MaxScene.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if defined(RAD_OPT_TOOLS)

#include "MaxScene.h"
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

namespace
{
	enum
	{
		Id = RAD_FOURCC_LE('R', 'S', 'C', 'N'),
		Version2 = 2,
		Version = 3,

		HasMaterialFlag = 0x80000000,
		ContentsDetailFlag = 0x40000000,
		ContentsAreaportalFlag = 0x20000000,
		ContentsNoClipFlag = 0x00800000,
		ContentsNoDrawFlag = 0x00400000,
		HasAnimsFlag = 0x00200000,
		HasMeshFlag = 0x00100000,
		CinematicObjectFlag = 0x00080000,
		HideUntilRefedFlag = 0x00040000,
		HideWhenDoneFlag = 0x00010000
	};

	struct Material
	{
		String name;
		U32 flags;
		UReg emitId;

		struct Sub
		{
			String name;
			UReg id;
			UReg emitId;
		};

		typedef zone_map<UReg, Sub, ZToolsT>::type SubHash;
		SubHash subs;
	};

	typedef zone_vector<Material, Z3DXT>::type MatVec;
	
	String ReadString(InputStream &stream)
	{
		String s;
		for (;;)
		{
			S8 c;
			stream >> c;
			if (!c) break;
			s += (char)c;
		}

		return s;
	}

	Map::Vec2 ReadVec2(InputStream &stream)
	{
		float v[2];
		stream >> v[0];
		stream >> v[1];
		return Map::Vec2(Map::ValueType(v[0]), Map::ValueType(v[1]));
	}

	Map::Vec3 ReadVec3(InputStream &stream)
	{
		float v[3];
		stream >> v[0];
		stream >> v[1];
		stream >> v[2];
		return Map::Vec3(Map::ValueType(v[0]), Map::ValueType(v[1]), Map::ValueType(v[2]));
	}

	Map::Mat4 ReadMat3(InputStream &stream)
	{
		float v[12];
		for (int i = 0; i < 12; ++i)
			stream >> v[i];
		return Map::Mat4(
			Map::Vec4(v[0], v[1], v[2],  0.f),
			Map::Vec4(v[3], v[4], v[5],  0.f),
			Map::Vec4(v[6], v[7], v[8],  0.f),
			Map::Vec4(v[9], v[10], v[11], 1.f)
		);
	}

	Map::Quat ReadQuat(InputStream &stream)
	{
		float w, x, y, z;
		stream >> w >> x >> y >> z;
		return Map::Quat(x, y, z, w);
	}

	
	void DecompAffine(Map::Vec3 &s, Map::Quat &q, Map::Vec3 &t, const Map::Mat4 &in)
	{
		int i;
		
		Map::Mat4 m = in;

		for (i = 0; i < 3; ++i)
		{
			double d = sqrt(in[i][0] * in[i][0] + in[i][1] * in[i][1] + in[i][2] * in[i][2]);
			s[i] = (float)d;
			m[i][0] = (float)(in[i][0] / d);
			m[i][1] = (float)(in[i][1] / d);
			m[i][2] = (float)(in[i][2] / d);
		}
		
		q = in.Rotation();
		
		for (i = 0; i < 3; ++i)
		{
			t[i] = m[3][i];
		}
	}

	Map::BoneTM ReadBoneTM(InputStream &stream)
	{
		Map::BoneTM tm;
		Map::Mat4 m = ReadMat3(stream);
		DecompAffine(tm.s, tm.r, tm.t, m);
		return tm;
	}

	typedef Map::Vec2 UV;
	typedef zone_set<UReg, Z3DXT>::type TriFaceIdxSet;
//	typedef zone_set<UReg, Z3DXT>::pool_type TriFaceIdxSetPool;
	typedef zone_vector<UV, Z3DXT>::type UVVec;
//	typedef zone_vector<UV, Z3DXT>::pool_type UVVecPool;

	struct TriVert
	{
		Map::Vec3 pos;
		Map::Vec3 orgPos;
	};

	typedef zone_vector<TriVert, Z3DXT>::type TriVertVec;

	struct UVFace
	{
		int v[3];
	};

	typedef zone_vector<UVFace, Z3DXT>::type UVFaceVec;

	struct TriFace
	{
		int v[3];
		int sm[3];
		int smg;
		int mat;
		Map::Plane plane;
	};

	typedef zone_vector<TriFace, Z3DXT>::type TriFaceVec;
//	typedef zone_vector<TriFace, Z3DXT>::pool_type TriFaceVecPool;

	struct TriModel
	{
		UVVec      uvs[Map::MaxUVChannels];
		UVFaceVec  uvtris[Map::MaxUVChannels];
		TriVertVec verts;
		TriFaceVec tris;
		Map::BBox bounds;
		int contents;
		int id;
		int skel;
		int numChannels;
		Map::SkinRef skin;
		Map::AnimMap anims;
		bool cinematic;
		bool hideUntilRef;
		bool hideWhenDone;
	};

	void ReadTriModel(InputStream &stream, int version, TriModel &mdl, int flags, const Map::SkelVec &skels)
	{
		U32 nv, nf, nc;

		if (flags&HasMeshFlag)
		{
			stream >> nv;
			stream >> nf;
			stream >> nc;

			mdl.numChannels = nc;

			mdl.verts.reserve(nv);
			for (U32 i = 0; i < nv; ++i)
			{
				TriVert v;
				v.orgPos = ReadVec3(stream);
				/*v.pos[0] = math::Floor(v.orgPos[0] - Map::ValueType(0.5)) + Map::ValueType(1.0);
				v.pos[1] = math::Floor(v.orgPos[1] - Map::ValueType(0.5)) + Map::ValueType(1.0);
				v.pos[2] = math::Floor(v.orgPos[2] - Map::ValueType(0.5)) + Map::ValueType(1.0);*/
				v.pos = v.orgPos;
				mdl.bounds.Insert(v.pos);
				mdl.verts.push_back(v);
			}

			UVFaceVec  uvtris[Map::MaxUVChannels];

			for (U32 i = 0; i < nc; ++i)
			{
				U32 nuv;
				stream >> nuv;

				if (i < Map::MaxUVChannels)
				{
					mdl.uvs[i].reserve(nuv);
					mdl.uvtris[i].reserve(nf);
				}

				for (U32 j = 0; j < nuv; ++j)
				{
					Map::Vec2 v = ReadVec2(stream); // ignore
					
					if (i < Map::MaxUVChannels)
					{
						mdl.uvs[i].push_back(v);
					}
				}
				for (U32 j = 0; j < nf; ++j)
				{
					UVFace f;
					stream >> f.v[2];
					stream >> f.v[1];
					stream >> f.v[0];
					
					if (i < Map::MaxUVChannels)
					{
						uvtris[i].push_back(f);
					}
				}
			}
			
			mdl.tris.reserve(nf);
			bool warn = false;
			for (U32 i = 0; i < nf; ++i)
			{
				TriFace f;
				stream >> f.v[2];
				stream >> f.v[1];
				stream >> f.v[0];
				stream >> f.smg;
				stream >> f.mat;

				int z;
				for (z = 0; z < 2; ++z)
				{
					if (mdl.verts[f.v[z]].pos == mdl.verts[f.v[z+1]].pos) 
						break;
					if (mdl.verts[f.v[z]].pos == mdl.verts[f.v[(z+2)%3]].pos) 
						break;
				}
				if (z == 2)
				{
					f.plane.Initialize(mdl.verts[f.v[2]].pos, mdl.verts[f.v[1]].pos, mdl.verts[f.v[0]].pos);
					mdl.tris.push_back(f);

					for (U32 k = 0; k < Map::MaxUVChannels && k < nc; ++k)
						mdl.uvtris[k].push_back(uvtris[k][i]);
				}
				else if (!warn)
				{
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

		if (flags&HasMeshFlag)
		{
			mdl.skin.reset(new Map::Skin());
			mdl.skin->reserve(nv);

			// skin
			for (U32 i = 0; i < nv; ++i)
			{
				Map::BoneWeights weights;

				U32 numWeights;
				stream >> numWeights;
				weights.reserve(numWeights);

				for (U32 j = 0; j < numWeights; ++j)
				{
					Map::BoneWeight w;
					stream >> w.weight;
					stream >> w.bone;
					if (w.weight > 0.0f)
						weights.push_back(w);
				}

				mdl.skin->push_back(weights);
			}
		}

		// anims
		for (int i = 0; i < numAnims; ++i)
		{
			const Map::Skel::Ref &skel = skels[mdl.skel];

			Map::Anim::Ref a(new Map::Anim());
			a->name = ReadString(stream);
			
			U32 flags, numFrames, firstFrame;

			if (version > Version2)
			{
				stream >> flags >> firstFrame >> numFrames;
			}
			else
			{
				firstFrame = 0;
				stream >> flags >> numFrames;
			}

			a->looping = flags&1;
			a->frameRate = frameRate;
			a->firstFrame = (int)firstFrame;
			a->frames.resize(numFrames);

			for (U32 j = 0; j < numFrames; ++j)
			{
				Map::BonePoseVec &tms = a->frames[j];
				tms.resize(skel->bones.size());
				for (Map::BonePoseVec::iterator tm = tms.begin(); tm != tms.end(); ++tm)
				{
					(*tm).fov = 0.f;
					(*tm).m = ReadBoneTM(stream);
					(*tm).tag = ReadString(stream);
				}
			}

			mdl.anims.insert(Map::AnimMap::value_type(a->name, a));
		}
	}

	struct SmoothVert : public Map::WeightedTriVert
	{
		TriFaceIdxSet faces;
		int sm;
		int id;
	};

	typedef zone_multimap<SmoothVert, int, Z3DXT>::type SmoothVertIdxMap;
//	typedef zone_multimap<SmoothVert, int, Z3DXT>::pool_type SmoothVertIdxMapPool;
	typedef zone_vector<SmoothVert, Z3DXT>::type SmoothVertVec;
//	typedef zone_vector<SmoothVert, Z3DXT>::pool_type SmoothVertVecPool;

	void AddFaces(const SmoothVert &src, SmoothVert &dst)
	{
		for (TriFaceIdxSet::const_iterator it = src.faces.begin(); it != src.faces.end(); ++it)
			dst.faces.insert(*it);
	}

	void CombineFaces(SmoothVert &a, SmoothVert &b)
	{
		AddFaces(a, b);
		a.faces = b.faces;
	}

//#define SMOOTHING_GROUPS

	int HashVert(const SmoothVert &v, SmoothVertVec &vec, SmoothVertIdxMap &idxMap, bool smooth)
	{
		std::pair<SmoothVertIdxMap::const_iterator,
		          SmoothVertIdxMap::const_iterator> pair = idxMap.equal_range(v);

		while (pair.first != pair.second)
		{
#if defined(SMOOTHING_GROUPS)
			if (smooth)
			{
				SmoothVert &x = vec[pair.first->second];
				if (v.sm & x.sm)
				{
					x.sm |= v.sm;
					AddFaces(v, x);
					return pair.first->second;
				}
				++pair.first;
				continue;
			}
#endif
			return pair.first->second;
		}

		vec.push_back(v);
		int ofs = (int)(vec.size()-1);
		idxMap.insert(SmoothVertIdxMap::value_type(v, ofs));
		return ofs;
	}

	void MakeNormals(const TriModel &mdl, SmoothVertVec &vec, bool smooth)
	{
#if defined(SMOOTHING_GROUPS)
		// some coincedent vertices may not have been combined
		// because their UV's are different. smooth over these as well.
		if (smooth)
		{
			for (SmoothVertVec::iterator it = vec.begin(); it != vec.end(); ++it)
			{
				SmoothVert &a = *it;
				for (SmoothVertVec::iterator it2 = vec.begin(); it2 != vec.end(); ++it2)
				{
					if (it == it2) 
						continue;

					SmoothVert &b = *it2;

					if (a.sm & b.sm) // smooth?
					{
						if (a.pos != b.pos) 
							continue;

						a.sm = b.sm = (a.sm|b.sm);
						CombineFaces(a, b);
					}
				}
			}
		}
#endif

		for (SmoothVertVec::iterator it = vec.begin(); it != vec.end(); ++it)
		{
			SmoothVert &v = *it;
			v.normal = Map::Vec3::Zero;
			
#if defined(SMOOTHING_GROUPS)
			for (TriFaceIdxSet::iterator it2 = v.faces.begin(); it2 != v.faces.end(); ++it2)
			{
				U32 idx = *it2;
				v.normal += mdl.tris[idx].plane.Normal();
				v.normal.Normalize();
			}
#endif
		}
	}

	// apply smoothing groups and build the real trimodel.
	Map::TriModel::Ref Build(TriModel &mdl, int id, int count, bool smooth)
	{
		COut(C_Debug) << "(3DX) processing mesh " << (id+1) << "/" << count << std::endl;
		
		Map::TriModel::Ref mmdl(new Map::TriModel());
		mmdl->contents = mdl.contents;
		mmdl->id = mdl.id;
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
		for (TriFaceVec::iterator it = mdl.tris.begin(); it != mdl.tris.end(); ++it, ++idx)
		{
			TriFace &tri = *it;

			for (int i = 0; i < 3; ++i)
			{
				v.faces.clear();
				v.weights.clear();
				v.faces.insert(idx);
				v.pos = mdl.verts[tri.v[i]].pos;
				v.orgPos = mdl.verts[tri.v[i]].orgPos;
				if (mdl.skin)
					v.weights = (*mdl.skin)[tri.v[i]];
				v.sm  = tri.smg;
				v.id = tri.v[i];

				for (int j = 0; j < Map::MaxUVChannels; ++j)
				{
					if (uvv[j].empty())
					{
						v.st[j] = UV::Zero;
					}
					else
					{
						v.st[j] = uvv[j][uvf[j][idx].v[i]];
					}
				}

				tri.sm[i] = HashVert(v, smv, smidxm, smooth);
			}

			mmdl->tris.push_back(Map::TriFace(tri.sm[0], tri.sm[1], tri.sm[2], tri.mat, tri.plane, mmdl.get()));
		}

		MakeNormals(mdl, smv, smooth);

		mmdl->verts.reserve(smv.size());
		mmdl->bounds = mdl.bounds;

		if (mdl.skin)
			mmdl->skin.reset(new Map::Skin());
				
		for (SmoothVertVec::iterator it = smv.begin(); it != smv.end(); ++it)
		{
			mmdl->verts.push_back(*it);
			if (mmdl->skin)
				mmdl->skin->push_back((*it).weights);
		}

		return mmdl;
	}

	void CompactStaticPools()
	{
//		TriFaceIdxSetPool::release_memory();
//		UVVecPool::release_memory();
//		SmoothVertIdxMapPool::release_memory();
//		SmoothVertVecPool::release_memory();
	}
}

int EmitMaterial(const String &name, Map &map)
{
	for (size_t i = 0; i < map.mats.size(); ++i)
	{
		if (map.mats[i].name == name)
			return (int)i;
	}

	Map::Material mm;
	mm.name = name;
	map.mats.push_back(mm);
	return (int)(map.mats.size()-1);
}

bool LoadMaxScene(InputStream &nakedstr, Map &map, bool smooth)
{
	LittleInputStream stream(nakedstr.Buffer());
	U32 id, version;

	stream >> id;
	stream >> version;

	if (id != Id || (version < Version2 || version > Version)) 
		return false;

	MatVec mats;
	U32 n;

	// materials.

	stream >> n;
	for (U32 i = 0; i < n; ++i)
	{
		Material m;
		m.name = ReadString(stream);
		stream >> m.flags;
		if (m.flags & 1) // multisub
		{
			U32 z;
			stream >> z;
			for (U32 j = 0; j < z; ++j)
			{
				Material::Sub s;
				stream >> s.id;
				s.name = ReadString(stream);
				s.emitId = -1;//(UReg)EmitMaterial(s.name, map);
				m.subs.insert(Material::SubHash::value_type(s.id, s));
			}
		}
		else
		{
			m.emitId = -1;//(UReg)EmitMaterial(m.name, map);
		}
		mats.push_back(m);
	}

	if (version > Version2) // load cameras
	{
		stream >> n;

		for (U32 i = 0; i < n; ++i)
		{
			Map::Camera::Ref cam(new Map::Camera());

			cam->name = ReadString(stream);

			U32 flags;
			stream >> flags;

			U32 z, frameRate;
			stream >> z; // skip skel index

			stream >> z >> frameRate; // num anims

			for (U32 k = 0; k < z; ++k)
			{
				Map::Anim::Ref a(new Map::Anim());
				a->name = ReadString(stream);
				
				S32 firstFrame;
				U32 numFrames;
				stream >> flags >> firstFrame >> numFrames;

				a->looping = flags&1;
				a->frameRate = frameRate;
				a->firstFrame = (int)firstFrame;
				a->frames.resize(numFrames);

				for (U32 j = 0; j < numFrames; ++j)
				{
					Map::BonePoseVec &tms = a->frames[j];
					tms.resize(1);
					for (Map::BonePoseVec::iterator tm = tms.begin(); tm != tms.end(); ++tm)
					{
						stream >> (*tm).fov;
						(*tm).m = ReadBoneTM(stream);
						(*tm).tag = ReadString(stream);
					}
				}

				cam->anims.insert(Map::AnimMap::value_type(a->name, a));
				map.cameras.push_back(cam);
			}
		}
	}

	// entities
	stream >> n;

	{
		TriModel mdl;

		for (U32 i = 0; i < n; ++i)
		{
			Map::Entity::Ref ent(new Map::Entity());
			ent->maxEnt = true;
			ent->name = ReadString(stream);
			stream >> ent->id;
			ent->origin = ReadVec3(stream);
			U32 z;
			
			stream >> z;
			for (U32 j = 0; j < z; ++j) // skels.
			{
				Map::Skel::Ref sk(new Map::Skel());
				U32 numBones;
				stream >> numBones;
				for (U32 b = 0; b < numBones; ++b)
				{
					Map::Bone bone;
					bone.name = ReadString(stream);
					stream >> bone.parent;
					bone.world = ReadMat3(stream);
					sk->bones.push_back(bone);
				}

				ent->skels.push_back(sk);
			}

			stream >> z;
			for (U32 j = 0; j < z; ++j) // trimodels
			{
				Material *m = 0;
				U32 flags;

				stream >> mdl.id;
				stream >> flags;
				stream >> mdl.skel;

				if (flags & HasMaterialFlag) // has material
				{
					U32 idx;
					stream >> idx;
					m = &mats[idx];
				}

				if (flags & ContentsDetailFlag)
				{
					mdl.contents = Map::ContentsDetail;
				}
				else if (flags & ContentsAreaportalFlag)
				{
					mdl.contents = Map::ContentsAreaportal;
				}
				else
				{
					mdl.contents = Map::ContentsSolid;
				}

				if (!(flags & ContentsNoClipFlag))
				{
					mdl.contents |= Map::ContentsNoClip;
				}

				if (!(flags & ContentsNoDrawFlag))
				{
					mdl.contents |= Map::ContentsNoDraw;
				}

				if (version > Version2)
				{
					mdl.cinematic = (flags&CinematicObjectFlag) ? true : false;
					mdl.hideUntilRef = (flags&HideUntilRefedFlag) ? true : false;
					mdl.hideWhenDone = (flags&HideWhenDoneFlag) ? true : false;
				}
				else
				{
					mdl.cinematic = false;
					mdl.hideUntilRef = false;
					mdl.hideWhenDone = false;
				}

				mdl.verts.clear();
				mdl.tris.clear();
				mdl.bounds.Initialize();
				mdl.anims.clear();
				mdl.skin.reset();
				
				for (int i = 0; i < Map::MaxUVChannels; ++i)
				{
					mdl.uvs[i].clear();
					mdl.uvtris[i].clear();
				}
				
				if (flags&(HasMeshFlag|HasAnimsFlag))
					ReadTriModel(stream, version, mdl, flags, ent->skels);

				for (TriFaceVec::iterator it = mdl.tris.begin(); it != mdl.tris.end(); ++it)
				{
					if (m)
					{
						Material::SubHash::iterator sub = m->subs.find((U32)it->mat);
						if (sub != m->subs.end())
						{
							if (sub->second.emitId == -1)
								sub->second.emitId = EmitMaterial(sub->second.name, map);
							it->mat = (int)sub->second.emitId;
						}
						else
						{
							if (m->emitId == -1)
								m->emitId = EmitMaterial(m->name, map);
							it->mat = (int)m->emitId;
						}
					}
					else
					{
						it->mat = -1;
					}
				}

				if (!mdl.tris.empty())
					ent->models.push_back(Build(mdl, j, z, smooth));
			}

			if (ent->name == "worldspawn")
			{
				if (map.worldspawn)
				{ // move to worldspawn
					std::copy(
						ent->models.begin(), 
						ent->models.end(), 
						std::back_inserter(map.worldspawn->models)
					);

					map.worldspawn->skels = ent->skels;
				}
				else
				{
					map.worldspawn = ent;
				}
			}
			else
			{
				map.ents.push_back(ent);
			}
		}
	}

	CompactStaticPools();

	return true;
}

} // tools

#endif // RAD_OPT_TOOLS
