// EditorContentTexture.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include "../PropertyGrid/EditorProperty.h"
#include "../../../Packages/Packages.h"
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {
namespace content_property_details {

RADENG_API void RADENG_CALL AddTextureProperties(
	PropertyList &l,
	const pkg::Package::Entry::Ref &e, 
	int flags, 
	QWidget &widget
);

RADENG_API pkg::IdVec RADENG_CALL CreateTextures(QWidget *parent, const pkg::Package::Ref &pkg, pkg::IdVec &sel);

} // content_property_details
} // editor
} // tools

#include <Runtime/PopPack.h>

