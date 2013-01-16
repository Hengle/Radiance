/*! \file EditorDebugConsoleMenuBuilder.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#include RADPCH
#include "EditorDebugConsoleMenuBuilder.h"
#include "EditorDebugConsoleWidget.h"
#include "EditorMainWindow.h"
#include "EditorUtils.h"
#include <QtGui/QAction>
#include <QtGui/QMenu>

namespace tools {
namespace editor {

DebugConsoleMenuBuilder *DebugConsoleMenuBuilder::s_instance = 0;

DebugConsoleMenuBuilder::DebugConsoleMenuBuilder(QObject *parent) : QObject(parent), m_wasEmpty(false) {
	RAD_ASSERT(!s_instance);
	s_instance = this;
	m_empty = new QAction("(no sessions)", this);
	m_empty->setEnabled(false);
}

DebugConsoleMenuBuilder::~DebugConsoleMenuBuilder() {
	s_instance = 0;
}

void DebugConsoleMenuBuilder::OnMenuItem() {
	QAction *action = qobject_cast<QAction*>(sender());
	RAD_ASSERT(action);

	for (DebugServerItem::Vec::iterator it = m_dbgServers.begin(); it != m_dbgServers.end(); ++it) {
		DebugServerItem &item = *it;
		if (item.action == action) {
			if (!item.window) {
				DebugConsoleWidget *window = new (ZEditor) DebugConsoleWidget(
					0,
					Qt::Window|
					Qt::CustomizeWindowHint|
					Qt::WindowTitleHint|
					Qt::WindowMinimizeButtonHint|
					Qt::WindowMaximizeButtonHint|
					Qt::WindowCloseButtonHint
				);
				window->setAttribute(Qt::WA_DeleteOnClose);
				window->setWindowTitle(action->text());
				PercentSize(*window, *MainWindow::Get(), 0.65f, 0.85f);
				CenterWidget(*window, *MainWindow::Get());
				RAD_VERIFY(connect(MainWindow::Get(), SIGNAL(Closing()), window, SLOT(close())));
				window->show();
				QCoreApplication::postEvent(this, new (ZEditor) ConnectEvent(window, item.id));
			} else {
				item.window->raise();
			}

			break;
		}
	}
}

void DebugConsoleMenuBuilder::RefreshServers(QMenu *base) {
	const String kLocalHost(CStr("(localhost) "));
	const DebugConsoleServerId::Vec &servers = DebugConsoleClient::ServerList();

	// add any new servers that don't exist
	for (DebugConsoleServerId::Vec::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		const DebugConsoleServerId &id = *it;

		DebugServerItem::Vec::iterator it2 = m_dbgServers.begin();
		for (;it2 != m_dbgServers.end(); ++it2) {
			const DebugServerItem &item = *it2;
			if (item.id == id)
				break;
		}

		String description(CStr(id.description.get()));
		if (id.addr->s_addr == net::GetLocalIP().s_addr) {
			description = kLocalHost + description;
		}
		
		if (it2 == m_dbgServers.end()) {
			// add a new server.
			DebugServerItem item;
			item.id = id;
			item.window = 0;

			item.action = new (ZEditor) QAction(description.c_str.get(), this);
			item.action->setCheckable(true);
			item.action->setChecked(false);
			RAD_VERIFY(connect(item.action, SIGNAL(triggered(bool)), SLOT(OnMenuItem())));

			base->addAction(item.action);
			m_dbgServers.push_back(item);

		} else {
			DebugServerItem &item = *it2;
			if (string::cmp(id.description.get(), item.id.description.get())) {
				item.id = id;
				item.action->setText(description.c_str.get());
				if (item.window)
					item.window->setWindowTitle(item.action->text());
			}
		}
	}

	// expire invalid servers

	for (DebugServerItem::Vec::iterator it = m_dbgServers.begin(); it != m_dbgServers.end();) {
		if (!DebugConsoleServerId::Contains((*it).id, servers)) {
			base->removeAction((*it).action);
			delete (*it).action;
			it = m_dbgServers.erase(it);
		} else {
			++it;
		}
	}

	if (m_wasEmpty != m_dbgServers.empty()) {
		m_wasEmpty = m_dbgServers.empty();
		if (m_dbgServers.empty()) {
			base->addAction(m_empty);
		} else {
			base->removeAction(m_empty);
		}
	}
}

void DebugConsoleMenuBuilder::DoConnect(ConnectEvent &e) {
	e.window->Connect(e.id, true);
}

void DebugConsoleMenuBuilder::customEvent(QEvent *e) {
	switch (e->type()) {
	case EV_DebugConsoleMenuBuilderConnectWindow:
		DoConnect(*static_cast<ConnectEvent*>(e));
		break;
	default:
		break;
	}
}

void DebugConsoleMenuBuilder::Register(DebugConsoleWidget *window, const DebugConsoleServerId &id) {
	if (!s_instance)
		return;

	for (DebugServerItem::Vec::iterator it = s_instance->m_dbgServers.begin(); it != s_instance->m_dbgServers.end(); ++it) {
		DebugServerItem &item = *it;
		if (item.id == id) {
			RAD_ASSERT(item.window == 0);
			item.window = window;
			RAD_ASSERT(item.action);
			item.action->setChecked(true);
			break;
		}
	}
}

void DebugConsoleMenuBuilder::Unregister(DebugConsoleWidget *window) {
	if (!s_instance)
		return;

	for (DebugServerItem::Vec::iterator it = s_instance->m_dbgServers.begin(); it != s_instance->m_dbgServers.end(); ++it) {
		DebugServerItem &item = *it;
		if (item.window == window) {
			item.action->setChecked(false);
			item.window = 0;
			break;
		}
	}
}

} // editor
} // tools

#include "moc_EditorDebugConsoleMenuBuilder.cc"
