// EditorLineEditDialog.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorLineEditDialog.h"
#include "EditorUtils.h"
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>
#include <QtGui/QSpacerItem>
#include <QtGui/QPushButton>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>


namespace tools {
namespace editor {

LineEditDialog::LineEditDialog(
	const QString &caption, 
	const QString &prompt,
	const QString &edit,
	QWidget *parent
) : QDialog(parent)
{
	setWindowTitle(caption);
	QGridLayout *l = new QGridLayout(this);

	l->addItem(new QSpacerItem(0, 0), 0, 0);
	l->setRowStretch(0, 1);
	m_label = new QLabel(this);
	m_label->setAlignment(Qt::AlignCenter);
	m_label->setText(prompt);
	l->addWidget(m_label, 1, 0);
	m_line = new QLineEdit(this);
	m_line->setText(edit);
	m_line->selectAll();
	l->addWidget(m_line, 2, 0);
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

} // editor
} // tools

#include "moc_EditorLineEditDialog.cc"
