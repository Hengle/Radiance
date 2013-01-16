/*! \file EditorDebugConsoleWidget.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once

#include "EditorTypes.h"
#include "../DebugConsoleClient.h"
#include <QtGui/QWidget>
#include <Runtime/PushPack.h>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QAction;
class QMenu;

namespace tools {
namespace editor {

class DebugConsoleWidget : public QWidget {
	Q_OBJECT
public:

	DebugConsoleWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	virtual ~DebugConsoleWidget();

	bool Connect(const DebugConsoleServerId &id, bool connectToMainMenu);
	void Disconnect();

	virtual void setVisible(bool visibile);

private slots:

	void ReturnPressed();
	void ClearScrollback();
	void TextChanged(const QString &text);

private:

	class Client : public DebugConsoleClient {
	public:
		Client(
			const net::Socket::Ref &socket, 
			const DebugConsoleServerId &id
		) : DebugConsoleClient(socket, id), m_w(0) {
		}

	protected:

		virtual void HandleLogMessage(const String &msg) {
			if (m_w)
				m_w->Print(msg);
		}

	private:

		friend class DebugConsoleWidget;

		DebugConsoleWidget *m_w;
	};

	friend class Client;

	void CreateUI();
	void EnableUI(bool enable=true);
	void Print(const String &msg);

	Client::Ref m_client;
	QLineEdit *m_lineEdit;
	QPlainTextEdit *m_textArea;
	QPushButton *m_exec;
	QPushButton *m_cls;
	bool m_fromMainMenu;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
