// Engine.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "App.h"
#include "Engine.h"
#include "ComponentExports.h"
#include "World/Entities/E_Exports.h"
#include <Runtime/Thread.h>
#include <Runtime/File.h>
#include <Runtime/Interface/ComponentManagerDef.h>
#include <Runtime/Base/SIMD.h>
#include "Zones.h"
#include "Assets/Assets.h"
#include "Sound/ALDriver.h"
#include <sstream>

#if defined(RAD_OPT_PC)
	#include "Renderer/GL/RBackend.h"
#endif

Engine *Engine::New() { 
	return new (ZEngine) Engine(); 
}

Engine::Engine() {
	spawn::E_Exports();
	m_comTable.components = IComponentManager::Instance();
	RAD_REGISTER_COMPONENTS(m_comTable.components, ExportEngineComponents);
}

Engine::~Engine() {
}

bool Engine::PreInit() {
#if defined(RAD_OPT_PC_TOOLS)
	m_scc = SCC::Create("null");
	RAD_ASSERT(m_scc);
#endif

	if (!LoadComponents())
		return false;

	COut(C_Info) << "num processors: " << thread::NumContexts() << std::endl;
	COut(C_Info) << "hyper-threading enabled: " << (thread::details::IsHyperThreadingOn() ? "true" : "false") << std::endl;
	COut(C_Info) << SIMD->name << " driver bound." << std::endl;

#if defined(RAD_OPT_TOOLS)
//	SIMDSkinTest(COut(C_Info));
#endif

	const char *baseDir = App::Get()->ArgArg("-base");
	if (!baseDir)
		baseDir = "Base";

	const char *root = App::Get()->ArgArg("-root");
	if (root)
		sys->files->SetAlias('r', root);

	sys->files->AddDirectory((CStr("@r:/") + baseDir).c_str, file::kFileMask_Base);

	sys->r->Initialize();
	m_comTable.alDriver = ALDriver::New(ALDRIVER_SIG 0);

	if (!m_comTable.alDriver)
	{
		COut(C_Info) << "Error initializing sound system!" << std::endl;
		return false;
	}

	m_comTable.globals = Persistence::Load("globals.data");

	bool r = sys->packages->Initialize();
	asset::RegisterParsers(*this);
#if defined(RAD_OPT_TOOLS)
	asset::RegisterCookers(*this);
#endif
	return r;
}

bool Engine::Initialize() {
	return true;
}

void Engine::Finalize()
{
	m_comTable.alDriver.reset();
	m_comTable.components->ReleaseCachedComponents();
	m_comTable.packages.reset();

	if (m_comTable.r)
		m_comTable.r->ctx = r::HContext();

	m_comTable.r.Close();
	m_comTable.files.reset();
}

void Engine::Tick(float elapsed)
{
}

#if defined(RAD_OPT_PC)
void Engine::VidBind()
{
	r::HRBackend rb = m_comTable.r.Cast<r::IRBackend>();
	RAD_VERIFY(rb);
	rb->VidBind();

	m_comTable.packages->ProcessAll(
		pkg::Z_All,
		xtime::TimeSlice::Infinite,
		pkg::P_VidBind,
		true
	);
}

void Engine::VidReset()
{
	if (m_comTable.packages)
	{
		m_comTable.packages->ProcessAll(
			pkg::Z_All,
			xtime::TimeSlice::Infinite,
			pkg::P_VidReset,
			true
		);
	}

	if (m_comTable.r)
	{
		r::HRBackend rb = m_comTable.r.Cast<r::IRBackend>();
		RAD_VERIFY(rb);
		rb->VidReset();
	}
}
#endif

#define LOAD_COM(_iface, _com, _name) \
	if (!m_comTable._com) { \
		m_comTable._com = m_comTable.components->Create(#_name).Cast<_iface>();\
		if (!m_comTable._com) { throw MissingComponentException(#_name); }\
	}

bool Engine::LoadComponents()
{
	m_comTable.files = file::FileSystem::New();
	LOAD_COM(r::IRenderer, r, r.RBackend);
	m_comTable.packages = pkg::PackageMan::New(*this, "Packages");
	return true;
}

