// EditorContentPropertyGrid.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include "../PropertyGrid/EditorProperty.h"
#include <QtGui/QWidget>
#include "../../../Packages/Packages.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class PropertyGrid;
class ComboCheckBox;

class RADENG_CLASS ContentPropertyGrid : public QWidget
{
	Q_OBJECT
public:

	ContentPropertyGrid(
		int platFlags, // see Packages.h
		bool editable,
		QWidget *parent = 0
	);

	~ContentPropertyGrid();

	void Add(int id);
	void Remove(int id);
	void Clear();

	static void PropertyChanged(
		const pkg::Package::Entry::Ref &entry,
		const pkg::KeyVal::Ref &key
	);

	static void PropertyChanged(const pkg::Package::Entry::Ref &entry)
	{
		PropertyChanged(entry, pkg::KeyVal::Ref());
	}

	static void Rebuild();

private slots:

	void PlatformChecked(int index, bool checked);
	void AllPlatformsChecked(bool checked);

private:

	typedef zone_set<ContentPropertyGrid*, ZEditorT>::type GridSet;

	struct IdProps
	{
		typedef boost::shared_ptr<IdProps> Ref;
		int id;
		PropertyList all;
		PropertyList p[pkg::P_NumTargets];
	};

	typedef zone_map<int, IdProps::Ref, ZEditorT>::type IdPropsMap;

	void Update();
	void Update(const IdProps::Ref &p);
	void Remove(const IdProps::Ref &p);
	void SendRebuildEvent();
	virtual void customEvent(QEvent *e);
	void OnRebuildPropertyGrid();

	ComboCheckBox *m_platList;
	PropertyGrid *m_grid;
	IdPropsMap m_map;
	int m_multiPlatIdx;
	int m_selPlats;
	int m_allPlats;
	bool m_editable;

	static bool s_rebuild;
	static bool s_inPropChange;
	static GridSet s_grids;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
