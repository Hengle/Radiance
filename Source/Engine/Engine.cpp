// Engine.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

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
	#include "Renderer/PC/RBackend.h"
#endif

enum
{
	FiberStackSize = 32*Kilo,
	MaxTotalFiberMem = 8*Meg,
	MaxFibers = MaxTotalFiberMem/FiberStackSize
};

using namespace string;

int __Argc();
const char **__Argv();

Engine *Engine::New() { return new (ZEngine) Engine(); }

Engine::Engine()
{
	spawn::E_Exports();
	m_comTable.components = IComponentManager::Instance();
	RAD_REGISTER_COMPONENTS(m_comTable.components, ExportEngineComponents);
}

Engine::~Engine()
{
}

bool Engine::PreInit()
{
#if defined(RAD_OPT_PC_TOOLS)
	m_scc = SCC::Create("null");
	RAD_ASSERT(m_scc);
#endif

	const char *baseDir = ArgArg("-base");
	if (!baseDir)
	{
		baseDir = "Base";
	}

	m_baseDir = ::string::Widen(baseDir);

	const char *root = ArgArg("-root");
	if (root)
	{
		RAD_DEBUG_ONLY(bool b=file::EnforcePortablePaths(false));
		file::SetAlias(file::AliasRoot, ::string::Widen(root).c_str());
		RAD_DEBUG_ONLY(file::EnforcePortablePaths(b));
	}

	if (!LoadComponents())
	{
		return false;
	}

	COut(C_Info) << "num processors: " << thread::NumContexts() << std::endl;
	COut(C_Info) << "hyper-threading enabled: " << (thread::details::IsHyperThreadingOn() ? "true" : "false") << std::endl;
	COut(C_Info) << SIMD->name << " driver bound." << std::endl;

	sys->files->Initialize(file::AllMedia & ~(file::CDDVD|file::Mod));
	sys->files->hddRoot = m_baseDir.c_str();
	sys->files->cddvdRoot = m_baseDir.c_str();
	sys->paks->Initialize(sys->files);

	sys->r->Initialize();
	m_comTable.alDriver = ALDriver::create(ALDRIVER_SIG 0);

	if (!m_comTable.alDriver)
	{
		COut(C_Info) << "Error initializing sound system!" << std::endl;
		return false;
	}

	m_comTable.globals = Persistence::Load("globals.data");

	return true;
}

bool Engine::Initialize()
{
	bool r = sys->packages->Initialize();
	asset::RegisterParsers(*this);
#if defined(RAD_OPT_TOOLS)
	asset::RegisterCookers(*this);
#endif
	return r;
}

void Engine::Finalize()
{
	m_comTable.alDriver.reset();
	m_comTable.components->ReleaseCachedComponents();
	m_comTable.packages.reset();

	m_comTable.r.Close();
	m_comTable.paks.Close();
	m_comTable.files.Close();
}

bool Engine::FindArg(const char *arg)
{
	for (int i = 0; i < __Argc(); ++i)
	{
		if (!icmp(arg, Argv(i))) return true;
	}
	return false;
}

const char *Engine::ArgArg(const char *arg)
{
	for (int i = 0; i < __Argc(); ++i)
	{
		if (!icmp(arg, Argv(i)))
		{
			return Argv(i+1);
		}
	}
	return 0;
}

const char *Engine::Argv(int arg)
{
	if (arg >= 0 && arg < __Argc()) return __Argv()[arg];
	return 0;
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

#if defined(RAD_OPT_PC_TOOLS)
void Engine::SwitchEditorMode(bool editor)
{
}
#endif

int Engine::RAD_IMPLEMENT_GET(argc)
{
	return __Argc();
}

#define LOAD_COM(_iface, _com, _name) \
	if (!m_comTable._com) { \
		m_comTable._com = m_comTable.components->Create(#_name).Cast<_iface>();\
		if (!m_comTable._com) { throw MissingComponentException(#_name); }\
	}

bool Engine::LoadComponents()
{
	LOAD_COM(file::IFileSystem, files, file.FileSystem);
	LOAD_COM(file::IDPakReader, paks, file.DPakReader);
	LOAD_COM(r::IRenderer, r, r.RBackend);
	m_comTable.packages = pkg::PackageMan::New(*this, "Packages");
	return true;
}

