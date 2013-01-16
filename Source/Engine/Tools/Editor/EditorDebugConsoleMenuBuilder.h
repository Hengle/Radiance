/*! \file EditorDebugConsoleMenuBuilder.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once

#include "EditorTypes.h"
#include "EditorEventRegistry.h"
#include "../DebugConsoleClient.h"
#include <QtCore/QObject>
#include <QtCore/QEvent>
#include <Runtime/PushPack.h>

class QAction;
class QMenu;

namespace tools {
namespace editor {

class DebugConsoleWidget;

class DebugConsoleMenuBuilder : public QObject {
	Q_OBJECT
public:

	DebugConsoleMenuBuilder(QObject *parent = 0);
	virtual ~DebugConsoleMenuBuilder();

	void RefreshServers(QMenu *base);

private slots:

	void OnMenuItem();

private:

	friend class DebugConsoleWidget;

	struct DebugServerItem {
		typedef zone_vector<DebugServerItem, ZEditorT>::type Vec;
		QAction *action;
		DebugConsoleServerId id;
		DebugConsoleWidget *window;
	};

	struct ConnectEvent : QEvent {
		ConnectEvent(DebugConsoleWidget *_window, const DebugConsoleServerId &_id)
		: QEvent((QEvent::Type)EV_DebugConsoleMenuBuilderConnectWindow), window(_window), id(_id) {
		}

		DebugConsoleWidget *window;
		DebugConsoleServerId id;
	};

	void DoConnect(ConnectEvent &e);
	virtual void customEvent(QEvent *e);

	DebugServerItem::Vec m_dbgServers;
	QAction *m_empty;
	bool m_wasEmpty;

	static void Register(DebugConsoleWidget *window, const DebugConsoleServerId &id);
	static void Unregister(DebugConsoleWidget *window);

	static DebugConsoleMenuBuilder *s_instance;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
