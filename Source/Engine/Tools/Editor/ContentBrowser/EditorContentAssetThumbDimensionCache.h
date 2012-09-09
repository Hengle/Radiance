// EditorContentAssetThumbDimensionCache.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include <Runtime/Base.h>
#include <Runtime/Time.h>
#include <Runtime/Container/ZoneMap.h>

namespace tools {
namespace editor {

///////////////////////////////////////////////////////////////////////////////

class ContentAssetThumbDimensionCache {
public:

	static ContentAssetThumbDimensionCache *Get();

	struct Info {
		xtime::TimeDate modified;
		int width;
		int height;
	};

	void Load();
	void Save() const;
	void Delete(int id);
	const Info *Find(int id) const;
	void Update(int id, const Info &info);

	RAD_DECLARE_PROPERTY(ContentAssetThumbDimensionCache, enableSaves, bool, bool);

private:

	RAD_DECLARE_GET(enableSaves, bool) {
		return m_enableSaves;
	}

	RAD_DECLARE_SET(enableSaves, bool) {
		m_enableSaves = value;
	}

	typedef zone_map<int, Info, ZEditorT>::type Map;

	Map m_map;
	bool m_enableSaves;

	ContentAssetThumbDimensionCache();
	~ContentAssetThumbDimensionCache();
};

} // editor
} // tools
