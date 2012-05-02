// EditorContentBrowserWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include "EditorContentBrowserDef.h"
#include "EditorContentBrowserView.h"
#include "../../../Packages/Packages.h"
#include <QtGui/QDialog>
#include <QtCore/QModelIndex>
#include <Runtime/PushPack.h>

class QResizeEvent;
class QPushButton;
class QKeyEvent;

namespace tools {
namespace editor {

class ContentBrowserTree;
class ContentBrowserView;
class ContentPropertyGrid;

class RADENG_CLASS ContentBrowserWindow : public QDialog
{
	Q_OBJECT
public:

	typedef ContentBrowserView::SelSet SelSet;

	ContentBrowserWindow(
        WidgetStyle style,
        bool editable,
        bool multiSelect,
		QWidget *parent = 0
	);

	void UpdateFilter(bool redraw=true);
	bool FilterContent(const pkg::IdVec &ids);

	RAD_DECLARE_READONLY_PROPERTY(ContentBrowserWindow, selection, const SelSet&);
	RAD_DECLARE_READONLY_PROPERTY(ContentBrowserWindow, view, ContentBrowserView*);
	RAD_DECLARE_READONLY_PROPERTY(ContentBrowserWindow, typeFilter, asset::TypeBits*);

	static void NotifyAddRemovePackages();
	static void NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed);
	static void NotifyContentChanged(const ContentChange::Vec &changed);

private slots:

	void OnTreeSelectionChanged();
	void OnViewSelectionChanged(
		const SelSet &sel,   // what was selected (if any) 
		const SelSet &unsel, // what was deselected
		const SelSet &total  // total selection
	);

	void TreeItemDoubleClicked(const QModelIndex &index);
	void AddPackage();
	void DeletePackage();
	void OnCloneSelection(const SelSet &sel);
	void OnMoveSelection(const SelSet &sel);
	void OnViewItemDoubleClicked(int id, bool &openEditor);

private:

	RAD_DECLARE_GET(selection, const SelSet&) { return m_view->selection; }
	RAD_DECLARE_GET(view, ContentBrowserView*) { return m_view; }
	RAD_DECLARE_GET(typeFilter, asset::TypeBits*) { return &const_cast<ContentBrowserWindow*>(this)->m_types; }

	virtual void keyPressEvent(QKeyEvent *e);
	pkg::IdVec CreateAsset(asset::Type type, const pkg::Package::Ref &pkg, pkg::IdVec &sel);
	pkg::IdVec GenericImportAssetFiles(asset::Type type, const pkg::Package::Ref &pkg, pkg::IdVec &sel);
	
	WidgetStyle          m_style;
	ContentBrowserView  *m_view;
	ContentBrowserTree  *m_tree;
	ContentPropertyGrid *m_grid;
	QPushButton         *m_delButton;
	QPushButton         *m_okButton;
	asset::TypeBits      m_types;
	QString              m_clonePkgName;
	bool                 m_editable;
};
	
} // editor
} // tools

#include <Runtime/PopPack.h>
