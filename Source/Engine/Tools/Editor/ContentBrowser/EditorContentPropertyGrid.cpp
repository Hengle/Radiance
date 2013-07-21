// EditorContentPropertyGrid.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorContentPropertyGrid.h"
#include "EditorContentProperties.h"
#include "../PropertyGrid/EditorPropertyGrid.h"
#include "EditorContentBrowserWindow.h"
#include "../EditorEventRegistry.h"
#include "../EditorComboCheckBox.h"
#include "../../../Packages/Packages.h"
#include <QtGui/QGridLayout>
#include <QtGui/QMessageBox>
#include <QtCore/QEvent>
#include <QtGui/QApplication>

using namespace tools::editor::content_property_details;

namespace tools {
namespace editor {

enum { AllTargetsIdx };

ContentPropertyGrid::GridSet ContentPropertyGrid::s_grids;
bool ContentPropertyGrid::s_rebuild=false;
bool ContentPropertyGrid::s_inPropChange=false;

ContentPropertyGrid::ContentPropertyGrid(int platFlags, bool editable, QWidget *parent)
: QWidget(parent)
{
	QGridLayout *layout = new QGridLayout(this);

	m_allPlats = platFlags;

	m_platList = new ComboCheckBox(this);
	m_platList->setStrings("All Platforms", "");

	for (int i = pkg::P_FirstTarget; i <= pkg::P_LastTarget; i <<= 1)
	{
		if (i&m_allPlats)
		{
			m_platList->addItem(pkg::PlatformNameForFlags(i), true);
		}
	}

	m_selPlats = m_allPlats;
	RAD_VERIFY(connect(m_platList, SIGNAL(OnItemChecked(int, bool)), SLOT(PlatformChecked(int, bool))));
	RAD_VERIFY(connect(m_platList, SIGNAL(OnAllChecked(bool)), SLOT(AllPlatformsChecked(bool))));

	layout->addWidget(m_platList, 0, 0);

	m_grid = new PropertyGrid(editable, this);
	layout->addWidget(m_grid, 1, 0);
	layout->setRowStretch(0, 1);

	s_grids.insert(this);
}

ContentPropertyGrid::~ContentPropertyGrid()
{
	s_grids.erase(this);
}

void ContentPropertyGrid::Add(int id)
{
	IdPropsMap::iterator it = m_map.find(id);
	if (it != m_map.end())
		return;
	IdProps::Ref p(new (ZEditor) IdProps());
	p->id = id;
	RAD_VERIFY(m_map.insert(IdPropsMap::value_type(id, p)).second);

	if (s_inPropChange)
	{
		s_rebuild = true;
	}
	else
	{
		Update(p);
	}
}

void ContentPropertyGrid::Remove(int id)
{
	IdPropsMap::iterator it = m_map.find(id);
	if (it == m_map.end())
		return;

	IdProps::Ref p = it->second;
	m_map.erase(it);

	if (s_inPropChange)
	{
		s_rebuild = true;
	}
	else
	{
		Remove(p);
	}
}

void ContentPropertyGrid::Clear()
{
	m_grid->Clear();
	m_map.clear();
}

void ContentPropertyGrid::SendRebuildEvent()
{
	// send posted event because a property in the grid may be
	// invoking this action and we cannot delete it now.
	QCoreApplication::postEvent(
		this, 
		new (ZEditor) QEvent((QEvent::Type)EV_RebuildContentPropertyGrid)
	);			
}

void ContentPropertyGrid::PropertyChanged(
	const pkg::Package::Entry::Ref &entry,
	const pkg::KeyVal::Ref &key
)
{
	s_rebuild = false;
	s_inPropChange = true;
	
	ContentChange::Vec changed;
	changed.push_back(ContentChange(entry, key));
	ContentBrowserWindow::NotifyContentChanged(changed);

	s_inPropChange = false;

	if (s_rebuild)
	{
		s_rebuild = false;
		Rebuild();
	}
}

void ContentPropertyGrid::Rebuild()
{
	if (s_inPropChange)
	{
		s_rebuild = true;
		return;
	}

	for (GridSet::const_iterator it = s_grids.begin(); it != s_grids.end(); ++it)
	{
		(*it)->SendRebuildEvent();
	}
}

void ContentPropertyGrid::PlatformChecked(int index, bool checked)
{
	int f = pkg::PlatformFlagsForName(m_platList->itemText(index).toAscii().constData());
	m_selPlats = checked ? (m_selPlats|f) : (m_selPlats&~f);
	Update();
}

void ContentPropertyGrid::AllPlatformsChecked(bool checked)
{
	m_selPlats = checked ? m_allPlats : 0;
	Update();
}

void ContentPropertyGrid::Update()
{
	for (IdPropsMap::const_iterator it = m_map.begin(); it != m_map.end(); ++it)
	{
		Update(it->second);
	}
}

void ContentPropertyGrid::Update(const IdProps::Ref &p)
{
	pkg::Package::Entry::Ref e = Packages()->FindEntry(p->id);

	// all plats...
	if (p->all.isEmpty())
	{
		p->all = CreatePropertiesForAsset(e, 0, *this);
		m_grid->Add(p->all);
	}

	for (int i = 0; i < pkg::P_NumTargets; ++i)
	{
		int f = pkg::P_FirstTarget<<i;

		if (f&m_selPlats)
		{
			if (p->p[i].isEmpty())
			{
				if (e)
				{
					p->p[i] = CreatePropertiesForAsset(e, f, *this);
					m_grid->Add(p->p[i]);
				}
			}
		}
		else
		{
			if (!p->p[i].isEmpty())
			{
				m_grid->Remove(p->p[i]);
				p->p[i].clear();
			}
		}
	}

	m_grid->sortByColumn(0);
}

void ContentPropertyGrid::Remove(const IdProps::Ref &p)
{
	m_grid->Remove(p->all);

	for (int i = 0; i < pkg::P_NumTargets; ++i)
	{
		m_grid->Remove(p->p[i]);
	}
}

void ContentPropertyGrid::customEvent(QEvent *e)
{
	switch (e->type())
	{
	case EV_RebuildContentPropertyGrid:
		OnRebuildPropertyGrid();
		break;
	default:
		break;
	}
}

void ContentPropertyGrid::OnRebuildPropertyGrid()
{
	IdPropsMap map;
	
	QString selName;
	Property *p = m_grid->Selection();
	if (p)
	{
		selName = p->Name();
	}

	map.swap(m_map);
	Clear();

	// restore

	for (IdPropsMap::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		Add(it->first);
	}

	if (p)
	{
		QModelIndex index = m_grid->FindIndex(selName);
		if (index.isValid())
		{
			m_grid->SetSelection(index);
			m_grid->setFocus();
		}
	}
}

} // editor
} // tools

#include "moc_EditorContentPropertyGrid.cc"

