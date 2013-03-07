/*! \file AssetCookers.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/


#include RADPCH

#if defined(RAD_OPT_TOOLS) && !defined(RAD_OPT_IOS)

// Engine supports IntermediateCookers on any RAD_OPT_TOOLS target
// which means consoles since they can load uncooked data. iOS cannot
// however, and it is unlikely we would want the phone BSP'ing and
// what-not.

// May need to do start thinking about distributing the processing by
// doing the cooking on PC on demand for the target device and then
// sending the results over the network.

#include "MapCooker.h"
#include "MaterialCooker.h"
#include "TextureCooker.h"
#include "SoundCooker.h"
#include "SkModelCooker.h"
#include "SkAnimSetCooker.h"
#include "SkAnimStatesCooker.h"
#include "MeshCooker.h"
#include "MusicCooker.h"
#include "FontCooker.h"
#include "TypefaceCooker.h"
#include "StringTableCooker.h"

namespace asset {

void RB_RegisterCookers(Engine&);

RADENG_API void RADENG_CALL RegisterCookers(Engine &engine) {
	MapCooker::Register(engine);
	MaterialCooker::Register(engine);
	TextureCooker::Register(engine);
	SkModelCooker::Register(engine);
	SkAnimSetCooker::Register(engine);
	SkAnimStatesCooker::Register(engine);
	SoundCooker::Register(engine);
	MeshCooker::Register(engine);
	MusicCooker::Register(engine);
	FontCooker::Register(engine);
	TypefaceCooker::Register(engine);
	StringTableCooker::Register(engine);
	RB_RegisterCookers(engine);
}

} // asset

#else

namespace asset {

RADENG_API void RADENG_CALL RegisterCookers(Engine &engine) {
}

} // asset

#endif

