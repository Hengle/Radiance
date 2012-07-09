// QtLogWindow.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorLogWindow.h"
#include "EditorMainWindow.h"
#include "EditorUtils.h"
#include "../../COut.h"
#include <QtGui/QTextCursor>
#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QAction>

namespace tools {
namespace editor {

LogWindow *LogWindow::s_instance = 0;
QMutex LogWindow::s_m;

LogWindow::LogWindow(QWidget *parent) :
QDialog(parent),
m_exitAfterDialog(false)
{
	resize(517, 353);
	m_textArea = new QPlainTextEdit(this);
    m_textArea->setUndoRedoEnabled(false);
    m_textArea->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_textArea->setReadOnly(true);

	setWindowFlags(
		Qt::Dialog|
		Qt::CustomizeWindowHint|
		Qt::WindowTitleHint
	);

	if (parent)
	{
		setModal(true);
		CenterWidget(*this, *parent);
	}

	s_instance = this;
}

LogWindow::~LogWindow()
{
	QMutexLocker L(&s_m);
	s_instance = 0;
}

void LogWindow::setVisible(bool visible)
{
	if (MainWindow::Get()->logWinShowHideAction.get())
		MainWindow::Get()->logWinShowHideAction->setChecked(visible);
	QDialog::setVisible(visible);
}

bool LogWindow::SPrint(int level, const char *msg)
{
	QMutexLocker l(&s_m);
	bool dialog = false;

	if (s_instance)
	{
		dialog = level == C_ErrMsgBox;
		if (dialog && !MainWindow::Get()->run)
		{
			dialog = false;
			level = C_Error;
		}
		s_instance->Print(level, msg);
	}

	return dialog;
}

void LogWindow::Print(int level, const char *msg)
{
	QCoreApplication::postEvent(this, new (ZEditor) PrintMsgEvent(level, msg));
}

void LogWindow::EnableCloseButton()
{
	hide();
	setWindowFlags(
		Qt::Dialog|
		Qt::CustomizeWindowHint|
		Qt::WindowCloseButtonHint|
		Qt::WindowMaximizeButtonHint
	);
	setModal(false);
	setParent(0);
	show();
}

void LogWindow::ExitAfterDialog()
{
	m_exitAfterDialog = true;
}

void LogWindow::resizeEvent(QResizeEvent *e)
{
	m_textArea->resize(e->size());
}

void LogWindow::customEvent(QEvent *e)
{
	switch (e->type())
	{
	case EV_LogWindowPrint:
		OnPrintMsg(*static_cast<PrintMsgEvent*>(e));
		break;
	}
}

void LogWindow::OnPrintMsg(const PrintMsgEvent &msg)
{
	QTextCursor c = m_textArea->textCursor();
	c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
	m_textArea->setTextCursor(c);
	m_textArea->insertPlainText(msg.msg.c_str.get());

	if (msg.level == C_ErrMsgBox)
	{
		QMessageBox::critical(
			MainWindow::Get(),
			"Error",
			msg.msg.c_str.get()
		);

		if (m_exitAfterDialog)
		{
			App::Get()->exit = true;
		}
	}
}

} // editor
} // tools

#include "moc_EditorLogWindow.cc"
