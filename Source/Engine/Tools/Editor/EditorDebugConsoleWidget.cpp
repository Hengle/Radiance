/*! \file EditorDebugConsoleWidget.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#include RADPCH
#include "EditorDebugConsoleWidget.h"
#include "EditorDebugConsoleMenuBuilder.h"
#include <QtGui/QPushButton>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>

namespace tools {
namespace editor {

DebugConsoleWidget::DebugConsoleWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f) {
	CreateUI();
	EnableUI(false);
}

DebugConsoleWidget::~DebugConsoleWidget() {
	m_client.reset();
	if (m_fromMainMenu)
		DebugConsoleMenuBuilder::Unregister(this);
}

bool DebugConsoleWidget::Connect(const DebugConsoleServerId &id, bool connectToMainMenu) {
	Disconnect();

	m_fromMainMenu = connectToMainMenu;
	if (connectToMainMenu) {
		DebugConsoleMenuBuilder::Register(this, id);
	}

	m_client = DebugConsoleClient::Connect<Client>(id);
	if (m_client)
		static_cast<Client&>(*m_client).m_w = this;
	if (m_client) {
		EnableUI(true);
		m_exec->setEnabled(false);
		Print(CStr("Connected...\n"));
	}
	return m_client;
}

void DebugConsoleWidget::Disconnect() {
	if (m_client) {
		m_client.reset();
		m_lineEdit->setText("");
		Print(CStr("Disconnected.\n"));
		EnableUI(false);
	}
}

void DebugConsoleWidget::ReturnPressed() {
	QString s = m_lineEdit->text();
	m_lineEdit->setText("");
	m_exec->setEnabled(false);
	RAD_ASSERT(m_client);
	if (!s.isEmpty()) {
		if (!m_client->Exec(s.toAscii().constData())) {
			Disconnect();
		}
	}
}

void DebugConsoleWidget::ClearScrollback() {
	m_textArea->clear();
}

void DebugConsoleWidget::TextChanged(const QString &text) {
	m_exec->setEnabled(!text.isEmpty());
}

void DebugConsoleWidget::CreateUI() {
	QVBoxLayout *vbl = new (ZEditor) QVBoxLayout(this);

	m_textArea = new (ZEditor) QPlainTextEdit();
	m_textArea->setUndoRedoEnabled(false);
	m_textArea->setLineWrapMode(QPlainTextEdit::NoWrap);
	m_textArea->setReadOnly(true);

	vbl->addWidget(m_textArea, 1);

	QHBoxLayout *hbl = new (ZEditor) QHBoxLayout();

	QPushButton *b = new (ZEditor) QPushButton("CLS");
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(ClearScrollback())));
	hbl->addWidget(b);

	m_lineEdit = new (ZEditor) QLineEdit();
	m_lineEdit->setPlaceholderText("enter command");
	RAD_VERIFY(connect(m_lineEdit, SIGNAL(returnPressed()), SLOT(ReturnPressed())));
	RAD_VERIFY(connect(m_lineEdit, SIGNAL(textChanged(const QString&)), SLOT(TextChanged(const QString&))));
	hbl->addWidget(m_lineEdit, 1);

	m_exec = new (ZEditor) QPushButton("Exec");
	RAD_VERIFY(connect(m_exec, SIGNAL(clicked()), SLOT(ReturnPressed())));
	hbl->addWidget(m_exec);

	vbl->addLayout(hbl);
}

void DebugConsoleWidget::EnableUI(bool enable) {
	m_textArea->setEnabled(enable);
	m_lineEdit->setEnabled(enable);
	m_exec->setEnabled(enable);
}

void DebugConsoleWidget::Print(const String &msg) {
	QTextCursor c = m_textArea->textCursor();
	c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
	m_textArea->setTextCursor(c);
	m_textArea->insertPlainText(msg.c_str.get());
}

} // editor
} // tools

#include "moc_EditorDebugConsoleWidget.cc"
