// EditorContentBrowser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorContentBrowserWindow.h"
#include "EditorContentBrowserTree.h"
#include "EditorContentBrowserView.h"
#include "EditorContentBrowserModel.h"
#include "EditorContentPropertyGrid.h"
#include "EditorContentChoosePackageDialog.h"
#include "../EditorLineEditDialog.h"
#include "../EditorUtils.h"
#include "../../../Assets/AssetTypes.h"
#include "EditorContentTexture.h"
#include <QtGui/QSplitter>
#include <QtGui/QWidget>
#include <QtGui/QSizePolicy>
#include <QtGui/QGridLayout>
#include <QtGui/QResizeEvent>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QLineEdit>
#include <QtGui/QSpacerItem>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>

namespace tools {
namespace editor {

ContentBrowserWindow::ContentBrowserWindow(
	WidgetStyle style, 
	bool editable, 
	bool multiSelect,
	QWidget *parent
) : QDialog(parent, WindowTypeForStyle(style)),
m_delButton(0), m_editable(editable), m_style(style)
{
	setWindowTitle("Content Browser");
	m_types.set();

	if (style != WS_Widget)
	{
		setWindowFlags(
			Qt::Window|
			Qt::CustomizeWindowHint|
			Qt::WindowTitleHint|
			Qt::WindowSystemMenuHint|
			Qt::WindowCloseButtonHint|
			Qt::WindowMinMaxButtonsHint
		);
	}

	int left, right;
	right = (int)(this->width()*0.75f);
	left  = this->width() - right;

	QWidget *w = new (ZEditor) QWidget(this);
	QGridLayout *l = new (ZEditor) QGridLayout(w);

	QSplitter *s = new (ZEditor) QSplitter(w);
	s->setOpaqueResize(false);
	s->setOrientation(Qt::Vertical);

	{
		QWidget *w = new (ZEditor) QWidget();
		QGridLayout *l = new (ZEditor) QGridLayout(w);

		QPushButton *b = new (ZEditor) QPushButton(LoadIcon("Editor/add2_small.png"), "Add Package");
		b->setEnabled(editable);
		l->addWidget(b, 0, 0);
		l->addItem(new (ZEditor) QSpacerItem(0, 0), 0, 1);
		l->setColumnStretch(1, 1);
		RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(AddPackage())));
		b = new (ZEditor) QPushButton(LoadIcon("Editor/delete2_small.png"), "Delete Package(s)");
		b->setEnabled(editable);
		RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(DeletePackage())));
		m_delButton = b;
		b->setEnabled(false);
		l->addWidget(b, 1, 0);
		l->addItem(new QSpacerItem(0, 0), 1, 1);
		
		m_tree = new (ZEditor) ContentBrowserTree(
			editable,
			multiSelect,
			true
		);

		m_tree->resize(left, m_tree->height());
		m_tree->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

		l->addWidget(m_tree, 2, 0, 1, 2);
		l->setRowStretch(2, 1);

		w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
		s->addWidget(w);
	}

	m_grid = new (ZEditor) ContentPropertyGrid(pkg::P_AllTargets, editable);
	m_grid->resize(left, m_grid->height());
	m_grid->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	s->addWidget(m_grid);
	l->addWidget(s, 0, 0);

	w = s;

	l = new (ZEditor) QGridLayout(this);
	s = new (ZEditor) QSplitter(this);
	s->setOpaqueResize(false);

	s->addWidget(w);

	m_view = new (ZEditor) ContentBrowserView(
		true,
		editable,
		(style == WS_Dialog),
		multiSelect ? ContentBrowserView::SelMulti : ContentBrowserView::SelSingle
	);

	m_view->resize(right, m_view->height());
	
	s->addWidget(m_view);

	l->addWidget(s, 0, 0);

	m_okButton = 0;

	if (style == WS_Dialog)
	{
		QGridLayout *bL = new (ZEditor) QGridLayout(); // for buttons
		bL->addItem(new (ZEditor) QSpacerItem(0, 0), 0, 0);
		bL->setColumnStretch(0, 1);
		QPushButton *b = new (ZEditor) QPushButton("OK");
		b->setEnabled(false);
		m_okButton = b;
		bL->addWidget(b, 0, 1);
		RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(accept())));
		b = new (ZEditor) QPushButton("Cancel");
		bL->addWidget(b, 0, 2);
		RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(reject())));
		bL->addItem(new (ZEditor) QSpacerItem(0, 0), 0, 3);
		bL->setColumnStretch(3, 1);

		l->addLayout(bL, 1, 0);

		RAD_VERIFY(connect(m_view, SIGNAL(OnItemDoubleClicked(int, bool&)), SLOT(OnViewItemDoubleClicked(int, bool&))));
	}

	RAD_VERIFY(connect(m_tree, SIGNAL(OnSelectionChanged()), SLOT(OnTreeSelectionChanged())));
	RAD_VERIFY(connect(m_tree, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(TreeItemDoubleClicked(const QModelIndex&))));
	RAD_VERIFY(connect(m_view, SIGNAL(OnSelectionChanged(const SelSet&, const SelSet&, const SelSet&)), SLOT(OnViewSelectionChanged(const SelSet&, const SelSet&, const SelSet&))));
	RAD_VERIFY(connect(m_view, SIGNAL(OnCloneSelection(const SelSet&)), SLOT(OnCloneSelection(const SelSet&))));
	RAD_VERIFY(connect(m_view, SIGNAL(OnMoveSelection(const SelSet&)), SLOT(OnMoveSelection(const SelSet&))));
	m_view->BuildAssetList();
}

void ContentBrowserWindow::OnTreeSelectionChanged()
{
	UpdateFilter(true);
}

void ContentBrowserWindow::UpdateFilter(bool redraw)
{
	ContentBrowserView::Filter *f = m_view->filter;

	m_tree->ExtractSelectionFilter(*f);

	for (int i = 0; i < asset::AT_Max; ++i)
	{
		f->types.set(i, f->types[i]&&m_types[i]);
	}

	m_view->UpdateFilter(redraw);
	m_delButton->setEnabled(!f->pkgs.empty() && m_editable);
}

void ContentBrowserWindow::OnViewSelectionChanged(
	const SelSet &sel,   // what was selected (if any) 
	const SelSet &unsel, // what was deselected
	const SelSet &total  // total selection
)
{
	for (ContentBrowserView::SelSet::const_iterator it = unsel.begin(); it != unsel.end(); ++it)
	{
		m_grid->Remove(*it);
	}

	for (ContentBrowserView::SelSet::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		m_grid->Add(*it);
	}

	if (m_okButton)
		m_okButton->setEnabled(!total.empty());
}

pkg::IdVec ContentBrowserWindow::CreateAsset(asset::Type type, const pkg::Package::Ref &pkg, pkg::IdVec &sel)
{
	if (type == asset::AT_Texture)
		return content_property_details::CreateTextures(this, pkg, sel);

	if (type == asset::AT_Map ||
		type == asset::AT_SkAnimSet ||
		type == asset::AT_SkAnimStates ||
		type == asset::AT_Mesh ||
		type == asset::AT_Sound ||
		type == asset::AT_Music ||
		type == asset::AT_Font)
	{
		// all these types have a Source.File so we can create them
		// using the generics method.
		return GenericImportAssetFiles(type, pkg, sel);
	}

	// All other types must be created by user.

	pkg::IdVec ids;
	QString typeString(asset::TypeString(type));

	LineEditDialog d(QString("New ") + typeString, "Name", "[Enter Name]", this);

	while (d.exec())
	{
		d.LineEdit()->selectAll();

		if (d.LineEdit()->text() == "[Enter Name]")
			continue;

		pkg::Package::Entry::Ref entry = pkg->CreateEntry(
			d.LineEdit()->text().toAscii().constData(),
			type
		);

		if (entry)
		{
			entry->UpdateModifiedTime();
			ids.push_back(entry->id);
			sel.push_back(entry->id);
			break;
		}

		QMessageBox::critical(
			this,
			"Error",
			"An asset with that name already exists!"
		);
	}

	return ids;
}

pkg::IdVec ContentBrowserWindow::GenericImportAssetFiles(asset::Type type, const pkg::Package::Ref &pkg, pkg::IdVec &sel)
{
	pkg::IdVec ids;
	QFileDialog fd(this);

	QString typeString(asset::TypeString(type));
	if (!typeString.endsWith('s', Qt::CaseInsensitive))
		typeString += "s";

	fd.setWindowTitle(QString("Import ") + typeString);
	
	QString nativePrefix;
	
	{
		nativePrefix = QString("9:/") + QString(Files()->hddRoot.get());
		char native[file::MaxFilePathLen+1];
		if (!file::ExpandToNativePath(nativePrefix.toAscii().constData(), native, file::MaxFilePathLen+1))
			return ids;
		nativePrefix = native;
		for (int i = 0; i < nativePrefix.length(); ++i)
		{
			if (nativePrefix[i] == '\\')
				nativePrefix[i] = '/';
		}
	}

	QString subDir;

	switch (type)
	{
	case asset::AT_Map:
		subDir = "/Maps";
		break;
	case asset::AT_SkAnimSet:
	case asset::AT_SkAnimStates:
	case asset::AT_Mesh:
		subDir = "/Meshes";
		break;
	case asset::AT_Sound:
	case asset::AT_Music:
		subDir = "/Audio";
		break;
	case asset::AT_Font:
		subDir = "/Fonts";
		break;
	default:
		break;
	}

	fd.setDirectory(nativePrefix + subDir);
	fd.setNameFilter("All Files (*.*)");
	fd.setFileMode(QFileDialog::ExistingFiles);

	// lookup file filter from asset definition.
	pkg::KeyDef::MapRef defs = Packages()->KeyDefsForType(type);
	pkg::KeyDef::Map::const_iterator it = defs->find(String("Source.File"));
	if (it != defs->end())
	{
		pkg::KeyDef::Pair::Map::const_iterator it2 = it->second->pairs.find(String("filter"));
		if (it2 != it->second->pairs.end())
		{
			const String *s = static_cast<const String*>(it2->second.val);
			if (s)
				fd.setNameFilter(s->c_str.get());
		}
	}
	
	bool validated = false;
	QStringList files;

	while (fd.exec())
	{
		files = fd.selectedFiles();

		validated = true;

		foreach(QString s, files)
		{
			validated = s.startsWith(nativePrefix, Qt::CaseInsensitive);
			if (!validated)
				break;
		}

		if (validated)
			break;

		validated = false;

		QMessageBox::critical(
			this,
			"Invalid path",
			"Selected files must be located inside game directory."
		);
	}

	if (!validated)
		return ids;

	bool autoRename = false;
	bool hasConflicts = false;

	{
		QString conflictsMessage;

		foreach(QString path, files)
		{
			QFileInfo fileInfo(path);
			QString name = fileInfo.baseName();
			QString matName = name + "_M";

			if (pkg->FindEntry(name.toAscii().constData()))
			{
				if (!conflictsMessage.isEmpty())
					conflictsMessage += QString(RAD_NEWLINE)+name;
				else
					conflictsMessage = name;
			}		
		}

		if (!conflictsMessage.isEmpty())
		{
			QString m = QString("One or more files selected for import conflict with assets in this package. Would you like to automatically rename the conflicting items during import?"RAD_NEWLINE""RAD_NEWLINE"The following items have conflicts:"RAD_NEWLINE"%1").arg(conflictsMessage);
			QMessageBox::StandardButton b = QMessageBox::warning(
				this,
				"Conflicts",
				m,
				QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
			);

			if (b == QMessageBox::Cancel)
				return ids; // user wants to abort due to conflicts

			hasConflicts = true;
			autoRename = b == QMessageBox::Yes;
		}
	}

	// Create each asset.

	foreach(QString path, files)
	{
		QFileInfo fileInfo(path);
		QString name = fileInfo.baseName();
		
		pkg::Package::Entry::Ref item;

		for (int i = 2;; ++i)
		{
			item = pkg->CreateEntry(
				name.toAscii().constData(),
				type
			);

			if (item)
			{
				ids.push_back(item->id);
				sel.push_back(item->id);
				break;
			}

			if (!autoRename)
				break;

			name = QString("%1_%2").arg(fileInfo.baseName()).arg(i);
		}

		if (!item)
			continue;

		// Set Source.File input
		pkg::KeyDef::Ref def = item->FindKeyDef(String(), String("Source.File"));
		if (def)
		{
			pkg::KeyVal::Ref key = def->CreateKey(0);
			item->AddKey(key, true);

			String *s = static_cast<String*>(key->val);
			if (s)
				*s = path.right(path.length()-nativePrefix.length()-1).toAscii().constData();
		}

		item->UpdateModifiedTime();
	}

	if (hasConflicts && !autoRename && !ids.empty())
		QMessageBox::information(this, "Warning", "Some selected files were not imported due to conflicts.");

	return ids;
}

void ContentBrowserWindow::TreeItemDoubleClicked(const QModelIndex &index)
{
	if (!m_tree->Model()->IsPackageType(index))
		return;
	
	asset::Type type;
	pkg::Package::Ref pkg = m_tree->Model()->TypeForIndex(index, type);
	
	if (!pkg || type >= asset::AT_Max)
		return;

	pkg::IdVec sel;
	pkg::IdVec add = CreateAsset(type, pkg, sel);
	if (!add.empty())
	{
		Packages()->SavePackages(add);
		NotifyAddRemoveContent(add, pkg::IdVec());
		m_view->Select(sel, true, false);
		if (!FilterContent(sel) && !m_view->ScrollToSelection())
			m_view->Update(); // stim redraw if scroll didn't move.
	}
}

void ContentBrowserWindow::AddPackage()
{
	LineEditDialog d("New Package", "Name", "[Enter Name]", this);

	while (d.exec())
	{
		if (d.LineEdit()->text() == "[Enter Name]")
			continue;

		pkg::Package::Ref pkg = Packages()->CreatePackage(
			d.LineEdit()->text().toAscii().constData()
		);

		if (pkg)
		{
			pkg->Save();
			NotifyAddRemovePackages();
			break;
		}

		QMessageBox::critical(
			this,
			"Error",
			"A package with that name already exists!"
		);
	}
}

void ContentBrowserWindow::DeletePackage()
{
	m_tree->DeleteSelection();
}

void ContentBrowserWindow::OnCloneSelection(const SelSet &sel)
{
	ContentChoosePackageDialog d("Clone Selection", "Destination Package:", m_clonePkgName, this);

	if (!d.exec() || !d.package.get())
		return;

	pkg::Package::Ref pkg = d.package;
	if (!pkg)
		return;
	m_clonePkgName = pkg->name.get();

	pkg::IdVec ids;
	ids.reserve(sel.size());

	for (ContentBrowserView::SelSet::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		pkg::Package::Entry::Ref entry = Packages()->FindEntry(*it);
		if (!entry)
			continue;

		QString root(entry->name.get());
		QString name(root);

		for (int i = 2;; ++i)
		{
			pkg::Package::Entry::Ref clone = pkg->Clone(entry, name.toAscii().constData());
			if (clone)
			{
				ids.push_back(clone->id);
				break;
			}

			name = QString("%1_%2").arg(root).arg(i);
		}
	}

	if (!ids.empty())
	{
		Packages()->SavePackages(ids);
		NotifyAddRemoveContent(ids, pkg::IdVec());
		m_view->Select(ids, true, false);
		if (!FilterContent(ids) && !m_view->ScrollToSelection())
			m_view->Update(); // stim redraw if scroll didn't move.
	}
}

void ContentBrowserWindow::OnMoveSelection(const SelSet &sel)
{
	ContentChoosePackageDialog d("Move Selection", "Destination Package:", m_clonePkgName, this);

	if (!d.exec() || !d.package.get())
		return;

	pkg::Package::Ref pkg = d.package;
	if (!pkg)
		return;
	m_clonePkgName = pkg->name.get();

	pkg::IdVec add;
	pkg::IdVec del;

	add.reserve(sel.size());
	del.reserve(sel.size());

	// make sure there are no conflicts.
	for (ContentBrowserView::SelSet::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		pkg::Package::Entry::Ref entry = Packages()->FindEntry(*it);
		if (!entry)
			continue;

		if (pkg->FindEntry(entry->name))
		{
			QMessageBox::critical(
				this,
				"Error",
				"The target package contains assets with conflicting names, move aborted."
			);
			return;
		}
	}

	for (ContentBrowserView::SelSet::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		pkg::Package::Entry::Ref entry = Packages()->FindEntry(*it);
		if (!entry)
			continue;

		pkg::Package::Entry::Ref clone = pkg->Clone(entry, entry->name);
		if (clone)
		{
			add.push_back(clone->id);
			del.push_back(*it);
			Packages()->Delete(*it);
		}
	}

	if (!add.empty())
	{
		Packages()->SavePackages(add);
		Packages()->SavePackages(del);
		NotifyAddRemoveContent(add, del);
		m_view->Select(add, true, false);
		if (!FilterContent(add) && !m_view->ScrollToSelection())
			m_view->Update(); // stim redraw if scroll didn't move.
	}
}

void ContentBrowserWindow::OnViewItemDoubleClicked(int id, bool &openEditor) {
	openEditor = false;
	accept();
}

bool ContentBrowserWindow::FilterContent(const pkg::IdVec &ids)
{
	if (ids.empty())
		return false;

	// select tightest possible filter
	asset::Type type = asset::AT_Max;
	pkg::Package::Ref pkg;

	pkg::IdVec::const_iterator it = ids.begin();
	pkg::Package::Entry::Ref entry = Packages()->FindEntry(*it);
	if (!entry)
		return false;

	pkg = entry->pkg;
	type = entry->type;

	for (++it; it != ids.end(); ++it)
	{
		entry = Packages()->FindEntry(*it);
		if (!entry)
			continue;

		if (type != asset::AT_Max)
		{
			if (type != entry->type)
				type = asset::AT_Max;
		}

		if (pkg)
		{
			if (pkg.get() != entry->pkg.get().get())
				pkg.reset();
		}
	}

	return m_tree->SetSelection(pkg, type);
}

void ContentBrowserWindow::keyPressEvent(QKeyEvent *e)
{
	if (m_style == WS_Dialog || e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}

void ContentBrowserWindow::NotifyAddRemovePackages()
{
	ContentBrowserModel::NotifyAddRemovePackages();
	ContentBrowserView::NotifyAddRemovePackages();
}

void ContentBrowserWindow::NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed)
{
	ContentBrowserModel::NotifyAddRemoveContent(added, removed);
	ContentBrowserView::NotifyAddRemoveContent(added, removed);
}

void ContentBrowserWindow::NotifyContentChanged(const ContentChange::Vec &changed)
{
	pkg::IdVec ids;
	foreach(const ContentChange &c, changed)
		ids.push_back(c.entry->id);

	Packages()->SavePackages(ids);

	ContentBrowserModel::NotifyContentChanged(changed);
	ContentBrowserView::NotifyContentChanged(changed);
}

} // editor
} // tools

#include "moc_EditorContentBrowserWindow.cc"
