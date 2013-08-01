# Runs QtMoc on engine source

ROOT=$ENGINE_PATH/Engine/Tools/Editor

function m() {
	echo $QT_MOC -o$BASE/"moc_"$1".cc" $BASE/$1".h"
	$QT_MOC -o$BASE/"moc_"$1".cc" $BASE/$1".h"
}

BASE=$ROOT

m "EditorBSPDebugWidget"
m "EditorColorPicker"
m "EditorConversationTreeEditorItemEditDialog"
m "EditorConversationTreeEditorView"
m "EditorConversationTreeEditorWidget"
m "EditorConversationTreeEditorWindow"
m "EditorPathfindingDebugWidget"
m "EditorComboCheckBox"
m "EditorCookerDialog"
m "EditorDebugConsoleMenuBuilder"
m "EditorDebugConsoleWidget"
m "EditorGLNavWidget"
m "EditorGLWidget"
m "EditorLineEditDialog"
m "EditorLogWindow"
m "EditorMainWindow"
m "EditorModelEditorWindow"
m "EditorModelEditorWidget"
m "EditorParticleEditorWindow"
m "EditorParticleEditorWidget"
m "EditorPIEWidget"
m "EditorPopupMenu"
m "EditorProgressDialog"
m "EditorSearchLineWidget"
m "EditorStringTableEditorWindow"
m "EditorStringTableEditorWidget"
m "EditorStringTableEditorItemModel"
m "EditorTextEditorDialog"
m "EditorWindow"
m "EditorZoneViewWindow"

BASE=$ROOT/ContentBrowser

m "EditorContentBrowserModel"
m "EditorContentBrowserTree"
m "EditorContentBrowserView"
m "EditorContentBrowserWindow"
m "EditorContentChoosePackageDialog"
m "EditorContentImportFieldWidget"
m "EditorContentPropertyGrid"
m "EditorConversationTreeThumb"
m "EditorMapThumb"
m "EditorMaterialThumb"
m "EditorMeshThumb"
m "EditorMusicThumb"
m "EditorParticleThumb"
m "EditorSkModelThumb"
m "EditorSoundThumb"
m "EditorStringTableThumb"
m "EditorTextureThumb"
m "EditorVtModelThumb"

BASE=$ROOT/MapEditor

m "EditorMapEditorWindow"

BASE=$ROOT/PropertyGrid

m "EditorColorFieldWidget"
m "EditorFilePathFieldWidget"
m "EditorProperty"
m "EditorPropertyGrid"
m "EditorPropertyGridItemDelegate"
m "EditorPropertyGridModel"
m "EditorWidgetPropertyEditors"
