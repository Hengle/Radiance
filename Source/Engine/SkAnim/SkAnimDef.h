// SkAnimDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Skeletal Animation
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include <Runtime/PushPack.h>

namespace ska {

//#define SKA_NORMALS

class Animation;
class Ska;
typedef boost::shared_ptr<Ska> SkaRef;
typedef boost::shared_ptr<Ska> SkaWRef;
class Controller;
typedef boost::shared_ptr<Controller> ControllerRef;
typedef boost::shared_ptr<Controller> ControllerWRef;
class Notify;
typedef boost::shared_ptr<Notify> NotifyRef;

#if defined(RAD_OPT_TOOLS)
class ModelBuilder;
typedef boost::shared_ptr<ModelBuilder> ModelBuilderRef;
#endif

struct DAnim;
struct DSka;
struct DMesh;
struct DSkm;

enum
{
	EncBytes = 3,
	EncMask = 0x00ffffff
};

enum SkinType
{
	SkinCpu
};

enum
{
	SkaTag = RAD_FOURCC_LE('S', 'K', 'A', 'X'),
	SkaVersion = 1,
	SkmxTag = RAD_FOURCC_LE('S', 'K', 'M', 'X'),
	SkmpTag = RAD_FOURCC_LE('S', 'K', 'M', 'P'),
	SkmVersion = 1,
	DNameLen = 63, 
	BonesPerVert = 4,
	MaxUVChannels = 1
};

} // ska

#include <Runtime/PopPack.h>

