// SkAnimDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Skeletal Animation
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#if defined(RAD_OPT_TOOLS)
#include <Runtime/Container/ZoneMap.h>
#endif
#include <Runtime/PushPack.h>

namespace ska {

class Animation;
class Ska;
typedef boost::shared_ptr<Ska> SkaRef;
typedef boost::shared_ptr<Ska> SkaWRef;
class Vtm;
typedef boost::shared_ptr<Vtm> VtmRef;
typedef boost::shared_ptr<Vtm> VtmWRef;
class Controller;
typedef boost::shared_ptr<Controller> ControllerRef;
typedef boost::shared_ptr<Controller> ControllerWRef;
class Notify;
typedef boost::shared_ptr<Notify> NotifyRef;

typedef boost::shared_ptr<float> VertArrayRef;

#if defined(RAD_OPT_TOOLS)
class ModelBuilder;
typedef boost::shared_ptr<ModelBuilder> ModelBuilderRef;
#endif

struct DSkAnim;
struct DSka;
struct DSkMesh;
struct DSkm;

struct DVtAnim;
struct DVtMesh;
struct DVtm;

enum {
	kEncBytes = 3,
	kEncMask = 0x00ffffff
};

enum SkinType {
	kSkinType_CPU
};

enum {
	kSkaTag = RAD_FOURCC_LE('S', 'K', 'A', 'X'),
	kSkaVersion = 1,
	kSkmxTag = RAD_FOURCC_LE('S', 'K', 'M', 'X'),
	kSkmpTag = RAD_FOURCC_LE('S', 'K', 'M', 'P'),
	kSkmVersion = 2,
	kVtmxTag = RAD_FOURCC_LE('V', 'A', 'M', 'X'),
	kVtmpTag = RAD_FOURCC_LE('V', 'A', 'M', 'M'),
	kVtmVersion = 1,
	kDNameLen = 63, 
	kBonesPerVert = 4,
	kMaxUVChannels = 1
};

} // ska

#if defined(RAD_OPT_TOOLS)
namespace tools {
typedef zone_map<String, float, ZToolsT>::type SkaCompressionMap;
typedef zone_map<String, SkaCompressionMap, ZToolsT>::type CinematicActorCompressionMap;
}
#endif

#include <Runtime/PopPack.h>

