// EditorFilePathFieldWidget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorFilePathFieldWidget.h"
#include "../EditorMainWindow.h"
#include "../EditorUtils.h"
#include <Runtime/File.h>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>
#include <QtGui/QPainter>
#include <QtGui/QSizePolicy>
#include <QtGui/QMessageBox>

namespace tools {
namespace editor {

FilePathFieldWidget::FilePathFieldWidget(QWidget *parent)
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

void FilePathFieldWidget::SetPath(const QString &path)
{
	m_path = path;
	m_edit->setText(path);
	m_edit->selectAll();
}

void FilePathFieldWidget::SetPrefix(const QString &path)
{
	m_prefix = path;
	m_nativePrefix = path;

	if (path.isEmpty())
		return;

	char native[file::MaxFilePathLen+1];
	if (file::ExpandToNativePath(m_prefix.toAscii().constData(), native, file::MaxFilePathLen+1))
	{
		m_nativePrefix = native;
		for (int i = 0; i < m_nativePrefix.length(); ++i)
		{
			if (m_nativePrefix[i] == '\\')
				m_nativePrefix[i] = '/';
		}
	}
}

void FilePathFieldWidget::BrowseClicked()
{
	QFileDialog fd(m_edit);
	fd.setNameFilter(m_filter);

	// default to base directory.
	if (m_path.isEmpty())
	{
		fd.setDirectory(m_nativePrefix);
	}
	else
	{
		fd.selectFile(m_nativePrefix + '/' + m_path);
	}

	fd.setFileMode(QFileDialog::ExistingFile);
	
	while (fd.exec())
	{
		QString file = fd.selectedFiles().first();
		if (ValidatePath(file))
		{
			file = file.right(file.length()-m_nativePrefix.length());
			if (file[0] == '/' || file[0] == '\\')
				file = file.right(file.length()-1);
			SetPath(file);
			emit CloseEditor();
			break;
		}

		QMessageBox::critical(
			parentWidget(),
			"Invalid path",
			"Selected files must be located inside game directory."
		);
	}
}

void FilePathFieldWidget::EditLineFinished()
{
	m_path = m_edit->text();
}

bool FilePathFieldWidget::ValidatePath(const QString &s)
{
	if (m_prefix.isEmpty())
		return true;

	return s.startsWith(m_nativePrefix, Qt::CaseInsensitive);
}

void FilePathFieldWidget::focusInEvent(QFocusEvent*)
{
	m_edit->setFocus();
}

} // editor
} // tools

#include "moc_EditorFilePathFieldWidget.cc"
