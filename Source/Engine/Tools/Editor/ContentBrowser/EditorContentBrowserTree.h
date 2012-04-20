// EditorContentBrowserTree.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include "../../../Packages/Packages.h"
#include "EditorContentBrowserView.h"
#include <QtGui/QWidget>
#include <QtGui/QTreeView>
#include <Runtime/PushPack.h>

class QKeyEvent;

namespace tools {
namespace editor {

class ContentBrowserModel;
class RADENG_CLASS ContentBrowserTree : public QTreeView
{
	Q_OBJECT
public:

	ContentBrowserTree(
		bool editable,
		bool multiSelect,
		bool typesOnly,
		QWidget *parent = 0
	);

	void ExtractSelectionFilter(ContentBrowserView::Filter &filter);
	ContentBrowserModel *Model() const;

	bool SetSelection(const pkg::Package::Ref &pkg, asset::Type type);

public slots:

	void DeleteSelection();

signals:

	void OnSelectionChanged();

protected:

	virtual void keyPressEvent(QKeyEvent *e);
	virtual void selectionChanged(const QItemSelection&, const QItemSelection&);

private:

	void Delete(const pkg::PackageVec &pkgs, const pkg::IdVec &ids);

	bool m_editable;
};
	
} // editor
} // tools

#include <Runtime/PopPack.h>
