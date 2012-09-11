// MapTypes.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

namespace tools {
namespace solid_bsp {

// we use doubles in solid_bsp

typedef SceneFileD::ValueType ValueType;
typedef SceneFileD::Plane Plane;
typedef SceneFileD::Vec2  Vec2;
typedef SceneFileD::Vec3  Vec3;
typedef SceneFileD::Vec4  Vec4;
typedef SceneFileD::Mat4 Mat4;
typedef SceneFileD::Quat Quat;
typedef SceneFileD::Winding Winding;
typedef SceneFileD::BBox BBox;

typedef SceneFileD::Vec3Vec Vec3Vec;

typedef boost::shared_ptr<Winding> WindingRef;
typedef std::vector<WindingRef> WindingVec;

const ValueType kSplitEpsilon = ValueType(0.00019999999);

} // solid_bsp
} // tools
