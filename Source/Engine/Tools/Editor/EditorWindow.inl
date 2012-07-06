// EditorWindow.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <QtGui/QCloseEvent>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushSystemMacros.h>

namespace tools {
namespace editor {
namespace details {

class EditorWindowDetails {
public:
	static void OnDataChanged(EditorWindow *window, const QVariant &data) {
		window->OnDataChanged(data);
	}
	static void InitData(EditorWindow *window, int id, EditorWindow::RemoveWindowFn fn) {
		window->m_id = id;
		window->m_removeFn = fn;
	}
};

template <typename T>
class EditorWindowFactory {
public:

	static T *OpenWindow(const pkg::Asset::Ref &asset, QWidget *parent, ::Zone &zone) {
		int id = asset->id;
		T *window = BringToFront(id);
		if (!window) {
			window = new (zone) T (asset, true, WS_Window, parent);
			s_map[id] = window;
			EditorWindowDetails::InitData(window, id, &EditorWindowFactory<T>::Remove);
			window->setWindowTitle(QString("Editing ") + asset->path.get());
			window->show();
		}
		return window;
	}

	static T *CreateDialog(const pkg::Asset::Ref &asset, bool editable, QWidget *parent, ::Zone &zone) {
		T *window = new (zone) T (asset, editable, WS_Dialog, parent);
		if (editable) {
			window->setWindowTitle(QString("Editing ") + asset->path.get());
		} else {
			window->setWindowTitle(QString("Viewing ") + asset->path.get());
		}
		return window;
	}
	
	static T *BringToFront(int id) {
		T *target = Find(id);
		if (target)
			target->activateWindow();
		return target;
	}
	
	static T *Find(int id) {
		typename WindowMap::const_iterator it = s_map.find(id);
		if (it != s_map.end())
			return it->second;
		return 0;
	}

	void DataChanged(EditorWindow *src, const QVariant &data) {
		for (typename WindowMap::const_iterator it = s_map.begin(); it != s_map.end(); ++it) {
			EditorWindow *target = static_cast<EditorWindow*>(it->second);
			if (target != src)
				EditorWindowDetails::OnDataChanged(target, data);
		}
	}

	static void Remove(int id) {
		s_map.erase(id);
	}

private:

	typedef typename zone_map<int, T*, ZEditorT>::type WindowMap;

	static WindowMap s_map;
};

template <typename T>
typename EditorWindowFactory<T>::WindowMap EditorWindowFactory<T>::s_map;

} // details

template <typename T>
inline T *EditorWindow::Open(const pkg::AssetRef &asset, QWidget *parent, ::Zone &zone) {
	return details::EditorWindowFactory<T>::OpenWindow(asset, parent, zone);
}

template <typename T>
inline T *EditorWindow::CreateDialog(const pkg::AssetRef &asset, bool editable, QWidget *parent, ::Zone &zone) {
	return details::EditorWindowFactory<T>::CreateDialog(asset, editable, parent, zone);
}

template <typename T>
inline T *EditorWindow::BringToFront(int id) {
	return details::EditorWindowFactory<T>::BringToFront(id);
}

template <typename T>
inline T *EditorWindow::Find(int id) {
	return details::EditorWindowFactory<T>::Find(id);
}

template <typename T>
inline void EditorWindow::DataChanged(const QVariant &data) {
	details::EditorWindowFactory<T>::DataChanged(data);
}

inline EditorWindow::~EditorWindow() {
	if (m_removeFn)
		m_removeFn(m_id);
}

inline void EditorWindow::OnDataChanged(const QVariant &data) {}
inline void EditorWindow::OnTickEvent(float elapsed) {}

inline void EditorWindow::OnParentWindowClose(QCloseEvent *e) {
	if (e->isAccepted())
		e->setAccepted(close());
}

inline void EditorWindow::OnMainWindowTick(float elapsed) {
	OnTickEvent(elapsed);
	emit OnTick(elapsed);
}

} // editor
} // tool

#include <Runtime/PopSystemMacros.h>
