// SkAnim.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Skeletal Animation
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "SkAnimDef.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Event.h>
#include <Runtime/TimeDef.h>
#include <Runtime/PushPack.h>

class Engine;

namespace ska {

struct AnimTagEventData {
	Ska* ska;
	const Animation* anim;
	String tag;
	int bone;
};

struct AnimStateEventData {
	Ska *ska;
	const Animation *anim;
};

class RADENG_CLASS Notify {
public:
	typedef NotifyRef Ref;

	enum MaskFlags {
		RAD_FLAG(kMaskFlag_Tags),
		RAD_FLAG(kMaskFlag_EndFrame),
		RAD_FLAG(kMaskFlag_Finished),
		kMaskFlag_All = kMaskFlag_Tags|kMaskFlag_EndFrame|kMaskFlag_Finished,
		kMaskFlag_None = 0
	};

	Notify() : m_masked(0) {}

	// If masked, will not emit events.
	RAD_DECLARE_PROPERTY(Notify, masked, int, int);

	void EmitTag(const AnimTagEventData &data) {
		if (!(m_masked&kMaskFlag_Tags))
			OnTag(data);
	}

	void EmitEndFrame(const AnimStateEventData &data) {
		if (!(m_masked&kMaskFlag_EndFrame))
			OnEndFrame(data);
	}

	void EmitFinish(const AnimStateEventData &data, bool masked) {
		if (!(m_masked&kMaskFlag_Finished))
			OnFinish(data, masked);
	}

protected:
	virtual void OnTag(const AnimTagEventData &data) = 0;
	virtual void OnEndFrame(const AnimStateEventData &data) = 0;
	virtual void OnFinish(const AnimStateEventData &data, bool masked) = 0;

private:

	RAD_DECLARE_GET(masked, int) { 
		return m_masked; 
	}

	RAD_DECLARE_SET(masked, int) { 
		m_masked = value; 
	}

	int m_masked;
};

struct BoneTM {
	typedef boost::shared_array<BoneTM> Ref;
	Quat r;
	Vec3 s;
	Vec3 t;
};

// A model contains its animation data in a table.
// There is a seperate table for rotation, scale
// and translation.
//
// Each entry in the rotation table consists of 4 floats (quat) (x y z w)
// Each entry in the scale and translation tables are 3 floats (vec3) (x y z)
//
// Animations contain indexes for each frame for each bone. If a model has N
// bones then the first N shorts in each table are the indexes for frame 0 for
// all N bones. Indexes from N*2 to N*2+N are the indexes for N bones for
// frame 1, etc.

struct DSkTag {
	U16 frame; // frame that tag should be emitted on.
	U16 numBones; // number of bones with tags on this frame.
	U16 tagOfs; // offset into boneTags
};

struct DSkAnim {
	typedef zone_vector<DSkAnim, ZSkaT>::type Vec;
	typedef zone_vector<int, ZSkaT>::type IntVec;

	const char *name;
	float distance;
	U16 fps;
	U16 numFrames;
	U16 numTags;
	const U8 *rFrames;
	const U8 *sFrames;
	const U8 *tFrames;
	const DSkTag *tags;
	const U8 *boneTags; // (U16) bone index, + (U8) string index (3 bytes per bone in this field).
};

struct RADENG_CLASS DSka {
	U16 numBones;

	const char *boneNames;
	const S16 *boneParents;
	const float *invWorld; // 4x3 column-major matrix per bone (12 floats)
	
	float sDecodeMag;
	float tDecodeMag;

	// tables
	const S16 *rTable; // rotation
	const S16 *sTable; // scale
	const S16 *tTable; // translate
	const U16 *stringOfs;
	const char *strings;

	// animations
	DSkAnim::Vec anims;

	void Clear();
	int Parse(const void *data, AddrSize len);
};

struct DSkMesh {
	typedef zone_vector<DSkMesh, ZSkaT>::type Vec;
	
	enum {
		kNumVertexFloats = 12
	};

	U16 totalVerts;
	U16 numVerts[kBonesPerVert]; // 1,2,3,4 bone counts
	U16 numTris;
	U16 numChannels;

	const char *material;

	const float *verts; // prescaled by weights, N copies per bone
						// this is a packed field:
						// floats:
						// 0-3   -> vertex
						// 4-7   -> normal
						// 8-11  -> tangent

	const float *texCoords; // 2 floats * numChannels

	// see r::SkMesh::BonesPerVert (Renderer/SkMesh.h)

	const U16 *bones[kBonesPerVert]; // N per vert, depending on how many bones the vertex has.

	const U16 *indices;
};

struct RADENG_CLASS DSkm {

	BBox bounds;
	DSkMesh::Vec meshes;

	void Clear();

	// if SkinType == kSkinType_CPU then:
	// data[0] == non-persisted data: material names, texCoords, tris
	// data[1] == persisted data: vertices, normals, tangents, weights, bone indices
	int Parse(const void * const *data, const AddrSize *len, SkinType type);
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS Animation {
public:

	typedef zone_map<String, Animation*, ZSkaT>::type Map;

	~Animation();

	RAD_DECLARE_READONLY_PROPERTY(Animation, name, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Animation, numFrames, int);
	RAD_DECLARE_READONLY_PROPERTY(Animation, fps, float);
	RAD_DECLARE_READONLY_PROPERTY(Animation, distance, float);
	RAD_DECLARE_READONLY_PROPERTY(Animation, length, float); // in seconds
	RAD_DECLARE_READONLY_PROPERTY(Animation, ska, Ska*);

	void BlendFrames(
		int frameSrc,
		int frameDst,
		float blend,
		BoneTM *out,
		int firstBone,
		int numBones
	) const;

	void EmitTags(
		int frame,
		int numFrames,
		int firstBone,
		int numBones,
		const Notify::Ref &notify
	) const;

private:

	friend class Ska;

	Animation(Ska &ska, const DSkAnim &anim);

	RAD_DECLARE_GET(name, const char *);
	RAD_DECLARE_GET(numFrames, int);
	RAD_DECLARE_GET(fps, float);
	RAD_DECLARE_GET(distance, float);
	RAD_DECLARE_GET(length, float);
	RAD_DECLARE_GET(ska, Ska*) { return m_ska; }

	Ska *m_ska;
	const DSkAnim *m_danim;
};

///////////////////////////////////////////////////////////////////////////////

struct ColumnMajorTag {};
struct RowMajorTag {};

// skeletal model
class RADENG_CLASS Ska {
	RAD_EVENT_CLASS(EventNoAccess)
public:

	typedef SkaRef Ref;
	typedef SkaWRef WRef;

	typedef Event<AnimTagEventData, EventNoAccess> AnimTagEvent;
	typedef Event<AnimStateEventData, EventNoAccess> AnimStateEvent;

	Ska(const DSka &dska);
	~Ska();

	int FindBone(const char *name) const;
	int ParentOf(int bone) const;
	Mat4 BoneWorldMat(int bone) const;

	const float *BoneTMs(const ColumnMajorTag&) const;
	const float *BoneTMs(const RowMajorTag&) const;
	
	// Ticks the model animations, composes
	// the bone array, and emits animation tags.
	void Tick(
		float dt, 
		float distance,
		bool advance, 
		bool emitTags, 
		const Mat4 &root
	);

	RAD_DECLARE_READONLY_PROPERTY(Ska, numBones, int);
	RAD_DECLARE_READONLY_PROPERTY(Ska, anims, const Animation::Map*);
	RAD_DECLARE_READONLY_PROPERTY(Ska, boneFrame, int); //++ per Tick()
	RAD_DECLARE_READONLY_PROPERTY(Ska, deltaMotion, const BoneTM*);
	RAD_DECLARE_READONLY_PROPERTY(Ska, absMotion, const BoneTM*);
	RAD_DECLARE_PROPERTY(Ska, root, const ControllerRef&, const ControllerRef&);

	AnimTagEvent OnTag;
	AnimStateEvent OnFinished;
	AnimStateEvent OnMasked;
	AnimStateEvent OnEndFrame;

private:

	RAD_DECLARE_GET(numBones, int) { 
		return m_dska->numBones; 
	}

	RAD_DECLARE_GET(anims, const Animation::Map*) { 
		return &m_anims; 
	}

	RAD_DECLARE_GET(root, const ControllerRef&) { 
		return m_root; 
	}

	RAD_DECLARE_SET(root, const ControllerRef&) { 
		m_root = value; 
	}

	RAD_DECLARE_GET(boneFrame, int) { 
		return m_boneFrame; 
	}

	RAD_DECLARE_GET(deltaMotion, const BoneTM*) {
		return &m_deltaMotion;
	}

	RAD_DECLARE_GET(absMotion, const BoneTM*) {
		return &m_absMotion;
	}

	friend class Animation;
	friend class Controller;
	void Init();
	
	Animation::Map m_anims;
	ControllerRef m_root;
	BoneTM m_deltaMotion;
	BoneTM m_absMotion;
	float *m_boneFloats;
	float *m_cmBoneFloats;
	float *m_worldBones;
	BoneTM *m_boneTMs;
	const DSka *m_dska;
	int m_boneFrame;
	bool m_cmBoneTMsDirty;
	bool m_ident;
};

} // ska

#include <Runtime/PopPack.h>

