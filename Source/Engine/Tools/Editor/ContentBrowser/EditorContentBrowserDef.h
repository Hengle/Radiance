// EditorContentBrowserDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include "../../../Packages/Packages.h"
#include <Runtime/Container/ZoneVector.h>

namespace tools {
namespace editor {

struct RADENG_CLASS ContentChange
{
	typedef zone_vector<ContentChange, ZEditorT>::type Vec;
	
	ContentChange() {}
	ContentChange(const ContentChange &c)
		: entry(c.entry), key(c.key)
	{
	}

	ContentChange(const pkg::Package::Entry::Ref &_entry, const pkg::KeyVal::Ref &_key)
		: entry(_entry), key(_key)
	{
	}

	explicit ContentChange(const pkg::Package::Entry::Ref &_entry)
		: entry(_entry)
	{
	}

	pkg::Package::Entry::Ref entry;
	pkg::KeyVal::Ref key;
};

} // editor
} // tools
