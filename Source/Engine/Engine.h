// Engine.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"
#include <Runtime/Base.h>
#include <Runtime/String.h>
#include <Runtime/File.h>
#include <Runtime/Interface/ComponentManager.h>
#include "Packages/Packages.h"
#include "Renderer/Renderer.h"
#include "COut.h"
#include "LogFile.h"
#include "Sound/ALDriverDef.h"
#include "Persistence.h"

#if defined(RAD_OPT_PC_TOOLS)
	#include "SCC/SCC.h"
#endif

class RADENG_CLASS Engine
{
public:

	struct ComTable
	{
		r::HRenderer         r;
		pkg::PackageMan::Ref packages;
		file::FileSystem::Ref files;
		HComponentManager    components;
		ALDriverRef          alDriver;
		Persistence::Ref     globals;
	};

	static Engine *New();

	Engine();
	virtual ~Engine();

	bool PreInit();
	bool Initialize();
	void Finalize();

	void Tick(float elapsed);

#if defined(RAD_OPT_PC)
	void VidBind();
	void VidReset();
#endif

	RAD_DECLARE_READONLY_PROPERTY(Engine, sys, const ComTable*);
	RAD_DECLARE_READONLY_PROPERTY(Engine, baseDir, const char*);

private:

	RAD_DECLARE_GET(argc, int);
	RAD_DECLARE_GET(sys, const ComTable*) { return &m_comTable; }
	RAD_DECLARE_GET(baseDir, const char*) { return m_baseDir.c_str; }

	bool LoadComponents();

	mutable ComTable m_comTable;
	String m_baseDir;

#if defined(RAD_OPT_PC_TOOLS)
	SCC::Ref m_scc;
	RAD_DECLARE_GET(scc, SCC*) { return m_scc.get(); }
#endif

};
