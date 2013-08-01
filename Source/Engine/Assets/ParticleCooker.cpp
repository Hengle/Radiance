/*! \file ParticleCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "ParticleCooker.h"
#include "ParticleParser.h"
#include "../Engine.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

ParticleCooker::ParticleCooker() : Cooker(0) {
}

ParticleCooker::~ParticleCooker() {
}

CookStatus ParticleCooker::Status(int flags) {
	if (CompareVersion(flags) || CompareModifiedTime(flags))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

int ParticleCooker::Compile(int flags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);

	int r = asset->Process(
		xtime::TimeSlice::Infinite, 
		flags|P_Parse|P_TargetDefault,
		pkg::SS_Parser
	);

	if (r < SR_Success)
		return r;

	String path(CStr(asset->path));
	path += ".bin";

	BinFile::Ref fp = OpenWrite(path.c_str);
	if (!fp)
		return SR_IOError;

	stream::OutputStream os(fp->ob);

	ParticleParser *parser = ParticleParser::Cast(asset);
	if (!parser)
		return SR_MetaError;

	AddImport(parser->material);

	os << parser->particleStyle->mass[0] << parser->particleStyle->mass[1];
	os << parser->particleStyle->vel[0] << parser->particleStyle->vel[1];
	os << parser->particleStyle->maxvel[0] << parser->particleStyle->maxvel[1];
	os << parser->particleStyle->cgravity[0] << parser->particleStyle->cgravity[1];
	os << parser->particleStyle->cdrag[0] << parser->particleStyle->cdrag[1];
	os << parser->particleStyle->fadein[0] << parser->particleStyle->fadein[1];
	os << parser->particleStyle->fadeout[0] << parser->particleStyle->fadeout[1];
	os << parser->particleStyle->lifetime[0] << parser->particleStyle->lifetime[1];
	os << parser->particleStyle->rotation[0] << parser->particleStyle->rotation[1];
	os << parser->particleStyle->rotationRate[0] << parser->particleStyle->rotationRate[1];
	os << parser->particleStyle->rotationDrift[0] << parser->particleStyle->rotationDrift[1];
	os << parser->particleStyle->rotationDriftTime[0] << parser->particleStyle->rotationDriftTime[1];
	os << parser->particleStyle->xforce[0] << parser->particleStyle->xforce[1];
	os << parser->particleStyle->yforce[0] << parser->particleStyle->yforce[1];
	os << parser->particleStyle->zforce[0] << parser->particleStyle->zforce[1];
	os << parser->particleStyle->xdrift[0] << parser->particleStyle->xdrift[1];
	os << parser->particleStyle->ydrift[0] << parser->particleStyle->ydrift[1];
	os << parser->particleStyle->zdrift[0] << parser->particleStyle->zdrift[1];
	os << parser->particleStyle->xdriftPhase[0] << parser->particleStyle->xdriftPhase[1];
	os << parser->particleStyle->ydriftPhase[0] << parser->particleStyle->ydriftPhase[1];
	os << parser->particleStyle->zdriftPhase[0] << parser->particleStyle->zdriftPhase[1];
	os << parser->particleStyle->xdriftTime[0] << parser->particleStyle->xdriftTime[1];
	os << parser->particleStyle->ydriftTime[0] << parser->particleStyle->ydriftTime[1];
	os << parser->particleStyle->zdriftTime[0] << parser->particleStyle->zdriftTime[1];
	os << parser->particleStyle->sizeX[0] << parser->particleStyle->sizeX[1];
	os << parser->particleStyle->sizeY[0] << parser->particleStyle->sizeY[1];
	os << parser->particleStyle->sizeScaleX[0] << parser->particleStyle->sizeScaleX[1];
	os << parser->particleStyle->sizeScaleY[0] << parser->particleStyle->sizeScaleY[1];
	os << parser->particleStyle->sizeScaleXTime[0] << parser->particleStyle->sizeScaleXTime[1];
	os << parser->particleStyle->sizeScaleYTime[0] << parser->particleStyle->sizeScaleYTime[1];
	os << parser->particleStyle->rgba[0] << parser->particleStyle->rgba[1] << parser->particleStyle->rgba[2] << parser->particleStyle->rgba[3];

	return SR_Success;
}

void ParticleCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<ParticleCooker>();
}

};