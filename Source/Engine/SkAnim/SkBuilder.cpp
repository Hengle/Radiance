// SkBuilder.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if defined(RAD_OPT_TOOLS)

#include "SkBuilder.h"
#include "../COut.h"
#include "../Packages/PackagesDef.h"
#include "../Tools/Progress.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Stream.h>
#include <Runtime/Stream/MemoryStream.h>
#include <Runtime/Endian/EndianStream.h>
#include <Runtime/Base/SIMD.h>
#include <limits>
#undef min
#undef max

BOOST_STATIC_ASSERT((int)ska::kMaxUVChannels <= (int)tools::SceneFile::kMaxUVChannels);

namespace tools {

namespace {

// [0] = compression floor
// [1] = quat zero_children basis
// [2] = compression basis

// compression = max(compression_floor, compression_basis / (numchildren+1))

static const float kQuatCompressionBasis[3] = {0.01f, 2.0f, 1.0f};

// [0] = compression floor
// [1] = compression basis

// compression = max(compression_floor, compression_basis / (numchildren+1))

static const float kScaleCompressionBasis[2] = {0.1f*0.1f, 0.5f*0.5f}; // units squared
static const float kTranslationCompressionBasis[2] = {0.5f*0.5f, 1.f*1.f};

typedef zone_vector<int, ZToolsT>::type IntVec;
typedef zone_set<int, ZToolsT>::type IntSet;
struct BoneMap {
	typedef zone_vector<BoneMap, ZToolsT>::type Vec;
	int idx;
	IntVec children;
};

class SkaBuilder {
public:
	typedef SkaBuilder Ref;

	SkaBuilder();
	~SkaBuilder();

	bool Compile(
		const char *name, 
		const SceneFileVec &map, 
		int trimodel, 
		const SkaCompressionMap *compression,
		SkaData &sk
	);

	bool Compile(
		const char *name, 
		const SceneFile &map, 
		int trimodel, 
		const SkaCompressionMap *compression,
		SkaData &sk
	);

private:

	struct BoneTM {
		ska::BoneTM tm;
		String tag;
	};

	typedef zone_vector<BoneTM, ZToolsT>::type BoneTMVec;

	struct BoneDef {
		typedef zone_vector<BoneDef, ZToolsT>::type Vec;

		String name;
		int remap;
		int childDepth; // deepest child branch
		S16 parent;
		Mat4 invWorld;
	};

	struct Compression {
		typedef zone_vector<Compression, ZToolsT>::type Vec;
		float quatCompression;
		float scaleCompression;
		float translateCompression;
	};

	typedef zone_vector<BoneTMVec, ZToolsT>::type FrameVec;

	struct Anim {
		typedef zone_vector<Anim, ZToolsT>::type Vec;
		String name;
		float fps;
		float distance;
		bool removeMotion;
		FrameVec frames;
		Compression::Vec boneCompression;
	};

	bool Compile(stream::IOutputBuffer &ob) const;
	
	void SetBones(const BoneDef::Vec &bones);

	void BeginAnim(
		const char *name,
		float fps,
		int numFrames,
		bool removeMotion,
		const SkaCompressionMap *compression
	);

	void AddFrame(const BoneTM *bones);

	void EndAnim(int motionBone);

	static bool EmitBones(
		const char *name, 
		int idx, 
		int parent, 
		BoneMap::Vec &map, 
		BoneDef::Vec &bones, 
		const SceneFile::BoneVec &skel
	);

	BoneDef::Vec m_bones;
	Anim::Vec m_anims;
};

///////////////////////////////////////////////////////////////////////////////

SkaBuilder::SkaBuilder() {
}

SkaBuilder::~SkaBuilder() {
}

bool SkaBuilder::EmitBones(const char *name, int idx, int parent, BoneMap::Vec &map, BoneDef::Vec &bones, const SceneFile::BoneVec &skel) {
	BoneMap &bm = map[idx];
	const SceneFile::Bone &mb = skel[idx];

	if (bm.idx > -1) {
		COut(C_Error) << "EmitBones(\"" << name << "\"): \"" << skel[idx].name << "\" has multiple parent bones!" << std::endl;
		return false;
	}

	bm.idx = (int)bones.size();

	BoneDef bone;
	bone.name = mb.name;
	bone.parent = (S16)parent;
	bone.remap = idx;
	bone.childDepth = 0;
	bone.invWorld = mb.world.Inverse();
	bones.push_back(bone);

	// propogate depth.
	int depth = 1;

	while (parent > -1) {
		if (bones[parent].childDepth < depth) {
			bones[parent].childDepth = depth;
			++depth;
			parent = bones[parent].parent;
		} else {
			break;
		}
	}

	for (IntVec::const_iterator it = bm.children.begin(); it != bm.children.end(); ++it) {
		if (!EmitBones(name, *it, bm.idx, map, bones, skel))
			return false;
	}

	return true;
}

bool SkaBuilder::Compile(
	const char *name, 
	const SceneFileVec &maps, 
	int trimodel,
	const SkaCompressionMap *compression,
	SkaData &sk
) {
	RAD_ASSERT(maps.size() == 1 || trimodel == 0);

	SceneFile::Entity::Ref e = maps[0]->worldspawn;
	if (e->models[trimodel]->skel < 0)
		return false;

	const SceneFile::Skel &skel = *e->skels[e->models[trimodel]->skel];

	// apply constraint: bones[boneidx].parent < boneidx
	// since we do not walk the bone heirarchy in max from
	// root->children (and instead use skinops) the bone
	// array is not guaranteed to be sorted in this fashion.
	BoneMap::Vec bmap;

	bmap.resize(skel.bones.size());
	int root = -1;

	for (int i = 0; i < (int)skel.bones.size(); ++i) {
		const SceneFile::Bone &b = skel.bones[i];
		bmap[i].idx = -1;

		if (b.parent >= 0) {
			bmap[b.parent].children.push_back(i);
		} else {
			if (root != -1) {
				COut(C_Error) << "BuildAnimData(\"" << name << "\"): has multiple root bones!" << std::endl;
				return false;
			}
			root = i;
		}
	}

	if (root == -1) {
		COut(C_Error) << "BuildAnimData(\"" << name << "\"): no root bone found!" << std::endl;
		return false;
	}

	BoneDef::Vec bones;
	if (!EmitBones(name, root, -1, bmap, bones, skel.bones))
		return false;

	for (BoneDef::Vec::iterator it = bones.begin(); it != bones.end(); ++it) {
		BoneDef &bone = *it;
		if (bone.parent >= 0)
			bone.invWorld = bones[bone.parent].invWorld * bone.invWorld;
	}

	// bones is now a vector of bones sorted in parent->children order.
	SetBones(bones);

	std::vector<bool> badList;
	badList.resize(maps.size());
	badList[0] = false;

	// ensure identical skels
	for (size_t i = 1; i < maps.size(); ++i) {
		badList[i] = true;

		SceneFile::Entity::Ref e = maps[i]->worldspawn;
		if (e->models[0]->skel < 0) {
			COut(C_Error) << "BuildAnimData(\"" << name << "\"): anim file " << i << " is missing skel data (skipping)!" << std::endl;
			continue;
		}

		const SceneFile::Skel &otherSkel = *e->skels[e->models[0]->skel];
		if (otherSkel.bones.size() != skel.bones.size()) {
			COut(C_Error) << "BuildAnimData(\"" << name << "\"): anim file " << i << " has mismatched skel (code 1, skipping)!" << std::endl;
			continue;
		}

		size_t k;
		for (k = 0; k < skel.bones.size(); ++k) {
			if (skel.bones[k].name != otherSkel.bones[k].name ||
				skel.bones[k].parent != otherSkel.bones[k].parent) {
				COut(C_Error) << "BuildAnimData(\"" << name << "\"): anim file " << i << " has mismatched skel (code 2, skipping)!" << std::endl;
				break;
			}
		}

		if (k != skel.bones.size())
			continue;

		badList[i] = false;
	}
	
	// build animation data
	for (size_t i = 0; i < maps.size(); ++i) {
		if (badList[i])
			continue;

		const SceneFile::TriModel::Ref &m = maps[i]->worldspawn->models[trimodel];

		for (SceneFile::AnimMap::const_iterator it = m->anims.begin(); it != m->anims.end(); ++it) {
			const SceneFile::Anim &anim = *(it->second.get());
			if (anim.frames.empty())
				continue;

			BeginAnim(
				anim.name.c_str,
				(float)anim.frameRate,
				(int)anim.frames.size(),
				false,
				compression
			);

			BoneTMVec tms;
			tms.resize(skel.bones.size());

			for (SceneFile::BoneFrames::const_iterator it = anim.frames.begin(); it != anim.frames.end(); ++it) {
				const SceneFile::BonePoseVec &frame = *it;

				// TODO: dynamic remap bones by name, would be pretty 
				// useful to decouple animation from a skeleton.

				if (frame.size() != skel.bones.size()) {
					COut(C_Error) << "BuildAnimData(\"" << name << "\"): animation \"" << anim.name << "\" frame " << 
						(it-anim.frames.begin()) << " has mismatched bone count." << std::endl;
					return false;
				}

				for (SceneFile::BonePoseVec::const_iterator it = frame.begin(); it != frame.end(); ++it) {
					const SceneFile::BonePose &mtm = *it;
					BoneTM &tm = tms[bmap[it-frame.begin()].idx];

					tm.tm.r = mtm.m.r;
					tm.tm.s = mtm.m.s;
					tm.tm.t = mtm.m.t;
					tm.tag = mtm.tag;
				}

				AddFrame(&tms[0]);
			}

			EndAnim(root);
		}
	}

	// compile.
	stream::DynamicMemOutputBuffer ob(ska::ZSka);
	RAD_VERIFY(Compile(ob));
	sk.skaData = ob.OutputBuffer().Ptr();
	sk.skaSize = (AddrSize)ob.OutPos();
	ob.OutputBuffer().Set(0, 0); // so ob doesn't free the buffer.

	// trim
	sk.skaData = zone_realloc(ska::ZSka, sk.skaData, sk.skaSize);
	RAD_VERIFY(sk.dska.Parse(sk.skaData, sk.skaSize) == pkg::SR_Success);

	return true;
}

bool SkaBuilder::Compile(
	const char *name, 
	const SceneFile &map, 
	int trimodel, 
	const SkaCompressionMap *compression,
	SkaData &sk
) {
	SceneFile::Entity::Ref e = map.worldspawn;
	if (e->models[trimodel]->skel < 0)
		return false;

	const SceneFile::Skel &skel = *e->skels[e->models[trimodel]->skel];

	// apply constraint: bones[boneidx].parent < boneidx
	// since we do not walk the bone heirarchy in max from
	// root->children (and instead use skinops) the bone
	// array is not guaranteed to be sorted in this fashion.
	BoneMap::Vec bmap;

	bmap.resize(skel.bones.size());
	int root = -1;

	for (int i = 0; i < (int)skel.bones.size(); ++i) {
		const SceneFile::Bone &b = skel.bones[i];
		bmap[i].idx = -1;

		if (b.parent >= 0) {
			bmap[b.parent].children.push_back(i);
		} else {
			if (root != -1) {
				COut(C_Error) << "BuildAnimData(\"" << name << "\"): has multiple root bones!" << std::endl;
				return false;
			}
			root = i;
		}
	}

	if (root == -1) {
		COut(C_Error) << "BuildAnimData(\"" << name << "\"): no root bone found!" << std::endl;
		return false;
	}

	BoneDef::Vec bones;
	if (!EmitBones(name, root, -1, bmap, bones, skel.bones))
		return false;

	for (BoneDef::Vec::iterator it = bones.begin(); it != bones.end(); ++it) {
		BoneDef &bone = *it;
		if (bone.parent >= 0)
			bone.invWorld = bones[bone.parent].invWorld * bone.invWorld;
	}

	// bones is now a vector of bones sorted in parent->children order.
	SetBones(bones);


	const SceneFile::TriModel::Ref &m = map.worldspawn->models[trimodel];

	for (SceneFile::AnimMap::const_iterator it = m->anims.begin(); it != m->anims.end(); ++it) {
		const SceneFile::Anim &anim = *(it->second.get());
		if (anim.frames.empty())
			continue;

		BeginAnim(
			anim.name.c_str,
			(float)anim.frameRate,
			(int)anim.frames.size(),
			false,
			compression
		);

		BoneTMVec tms;
		tms.resize(skel.bones.size());

		for (SceneFile::BoneFrames::const_iterator it = anim.frames.begin(); it != anim.frames.end(); ++it) {
			const SceneFile::BonePoseVec &frame = *it;

			// TODO: dynamic remap bones by name, would be pretty 
			// useful to decouple animation from a skeleton.

			if (frame.size() != skel.bones.size()) {
				COut(C_Error) << "BuildAnimData(\"" << name << "\"): animation \"" << anim.name << "\" frame " << 
					(it-anim.frames.begin()) << " has mismatched bone count." << std::endl;
				return false;
			}

			for (SceneFile::BonePoseVec::const_iterator it = frame.begin(); it != frame.end(); ++it) {
				const SceneFile::BonePose &mtm = *it;
				BoneTM &tm = tms[bmap[it-frame.begin()].idx];

				tm.tm.r = mtm.m.r;
				tm.tm.s = mtm.m.s;
				tm.tm.t = mtm.m.t;
				tm.tag = mtm.tag;
			}

			AddFrame(&tms[0]);
		}

		EndAnim(root);
	}

	// compile.
	stream::DynamicMemOutputBuffer ob(ska::ZSka);
	RAD_VERIFY(Compile(ob));
	sk.skaData = ob.OutputBuffer().Ptr();
	sk.skaSize = (AddrSize)ob.OutPos();
	ob.OutputBuffer().Set(0, 0); // so ob doesn't free the buffer.

	// trim
	sk.skaData = zone_realloc(ska::ZSka, sk.skaData, sk.skaSize);
	RAD_VERIFY(sk.dska.Parse(sk.skaData, sk.skaSize) == pkg::SR_Success);

	return true;
}

typedef zone_vector<float, ZToolsT>::type FloatVec;

struct Tables {
	FloatVec rTable;
	FloatVec sTable;
	FloatVec tTable;
	StringVec strings;

	Tables() : tAbsMax(-1.0f), sAbsMax(-1.0f) {
		strings.reserve(64);
	}

	// building these tables is all slow and linear, need to figure out how to speed this up.

	float tAbsMax;
	float sAbsMax;

	static S16 QuantFloat(float f, float absMax) {
		if (absMax <= 0.f)
			return 0;
		const int shortMax = std::numeric_limits<S16>::max();
		const int shortMin = std::numeric_limits<S16>::min();
		int val = (int)floor(((f*shortMax)/absMax)+0.5);
		if (val > shortMax)
			return (S16)shortMax;
		if (val < shortMin)
			return (S16)shortMin;
		return (S16)val;
	}

	bool AddRotate(const Quat &r, float eqDist, int &out) {
		
		int best = -1;
		float bestError = eqDist*3.f;

		for (int i = 0; i+4 <= (int)rTable.size(); i += 4) {
			if (QuatEq(*((const Quat*)&rTable[i]), r, eqDist, bestError)) {
				best = i;
			}
		}

		if (best != -1) {
			out = best / 4;
			return true;
		}

		if ((rTable.size()/4) == ska::kEncMask)
			return false;
		
		for (size_t i = 0; i < 4; ++i)
			rTable.push_back(r[(int)i]);
		out = (int)(rTable.size()/4)-1;
		return true;
	}

	bool AddScale(const Vec3 &s, float max, int &out) {
		return AddVec3(sTable, s, out, sAbsMax, max);
	}

	bool AddTranslate(const Vec3 &t, float max, int &out) {
		return AddVec3(tTable, t, out, tAbsMax, max);
	}

	int AddString(const String &str) {
		if (str.empty)
			return -1;

		for (StringVec::const_iterator it = strings.begin(); it != strings.end(); ++it) {
			if (str == *it)
				return (int)(it-strings.begin());
		}

		int ofs = (int)strings.size();
		if (ofs > 254) {
			COut(C_Error) << "SkaBuilder::Tables: string table exceeds 255 elements!" << std::endl;
			return -1;
		}
		strings.push_back(str);
		return ofs;
	}

private:

	bool QuatEq(const Quat &a, const Quat &b, float max, float &error) {
		
		Mat4 ma = Mat4::Rotation(a);
		Mat4 mb = Mat4::Rotation(b);

		float e = 0.f;

		for (int row = 0; row < 3; ++row) {
			Vec3 d(
				(ma[row][0] - mb[row][0]), 
				(ma[row][1] - mb[row][1]), 
				(ma[row][2] - mb[row][2]) 
			);

			float m = d.MagnitudeSquared();
			e += m;

			if (m >= max)
				return false;
			if (e >= error)
				return false;
		}

		RAD_ASSERT(e < error);
		error = e;

		return true;
	}

	float VecEq(const Vec3 &a, const Vec3 &b) {
		return (a-b).MagnitudeSquared();
	}

	bool AddVec3(FloatVec &table, const Vec3 &v, int &out, float &outMax, float max) {

		int best = -1;
		
		for (int i = 0; i+3 <= (int)table.size(); i += 3) {
			const Vec3 &a = *((const Vec3*)&table[i]);
			float d = (a-v).MagnitudeSquared();

			if (d < max) {
				max = d;
				best = i;
			}
		}

		if (best != -1) {
			out = best / 3;
			return true;
		}

		if ((table.size()/3) == ska::kEncMask)
			return false;	

		for (size_t i = 0; i < 3; ++i) {
			table.push_back(v[(int)i]);
			outMax = std::max(math::Abs(v[(int)i]), outMax);
		}

		out = (int)(table.size()/3)-1;
		return true;
	}
};

typedef zone_vector<int, ZToolsT>::type IntVec;
struct AnimTables {
	typedef zone_vector<AnimTables, ZToolsT>::type Vec;

	AnimTables() : totalTags(0) {
	}

	IntVec rFrames;
	IntVec sFrames;
	IntVec tFrames;
	IntVec tags;
	int totalTags;
};
	
bool SkaBuilder::Compile(stream::IOutputBuffer &ob) const {
	
	stream::LittleOutputStream os(ob);

	{
		const U32 id  = ska::kSkaTag;
		const U32 ver = ska::kSkaVersion;
		if (!os.Write(id) || !os.Write(ver))
			return false;
	}

	if (!os.Write((U16)m_bones.size()) || !os.Write((U16)m_anims.size()))
		return false;

	for (BoneDef::Vec::const_iterator it = m_bones.begin(); it != m_bones.end(); ++it) {
		if ((*it).name.length > ska::kDNameLen) {
			COut(C_ErrMsgBox) << "ska::DNameLen exceeded, contact a programmer to increase." << std::endl;
			return false;
		}

		char name[ska::kDNameLen+1];
		string::ncpy(name, (*it).name.c_str.get(), ska::kDNameLen+1);
		if (!os.Write(name, ska::kDNameLen+1, 0))
			return false;
	}

	for (BoneDef::Vec::const_iterator it = m_bones.begin(); it != m_bones.end(); ++it) {
		if (!os.Write((*it).parent))
			return false;
	}

	if (m_bones.size()&1) // align?
		if (!os.Write((S16)0xffff))
			return false;

	for (BoneDef::Vec::const_iterator it = m_bones.begin(); it != m_bones.end(); ++it) {
		const BoneDef &b = *it;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 3; ++j)
				if (!os.Write(b.invWorld[i][j]))
					return false;
	}

	Tables t;
	AnimTables::Vec at;
	IntVec prevBoneRFrame;
	IntVec prevBoneSFrame;
	IntVec prevBoneTFrame;

	prevBoneRFrame.resize(m_bones.size());
	prevBoneSFrame.resize(m_bones.size());
	prevBoneTFrame.resize(m_bones.size());
	at.resize(m_anims.size());

	for (Anim::Vec::const_iterator it = m_anims.begin(); it != m_anims.end(); ++it) {
		AnimTables &ct = at[it-m_anims.begin()];
		const Anim &anim = *it;

		ct.rFrames.reserve(anim.frames.size()*m_bones.size());
		ct.sFrames.reserve(anim.frames.size()*m_bones.size());
		ct.tFrames.reserve(anim.frames.size()*m_bones.size());
		ct.tags.reserve(anim.frames.size()*m_bones.size());

		FrameVec::const_iterator last = anim.frames.end();
		for(FrameVec::const_iterator fit = anim.frames.begin(); fit != anim.frames.end(); ++fit) {
			const BoneTMVec &frame = *fit;

			bool tag = false;

			// apply progressive compression.
			// bones deeper in the hierarchy are compressed less
			// with the theory that don't contribute as much to noticable wobble.

			for (size_t boneIdx = 0; boneIdx < m_bones.size(); ++boneIdx) {
				RAD_ASSERT(frame.size() == m_bones.size());
				const BoneTM &tm = frame[boneIdx];
				const BoneTM *prevTM = 0;

				if (last != anim.frames.end())
					prevTM = &((*last)[boneIdx]);

				bool emitR = prevTM == 0;
				bool emitS = prevTM == 0;
				bool emitT = prevTM == 0;

				emitR = emitR || tm.tm.r != prevTM->tm.r;
				emitS = emitS || tm.tm.s != prevTM->tm.s;
				emitT = emitT || tm.tm.t != prevTM->tm.t;

				int tagId = t.AddString(tm.tag);
				ct.tags.push_back(tagId);
				if (!tag && tagId > -1) {
					tag = true;
					++ct.totalTags;
				}

				if (emitR) {
					int idx;
					if (!t.AddRotate(tm.tm.r, anim.boneCompression[boneIdx].quatCompression, idx))
						return false;
					ct.rFrames.push_back(idx);
					prevBoneRFrame[boneIdx] = idx;
				} else {
					ct.rFrames.push_back(prevBoneRFrame[boneIdx]);
				}

				if (emitS) {
					int idx;
					if (!t.AddScale(tm.tm.s, anim.boneCompression[boneIdx].scaleCompression, idx))
						return false;
					ct.sFrames.push_back(idx);
					prevBoneSFrame[boneIdx] = idx;
				} else {
					ct.sFrames.push_back(prevBoneSFrame[boneIdx]);
				}

				if (emitT) {
					int idx;
					if (!t.AddTranslate(tm.tm.t, anim.boneCompression[boneIdx].translateCompression, idx))
						return false;
					ct.tFrames.push_back(idx);
					prevBoneTFrame[boneIdx] = idx;
				} else {
					ct.tFrames.push_back(prevBoneTFrame[boneIdx]);
				}
			}

			last = fit;
		}
	}

	if (!os.Write((U32)(t.rTable.size())) ||
		!os.Write((U32)(t.sTable.size())) ||
		!os.Write((U32)(t.tTable.size()))) {
		return false;
	}
	
	if (!os.Write(t.sAbsMax/std::numeric_limits<S16>::max()))
		return false;
	if (!os.Write(t.tAbsMax/std::numeric_limits<S16>::max()))
		return false;

	// Encode float tables.
	for (size_t i = 0; i < t.rTable.size(); ++i) {
		S16 enc = Tables::QuantFloat(t.rTable[i], 1.0f);
		if (!os.Write(enc))
			return false;
	}

	for (size_t i = 0; i < t.sTable.size(); ++i) {
		S16 enc = Tables::QuantFloat(t.sTable[i], t.sAbsMax);
		if (!os.Write(enc))
			return false;
	}

	for (size_t i = 0; i < t.tTable.size(); ++i) {
		S16 enc = Tables::QuantFloat(t.tTable[i], t.tAbsMax);
		if (!os.Write(enc))
			return false;
	}

	int bytes = 
		(int)(t.rTable.size()*2)+
		(int)(t.sTable.size()*2)+
		(int)(t.tTable.size()*2);

	if (bytes&3) {
		bytes &= 3;
		U8 pad[3] = { 0, 0, 0 };
		if (os.Write(pad, 4-bytes, 0) != (4-bytes))
			return false;
	}

	// Write animations.

	RAD_ASSERT(at.size() == m_anims.size());

	for (size_t i = 0; i < at.size(); ++i) {
		char name[ska::kDNameLen+1];
		string::ncpy(name, m_anims[i].name.c_str.get(), ska::kDNameLen+1);
		if (os.Write(name, ska::kDNameLen+1, 0) != (ska::kDNameLen+1))
			return false;
		if (!os.Write(m_anims[i].distance))
			return false;
		U16 fps = (U16)(floorf(m_anims[i].fps+0.5f));
		if (!os.Write(fps))
			return false;
		if (!os.Write((U16)m_anims[i].frames.size()))
			return false;
		if (!os.Write((U16)at[i].totalTags))
			return false;
		if (!os.Write((U16)0))
			return false; // padd bytes.
	}

	// Write table indexes.
	bytes = 0;
	
	for (size_t i = 0; i < at.size(); ++i) {
		const AnimTables &tables = at[i];

		RAD_ASSERT(tables.rFrames.size() == (m_bones.size()*m_anims[i].frames.size()));
		
		for (IntVec::const_iterator it = tables.rFrames.begin(); it != tables.rFrames.end(); ++it) {
			int i = endian::SwapLittle((*it)&ska::kEncMask);
			if (os.Write(&i, ska::kEncBytes, 0) != ska::kEncBytes)
				return false;
			bytes += ska::kEncBytes;
		}

		RAD_ASSERT(tables.sFrames.size() == (m_bones.size()*m_anims[i].frames.size()));
		
		for (IntVec::const_iterator it = tables.sFrames.begin(); it != tables.sFrames.end(); ++it) {
			int i = endian::SwapLittle((*it)&ska::kEncMask);
			if (os.Write(&i, ska::kEncBytes, 0) != ska::kEncBytes)
				return false;
			bytes += ska::kEncBytes;
		}

		RAD_ASSERT(tables.tFrames.size() == (m_bones.size()*m_anims[i].frames.size()));
		
		for (IntVec::const_iterator it = tables.tFrames.begin(); it != tables.tFrames.end(); ++it) {
			int i = endian::SwapLittle((*it)&ska::kEncMask);
			if (os.Write(&i, ska::kEncBytes, 0) != ska::kEncBytes)
				return false;
			bytes += ska::kEncBytes;
		}

		RAD_ASSERT(tables.tags.size() == (m_bones.size()*m_anims[i].frames.size()));

		// Write DTags

		int boneTagOfs = 0;

		IntVec::const_iterator tagIt = tables.tags.begin();
		for (size_t j = 0; j < m_anims[i].frames.size(); ++j) {
			int numBones = 0;

			for (size_t k = 0; k < m_bones.size(); ++k, ++tagIt) {
				int tagIdx = *tagIt;
				if (tagIdx > -1)
					++numBones;
			}

			if (!numBones)
				continue;

			if (!os.Write((U16)j))
				return false;
			if (!os.Write((U16)numBones))
				return false;
			if (!os.Write((U16)boneTagOfs))
				return false;

			bytes += (int)sizeof(U16) * 3;

			boneTagOfs += numBones * 3;
			if (boneTagOfs > std::numeric_limits<U16>::max()) {
				COut(C_Error) << "SkaBuilder: Bone tag table exceeds 64k!" << std::endl;
				return false;
			}
		}

		if (!os.Write((U16)boneTagOfs))
			return false;
		bytes += (int)sizeof(U16);

		// build bone tag data.
		tagIt = tables.tags.begin();
		for (size_t j = 0; j < m_anims[i].frames.size(); ++j) {
			for (size_t k = 0; k < m_bones.size(); ++k, ++tagIt) {
				int tagIdx = *tagIt;
				if (tagIdx > -1) {
					if (!os.Write((U16)k))
						return false;
					if (!os.Write((U8)tagIdx))
						return false;
					bytes += 3;
				}
			}
		}
	}

	// write string table.
	if (!os.Write((U8)t.strings.size()))
		return false;
	++bytes;

	// compile string indexes.
	int stringIdx = 0;
	for (StringVec::const_iterator it = t.strings.begin(); it != t.strings.end(); ++it) {
		if (!os.Write((U16)stringIdx))
			return false;
		bytes += 2;
		const String &str = *it;
		stringIdx += (int)str.length+1;
		if (stringIdx > std::numeric_limits<U16>::max()) {
			COut(C_Error) << "SkaBuilder: String table exceeds 64k in size!" << std::endl;
			return false;
		}
	}

	// compile strings
	for (StringVec::const_iterator it = t.strings.begin(); it != t.strings.end(); ++it) {
		const String &str = *it;
		if (os.Write(str.c_str.get(), (stream::SPos)(str.length+1), 0) != (stream::SPos)(str.length+1))
			return false;
		bytes += (int)str.length+1;
	}
	
	if (bytes&3) { // padd to 4 byte alignment.
		bytes &= 3;
		U8 pad[3] = { 0, 0, 0 };
		if (os.Write(pad, 4-bytes, 0) != (4-bytes))
			return false;
	}

	return true;
}

void SkaBuilder::SetBones(const BoneDef::Vec &bones) {
	m_bones = bones;
}

void SkaBuilder::BeginAnim(
	const char *name,
	float fps,
	int numFrames,
	bool removeMotion,
	const SkaCompressionMap *compression
) {
	RAD_ASSERT(name);
	RAD_ASSERT(numFrames>0);

	m_anims.resize(m_anims.size()+1);
	Anim &a = m_anims.back();
	a.name = name;
	a.fps = fps;
	a.removeMotion = removeMotion;
	a.frames.reserve((size_t)numFrames);
	a.distance = 0.f;
	a.boneCompression.resize(m_bones.size());

	float compressionLevel = 1.f;
	if (compression) {
		SkaCompressionMap::const_iterator it = compression->find(CStr(name));
		if (it != compression->end())
			compressionLevel = it->second;
	}

	for (size_t i = 0; i < m_bones.size(); ++i) {
		Compression &c = a.boneCompression[i];
		const BoneDef &bone = m_bones[i];

		const float kFactor = math::Pow(0.7f, (float)bone.childDepth);

		c.quatCompression = std::max(
			kQuatCompressionBasis[0],
			kQuatCompressionBasis[std::min(1, bone.childDepth)+1] * kFactor * compressionLevel
		);

		float qsin = sin(c.quatCompression*3.1415926535f/180);
		c.quatCompression = qsin*qsin;

		c.scaleCompression = std::max(
			kScaleCompressionBasis[0],
			kScaleCompressionBasis[1] * kFactor * compressionLevel
		);

		c.translateCompression = std::max(
			kTranslationCompressionBasis[0],
			kTranslationCompressionBasis[1] * kFactor * compressionLevel
		);
	}
}

void SkaBuilder::AddFrame(const BoneTM *bones) {
	RAD_ASSERT(bones);
	Anim &a = m_anims.back();
	if (a.frames.empty())
		a.frames.reserve(256);
	a.frames.resize(a.frames.size()+1);
	a.frames.back().reserve(m_bones.size());
	for (size_t i = 0; i < m_bones.size(); ++i)
		a.frames.back().push_back(bones[i]);
}

void SkaBuilder::EndAnim(int motionBone) {
	if (motionBone < -1)
		return;
	Anim &a = m_anims.back();
	if (motionBone >= (int)a.frames.front().size())
		return;

	const BoneTMVec &start = a.frames.front();
	const BoneTMVec &end = a.frames.back();

	Vec3 d = end[motionBone].tm.t - start[motionBone].tm.t;
	a.distance = d.Magnitude();
}

///////////////////////////////////////////////////////////////////////////////

typedef SceneFile::WeightedNormalTriVert TriVert;
typedef zone_vector<TriVert, ZToolsT>::type TriVertVec;
typedef zone_map<TriVert, int, ZToolsT>::type TriVertMap;

struct SkTriModel {
	typedef boost::shared_ptr<SkTriModel> Ref;
	typedef zone_vector<Ref, ZToolsT>::type Vec;
	typedef zone_vector<Vec4, ZToolsT>::type Vec4Vec;
	typedef zone_vector<Vec3, ZToolsT>::type Vec3Vec;
	typedef zone_vector<Vec2, ZToolsT>::type Vec2Vec;

	struct VertIndex {
		typedef zone_vector<VertIndex, ZToolsT>::type Vec;

		VertIndex(int _numBones, int _idx) : numBones(_numBones), index(_idx) {}

		int numBones;
		int index;
	};

	int mat;
	int totalVerts;
	TriVertVec verts[ska::kBonesPerVert];
	TriVertMap vmap[ska::kBonesPerVert];
	VertIndex::Vec indices;

	Vec4Vec weightedVerts;
	Vec4Vec weightedNormals;
	Vec4Vec weightedTangents;
	Vec2Vec uvs[ska::kMaxUVChannels];
	IntVec sortedIndices;

	static TriVert CleanWeights(const TriVert &v) {
		TriVert z(v);
		SceneFile::BoneWeights w;

		// drop tiny weights
		for (size_t i = 0; i < z.weights.size(); ++i) {
			if (z.weights[i].weight < 0.009999999f)
				continue;
			w.push_back(z.weights[i]);
		}

		while (w.size() > ska::kBonesPerVert) { // drop smallest.
			size_t best = 0;
			float bestw = w[0].weight;

			for (size_t i = 1; i < w.size(); ++i) {
				if (w[i].weight < bestw) {
					bestw = w[i].weight;
					best = i;
				}
			}

			SceneFile::BoneWeights x;
			x.swap(w);

			for (size_t i = 0; i < x.size(); ++i) {
				if (i != best)
					w.push_back(x[i]);
			}
		}

		float total = 0.f;

		for (size_t i = 0; i < w.size(); ++i) {
			total += w[i].weight;
		}

		if (total <= 0.f)
			w.clear(); // bad

		// renormalize
		for (size_t i = 0; i < w.size(); ++i) {
			w[i].weight = w[i].weight / total;
		}

		if (w.empty()) {
			SceneFile::BoneWeight x;
			x.bone = 0;
			x.weight = 1.0f;
			w.push_back(x);
		}

		z.weights = w;
		return z;
	}

	void AddVertex(const TriVert &v) {
		TriVert z = CleanWeights(v);

		RAD_ASSERT(!z.weights.empty());
		RAD_ASSERT(z.weights.size() <= ska::kBonesPerVert);
		int mapIdx = (int)z.weights.size()-1;

		TriVertMap::iterator it = vmap[mapIdx].find(z);
		if (it != vmap[mapIdx].end()) {
#if defined(RAD_OPT_DEBUG)
			RAD_ASSERT(z.weights.size() == it->first.weights.size());
			RAD_ASSERT(z.pos == it->first.pos);
			for (int i = 0; i < SceneFile::kMaxUVChannels; ++i) {
				RAD_ASSERT(z.st[i] == it->first.st[i]);
				RAD_ASSERT(z.tangent[i] == it->first.tangent[i]);
			}
			RAD_ASSERT(z.st[0] == it->first.st[0]);
			for (size_t i = 0; i < z.weights.size(); ++i) {
				RAD_ASSERT(z.weights[i].weight == it->first.weights[i].weight);
				RAD_ASSERT(z.weights[i].bone == it->first.weights[i].bone);
			}
#endif
			indices.push_back(VertIndex(mapIdx, it->second));
			return;
		}

		int ofs = (int)verts[mapIdx].size();
		verts[mapIdx].push_back(z);
		vmap[mapIdx].insert(TriVertMap::value_type(z, ofs));
		indices.push_back(VertIndex(mapIdx, ofs));
	}

	void AddTriangles(const SceneFile::TriModel::Ref &m) {
		for (SceneFile::TriFaceVec::const_iterator it = m->tris.begin(); it != m->tris.end(); ++it) {
			const SceneFile::TriFace &tri = *it;
			if (tri.mat < 0)
				continue;
			if (tri.mat != mat)
				continue;
			for (int i = 0; i < 3; ++i)
				AddVertex(TriVert(m->verts[tri.v[i]]));
		}
	}

	void Compile() {
		// figure out how many final verts we'll have.
		int numWeightedVerts = 0;
		totalVerts = 0;

		for (int i = 0; i < ska::kBonesPerVert; ++i) {
			int c = (int)vmap[i].size();

			totalVerts += c;
			numWeightedVerts += c*i;
		}		

		sortedIndices.reserve(indices.size());
		weightedVerts.reserve(numWeightedVerts);
		weightedNormals.reserve(numWeightedVerts);
		weightedTangents.reserve(numWeightedVerts);

		for (int i = 0; i < ska::kMaxUVChannels; ++i) {
			uvs[i].reserve(totalVerts);
		}

		int vertIndex = 0;
		IntVec idxRemap[ska::kBonesPerVert];

		for (int i = 0; i < ska::kBonesPerVert; ++i)
			idxRemap[i].reserve(verts[i].size());

		// emit vertices premultiplied by bone weights
		for (int i = 0; i < ska::kBonesPerVert; ++i) {
			const TriVertVec &vec = verts[i];
			for (TriVertVec::const_iterator it = vec.begin(); it != vec.end(); ++it) {
				const TriVert &v = *it;
				RAD_ASSERT((i+1) == (int)v.weights.size());

				idxRemap[i].push_back(vertIndex++);

				for (int k = 0; k < ska::kMaxUVChannels; ++k) {
					uvs[k].push_back(v.st[k]);
				}

				for (int k = 0; k <= i; ++k) {
					Vec4 z(v.pos * v.weights[k].weight, v.weights[k].weight);
					weightedVerts.push_back(z);
				}

				for (int k = 0; k <= i; ++k) {
					Vec4 z(v.normal * v.weights[k].weight, 1.f);
					weightedNormals.push_back(z);
				}

				for (int k = 0; k <= i; ++k) {
					Vec4 z(Vec3(v.tangent[0]) * v.weights[k].weight, v.tangent[0][3]);
					weightedTangents.push_back(z);
				}
			}
		}

		for (VertIndex::Vec::const_iterator it = indices.begin(); it != indices.end(); ++it) {
			const VertIndex &idx = *it;
			sortedIndices.push_back(idxRemap[idx.numBones][idx.index]);
		}
	}
};

bool CompileCPUSkmData(const char *name, const SceneFile &map, int trimodel, SkmData &sk, const ska::DSka &ska) {
	SkTriModel::Ref m(new (ZTools) SkTriModel());
	SkTriModel::Vec models;

	SceneFile::Entity::Ref e = map.worldspawn;
	SceneFile::TriModel::Ref r = e->models[trimodel];

	if (r->skel < 0)
		return false;

	const SceneFile::Skel &skel = *e->skels[r->skel].get();

	for (int i = 0; i < (int)map.mats.size(); ++i){
		m->mat = i;
		m->AddTriangles(r);
		if (!m->indices.empty()) {
			m->Compile();
			models.push_back(m);
			m.reset(new (ZTools) SkTriModel());
		}
	}

	std::vector<int> remap;
	remap.reserve(skel.bones.size());

	// build bone remap table
	for (size_t i = 0; i < skel.bones.size(); ++i) {
		U16 k;
		for (k = 0; k < ska.numBones; ++k) {
			if (!string::cmp(skel.bones[i].name.c_str.get(), &ska.boneNames[k*(ska::kDNameLen+1)]))
				break;
		}

		remap.push_back((k==ska.numBones) ? 0 : (int)k);
	}

	m.reset();

	if (models.empty())
		return true;

	{ // file 1: non persistant data (material names, texCoords, tris)
		stream::DynamicMemOutputBuffer ob(ska::ZSka);
		stream::LittleOutputStream os(ob);

		if (!os.Write((U32)ska::kSkmxTag) || !os.Write((U32)ska::kSkmVersion))
			return false;

		// bounds
		for (int i = 0; i < 3; ++i) {
			if (!os.Write((float)r->bounds.Mins()[i]))
				return false;
		}
		for (int i = 0; i < 3; ++i) {
			if (!os.Write((float)r->bounds.Maxs()[i]))
				return false;
		}

		if (!os.Write((U16)models.size()))
			return false;
		if (!os.Write((U16)0))
			return false;

		for (SkTriModel::Vec::const_iterator it = models.begin(); it != models.end(); ++it) {
			const SkTriModel::Ref &m = *it;

			if (m->totalVerts > std::numeric_limits<U16>::max())
				return false;
			if (m->sortedIndices.size()/3 > std::numeric_limits<U16>::max())
				return false;

			if (!os.Write((U16)m->totalVerts))
				return false;

			for (int i = 0; i < ska::kBonesPerVert; ++i) {
				if (m->verts[i].size() > std::numeric_limits<U16>::max())
					return false;
				if (!os.Write((U16)m->verts[i].size()))
					return false;
			}

			if (!os.Write((U16)(m->sortedIndices.size()/3)))
				return false;

			if (!os.Write((U16)(r->numChannels)))
				return false;

			if ((ska::kBonesPerVert+3)&1) // align?
				if (!os.Write((U16)0))
					return false;

			if (map.mats[m->mat].name.length > ska::kDNameLen) {
				COut(C_ErrMsgBox) << "ska::DNameLen exceeded, contact a programmer to increase." << std::endl;
				return false;
			}

			char name[ska::kDNameLen+1];
			string::ncpy(name, map.mats[m->mat].name.c_str.get(), ska::kDNameLen+1);
			if (!os.Write(name, ska::kDNameLen+1, 0))
				return false;

			// texcoords
			for (int i = 0; i < r->numChannels; ++i) {
				const SkTriModel::Vec2Vec &uvs = m->uvs[i];
				for (SkTriModel::Vec2Vec::const_iterator it = uvs.begin(); it != uvs.end(); ++it) {
					const Vec2 &v = *it;
					for (int k = 0; k < 2; ++k) {
						if (!os.Write(v[k]))
							return false;
					}
				}
			}

			// tris (indices)
			for (IntVec::const_iterator it = m->sortedIndices.begin(); it != m->sortedIndices.end(); ++it) {
				if (!os.Write((U16)*it))
					return false;
			}

			if (m->sortedIndices.size()&1) { // align
				RAD_ASSERT((m->sortedIndices.size()/3)&1);
				if (!os.Write((U16)0))
					return false;
			}
		}

		sk.skmData[0] = ob.OutputBuffer().Ptr();
		sk.skmSize[0] = (AddrSize)ob.OutPos();
		ob.OutputBuffer().Set(0, 0);
		sk.skmData[0] = zone_realloc(ska::ZSka, sk.skmData[0], sk.skmSize[0]);
	}
	{ // file 2: persisted data (prescaled vertices, prescaled normals, bone indices)
		stream::DynamicMemOutputBuffer ob(ska::ZSka, SIMDDriver::kAlignment);
		stream::LittleOutputStream os(ob);

		if (!os.Write((U32)ska::kSkmpTag) || !os.Write((U32)ska::kSkmVersion))
			return false;

		int bytes = 8;

		for (SkTriModel::Vec::const_iterator it = models.begin(); it != models.end(); ++it) {
			const SkTriModel::Ref &m = *it;

			if (bytes&(SIMDDriver::kAlignment-1))  { // SIMD padd
				U8 padd[(SIMDDriver::kAlignment-1)];
				if (os.Write(padd, SIMDDriver::kAlignment-(bytes&(SIMDDriver::kAlignment-1)), 0) != (SIMDDriver::kAlignment-(bytes&(SIMDDriver::kAlignment-1))))
					return false;
				bytes = Align(bytes, SIMDDriver::kAlignment);
			}

			RAD_ASSERT(m->weightedVerts.size() == m->weightedNormals.size());
			RAD_ASSERT(m->weightedVerts.size() == m->weightedTangents.size());

			int ofs = 0;
			for (int i = 0; i < ska::kBonesPerVert; ++i) {
				const int kBoneVertCount = (int)m->verts[i].size();
				
				for (int z = 0; z < kBoneVertCount; ++z) {
					for (int k = 0; k <= i; ++k) {
						const Vec4 *v = &m->weightedVerts[k+ofs];

						if (!os.Write((*v)[0]) ||
							!os.Write((*v)[1]) ||
							!os.Write((*v)[2]) ||
							!os.Write((*v)[3])) {
							return false;
						}

						bytes += 16;
					}

					for (int k = 0; k <= i; ++k) {
						const Vec4 *v = &m->weightedNormals[k+ofs];

						if (!os.Write((*v)[0]) ||
							!os.Write((*v)[1]) ||
							!os.Write((*v)[2]) ||
							!os.Write((*v)[3])) {
							return false;
						}

						bytes += 16;
					}

					for (int k = 0; k <= i; ++k) {
						const Vec4 *v = &m->weightedTangents[k+ofs];

						if (!os.Write((*v)[0]) ||
							!os.Write((*v)[1]) ||
							!os.Write((*v)[2]) ||
							!os.Write((*v)[3])) {
							return false;
						}

						bytes += 16;
					}
					
					ofs += (i+1);
				}
			}

			// bone indices

			for (int i = 0; i < ska::kBonesPerVert; ++i) {
				if (bytes&(SIMDDriver::kAlignment-1)) { // SIMD padd
					U8 padd[(SIMDDriver::kAlignment-1)];
					if (os.Write(padd, SIMDDriver::kAlignment-(bytes&(SIMDDriver::kAlignment-1)), 0) != (SIMDDriver::kAlignment-(bytes&(SIMDDriver::kAlignment-1))))
						return false;
					bytes = Align(bytes, SIMDDriver::kAlignment);
				}

				const TriVertVec &verts = m->verts[i];
				for (TriVertVec::const_iterator it = verts.begin(); it != verts.end(); ++it) {
					const TriVert &v = *it;
					for (int k = 0; k <= i; ++k) {
						RAD_ASSERT(k <= (int)v.weights.size());
						int b = remap[v.weights[k].bone];
						if (!os.Write((U16)b))
							return false;
						bytes += 2;
					}
				}
			}
		}

		sk.skmData[1] = ob.OutputBuffer().Ptr();
		sk.skmSize[1] = (AddrSize)ob.OutPos();
		ob.OutputBuffer().Set(0, 0);
		sk.skmData[1] = zone_realloc(
			ska::ZSka, 
			sk.skmData[1], 
			sk.skmSize[1],
			0,
			SIMDDriver::kAlignment
		);
	}

	RAD_VERIFY(sk.dskm.Parse(sk.skmData, sk.skmSize, ska::kSkinType_CPU) == pkg::SR_Success);
	return true;
}

}

///////////////////////////////////////////////////////////////////////////////

RADENG_API SkaData::Ref RADENG_CALL CompileSkaData(
	const char *name, 
	const SceneFileVec &anims,
	int trimodel,
	const SkaCompressionMap *compression
) {
	SkaData::Ref sk(new (ZTools) SkaData());

	SkaBuilder b;
	if (!b.Compile(name, anims, trimodel, compression, *sk))
		return SkaData::Ref();

	SizeBuffer memSize;
	FormatSize(memSize, sk->skaSize);
	COut(C_Info) << "CompileSkaData(" << memSize << ")" << std::endl;
	
	return sk;
}

RADENG_API SkaData::Ref RADENG_CALL CompileSkaData(
	const char *name, 
	const SceneFile &anims,
	int trimodel,
	const SkaCompressionMap *compression
) {
	SkaData::Ref sk(new (ZTools) SkaData());

	SkaBuilder b;
	if (!b.Compile(name, anims, trimodel, compression, *sk))
		return SkaData::Ref();
	
	SizeBuffer memSize;
	FormatSize(memSize, sk->skaSize);
	COut(C_Info) << "CompileSkaData(" << memSize << ")" << std::endl;

	return sk;
}

RADENG_API SkmData::Ref RADENG_CALL CompileSkmData(
	const char *name, 
	const SceneFile &map, 
	int trimodel,
	ska::SkinType skinType,
	const ska::DSka &ska
) {
	SkmData::Ref sk(new (ZTools) SkmData());
	sk->skinType = skinType;

	if (!CompileCPUSkmData(name, map, trimodel, *sk, ska))
		return SkmData::Ref();

	SizeBuffer memSize[2];
	FormatSize(memSize[0], sk->skmSize[0]);
	FormatSize(memSize[1], sk->skmSize[1]);
	COut(C_Info) << "CompileSkmData(" << memSize[0] << ", " << memSize[1] << ")" << std::endl;

	return sk;
}

///////////////////////////////////////////////////////////////////////////////

SkaData::SkaData() :
skaData(0),
skaSize(0) {
	dska.Clear();
}

SkaData::~SkaData() {
	if (skaData)
		zone_free(skaData);
}

SkmData::SkmData() :
skinType(ska::kSkinType_CPU) {
	skmData[0] = skmData[1] = 0;
	skmSize[0] = skmSize[1] = 0;
	dskm.Clear();
}

SkmData::~SkmData() {
	if (skmData[0])
		zone_free(skmData[0]);
	if (skmData[1])
		zone_free(skmData[1]);
}

} // tools

#endif // RAD_OPT_TOOLS

