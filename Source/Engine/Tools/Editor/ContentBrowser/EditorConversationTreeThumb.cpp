/*! \file EditorConversationTreeThumb.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup content_browser
*/

#include RADPCH
#include "EditorConversationTreeThumb.h"
#include "../EditorConversationTreeEditorWindow.h"
#include <QtGui/QMessageBox>
#include <Runtime/PushSystemMacros.h>

using namespace pkg;

namespace tools {
namespace editor {

ConversationTreeThumb::ConversationTreeThumb(ContentBrowserView &view) : ContentAssetThumb(view) {
}

void ConversationTreeThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal) {
	pkg::Asset::Ref asset = entry->Asset(pkg::Z_ContentBrowser);

	int r = asset->Process(
		xtime::TimeSlice::Infinite,
		P_Load|P_Create
	);

	if (r != pkg::SR_Success) {
		QMessageBox::critical(0, "Error", "Failed to load conversation tree!");
		return;
	}

	if (modal) {
		EditorWindow::CreateDialog<ConversationTreeEditorWindow>(asset, editable);
	} else {
		EditorWindow::Open<ConversationTreeEditorWindow>(asset);
	}
}

void ConversationTreeThumb::New(ContentBrowserView &view) {
	ConversationTreeThumb *t = new (ZEditor) ConversationTreeThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_ConversationTree);
}

void CreateConversationTreeThumb(ContentBrowserView &view)
{
	ConversationTreeThumb::New(view);
}

} // editor
} // tools

#include <Runtime/PopSystemMacros.h>
#include "moc_EditorConversationTreeThumb.cc"
