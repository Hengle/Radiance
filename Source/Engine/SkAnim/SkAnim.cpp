// SkAnim.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Skeletal Animation
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SkAnim.h"
#include "SkControllers.h"
#include "../Engine.h"
#include "../Packages/PackagesDef.h"
#include <Runtime/StringBase.h>
#include <Runtime/Time.h>
#include <Runtime/Endian.h>
#include <Runtime/Base/SIMD.h>

// NOTE: Need to add byte-swapping...

namespace ska {

BOOST_STATIC_ASSERT(sizeof(Mat4)==(16*sizeof(float)));
BOOST_STATIC_ASSERT(sizeof(DTag)==6);

namespace details {

Quat Slerp(const Quat &from, const Quat &to, float t) {
	float to1[4];
	double omega, cosom, sinom, scale0, scale1;
	
	// calc cosine
	cosom = (from.X() * to.X()) + (from.Y() * to.Y()) + (from.Z() * to.Z()) + (from.W() * to.W());
	
	// adjust signs (if necessary)
	if (cosom < 0.0f) {
		cosom = -cosom;
		to1[0] = -to.X();
		to1[1] = -to.Y();
		to1[2] = -to.Z();
		to1[3] = -to.W();
	} else {
		to1[0] = to.X();
		to1[1] = to.Y();
		to1[2] = to.Z();
		to1[3] = to.W();
	}
	
	// calculate coefficients
	
	if ((1.0 - cosom) > 0.0006) {
		// standard case (slerp)
		omega = acosf((float)cosom);
		sinom = sinf((float)omega);
		scale0 = sinf((float)((1.0f - t) * omega)) / sinom;
		scale1 = sinf((float)(t * omega)) / sinom;
	} else {        
		// "from" and "to" quaternions are very close 
		//  ... so we can do a linear interpolation
		scale0 = 1.0 - t;
		scale1 = t;
	}
	
	// calculate final values
	to1[0] = (float)(scale0 * from.X() + scale1 * to1[0]);
	to1[1] = (float)(scale0 * from.Y() + scale1 * to1[1]);
	to1[2] = (float)(scale0 * from.Z() + scale1 * to1[2]);
	to1[3] = (float)(scale0 * from.W() + scale1 * to1[3]);

	return Quat(to1[0], to1[1], to1[2], to1[3]);
}

void BlendBones(BoneTM *out, const BoneTM *src, const BoneTM *dst, float weight, int first, int num) {
	if (weight < 0.01f) {
		if (out != src)
			memcpy(out+first, src+first, sizeof(BoneTM)*num);
		return;
	}

	if (weight > 0.99f) {
		if (out != dst)
			memcpy(out+first, dst+first, sizeof(BoneTM)*num);
		return;
	}

	for (int i = 0; i < num; ++i) {
		BoneTM &tm = out[i+first];
		const BoneTM &x = src[i+first];
		const BoneTM &y = dst[i+first];

		tm.r = Slerp(x.r, y.r, weight);
		tm.s = math::Lerp(x.s, y.s, weight);
		tm.t = math::Lerp(x.t, y.t, weight);
	}
}

void IdentBones(BoneTM *out, int first, int num) {
	for (int i = 0; i < num; ++i) {
		BoneTM &z = out[i+first];
		z.r = Quat::Identity;
		z.s = Vec3(1, 1, 1);
		z.t = Vec3::Zero;
	}
}

#define MA(m, r, c) m[r*3+c]

inline void Transform3x3(float *out, const float *mat, const float *vec) {
	out[0] = 
		MA(mat, 0, 0) * vec[0] +
		MA(mat, 1, 0) * vec[1] +
		MA(mat, 2, 0) * vec[2];
	out[1] = 
		MA(mat, 0, 1) * vec[0] +
		MA(mat, 1, 1) * vec[1] +
		MA(mat, 2, 1) * vec[2];
	out[2] =
		MA(mat, 0, 2) * vec[0] +
		MA(mat, 1, 2) * vec[1] +
		MA(mat, 2, 2) * vec[2];
}

// out = a * b
inline Mat4 MulMat4x3(const Mat4 &a, const Mat4 &b) {
	return Mat4(
		Vec4(b.Transform3X3(static_cast<const Vec3&>(a.Row(0))), 0.f),
		Vec4(b.Transform3X3(static_cast<const Vec3&>(a.Row(1))), 0.f),
		Vec4(b.Transform3X3(static_cast<const Vec3&>(a.Row(2))), 0.f),
		Vec4(b.Transform3X3(static_cast<const Vec3&>(a.Row(3))) + static_cast<const Vec3&>(b.Row(3)), 1.f)
	);
}

void MulMat4x3(float *out, const float *a, const float *b) {
	Transform3x3(
		&MA(out, 0, 0),
		b,
		&MA(a, 0, 0)
	);
	Transform3x3(
		&MA(out, 1, 0),
		b,
		&MA(a, 1, 0)
	);
	Transform3x3(
		&MA(out, 2, 0),
		b,
		&MA(a, 2, 0)
	);
	Transform3x3(
		&MA(out, 3, 0),
		b,
		&MA(a, 3, 0)
	);

	MA(out, 3, 0) += MA(b, 3, 0);
	MA(out, 3, 1) += MA(b, 3, 1);
	MA(out, 3, 2) += MA(b, 3, 2);
}

// Muls a 4x3 by a 4x4 (which is treated like a 4x3) = (a * b)
void MulMat4x3(float *out, const float *a, const Mat4 &b) {
	*reinterpret_cast<Vec3*>(&MA(out, 0, 0)) =
		b.Transform3X3(*reinterpret_cast<const Vec3*>(&MA(a, 0, 0)));
	*reinterpret_cast<Vec3*>(&MA(out, 1, 0)) =
		b.Transform3X3(*reinterpret_cast<const Vec3*>(&MA(a, 1, 0)));
	*reinterpret_cast<Vec3*>(&MA(out, 2, 0)) =
		b.Transform3X3(*reinterpret_cast<const Vec3*>(&MA(a, 2, 0)));
	*reinterpret_cast<Vec3*>(&MA(out, 3, 0)) =
		b.Transform3X3(*reinterpret_cast<const Vec3*>(&MA(a, 3, 0)));

	MA(out, 3, 0) += b[3][0];
	MA(out, 3, 1) += b[3][1];
	MA(out, 3, 2) += b[3][2];
}

void MulMat4x3(float *out, const Mat4 &a, const float *b) {
	Transform3x3(
		&MA(out, 0, 0),
		b,
		reinterpret_cast<const float*>(&a.Row(0)[0])
	);
	Transform3x3(
		&MA(out, 1, 0),
		b,
		reinterpret_cast<const float*>(&a.Row(1)[0])
	);
	Transform3x3(
		&MA(out, 2, 0),
		b,
		reinterpret_cast<const float*>(&a.Row(2)[0])
	);
	Transform3x3(
		&MA(out, 3, 0),
		b,
		reinterpret_cast<const float*>(&a.Row(3)[0])
	);

	MA(out, 3, 0) += MA(b, 3, 0);
	MA(out, 3, 1) += MA(b, 3, 1);
	MA(out, 3, 2) += MA(b, 3, 2);
}

inline void StoreMat4x3(float *out, const Mat4 &m) {
	*reinterpret_cast<Vec3*>(&MA(out, 0, 0)) = m.Row(0);
	*reinterpret_cast<Vec3*>(&MA(out, 1, 0)) = m.Row(1);
	*reinterpret_cast<Vec3*>(&MA(out, 2, 0)) = m.Row(2);
	*reinterpret_cast<Vec3*>(&MA(out, 3, 0)) = m.Row(3);
}

inline void MulMat4x3(float *out, const Mat4 &a, const Mat4 &b) {
	Mat4 r = MulMat4x3(a, b);
	StoreMat4x3(out, r);
}

inline void Mat4x3Ident(float *out) {
	memset(out, 0, sizeof(float)*SIMDDriver::kNumBoneFloats);

	if (SIMDDriver::kNumBoneFloats == 16) {
		out[0*4+0] = 1.f;
		out[1*4+1] = 1.f;
		out[2*4+2] = 1.f;
		out[3*4+3] = 1.f;
	} else {
		MA(out, 0, 0) = 1.f;
		MA(out, 1, 1) = 1.f;
		MA(out, 2, 2) = 1.f;
	}
}

inline void StoreMat4x3(float *out, const float *src) {
	for (int i = 0; i < 12; ++i)
		out[i] = src[i];
}

inline Mat4 Mat4From4x3(const float *src) {
	return Mat4(
		Vec4(*reinterpret_cast<const Vec3*>(&MA(src, 0, 0)), 0.f),
		Vec4(*reinterpret_cast<const Vec3*>(&MA(src, 1, 0)), 0.f),
		Vec4(*reinterpret_cast<const Vec3*>(&MA(src, 2, 0)), 0.f),
		Vec4(*reinterpret_cast<const Vec3*>(&MA(src, 3, 0)), 1.f)
	);
}

inline bool CompareMatrices(const float *a, const Mat4 &b) {
	for (int r = 0; r < 4; ++r)
		for (int c = 0; c < 3; ++c)
			if (fabs(MA(a, r, c)-b[r][c]) > 0.0001f)
				return false;
	return true;
}

}

///////////////////////////////////////////////////////////////////////////////

Animation::Animation(Ska &ska, const DAnim &anim) :
m_ska(&ska),
m_danim(&anim) {
}

Animation::~Animation() {
}

const char *Animation::RAD_IMPLEMENT_GET(name) {
	return m_danim->name;
}

int Animation::RAD_IMPLEMENT_GET(numFrames) {
	return m_danim->numFrames;
}

float Animation::RAD_IMPLEMENT_GET(fps) {
	return m_danim->fps;
}

float Animation::RAD_IMPLEMENT_GET(distance) {
	return m_danim->distance;
}

float Animation::RAD_IMPLEMENT_GET(length) {
	return ((float)m_danim->numFrames)/((float)m_danim->fps);
}

namespace {

inline int DecodeI(const U8 *src, int ofs) {
	return endian::SwapLittle(*reinterpret_cast<const int*>(src+ofs*kEncBytes))&kEncMask;
}

inline Quat DecodeQ(const S16 *table, int idx) {
	static const float s_decodeMag = 1.0f / std::numeric_limits<S16>::max();

	table += idx*4;

	return Quat(
		table[0]*s_decodeMag,
		table[1]*s_decodeMag,
		table[2]*s_decodeMag,
		table[3]*s_decodeMag
	);
}

inline Vec3 DecodeV(const S16 *table, int idx, float decodeMag) {
	table += idx*3;

	return Vec3(
		table[0]*decodeMag,
		table[1]*decodeMag,
		table[2]*decodeMag
	);
}

} // namespace

void Animation::BlendFrames(int frameSrc, int frameDst, float blend, BoneTM *out, int firstBone, int numBones) const {
	const DSka &dska = *m_ska->m_dska;

	RAD_ASSERT(frameSrc < m_danim->numFrames);
	RAD_ASSERT(frameDst < m_danim->numFrames);

	const S16 *rTable = dska.rTable;
	const S16 *sTable = dska.sTable;
	const S16 *tTable = dska.tTable;
	const U8 *srcRFrames = m_danim->rFrames+((frameSrc*(int)dska.numBones)+firstBone)*kEncBytes;
	const U8 *srcSFrames = m_danim->sFrames+((frameSrc*(int)dska.numBones)+firstBone)*kEncBytes;
	const U8 *srcTFrames = m_danim->tFrames+((frameSrc*(int)dska.numBones)+firstBone)*kEncBytes;
	const U8 *dstRFrames = m_danim->rFrames+((frameDst*(int)dska.numBones)+firstBone)*kEncBytes;
	const U8 *dstSFrames = m_danim->sFrames+((frameDst*(int)dska.numBones)+firstBone)*kEncBytes;
	const U8 *dstTFrames = m_danim->tFrames+((frameDst*(int)dska.numBones)+firstBone)*kEncBytes;

	if (blend < 0.01f) {
		for (int i = 0; i < numBones; ++i) {
			BoneTM &tm = out[i];
			tm.r = DecodeQ(rTable, DecodeI(srcRFrames, i));
			tm.s = DecodeV(sTable, DecodeI(srcSFrames, i), dska.sDecodeMag);
			tm.t = DecodeV(tTable, DecodeI(srcTFrames, i), dska.tDecodeMag);
		}
	} else if (blend > 0.99f) {
		for (int i = 0; i < numBones; ++i) {
			BoneTM &tm = out[i];
			tm.r = DecodeQ(rTable, DecodeI(dstRFrames, i));
			tm.s = DecodeV(sTable, DecodeI(dstSFrames, i), dska.sDecodeMag);
			tm.t = DecodeV(tTable, DecodeI(dstTFrames, i), dska.tDecodeMag);
		}
	} else {
		BoneTM x, y;

		for (int i = 0; i < numBones; ++i) {
			BoneTM &tm = out[i];

			x.r = DecodeQ(rTable, DecodeI(srcRFrames, i));
			x.s = DecodeV(sTable, DecodeI(srcSFrames, i), dska.sDecodeMag);
			x.t = DecodeV(tTable, DecodeI(srcTFrames, i), dska.tDecodeMag);

			y.r = DecodeQ(rTable, DecodeI(dstRFrames, i));
			y.s = DecodeV(sTable, DecodeI(dstSFrames, i), dska.sDecodeMag);
			y.t = DecodeV(tTable, DecodeI(dstTFrames, i), dska.tDecodeMag);

			tm.r = details::Slerp(x.r, y.r, blend);
			tm.s = math::Lerp(x.s, y.s, blend);
			tm.t = math::Lerp(x.t, y.t, blend);
		}
	}
}

void Animation::EmitTags(
	int frame,
	int numFrames,
	int firstBone,
	int numBones,
	const Notify::Ref &notify
) const {
	RAD_ASSERT(frame+numFrames <= (int)m_danim->numFrames);
	RAD_ASSERT(firstBone+numBones <= m_ska->numBones);
	
	const DSka *dska = m_ska->m_dska;
	
	if (!m_danim->tags)
		return;

	RAD_ASSERT(m_danim->boneTags);

	AnimTagEventData tag;
	tag.ska = m_ska;
	tag.anim = this;

	int numTags = (int)m_danim->numTags;
	const DTag *dtag = m_danim->tags;

	while (frame > (int)dtag->frame) {
		++dtag;
		if (--numTags < 1)
			break;
	}

	if (numTags < 1)
		return;

	int lastFrame = frame + numFrames - 1;

	while (numTags--) {
		int f = (int)dtag->frame;
		if (f > lastFrame)
			break; // no more tags here
		RAD_ASSERT(f >= frame);
		RAD_ASSERT(f <= lastFrame);

		const U8 *boneTags = m_danim->boneTags + dtag->tagOfs;

		for (U16 i = 0; i < dtag->numBones; ++i, boneTags += 3) {
			int bone = (int)*reinterpret_cast<const U16*>(boneTags);

			if (bone < firstBone)
				continue;
			if (bone >= (firstBone+numBones))
				break;

			U8 stringIdx = *(boneTags+2);
			const char *tagStr = dska->strings + dska->stringOfs[stringIdx];

			tag.tag = tagStr;
			if (notify)
				notify->EmitTag(tag);
			m_ska->OnTag.Trigger(tag);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

Ska::Ska(const DSka &dska) :
m_boneTMs(0),
m_boneFloats(0),
m_cmBoneFloats(0),
m_cmBoneTMsDirty(false),
m_dska(&dska),
m_boneFrame(1),
m_ident(false) {
	Init();
}

Ska::~Ska() {
	if (m_boneTMs)
		zone_free(m_boneTMs);
	if (m_boneFloats)
		zone_free(m_boneFloats);
	if (m_worldBones)
		zone_free(m_worldBones);
	if (m_cmBoneFloats)
		zone_free(m_cmBoneFloats);

	for (Animation::Map::iterator it = m_anims.begin(); it != m_anims.end(); ++it)
		delete it->second;
}

void Ska::Init() {
	m_boneFloats = (float*)zone_malloc(ZSka, sizeof(float)*SIMDDriver::kNumBoneFloats*((int)m_dska->numBones), 0, SIMDDriver::kAlignment);
	m_worldBones = (float*)zone_malloc(ZSka, sizeof(float)*SIMDDriver::kNumBoneFloats*((int)m_dska->numBones+1), 0, SIMDDriver::kAlignment);
	m_boneTMs = (BoneTM*)zone_malloc(ZSka, sizeof(BoneTM)*((int)m_dska->numBones));
	
	for (DAnim::Vec::const_iterator it = m_dska->anims.begin(); it != m_dska->anims.end(); ++it) {
		const DAnim &da = *it;

		m_anims.insert(Animation::Map::value_type(
			String(da.name),
			new Animation(*this, da)
		));
	}

	// all idents.
	for (int i = 0; i < m_dska->numBones+1; ++i)
		details::Mat4x3Ident(m_worldBones + i*SIMDDriver::kNumBoneFloats);
	for (int i = 0; i < m_dska->numBones; ++i)
		details::Mat4x3Ident(m_boneFloats + i*SIMDDriver::kNumBoneFloats);

	m_deltaMotion.s = Vec3(1, 1, 1);
	m_deltaMotion.r = Quat::Identity;
	m_deltaMotion.t = Vec3::Zero;

	m_absMotion.s = Vec3(1, 1, 1);
	m_absMotion.r = Quat::Identity;
	m_absMotion.t = Vec3::Zero;

	m_ident = true;
}

int Ska::FindBone(const char *name) const {
	const char *boneNames = m_dska->boneNames;

	for (int i = 0; i < m_dska->numBones; ++i, boneNames += (kDNameLen+1)) {
		if (!string::cmp(boneNames, name))
			return i;
	}

	return -1;
}

int Ska::ParentOf(int bone) const {
	RAD_ASSERT(bone > -1);
	RAD_ASSERT(bone < m_dska->numBones);
	return m_dska->boneParents[bone];
}

Mat4 Ska::BoneWorldMat(int bone) const {
	RAD_ASSERT(bone > -1);
	RAD_ASSERT(bone < m_dska->numBones);

	const float *x = m_worldBones + SIMDDriver::kNumBoneFloats + (bone*SIMDDriver::kNumBoneFloats);
	return details::Mat4From4x3(x);
}

const float *Ska::BoneTMs(const ColumnMajorTag&) const {
	return 0;
}

const float *Ska::BoneTMs(const RowMajorTag&) const {
	return m_boneFloats;
}

void Ska::Tick(
	float dt, 
	float distance,
	bool advance, 
	bool emitTags, 
	const Mat4 &root
) {
	if (!m_root) {
		if (!m_ident) {
			details::StoreMat4x3(m_worldBones, root); // root position.

			for (int i = 0; i < m_dska->numBones+1; ++i)
				details::Mat4x3Ident(m_worldBones + i*SIMDDriver::kNumBoneFloats);
			for (int i = 0; i < m_dska->numBones; ++i)
				details::Mat4x3Ident(m_boneFloats + i*SIMDDriver::kNumBoneFloats);

			m_ident = true;
			++m_boneFrame;
		}

		return;
	}

	m_root->Activate();

	bool valid = m_root->Tick(
		dt,
		distance,
		m_boneTMs,
		0,
		m_dska->numBones,
		advance,
		emitTags
	);

	if (!valid) {
		if (!m_ident) {
			details::StoreMat4x3(m_worldBones, root); // root position.
			for (int i = 0; i < m_dska->numBones+1; ++i)
				details::Mat4x3Ident(m_worldBones + i*SIMDDriver::kNumBoneFloats);
			for (int i = 0; i < m_dska->numBones; ++i)
				details::Mat4x3Ident(m_boneFloats + i*SIMDDriver::kNumBoneFloats);
			m_ident = true;
			++m_boneFrame;
		}

		return;
	}

	++m_boneFrame;
	m_ident = false;

	m_deltaMotion.s = Vec3(1, 1, 1);
	m_deltaMotion.r = m_root->deltaRot;
	m_deltaMotion.t = m_root->deltaPos;
	
	m_absMotion.s = Vec3(1, 1, 1);
	m_absMotion.r = m_root->rot;
	m_absMotion.t = m_root->pos;
	
	float tempBoneMtx[2][SIMDDriver::kNumBoneFloats];
	float *outMat[2] = { tempBoneMtx[0], tempBoneMtx[1] };
	float *worldBone = m_worldBones+SIMDDriver::kNumBoneFloats;

	details::StoreMat4x3(m_worldBones, root); // root position.

	for (int i = 0; i < m_dska->numBones; ++i, worldBone += SIMDDriver::kNumBoneFloats) {
		const BoneTM &tm = m_boneTMs[i];
		S16 parentIdx = m_dska->boneParents[i];

		const float *parent = (m_worldBones+SIMDDriver::kNumBoneFloats) + parentIdx*SIMDDriver::kNumBoneFloats;

		if (i > 0) {
			Mat4 s = Mat4::Scaling(*reinterpret_cast<const Scale3*>(&tm.s));
			Mat4 r = Mat4::Rotation(tm.r);
			Mat4 t = Mat4::Translation(tm.t);

			details::MulMat4x3(outMat[0], s, r);

			if (parentIdx >= 0) {
				const BoneTM &parentTM = m_boneTMs[parentIdx];
				Scale3 invScale(1.0f/parentTM.s[0], 1.0f/parentTM.s[1], 1.0f/parentTM.s[2]);
				s = Mat4::Scaling(invScale);
				details::MulMat4x3(outMat[1], outMat[0], s);
				std::swap(outMat[0], outMat[1]);
			}

			details::MulMat4x3(outMat[1], t, parent);
			details::MulMat4x3(worldBone, outMat[0], outMat[1]);
		} else { // remove root worldBone motion
			details::StoreMat4x3(worldBone, parent);
		}
	}

	const float *invWorld = m_dska->invWorld;
	float *boneFloats = m_boneFloats;
	worldBone = m_worldBones+SIMDDriver::kNumBoneFloats;

	for (int i = 0; i < m_dska->numBones; ++i, invWorld += 12, worldBone += SIMDDriver::kNumBoneFloats, boneFloats += SIMDDriver::kNumBoneFloats) {
		if (SIMDDriver::kNumBoneFloats == 16) { 
			// SIMD does 4x4 matrices
			details::MulMat4x3(tempBoneMtx[0], invWorld, worldBone);

			for (int r = 0; r < 4; ++r)
				for (int c = 0; c < 3; ++c)
					boneFloats[r*4+c] = MA(tempBoneMtx[0], r, c);

			boneFloats[0*4+3] = 0.f;
			boneFloats[1*4+3] = 0.f;
			boneFloats[2*4+3] = 0.f;
			boneFloats[3*4+3] = 1.f;
		} else {
			details::MulMat4x3(boneFloats, invWorld, worldBone);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
#define CHECK_SIZE(_size) if (((bytes+(_size))-reinterpret_cast<const U8*>(data)) > (int)len) return pkg::SR_CorruptFile;

void DSka::Clear() {
	numBones = 0;
	boneNames = 0;
	boneParents = 0;
	invWorld = 0;
	sDecodeMag = 0;
	tDecodeMag = 0;
	rTable = 0;
	sTable = 0;
	stringOfs = 0;
	strings = 0;
	anims.clear();
}

int DSka::Parse(const void *data, AddrSize len) {
	anims.clear();

	if (len < 8)
		return pkg::SR_InvalidFormat;

	const U8 *bytes = reinterpret_cast<const U8*>(data);
	const U32 *header = reinterpret_cast<const U32*>(data);
	if (header[0] != kSkaTag || header[1] != kSkaVersion)
		return pkg::SR_InvalidFormat;

	bytes += 8;
	CHECK_SIZE(sizeof(U16)*2);
	numBones = *reinterpret_cast<const U16*>(bytes);
	bytes += sizeof(U16);
	U16 numAnims = *reinterpret_cast<const U16*>(bytes);
	bytes += sizeof(U16);

	CHECK_SIZE(kDNameLen+1);
	boneNames = reinterpret_cast<const char*>(bytes);
	bytes += ((int)numBones) * (kDNameLen+1);

	CHECK_SIZE(((int)numBones) * sizeof(S16));
	boneParents = reinterpret_cast<const S16*>(bytes);
	bytes += ((int)numBones) * sizeof(S16);

	if (numBones&1) // padd
		bytes += sizeof(S16);

	CHECK_SIZE(((int)numBones) * sizeof(float) * 12);
	RAD_ASSERT((((AddrSize)bytes)&0x3)==0);
	invWorld = reinterpret_cast<const float*>(bytes);
	bytes += ((int)numBones) * sizeof(float) * 12;

	CHECK_SIZE(sizeof(U32)*3+(sizeof(float)*2));
	unsigned int r, s, t;
	r = *reinterpret_cast<const U32*>(bytes);
	s = *reinterpret_cast<const U32*>(bytes+sizeof(U32));
	t = *reinterpret_cast<const U32*>(bytes+sizeof(U32)*2);
	bytes += sizeof(U32)*3;
	
	sDecodeMag = *reinterpret_cast<const float*>(bytes);
	bytes += sizeof(float);
	tDecodeMag = *reinterpret_cast<const float*>(bytes);
	bytes += sizeof(float);

	CHECK_SIZE((r+s+t)*sizeof(S16));

	rTable = reinterpret_cast<const S16*>(bytes);
	bytes += r * sizeof(S16);

	sTable = reinterpret_cast<const S16*>(bytes);
	bytes += s * sizeof(S16);

	tTable = reinterpret_cast<const S16*>(bytes);
	bytes += t * sizeof(S16);

	// padd?
	{
		int padd = (r+s+t)*(int)sizeof(S16);
		if (padd&3)
		{ // skip padd bytes
			padd &= 3;
			CHECK_SIZE(padd);
			bytes += padd;
		}
	}

	anims.resize(numAnims);

	for (U16 i = 0; i < numAnims; ++i) {
		DAnim &m = anims[i];
		CHECK_SIZE(kDNameLen+1);
		m.name = reinterpret_cast<const char*>(bytes);
		bytes += kDNameLen+1;
		CHECK_SIZE(sizeof(float));
		m.distance = *reinterpret_cast<const float*>(bytes);
		bytes += sizeof(float);
		CHECK_SIZE(sizeof(U16));
		m.fps = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);
		CHECK_SIZE(sizeof(U16));
		m.numFrames = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);
		CHECK_SIZE(sizeof(U16));
		m.numTags = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);
		bytes += sizeof(U16); // padd bytes.
	}

	for (U16 i = 0; i < numAnims; ++i) {
		DAnim &m = anims[i];
		CHECK_SIZE(((int)numBones) * ((int)m.numFrames) * kEncBytes * 3);
		m.rFrames = bytes;
		bytes += ((int)numBones) * ((int)m.numFrames) * kEncBytes;
		m.sFrames = bytes;
		bytes += ((int)numBones) * ((int)m.numFrames) * kEncBytes;
		m.tFrames = bytes;
		bytes += ((int)numBones) * ((int)m.numFrames) * kEncBytes;
		CHECK_SIZE(((int)m.numTags) * sizeof(DTag));
		if (m.numTags > 0) {
			m.tags = reinterpret_cast<const DTag*>(bytes);
			bytes += ((int)m.numTags) * sizeof(DTag);
		} else {
			m.tags = 0;
		}
		CHECK_SIZE(sizeof(U16));
		U16 boneTagSize = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);
		CHECK_SIZE((int)boneTagSize);

		if (boneTagSize > 0) {
			m.boneTags = bytes;
			bytes += boneTagSize;
		} else {
			m.boneTags = 0;
		}
	}

	CHECK_SIZE(1);
	// decode string table.
	U8 numStrings = *bytes;
	++bytes;

	if (numStrings) {
		CHECK_SIZE(((int)numStrings) * sizeof(U16));
		stringOfs = reinterpret_cast<const U16*>(bytes);
		bytes += ((int)numStrings) * sizeof(U16);
		strings = reinterpret_cast<const char*>(bytes); // string table size is not verified
	} else {
		strings = 0;
		stringOfs = 0;
	}

	return pkg::SR_Success;
}

void DSkm::Clear() {
	meshes.clear();
}

int DSkm::Parse(const void * const *_data, const AddrSize *_len, SkinType type) {
	meshes.clear();

	const void *data = _data[0];
	AddrSize len = _len[0];

	if (len < 8)
		return pkg::SR_InvalidFormat;

	const U8 *bytes = reinterpret_cast<const U8*>(data);
	const U32 *header = reinterpret_cast<const U32*>(data);
	if (header[0] != kSkmxTag || header[1] != kSkmVersion)
		return pkg::SR_InvalidFormat;

	bytes += 8;

	CHECK_SIZE(sizeof(float)*6);
	float mins[3];
	float maxs[3];

	for (int i = 0; i < 3; ++i) {
		mins[i] = *reinterpret_cast<const float*>(bytes);
		bytes += sizeof(float);
	}

	for (int i = 0; i < 3; ++i) {
		maxs[i] = *reinterpret_cast<const float*>(bytes);
		bytes += sizeof(float);
	}

	bounds.Initialize(
		mins[0], mins[1], mins[2],
		maxs[0], maxs[1], maxs[2]
	);

	CHECK_SIZE(sizeof(U16)*2);
	U16 numMeshes = *reinterpret_cast<const U16*>(bytes);
	bytes += sizeof(U16)*2;

	if (numMeshes < 1)
		return pkg::SR_InvalidFormat;

	meshes.resize(numMeshes);
	for (U16 i = 0; i < numMeshes; ++i) {
		DMesh &m = meshes[i];

		CHECK_SIZE(sizeof(U16)*(kBonesPerVert+3));

		m.totalVerts = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);

		for (int k = 0; k < kBonesPerVert; ++k) {
			m.numVerts[k] = *reinterpret_cast<const U16*>(bytes);
			bytes += sizeof(U16);
		}

		m.numTris = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);

		m.numChannels = *reinterpret_cast<const U16*>(bytes);
		bytes += sizeof(U16);

		if ((kBonesPerVert+3)&1) { // align? 
			CHECK_SIZE(sizeof(U16));
			bytes += sizeof(U16);
		}

		CHECK_SIZE(ska::kDNameLen+1);
		m.material = reinterpret_cast<const char*>(bytes);
		bytes += ska::kDNameLen+1;
		CHECK_SIZE(((int)m.totalVerts)*sizeof(float)*2*m.numChannels);
		m.texCoords = reinterpret_cast<const float*>(bytes);
		bytes += ((int)m.totalVerts)*sizeof(float)*2*m.numChannels;
		CHECK_SIZE(((int)m.numTris)*3*sizeof(U16));
		m.indices = reinterpret_cast<const U16*>(bytes);
		bytes += ((int)m.numTris)*3*sizeof(U16);
		if (m.numTris&1) { // padd
			CHECK_SIZE(sizeof(U16));
			bytes += sizeof(U16);
		}
	}

	if (type == ska::kSkinType_CPU) {
		data = _data[1];
		len = _len[1];

		RAD_ASSERT(IsAligned(data, SIMDDriver::kAlignment)); // SIMD aligned

		bytes = reinterpret_cast<const U8*>(data);
		header = reinterpret_cast<const U32*>(data);
		if (header[0] != kSkmpTag || header[1] != kSkmVersion)
			return pkg::SR_InvalidFormat;

		bytes += 8;
		for (U16 i = 0; i < numMeshes; ++i) {
			DMesh &m = meshes[i];

			bytes = Align(bytes, SIMDDriver::kAlignment);
			
			m.verts = reinterpret_cast<const float*>(bytes);

			for (int k = 0; k < kBonesPerVert; ++k) {
				const int kNumFloats = ((int)m.numVerts[k])*(k+1)*DMesh::kNumVertexFloats;
				CHECK_SIZE(kNumFloats * sizeof(float));
				bytes += kNumFloats * sizeof(float);
			}

			for (int k = 0; k < kBonesPerVert; ++k) {
				bytes = Align(bytes, SIMDDriver::kAlignment);

				CHECK_SIZE(((int)m.numVerts[k])*(k+1)*sizeof(U16));
				if (m.numVerts[k] > 0) {
					m.bones[k] = reinterpret_cast<const U16*>(bytes);
					bytes += ((int)m.numVerts[k])*(k+1)*sizeof(U16);
				} else {
					m.bones[k] = 0;
				}
			}
		}
	}

	return pkg::SR_Success;
}

} // ska
