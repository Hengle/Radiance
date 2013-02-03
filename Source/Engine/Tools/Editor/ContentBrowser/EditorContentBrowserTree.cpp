// EditorContentBrowserTree.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorContentBrowserTree.h"
#include "EditorContentBrowserModel.h"
#include "EditorContentBrowserWindow.h"
#include "../EditorUtils.h"

#include <QtGui/QTreeView>
#include <QtGui/QMessageBox>

class QItemSelection;

namespace tools {
namespace editor {

ContentBrowserTree::ContentBrowserTree(
	bool editable,
	bool multiSelect,
	bool typesOnly,
	QWidget *parent
) : QTreeView(parent), m_editable(editable)
{
	setModel(new ContentBrowserModel(typesOnly, this));
	setHeaderHidden(true);
	setIndentation(10);
	setEditTriggers(
		editable ? 
		QAbstractItemView::SelectedClicked :
		QAbstractItemView::NoEditTriggers
	);
	setSelectionBehavior(QAbstractItemView::SelectItems);
	setSelectionMode(
		multiSelect ?
		QAbstractItemView::ExtendedSelection :
		QAbstractItemView::SingleSelection
	);
}

ContentBrowserModel *ContentBrowserTree::Model() const
{
	return static_cast<ContentBrowserModel*>(model());
}

bool ContentBrowserTree::SetSelection(const pkg::Package::Ref &pkg, asset::Type type)
{
	ContentBrowserModel *m = Model();

	QModelIndex index;

	if (pkg && (type != asset::AT_Max))
	{
		index = m->IndexForPackageType(pkg, type);
	}
	else if (pkg)
	{
		index = m->IndexForPackage(pkg);
	}
	else if (type != asset::AT_Max)
	{
		index = m->IndexForType(type);
	}

	if (index.isValid())
	{
		QModelIndexList sel = selectedIndexes();

		if ((sel.empty() || sel.count() > 1) || (sel.front() != index))
		{
			selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
			return true;
		}
	}
	else
	{
		QModelIndexList sel = selectedIndexes();
		if (!sel.empty())
		{
			clearSelection();
			return true;
		}
	}

	return false;
}

void ContentBrowserTree::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Delete && m_editable)
    {
        DeleteSelection();
    }
	else if (e->key() == Qt::Key_Escape)
	{
		clearSelection();
		parentWidget()->setFocus();
	}

	QTreeView::keyPressEvent(e);
}

void ContentBrowserTree::ExtractSelectionFilter(ContentBrowserView::Filter &filter)
{
	QModelIndexList sel = selectedIndexes();
	if (sel.empty())
	{
		filter.pkgs.clear();
		filter.assets.clear();
		filter.types.set();
		return;
	}

	typedef zone_vector<bool, ZEditorT>::type BoolVec;

	ContentBrowserView::Filter::Package::Vec pkgs;
	BoolVec touched;
	pkg::IdVec ids;

	filter.types.reset();

	// gather types
	for (QModelIndexList::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		if (Model()->IsType(*it))
		{
			filter.types[Model()->TypeForIndex(*it)] = 1;
		}
	}

	// gather packages
	for (QModelIndexList::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		if (Model()->IsPackage(*it))
		{
			// note Package constructor defaults to all types.
			pkgs.push_back(ContentBrowserView::Filter::Package(Model()->PackageForIndex(*it)));
			touched.push_back(false);
		}
	}

	// gather types
	for (QModelIndexList::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		if (Model()->IsPackageType(*it))
		{
			asset::Type type;
			pkg::Package::Ref pkg = Model()->TypeForIndex(*it, type);
			if (pkg)
			{ // mark types
			  // note unsets all types if first touch
				int c = 0;
				ContentBrowserView::Filter::Package::Vec::iterator p = pkgs.begin();
				for (; p != pkgs.end(); ++p, ++c)
				{
					if ((*p).pkg.get() == pkg.get())
					{
						if (!touched[c])
						{
							(*p).types.reset();
							touched[c] = true;
						}
						(*p).types[type] = true;
						break;
					}
				}
				if (p == pkgs.end())
				{ // package not selected, add it
					pkgs.push_back(ContentBrowserView::Filter::Package(pkg));
					touched.push_back(true);
					pkgs[pkgs.size()-1].types.reset();
					pkgs[pkgs.size()-1].types[type] = true;
				}
			}
		}
	}

	// gather assets.
	// only add the ones that aren't going to be contained
	// in a selected package.

	for (QModelIndexList::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		if (Model()->IsAsset(*it))
		{
			int id = Model()->IdForIndex(*it);
			pkg::Package::Entry::Ref entry;

			entry = Packages()->FindEntry(id);
			if (entry)
				continue;

			pkg::Package::Ref pkg = entry->pkg;
			if (!pkg) // dead?
				continue;

			ContentBrowserView::Filter::Package::Vec::const_iterator x = pkgs.begin();
			for (; x != pkgs.end(); ++x)
			{
				if ((*x).pkg.get() == pkg.get() &&
					(*x).types[entry->type])
				{
					break;
				}
			}

			if (x == pkgs.end())
			{
				ids.push_back(id); // not contained in a selected package.
			}
		}
	}

	if (filter.types.none())
		filter.types.set(); // no types selected, the enable all types.

	filter.pkgs.swap(pkgs);
	filter.assets.swap(ids);
}

void ContentBrowserTree::DeleteSelection()
{
	QModelIndexList sel = selectedIndexes();
	pkg::PackageVec pkgs;
	pkg::IdVec ids;

	// gather packages
	for (QModelIndexList::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		if (Model()->IsPackage(*it))
		{
			pkgs.push_back(Model()->PackageForIndex(*it));
		}
	}

	// gather assets.
	for (QModelIndexList::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		if (Model()->IsAsset(*it))
		{
			int id = Model()->IdForIndex(*it);

			pkg::PackageVec::const_iterator x = pkgs.begin();
			for (; x != pkgs.end(); ++x)
			{
				if ((*x)->Contains(id)) break;
			}

			if (x == pkgs.end())
			{
				ids.push_back(id); // not contained in a selected package.
			}
		}
	}

	if (!pkgs.empty() || !ids.empty())
	{
		Delete(pkgs, ids);
		emit OnSelectionChanged();
	}
}

void ContentBrowserTree::Delete(const pkg::PackageVec &pkgs, const pkg::IdVec &ids)
{
	enum { MaxItems = 10 };
	String msg;
	msg.Printf("Are you sure you want to delete the following %d item(s)?" RAD_NEWLINE, pkgs.size()+ids.size());

	if (!pkgs.empty())
	{
		String x;
		int c = 0;
		pkg::PackageVec::const_iterator it = pkgs.begin();

		for (; it != pkgs.end() && c < MaxItems; ++it, ++c)
		{
			if (!x.empty)
			{
				x += RAD_NEWLINE;
			}
			x += (*it)->name;
		}

		if (it != pkgs.end())
		{
			x += "...";
		}

		msg += "Packages:" RAD_NEWLINE+x+RAD_NEWLINE;
	}

	if (!ids.empty())
	{
		String x;
		int c = 0;
		pkg::IdVec::const_iterator it = ids.begin();

		for (; it != ids.end() && c < MaxItems; ++it, ++c)
		{
			if (!x.empty)
			{
				x += RAD_NEWLINE;
			}
			pkg::Package::Entry::Ref r = Packages()->FindEntry(*it);
			if (r)
			{
				x += r->name;
			}
		}

		if (it != ids.end())
		{
			x += "...";
		}

		msg += "Assets:" RAD_NEWLINE+x;
	}

	if (QMessageBox::question(
		parentWidget(),
		"Confirmation",
		msg.c_str.get(),
		QMessageBox::Yes|QMessageBox::No, 
		QMessageBox::No
	) != QMessageBox::Yes )
	{
		return;
	}

	for (pkg::IdVec::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		Packages()->Delete(*it);
	}

	if (!ids.empty())
		ContentBrowserWindow::NotifyAddRemoveContent(pkg::IdVec(), ids);

	for (pkg::PackageVec::const_iterator it = pkgs.begin(); it != pkgs.end(); ++it)
	{
		(*it)->Delete();
	}

	if (!pkgs.empty())
		ContentBrowserWindow::NotifyAddRemovePackages();
}

void ContentBrowserTree::selectionChanged(const QItemSelection&, const QItemSelection&)
{
	emit OnSelectionChanged();
}

} // editor
} // tools

#include "moc_EditorContentBrowserTree.cc"
