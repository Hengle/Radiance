/*! \file ParticleParser.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "ParticleParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

ParticleParser::ParticleParser() : m_loaded(false) {
}

ParticleParser::~ParticleParser() {
}

int ParticleParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (m_loaded && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked)
		return Load(time, engine, asset, flags);
#endif
	return LoadCooked(time, engine, asset, flags);
}

#if defined(RAD_OPT_TOOLS)
int ParticleParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	const String *s = asset->entry->KeyValue<String>("Source.Material", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	if (s->empty)
		return SR_FileNotFound;
	m_material = *s;

	s = asset->entry->KeyValue<String>("Physics.Mass", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.mass[0] = m_style.mass[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.mass[0], &m_style.mass[1]);

	s = asset->entry->KeyValue<String>("Physics.Drag", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.cdrag[0] = m_style.cdrag[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.cdrag[0], &m_style.cdrag[1]);

	s = asset->entry->KeyValue<String>("Physics.InitialVelocity", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.vel[0] = m_style.vel[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.vel[0], &m_style.vel[1]);

	s = asset->entry->KeyValue<String>("Physics.Gravity", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.cgravity[0] = m_style.cgravity[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.cgravity[0], &m_style.cgravity[1]);

	s = asset->entry->KeyValue<String>("Physics.Force.X", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.xforce[0] = m_style.xforce[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.xforce[0], &m_style.xforce[1]);

	s = asset->entry->KeyValue<String>("Physics.Force.Y", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.yforce[0] = m_style.yforce[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.yforce[0], &m_style.yforce[1]);

	s = asset->entry->KeyValue<String>("Physics.Force.Z", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.zforce[0] = m_style.zforce[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.zforce[0], &m_style.zforce[1]);

	s = asset->entry->KeyValue<String>("Physics.Drift.X", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.xdrift[0] = m_style.xdrift[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.xdrift[0], &m_style.xdrift[1]);

	s = asset->entry->KeyValue<String>("Physics.Drift.Y", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.ydrift[0] = m_style.ydrift[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.ydrift[0], &m_style.ydrift[1]);

	s = asset->entry->KeyValue<String>("Physics.Drift.Z", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.zdrift[0] = m_style.zdrift[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.zdrift[0], &m_style.zdrift[1]);

	s = asset->entry->KeyValue<String>("Physics.Drift.X.Time", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.xdriftTime[0] = m_style.xdriftTime[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.xdriftTime[0], &m_style.xdriftTime[1]);

	s = asset->entry->KeyValue<String>("Physics.Drift.Y.Time", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.ydriftTime[0] = m_style.ydriftTime[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.ydriftTime[0], &m_style.ydriftTime[1]);

	s = asset->entry->KeyValue<String>("Physics.Drift.Z.Time", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.zdriftTime[0] = m_style.zdriftTime[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.zdriftTime[0], &m_style.zdriftTime[1]);

	s = asset->entry->KeyValue<String>("Physics.Rotation", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.rotation[0] = m_style.rotation[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.rotation[0], &m_style.rotation[1]);

	s = asset->entry->KeyValue<String>("Physics.Rotation.Rate", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.rotationRate[0] = m_style.rotationRate[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.rotationRate[0], &m_style.rotationRate[1]);

	s = asset->entry->KeyValue<String>("Physics.Rotation.Drift", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.rotationDrift[0] = m_style.rotationDrift[1] = 0.f;
	sscanf(s->c_str, "%f %f", &m_style.rotationDrift[0], &m_style.rotationDrift[1]);

	s = asset->entry->KeyValue<String>("Physics.Rotation.Drift.Time", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.rotationDriftTime[0] = m_style.rotationDriftTime[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.rotationDriftTime[0], &m_style.rotationDriftTime[1]);

	s = asset->entry->KeyValue<String>("Style.FadeIn", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.fadein[0] = m_style.fadein[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.fadein[0], &m_style.fadein[1]);

	s = asset->entry->KeyValue<String>("Style.FadeOut", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.fadeout[0] = m_style.fadeout[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.fadeout[0], &m_style.fadeout[1]);

	s = asset->entry->KeyValue<String>("Style.Lifetime", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.lifetime[0] = m_style.lifetime[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.lifetime[0], &m_style.lifetime[1]);

	s = asset->entry->KeyValue<String>("Style.Size.X", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.sizeX[0] = m_style.sizeX[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.sizeX[0], &m_style.sizeX[1]);

	s = asset->entry->KeyValue<String>("Style.Size.Y", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.sizeY[0] = m_style.sizeY[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.sizeY[0], &m_style.sizeY[1]);

	s = asset->entry->KeyValue<String>("Style.Size.X.Scale", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.sizeScaleX[0] = m_style.sizeScaleX[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.sizeScaleX[0], &m_style.sizeScaleX[1]);

	s = asset->entry->KeyValue<String>("Style.Size.Y.Scale", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.sizeScaleY[0] = m_style.sizeScaleY[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.sizeScaleY[0], &m_style.sizeScaleY[1]);

	s = asset->entry->KeyValue<String>("Style.Size.X.Scale.Time", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.sizeScaleXTime[0] = m_style.sizeScaleXTime[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.sizeScaleXTime[0], &m_style.sizeScaleXTime[1]);

	s = asset->entry->KeyValue<String>("Style.Size.Y.Scale.Time", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;
	m_style.sizeScaleYTime[0] = m_style.sizeScaleYTime[1] = 1.f;
	sscanf(s->c_str, "%f %f", &m_style.sizeScaleYTime[0], &m_style.sizeScaleYTime[1]);

	m_style.rgba[0] = 1.f;
	m_style.rgba[1] = 1.f;
	m_style.rgba[2] = 1.f;
	m_style.rgba[3] = 1.f;
	return SR_Success;
}
#endif

int ParticleParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	String path(CStr("Cooked/"));
	path += CStr(asset->path);
	path += ".bin";

	file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(path.c_str, r::ZRender);
	if (!ib)
		return SR_FileNotFound;

	stream::InputStream is(*ib);

	try {
		is >> m_style.mass[0] >> m_style.mass[1];
		is >> m_style.vel[0] >> m_style.vel[1];
		is >> m_style.cgravity[0] >> m_style.cgravity[1];
		is >> m_style.cdrag[0] >> m_style.cdrag[1];
		is >> m_style.fadein[0] >> m_style.fadein[1];
		is >> m_style.fadeout[0] >> m_style.fadeout[1];
		is >> m_style.lifetime[0] >> m_style.lifetime[1];
		is >> m_style.rotation[0] >> m_style.rotation[1];
		is >> m_style.rotationRate[0] >> m_style.rotationRate[1];
		is >> m_style.rotationDrift[0] >> m_style.rotationDrift[1];
		is >> m_style.rotationDriftTime[0] >> m_style.rotationDriftTime[1];
		is >> m_style.xforce[0] >> m_style.xforce[1];
		is >> m_style.yforce[0] >> m_style.yforce[1];
		is >> m_style.zforce[0] >> m_style.zforce[1];
		is >> m_style.xdrift[0] >> m_style.xdrift[1];
		is >> m_style.ydrift[0] >> m_style.ydrift[1];
		is >> m_style.zdrift[0] >> m_style.zdrift[1];
		is >> m_style.xdriftTime[0] >> m_style.xdriftTime[1];
		is >> m_style.ydriftTime[0] >> m_style.ydriftTime[1];
		is >> m_style.zdriftTime[0] >> m_style.zdriftTime[1];
		is >> m_style.sizeX[0] >> m_style.sizeX[1];
		is >> m_style.sizeY[0] >> m_style.sizeY[1];
		is >> m_style.sizeScaleX[0] >> m_style.sizeScaleX[1];
		is >> m_style.sizeScaleY[0] >> m_style.sizeScaleY[1];
		is >> m_style.sizeScaleXTime[0] >> m_style.sizeScaleXTime[1];
		is >> m_style.sizeScaleYTime[0] >> m_style.sizeScaleYTime[1];
		is >> m_style.rgba[0] >> m_style.rgba[1] >> m_style.rgba[2] >> m_style.rgba[3];
	} catch (exception&) {
		return SR_IOError;
	}

	return SR_Success;
}

void ParticleParser::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<ParticleParser>();
}

} // asset
