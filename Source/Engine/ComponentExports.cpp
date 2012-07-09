// ComponentExports.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "ComponentExports.h"
#include "FileSystem/Backend/FileSystem.h"
#include "FileSystem/Backend/DPak.h"
#include "Renderer/Renderer.h"
#include <Runtime/Interface/ComponentManager.h>

RAD_BEGIN_EXPORT_COMPONENTS(RADENG_API, ExportEngineComponents)
	RAD_EXPORT_COMPONENT_NAMESPACE(file::details, FileSystem)
	RAD_EXPORT_COMPONENT_NAMESPACE(file::details, DPakReader)
	RAD_LINK_EXPORT_COMPONENTS_FN(r::ExportRBackendComponents)
RAD_END_EXPORT_COMPONENTS

