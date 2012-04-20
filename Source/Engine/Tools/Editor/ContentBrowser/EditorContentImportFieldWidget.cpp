// EditorContentImportFieldWidget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorContentImportFieldWidget.h"
#include "EditorContentBrowser.h"
#include "../EditorUtils.h"
#include "../EditorMainWindow.h"
#include "../../../Assets/AssetTypes.h"
#include <QtGui/QHBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSizePolicy>

namespace tools {
namespace editor {
namespace content_property_details {

ContentImportFieldWidget::ContentImportFieldWidget(QWidget *parent)
: QWidget(parent)
{
	QHBoxLayout *l = new QHBoxLayout(this);
	l->setContentsMargins(0, 0, 0, 0);
	m_edit = new QLineEdit();
	l->addWidget(m_edit, 1);
	QHBoxLayout *l2 = new QHBoxLayout();
	QPushButton *b = new QPushButton("...");
	b->setMinimumWidth(30);
	b->setMaximumWidth(30);
	b->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
	l2->addWidget(b, 0);
	l->addLayout(l2, 0);
	l->setSpacing(0);
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(BrowseClicked())));
	RAD_VERIFY(connect(m_edit, SIGNAL(editingFinished()), SLOT(EditLineFinished())));
}

void ContentImportFieldWidget::SetPath(const QString &path)
{
	m_path = path;
	m_edit->setText(path);
	m_edit->selectAll();
}

void ContentImportFieldWidget::BrowseClicked()
{
	ContentBrowser b(ContentBrowser::S_Dialog, false, false, m_edit);
	asset::TypeBits *typeFilter = b.typeFilter;

	PercentSize(b, *MainWindow::Get(), 0.85f, 0.80f);
	CenterWidget(b, *MainWindow::Get());

	if (m_filter.isEmpty())
	{
		typeFilter->set();
	}
	else
	{
		QStringList values = m_filter.split(';', QString::SkipEmptyParts);
		typeFilter->reset();
		foreach(QString v, values)
		{
			asset::Type type = asset::TypeFromName(v.toAscii().constData());
			if (type != asset::AT_Max)
				typeFilter->set(type);
		}
	}

	b.UpdateFilter(false);
	
	if (!m_path.isEmpty())
	{
		int id = Packages()->ResolveId(m_path.toAscii().constData(), 0);
		if (id != -1)
		{
			b.view->Select(id, true, false);
			b.view->ScrollToSelection();
		}
	}

	if (b.exec())
	{
		const ContentBrowser::SelSet &sel = b.selection;
		if (!sel.empty())
		{
			pkg::Package::Entry::Ref ref = Packages()->FindEntry(*sel.begin());
			if (ref)
				SetPath(ref->path.get());
		}
	}

	emit CloseEditor();
}

void ContentImportFieldWidget::EditLineFinished()
{
	m_path = m_edit->text();
}

void ContentImportFieldWidget::focusInEvent(QFocusEvent*)
{
	m_edit->setFocus();
}

} // content_property_details
} // editor
} // tools

#include "moc_EditorContentImportFieldWidget.cc"
