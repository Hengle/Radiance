// MaxScene.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "MaxScene.h"
#include <Runtime/Runtime.h>
#include <Runtime/Container/HashMap.h>
#include <Runtime/Endian/EndianStream.h>
#include "Log.h"
#include <string>
#include <vector>
#include <map>
#include <iterator>


using namespace stream;

namespace
{
	enum
	{
		Id = RAD_FOURCC_LE('R', 'S', 'C', 'N'),
		Version = 2
	};

	struct Material
	{
		std::string name;
		U32 flags;
		int emitId;

		struct Sub
		{
			std::string name;
			int id;
			int emitId;
		};

		typedef container::hash_map<int, Sub>::type SubHash;
		SubHash subs;
	};

	typedef std::vector<Material> MatVec;
	
	std::string ReadString(InputStream &stream)
	{
		std::string s;
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

	typedef Map::Vec2 UV;
	typedef std::vector<UReg> TriFaceIdxVec;
	typedef std::vector<UV> UVVec;

	struct TriVert
	{
		Map::Vec3 pos;
		Map::Vec3 orgPos;
	};

	typedef std::vector<TriVert> TriVertVec;

	struct UVFace
	{
		int v[3];
	};

	typedef std::vector<UVFace> UVFaceVec;

	struct TriFace
	{
		int v[3];
		int sm[3];
		int smg;
		int mat;
		Map::Plane plane;
	};

	typedef std::vector<TriFace> TriFaceVec;

	struct TriModel
	{
		UVVec      uvs[Map::MaxUVChannels];
		UVFaceVec  uvtris[Map::MaxUVChannels];
		TriVertVec verts;
		TriFaceVec tris;
		Map::BBox bounds;
		int contents;
		int id;
	};

	void ReadTriModel(InputStream &stream, TriModel &mdl)
	{
		mdl.verts.clear();
		mdl.tris.clear();
		mdl.bounds.Initialize();
		
		for (int i = 0; i < Map::MaxUVChannels; ++i)
		{
			mdl.uvs[i].clear();
			mdl.uvtris[i].clear();
		}

		U32 nv, nf, nc;
		stream >> nv;
		stream >> nf;
		stream >> nc;

		mdl.verts.reserve(nv);
		for (U32 i = 0; i < nv; ++i)
		{
			TriVert v;
			v.orgPos = ReadVec3(stream);
			v.pos[0] = math::Floor(v.orgPos[0] - Map::ValueType(0.5)) + Map::ValueType(1.0);
			v.pos[1] = math::Floor(v.orgPos[1] - Map::ValueType(0.5)) + Map::ValueType(1.0);
			v.pos[2] = math::Floor(v.orgPos[2] - Map::ValueType(0.5)) + Map::ValueType(1.0);
			mdl.bounds.Insert(v.pos);
			mdl.verts.push_back(v);
		}

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
				Map::Vec2 v = ReadVec2(stream);
				
				if (i < Map::MaxUVChannels) // ignore?
				{
					mdl.uvs[i].push_back(v);
				}
			}
			for (U32 j = 0; j < nf; ++j)
			{
				UVFace f;
				stream >> f.v[0];
				stream >> f.v[1];
				stream >> f.v[2];
				
				if (i < Map::MaxUVChannels)
				{
					mdl.uvtris[i].push_back(f);
				}
			}
		}
		
		mdl.tris.reserve(nf);
		bool warn = false;
		for (U32 i = 0; i < nf; ++i)
		{
			TriFace f;
			stream >> f.v[0];
			stream >> f.v[1];
			stream >> f.v[2];
			stream >> f.smg;
			stream >> f.mat;

			int z;
			for (z = 0; z < 2; ++z)
			{
				if (mdl.verts[f.v[z]].pos == mdl.verts[f.v[z+1]].pos) break;
				if (mdl.verts[f.v[z]].pos == mdl.verts[f.v[(z+2)%3]].pos) break;
			}
			if (z == 2)
			{
				f.plane.Initialize(mdl.verts[f.v[0]].pos, mdl.verts[f.v[1]].pos, mdl.verts[f.v[2]].pos);
				mdl.tris.push_back(f);
			}
			else if (!warn)
			{
				warn = true;
				Log(LogWarning, "WARNING: mesh id %d has degenerate triangles.\n", mdl.id);
			}
		}

		// skin

		U32 numAnims;
		U32 frameRate;

		stream >> numAnims >> frameRate;

	}

	struct SmoothVert : public Map::TriVert
	{
		TriFaceIdxVec faces;
		int sm;

		bool operator < (const SmoothVert &v)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (pos[i] < v.pos[i]) return true;
			}
			for (int i = 0; i < Map::MaxUVChannels; ++i)
			{
				for (int j = 0; j < 2; ++j)
				{
					if (st[i][j] < v.st[i][j]) return true;
				}
			}
			return sm < v.sm;
		}
	};

	typedef std::vector<SmoothVert> SmoothVertVec;

	void AddFaces(const SmoothVert &src, SmoothVert &dst)
	{
		dst.faces.reserve(dst.faces.size()+src.faces.size());
		TriFaceIdxVec faces;
		for (TriFaceIdxVec::const_iterator it = src.faces.begin(); it != src.faces.end(); ++it)
		{
			TriFaceIdxVec::iterator it2;
			for (it2 = dst.faces.begin(); it2 != dst.faces.end(); ++it2)
			{
				if (*it == *it2) break;
			}
			if (it2 != dst.faces.end()) continue;
			faces.push_back(*it);
		}

		std::copy(faces.begin(), faces.end(), std::back_insert_iterator<TriFaceIdxVec>(dst.faces));
	}

	void CombineFaces(SmoothVert &a, SmoothVert &b)
	{
		a.faces.reserve(a.faces.size()+b.faces.size());
		b.faces.reserve(a.faces.size()+b.faces.size());
		TriFaceIdxVec faces = a.faces;
		for (TriFaceIdxVec::iterator it = b.faces.begin(); it != b.faces.end(); ++it)
		{
			TriFaceIdxVec::iterator it2;
			for (it2 = faces.begin(); it2 != faces.end(); ++it2)
			{
				if (*it == *it2) break;
			}
			if (it2 != faces.end()) continue;
			faces.push_back(*it);
		}

		a.faces = faces;
		b.faces = faces;
	}

	int HashVert(const SmoothVert &v, SmoothVertVec &vec)
	{
		SmoothVertVec::iterator it;
		for (it = vec.begin(); it != vec.end(); ++it)
		{
			SmoothVert &x = *it;
			if (x.pos != v.pos) continue;
			int i;
			for (i = 0; i < Map::MaxUVChannels; ++i)
			{
				if (x.st[i] != v.st[i]) break;
			}
			if (i != Map::MaxUVChannels) continue;

			if (v.sm & x.sm) // smooth across this vertex?
			{
				x.sm |= v.sm;
				AddFaces(v, x);
				return (int)(it-vec.begin());
			}
		}

		vec.push_back(v);
		return (int)(vec.size()-1);
	}

	void MakeNormals(const TriModel &mdl, SmoothVertVec &vec)
	{
		// some coincedent vertices may not have been combined
		// because their UV's are different. smooth over these as well.
		for (SmoothVertVec::iterator it = vec.begin(); it != vec.end(); ++it)
		{
			SmoothVert &a = *it;
			for (SmoothVertVec::iterator it2 = vec.begin(); it2 != vec.end(); ++it2)
			{
				if (it == it2) continue;
				SmoothVert &b = *it2;

				if (a.pos != b.pos) continue;
				if (a.sm & b.sm) // smooth?
				{
					a.sm = b.sm = (a.sm|b.sm);
					CombineFaces(a, b);
				}
			}
		}

		for (SmoothVertVec::iterator it = vec.begin(); it != vec.end(); ++it)
		{
			SmoothVert &v = *it;
			v.normal = Map::Vec3::Zero;
			
			for (TriFaceIdxVec::iterator it2 = v.faces.begin(); it2 != v.faces.end(); ++it2)
			{
				U32 idx = *it2;
				v.normal += mdl.tris[idx].plane.Normal();
				v.normal.Normalize();
			}
		}
	}

	// apply smoothing groups and build the real trimodel.
	Map::TriModelRef Build(TriModel &mdl)
	{
		Map::TriModelRef mmdl(new Map::TriModel());
		mmdl->contents = mdl.contents;
		mmdl->id = mdl.id;

		SmoothVertVec smv;
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
				v.faces.push_back(idx);
				v.pos = mdl.verts[tri.v[i]].pos;
				v.orgPos = mdl.verts[tri.v[i]].orgPos;
				v.sm  = tri.smg;

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

				tri.sm[i] = HashVert(v, smv);
			}

			mmdl->tris.push_back(Map::TriFace(tri.sm[0], tri.sm[1], tri.sm[2], tri.mat, tri.plane, mmdl.get()));
		}

		MakeNormals(mdl, smv);

		mmdl->verts.reserve(smv.size());
		mmdl->bounds = mdl.bounds;
		
		for (SmoothVertVec::iterator it = smv.begin(); it != smv.end(); ++it)
		{
			mmdl->verts.push_back(*it);
		}

		return mmdl;
	}
}

void LoadMaxScene(InputStream &nakedstr, Map &map)
{
	LittleInputStream stream(nakedstr.Buffer());
	U32 id, version;

	stream >> id;
	stream >> version;

	if (id != Id || version != Version) throw std::exception("Invalid max scene file!");

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
				Map::Material mm;
				mm.name = s.name;
				s.emitId = (UReg)map.mats.size();
				map.mats.push_back(mm);
				m.subs.insert(Material::SubHash::value_type(s.id, s));
			}
		}
		else
		{
			Map::Material mm;
			mm.name = m.name;
			m.emitId = (UReg)map.mats.size();
			map.mats.push_back(mm);
		}
		mats.push_back(m);
	}

	// entities

	stream >> n;

	TriModel mdl;

	for (U32 i = 0; i < n; ++i)
	{
		Map::EntityRef ent(new Map::Entity());
		ent->name = ReadString(stream);
		stream >> ent->id;
		ent->origin = ReadVec3(stream);
		U32 z;
		stream >> z;
		for (U32 j = 0; j < z; ++j)
		{
			Material *m = 0;
			U32 flags;

			stream >> mdl.id;
			stream >> flags;
			
			if (flags & 0x80000000) // has material
			{
				U32 idx;
				stream >> idx;
				m = &mats[idx];
			}

			stream >> mdl.skel;

			if (flags & 0x40000000)
			{
				mdl.contents = Map::ContentsDetail;
			}
			else if (flags & 0x20000000)
			{
				mdl.contents = Map::ContentsAreaportal;
			}
			else
			{
				mdl.contents = Map::ContentsSolid;
			}

			if (!(flags & 0x00800000))
			{
				mdl.contents |= Map::ContentsNoClip;
			}

			if (!(flags & 0x00400000))
			{
				mdl.contents |= Map::ContentsNoDraw;
			}

			ReadTriModel(stream, mdl);

			if (mdl.tris.empty())
			{
				Log(LogWarning, "WARNING: model id %d has no valid triangles.\n", mdl.id);
			}
			else
			{
				for (TriFaceVec::iterator it = mdl.tris.begin(); it != mdl.tris.end(); ++it)
				{
					if (m)
					{
						Material::SubHash::iterator sub = m->subs.find((U32)it->mat);
						if (sub != m->subs.end())
						{
							it->mat = (int)sub->second.emitId;
						}
						else
						{
							it->mat = m->emitId;
						}
					}
					else
					{
						it->mat = -1;
					}
				}

				ent->models.push_back(Build(mdl));
			}
		}

		map.ents.push_back(ent);
		if (ent->name == "worldspawn")
		{
			map.worldspawn = ent;
		}
	}
}