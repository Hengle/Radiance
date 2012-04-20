// EditorContentChoosePackageDialog.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorContentChoosePackageDialog.h"
#include "../EditorUtils.h"
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>
#include <QtGui/QSpacerItem>
#include <QtGui/QPushButton>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtCore/QVariant>

using namespace pkg;

namespace tools {
namespace editor {

ContentChoosePackageDialog::ContentChoosePackageDialog(
	const QString &caption, 
	const QString &prompt,
	const QString &defaultPkg, 
	QWidget *parent
) : QDialog(parent)
{
	setWindowTitle(caption);

	QGridLayout *l = new QGridLayout(this);

	l->addItem(new QSpacerItem(0, 0), 0, 0);
	l->setRowStretch(0, 1);
	QLabel *label = new (ZEditor) QLabel(this);
	label->setAlignment(Qt::AlignCenter);
	label->setText(prompt);
	l->addWidget(label, 1, 0);
	
	QIcon packageIcon = LoadIcon(L"Editor/package_tiny.png");
	m_packages = new (ZEditor) QComboBox(this);
	RAD_VERIFY(connect(m_packages, SIGNAL(currentIndexChanged(int)), SLOT(SelectedPackageChanged(int))));

	int sel = 0;
	int cur = 0;
	const Package::Map &packages = Packages()->packages;
	for (Package::Map::const_iterator it = packages.begin(); it != packages.end(); ++it, ++cur)
	{
		const Package::Ref &package = it->second;
		m_packages->addItem(
			packageIcon, 
			package->name.get(), 
			qVariantFromValue(reinterpret_cast<void*>(const_cast<Package::Ref*>(&package)))
		);

		if (defaultPkg == package->name.get())
			sel = cur;
	}

	m_packages->setCurrentIndex(sel);
	l->addWidget(m_packages, 2, 0);
	l->addItem(new QSpacerItem(0, 0), 3, 0);
	l->setRowStretch(3, 1);

	QGridLayout *l2 = new QGridLayout();
	l2->addItem(new QSpacerItem(0, 0), 0, 0);
	l2->setColumnStretch(0, 1);
	QPushButton *b = new QPushButton("Ok", this);
	b->setDefault(true);
	l2->addWidget(b, 0, 1);
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(accept())));
	b = new QPushButton("Cancel", this);
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(reject())));
	l2->addWidget(b, 0, 2);
	l2->addItem(new QSpacerItem(0, 0), 0, 3);
	l2->setColumnStretch(3, 1);
	l->addLayout(l2, 4, 0);

	PercentSize(*this, QApplication::desktop()->screenGeometry(), 0.15f, 0.15f);
	if (parent)
		CenterWidget(*this, *parent);
}

void ContentChoosePackageDialog::SelectedPackageChanged(int index)
{
	if (index == -1)
	{
		m_selected.reset();
		return;
	}

	m_selected = *reinterpret_cast<Package::Ref*>(m_packages->itemData(index).value<void*>());
}

} // editor
} // tools

#include "moc_EditorContentChoosePackageDialog.cc"
