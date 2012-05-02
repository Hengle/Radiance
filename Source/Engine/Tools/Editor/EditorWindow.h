// EditorWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include <QtGui/QDialog>
#include <QtCore/QFlags>
#include "../../Packages/Packages.h"
#include <Runtime/PushPack.h>

class QCloseEvent;
class QKeyEvent;
class QLayout;
class QVariant;

namespace tools {
namespace editor {

namespace details {
class EditorWindowDetails;
} // details

class RADENG_CLASS EditorWindow : public QDialog {
	Q_OBJECT
public:

	enum Button {
		BTN_OK = 0x1,
		BTN_Cancel = 0x2,
		BTN_Apply = 0x4,
		BTN_DefaultOK = 0x8,
		BTN_DefaultCancel = 0x10,
		BTN_DefaultApply = 0x20,
		BTN_OKCancel = BTN_OK|BTN_Cancel,
		BTN_OKCancelApply = BTN_OK|BTN_Cancel|BTN_Apply
	};

	Q_DECLARE_FLAGS(ButtonFlags, Button)

	EditorWindow(
		WidgetStyle style,
		ButtonFlags buttons,
		bool deleteOnClose,
		bool enableTick,
		QWidget *parent = 0,
		Qt::WindowFlags f = 
			Qt::WindowSystemMenuHint|
			Qt::WindowCloseButtonHint|
			Qt::WindowMinMaxButtonsHint
	);

	virtual ~EditorWindow();

	template <typename T>
	static T *Open(const pkg::Asset::Ref &asset, QWidget *parent = 0, ::Zone &zone = ZEditor);

	template <typename T>
	static T *CreateDialog(const pkg::Asset::Ref &asset, bool editable = true, QWidget *parent = 0, ::Zone &zone = ZEditor);

	template <typename T>
	static T *BringToFront(int id);

	template <typename T>
	static T *Find(int id);

	template <typename T>
	void DataChanged(const QVariant &data = QVariant());

	//! Sets the enabled state of the specified button.
	/*! This function is undefined when called before a center layout
	 ** or widget has been set via SetCenterLayout() or SetCenterWidget()
	 */
	void EnableButton(Button btn, bool enable = true);

	void SetCenterLayout(QLayout *layout);

	void SetCenterWidget(QWidget *widget);

	//! Centers the window inside the parent and makes it a percentage of the parents size.
	void CenterParent(float x, float y);

	//! Centers the windows inside the parent and makes it a certain size.
	void CenterParent(int w, int h);

	//! Centers the window inside the parent.
	void CenterParent();

signals:

	void OnTick(float elapsed);
	void OnClose(QCloseEvent *e);
	void Closing();
	void OnApply(bool &accepted);
	void OnApply();
	void OnOK(bool &accepted);

protected:

	virtual void closeEvent(QCloseEvent*);
	virtual void keyPressEvent(QKeyEvent*);

	virtual void OnDataChanged(const QVariant &data);
	virtual void OnTickEvent(float elapsed);

private slots:

	void OnParentWindowClose(QCloseEvent*);
	void OnMainWindowTick(float elapsed);
	void HandleOK();
	void HandleApply();

private:
	
	friend class details::EditorWindowDetails;

	typedef void (*RemoveWindowFn) (int id);

	void CreateLayout(QLayout *centerLayout, QWidget *centerWidget);

	ButtonFlags m_btFlags;
	QPushButton *m_buttons[3];
	RemoveWindowFn m_removeFn;
	WidgetStyle m_style;
	int m_id;

};

} // editor
} // tools

Q_DECLARE_OPERATORS_FOR_FLAGS(tools::editor::EditorWindow::ButtonFlags)

#include <Runtime/PopPack.h>

#include "EditorWindow.inl"
