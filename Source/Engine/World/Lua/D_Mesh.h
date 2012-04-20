// D_Mesh.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../../Renderer/Mesh.h"
#include "../EntityDef.h"
#include "D_Asset.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_Mesh : public D_Asset
{
public:
	typedef boost::shared_ptr<D_Mesh> Ref;

	static Ref New(const r::MeshBundle::Ref &bundle);

	RAD_DECLARE_READONLY_PROPERTY(D_Mesh, bundle, const r::MeshBundle::Ref&);

private:

	D_Mesh(const r::MeshBundle::Ref &bundle);

	RAD_DECLARE_GET(bundle, const r::MeshBundle::Ref&) { return m_bundle; }

	r::MeshBundle::Ref m_bundle;
};

} // world

#include <Runtime/PopPack.h>
