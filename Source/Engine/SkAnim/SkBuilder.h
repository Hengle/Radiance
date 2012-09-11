// SkBuilder.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "../Tools/SceneFile.h"
#include "SkAnim.h"
#include <Runtime/PushPack.h>

namespace tools {

struct SkaData
{
	// cleans itself up when destructed.
	typedef boost::shared_ptr<SkaData> Ref;

	SkaData();
	~SkaData();

	ska::DSka dska;

	void *skaData; // dska references this in place.
	AddrSize skaSize;
};

struct SkmData
{ // cleans itself up when destructed.
	typedef boost::shared_ptr<SkmData> Ref;

	SkmData();
	~SkmData();

	ska::DSkm dskm;
	ska::SkinType skinType;

	void *skmData[2]; // dskm references this in place
	AddrSize skmSize[2];
};

RADENG_API SkaData::Ref RADENG_CALL CompileSkaData(
	const char *name,
	const SceneFileVec &anims,
	int trimodel
);

RADENG_API SkaData::Ref RADENG_CALL CompileSkaData(
	const char *name,
	const SceneFile &anims,
	int trimodel
);

RADENG_API SkmData::Ref RADENG_CALL CompileSkmData(
	const char *name, 
	const SceneFile &meshes, 
	int trimodel,
	ska::SkinType skinType,
	const ska::DSka &ska
);

} // tools

#include <Runtime/PopPack.h>
