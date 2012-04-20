// EditorContentChoosePackageDialog.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include "../../../Packages/PackagesDef.h"
#include <QtGui/QDialog>
#include <Runtime/PushPack.h>

class QComboBox;

namespace tools {
namespace editor {

class ContentChoosePackageDialog : public QDialog
{
	Q_OBJECT
public:
	ContentChoosePackageDialog(
		const QString &caption,
		const QString &prompt,
		const QString &defaultPkg,
		QWidget *parent = 0
	);

	RAD_DECLARE_READONLY_PROPERTY(ContentChoosePackageDialog, package, const pkg::PackageRef&);

private slots:

	void SelectedPackageChanged(int index);

private:

	RAD_DECLARE_GET(package, const pkg::PackageRef&) { return m_selected; }

	QComboBox *m_packages;
	pkg::PackageRef m_selected;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
