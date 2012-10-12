// EditorContentBrowserView.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorContentAssetThumbDimensionCache.h"
#include "EditorContentBrowserWindow.h"
#include "EditorContentBrowserView.h"
#include "EditorContentBrowserModel.h"
#include "../EditorSearchLineWidget.h"
#include "../EditorGLWidget.h"
#include "../EditorMainWindow.h"
#include "../EditorUtils.h"
#include "../EditorPopupMenu.h"
#include "../../../Packages/Packages.h"
#include "../../../Renderer/GL/GLState.h"
#include "../../../Renderer/GL/GLTexture.h"
#include <Runtime/StringBase.h>
#include <Runtime/ImageCodec/ImageCodec.h>
#include <Runtime/ImageCodec/Png.h>
#include <QtGui/QGridLayout>
#include <QtGui/QFontMetrics>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtCore/QThread>
#include <QtGui/QScrollBar>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QMessageBox>
#include <algorithm>

#undef min
#undef max
#undef LoadIcon

namespace tools {
namespace editor {

enum
{
	HItemSpace = 6,
	VItemSpace = 6,
	VLabelBorder = 4,
	HLabelBorder = 4
};

void CreateTextureThumb(ContentBrowserView &view);
void CreateMapThumb(ContentBrowserView &view);
void CreateMaterialThumb(ContentBrowserView &view);
void CreateSkModelThumb(ContentBrowserView &view);
void CreateMeshThumb(ContentBrowserView &view);
void CreateSoundThumb(ContentBrowserView &view);
void CreateMusicThumb(ContentBrowserView &view);
void CreateStringTableThumb(ContentBrowserView &view);

void CreateThumbs(ContentBrowserView &view) {
	CreateTextureThumb(view);
	CreateMapThumb(view);
	CreateMaterialThumb(view);
	CreateSkModelThumb(view);
	CreateMeshThumb(view);
	CreateSoundThumb(view);
	CreateMusicThumb(view);
	CreateStringTableThumb(view);
}

inline int makeRes(int w, int h) {
	return w|(h<<16);
}

inline std::pair<int, int> getRes(int x) {
	return std::pair<int, int>(x & 0xffff, (x>>16)&0xffff);
}

///////////////////////////////////////////////////////////////////////////////

ContentAssetThumb::ContentAssetThumb(ContentBrowserView &view)
: m_view(&view), m_editable(true), m_modal(false)
{
	m_menu[0] = 0;
	m_menu[1] = 0;
}

void ContentAssetThumb::ThumbChanged()
{
	m_view->ThumbChanged();
}

void ContentAssetThumb::Register(Ref &self, asset::Type type)
{
	m_view->RegisterThumb(self, type);
}

PopupMenu *ContentAssetThumb::CreateMenu(bool mutableMenu)
{
	int idx = mutableMenu ? 0 : 1;
	RAD_ASSERT(!m_menu[idx]);
	return (m_menu[idx] = new PopupMenu(m_view));
}

void ContentAssetThumb::Dimensions(const pkg::Package::Entry::Ref &entry, int &w, int &h)
{
	w = 256;
	h = 256;
}

bool ContentAssetThumb::Render(const pkg::Package::Entry::Ref &entry, int x, int y, int w, int h)
{
	return false;
}

void ContentAssetThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal)
{
}

void ContentAssetThumb::RightClick(const pkg::Package::Entry::Ref &entry, QMouseEvent *e, bool editable, bool modal)
{
	m_item = entry;
	m_editable = editable;
	m_modal = modal;
	PopupMenu *m = Menu(editable);
	if (!m && editable)
		m = Menu(false); // try readonly menu
	if (m)
		m->Exec(e->globalPos());
	m_item.reset();
}

ContentBrowserView &ContentAssetThumb::View() const
{
	return *m_view;
}

void ContentAssetThumb::Begin()
{
}

void ContentAssetThumb::End()
{
}

void ContentAssetThumb::NotifyAddRemovePackages()
{
}

void ContentAssetThumb::NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed)
{
}

void ContentAssetThumb::NotifyContentChanged(const ContentChange::Vec &changed)
{
}

void ContentAssetThumb::Tick(float dt, const xtime::TimeSlice &time)
{
}

///////////////////////////////////////////////////////////////////////////////

ContentBrowserView::ViewSet ContentBrowserView::s_views;
ContentBrowserView::Mutex ContentBrowserView::s_m;

int ContentBrowserView::AddView(ContentBrowserView *view)
{
	Lock L(s_m);
	s_views.insert(view);
	return (int)s_views.size();
}

void ContentBrowserView::RemoveView(ContentBrowserView *view)
{
	Lock L(s_m);
	s_views.erase(view);
}

ContentAssetThumb *ContentBrowserView::ThumbForType(asset::Type type)
{
	ContentAssetThumb::Map::const_iterator it = m_thumbs.find(type);
	return (it == m_thumbs.end()) ? 0 : it->second.get();
}

r::GLTexture::Ref ContentBrowserView::GetIcon(Icon i) const
{
	RAD_ASSERT(i >= I_OneDisk && i < I_Max);
	return m_icons[i];
}

std::pair<int, int> ContentBrowserView::selectedResolution() const {
	int mask = m_resolutions->itemData(m_resolution).toInt();
	return getRes(mask);
}

void ContentBrowserView::NotifyAddRemovePackages()
{
	for (ViewSet::iterator it = s_views.begin(); it != s_views.end(); ++it)
	{
		ContentBrowserView *view = *it;
		for (ContentAssetThumb::Map::const_iterator it2 = view->m_thumbs.begin(); it2 != view->m_thumbs.end(); ++it2)
		{
			it2->second->NotifyAddRemovePackages();
		}

		view->ClearSelection(true);
		view->BuildAssetList();
		view->RecalcLayout(true);
	}
}

void ContentBrowserView::NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed)
{
	for (ViewSet::iterator it = s_views.begin(); it != s_views.end(); ++it)
	{
		ContentBrowserView *view = *it;
		for (ContentAssetThumb::Map::const_iterator it2 = view->m_thumbs.begin(); it2 != view->m_thumbs.end(); ++it2)
		{
			it2->second->NotifyAddRemoveContent(added, removed);
		}

		view->ClearSelection(true);
		view->BuildAssetList();
		view->RecalcLayout(true);
	}
}

void ContentBrowserView::NotifyContentChanged(const ContentChange::Vec &changed)
{
	for (ViewSet::iterator it = s_views.begin(); it != s_views.end(); ++it)
	{
		ContentBrowserView *view = *it;
		for (ContentAssetThumb::Map::const_iterator it2 = view->m_thumbs.begin(); it2 != view->m_thumbs.end(); ++it2)
		{
			it2->second->NotifyContentChanged(changed);
		}

		view->BuildAssetList();
		view->RecalcLayout(true);
		view->ScrollToSelection();
	}
}

void ContentBrowserView::Tick(float dt)
{
	for (ViewSet::iterator it = s_views.begin(); it != s_views.end(); ++it)
	{
		ContentBrowserView *view = *it;
		view->DoTick(dt);
	}
}

ContentBrowserView::ContentBrowserView(
	bool vscroll,
	bool editable,
	bool modal,
	SelMode selMode,
	QWidget *parent,
	Qt::WindowFlags f
)
: 
QWidget(parent, f),
m_vscroll(vscroll),
m_editable(editable),
m_modal(modal),
m_font("Courier New"),
m_hoverId(-1),
m_selMode(selMode),
m_loadedIcons(false),
m_inTick(false),
m_tickRedraw(false)
{
	m_resolution = MainWindow::Get()->userPrefs->value("contentBrowser/resolution", 1).toInt();
	m_font.setPixelSize(12);

	QGridLayout *l = new (ZEditor) QGridLayout(this);
	
	QGridLayout *s = new (ZEditor) QGridLayout();
	l->addLayout(s, 0, 0, 1, -1);
	s->setColumnStretch(0, 1);

	int col = 1;
	
	s->addWidget(new (ZEditor) QLabel(QString("Size:")), 0, col++);
	
	m_sizes = new (ZEditor) QComboBox(this);
	m_sizes->addItem(QString("32x32"));
	m_sizes->addItem(QString("64x64"));
	m_sizes->addItem(QString("128x128"));
	m_sizes->addItem(QString("256x256"));
	m_sizes->addItem(QString("512x512"));
	m_sizes->addItem(QString("1024x1024"));
	m_sizes->addItem(QString("2048x2048"));
	m_sizes->addItem(QString("Full Size"));
	m_sizes->setCurrentIndex(3);
	RAD_VERIFY(connect(m_sizes, SIGNAL(currentIndexChanged(int)), SLOT(SizeChanged(int))));
	s->addWidget(m_sizes, 0, col++);
	m_sizes->setToolTip("Set Thumnail Size");
	
	s->addItem(new (ZEditor) QSpacerItem(32, 0), 0, col++);
	s->addWidget(new (ZEditor) QLabel(QString("Sort By:")), 0, col++);
	
	m_sort = new (ZEditor) QComboBox(this);
	m_sort->addItem(QString("Name"));
	m_sort->addItem(QString("Size"));
	m_sort->addItem(QString("Size+Name"));
	m_sort->setCurrentIndex(m_filter.sort);
	RAD_VERIFY(connect(m_sort, SIGNAL(currentIndexChanged(int)), SLOT(SortChanged(int))));
	s->addWidget(m_sort, 0, col++);
	m_sort->setToolTip("Set Sort Mode");
	
	s->addItem(new (ZEditor) QSpacerItem(32, 0), 0, col++);
	s->addWidget(new (ZEditor) QLabel(QString("Resolution:")), 0, col++);
	
	m_resolutions = new (ZEditor) QComboBox(this);
	m_resolutions->addItem(QString("320x480 (3GS)"), makeRes(320, 480));
	m_resolutions->addItem(QString("1024x768 (iPad)"), makeRes(1024, 768));
	m_resolutions->addItem(QString("1280x720 (16:9)"), makeRes(1280, 720));
	m_resolutions->addItem(QString("1366x768 (16:9)"), makeRes(1366, 768));
	m_resolutions->addItem(QString("1440x900 (16:10)"), makeRes(1440, 900));
	m_resolutions->addItem(QString("16000x900 (16:9)"), makeRes(1600, 900));
	m_resolutions->addItem(QString("1680x1050 (16:10)"), makeRes(1680, 1050));
	m_resolutions->addItem(QString("1920x1080 (16:9)"), makeRes(1920, 1080));
	m_resolutions->setCurrentIndex(m_resolution);
	
	RAD_VERIFY(connect(m_resolutions, SIGNAL(currentIndexChanged(int)), SLOT(resolutionChanged(int))));
	s->addWidget(m_resolutions, 0, col++);
	m_sort->setToolTip("Set Game Resolution");
	
	m_searchLine = new (ZEditor) SearchLineWidget(this);
	RAD_VERIFY(connect(m_searchLine, SIGNAL(textChanged(const QString&)), SLOT(UpdateFilter())));
	s->addWidget(m_searchLine, 0, col++);
	
	s->setColumnStretch(col, 1);

	s = new (ZEditor) QGridLayout();
	l->addLayout(s, 1, 0, 1, -1);
	s->setColumnStretch(0, 1);
	
	col = 1;
	
	m_cloneButton = new (ZEditor) QPushButton(
											  LoadIcon("Editor/clone.png"),
											  QString(),
											  this
											  );
	RAD_VERIFY(connect(m_cloneButton, SIGNAL(clicked()), SLOT(CloneSelection())));
	m_cloneButton->setIconSize(QSize(32, 32));
	s->addWidget(m_cloneButton, 0, col++);
	m_cloneButton->setEnabled(false);
	m_cloneButton->setToolTip("Clone Selection");
	
	m_moveButton = new (ZEditor) QPushButton(
											 LoadIcon("Editor/move.png"),
											 QString(),
											 this
											 );
	RAD_VERIFY(connect(m_moveButton, SIGNAL(clicked()), SLOT(MoveSelection())));
	m_moveButton->setIconSize(QSize(32, 32));
	s->addWidget(m_moveButton, 0, col++);
	m_moveButton->setEnabled(false);
	m_moveButton->setToolTip("Move Selection");
	
	s->addItem(new (ZEditor) QSpacerItem(32, 0), 0, col++);
	
	QPushButton *zoomIn = new (ZEditor) QPushButton(
													LoadIcon("Editor/zoom_in.png"),
													QString(),
													this
													);
	RAD_VERIFY(connect(zoomIn, SIGNAL(clicked()), SLOT(ZoomIn())));
	zoomIn->setIconSize(QSize(32, 32));
	s->addWidget(zoomIn, 0, col++);
	zoomIn->setToolTip("Preview Bigger");
	
	QPushButton *zoomOut = new (ZEditor) QPushButton(
													 LoadIcon("Editor/zoom_out.png"),
													 QString(),
													 this
													 );
	zoomOut->setIconSize(QSize(32, 32));
	RAD_VERIFY(connect(zoomOut, SIGNAL(clicked()), SLOT(ZoomOut())));
	s->addWidget(zoomOut, 0, col++);
	zoomOut->setToolTip("Preview Smaller");
	
	QPushButton *zoomOne = new (ZEditor) QPushButton(
													 LoadIcon("Editor/view_1_1.png"),
													 QString(),
													 this
													 );
	zoomOne->setIconSize(QSize(32, 32));
	RAD_VERIFY(connect(zoomOne, SIGNAL(clicked()), SLOT(ZoomFull())));
	s->addWidget(zoomOne, 0, col++);
	zoomOne->setToolTip("Preview Full Size");
	
	m_delButton = new (ZEditor) QPushButton(
											LoadIcon("Editor/delete2.png"),
											QString(),
											this
											);
	m_delButton->setIconSize(QSize(32, 32));
	RAD_VERIFY(connect(m_delButton, SIGNAL(clicked()), SLOT(DeleteSelection())));
	s->addWidget(m_delButton, 0, col++);
	m_delButton->setEnabled(false);
	m_delButton->setToolTip("Delete Selection");
	
	s->setColumnStretch(col, 1);

	m_glw = new GLWidget(this);
	m_glw->setFocusPolicy(Qt::ClickFocus);
	setFocusProxy(m_glw);
	RAD_VERIFY(connect(m_glw, SIGNAL(OnRenderGL(GLWidget&)), SLOT(OnRenderGL(GLWidget&))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnResizeGL(GLWidget&, int, int)), SLOT(OnResizeGL(GLWidget&, int, int))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnInitializeGL(GLWidget&)), SLOT(OnInitializeGL(GLWidget&))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnWheelEvent(QWheelEvent*)), SLOT(OnWheelEvent(QWheelEvent*))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnMouseMoveEvent(QMouseEvent*)), SLOT(OnMouseMoveEvent(QMouseEvent*))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnMousePressEvent(QMouseEvent*)), SLOT(OnMousePressEvent(QMouseEvent*))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnMouseReleaseEvent(QMouseEvent*)), SLOT(OnMouseReleaseEvent(QMouseEvent*))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnMouseDoubleClickEvent(QMouseEvent*)), SLOT(OnMouseDoubleClickEvent(QMouseEvent*))));
	RAD_VERIFY(connect(m_glw, SIGNAL(OnKeyPressEvent(QKeyEvent*)), SLOT(OnKeyPressEvent(QKeyEvent*))));
	m_glw->setMouseTracking(true);
	l->addWidget(m_glw, 2, 0);
	l->setColumnStretch(0, 1);

	m_scroll = new QScrollBar(Qt::Vertical, this);
	m_scroll->setTracking(true);
	m_scroll->hide();
	RAD_VERIFY(connect(m_scroll, SIGNAL(valueChanged(int)), SLOT(OnScrollMove(int))));
	l->addWidget(m_scroll, 2, 1);

	m_maxs[0] = m_maxs[1] = 256;
	m_scroll->setPageStep(256);
	m_scroll->setSingleStep(64);
	m_step = 32;
	m_y[0] = m_y[1] = 0;
}

ContentBrowserView::~ContentBrowserView()
{
	RemoveView(this);
	MainWindow::Get()->BindGL();
	m_thumbs.clear();
	for (int i = 0; i < I_Max; ++i)
	{
		m_icons[i].reset();
	}
	MainWindow::Get()->UnbindGL();
}

void ContentBrowserView::DoTick(float dt)
{
	m_inTick = true;
	m_tickRedraw = false;

	for (ContentAssetThumb::Map::const_iterator it = m_thumbs.begin(); it != m_thumbs.end(); ++it)
	{
		xtime::TimeSlice time(50);
		it->second->Tick(dt, time);
	}

	m_inTick = false;
	if (m_tickRedraw)
		m_glw->updateGL();
}

void ContentBrowserView::UpdateFilter(bool redraw)
{
	m_filter.filter = String(m_searchLine->text.get().toAscii().constData()).Lower();
	BuildAssetList();
	RecalcLayout(false);
	if (!ScrollToSelection() && redraw)
		Update();
}

void ContentBrowserView::RecalcLayout(bool redraw)
{
	BuildRows();
	if (redraw)
		Update();
}

void ContentBrowserView::ShowScroll(bool show)
{
	if (m_vscroll)
	{
		if (show)
		{
			m_scroll->show();
		}
		else
		{
			m_scroll->hide();
		}
	}
}

void ContentBrowserView::OnResizeGL(GLWidget&, int, int)
{
	RecalcLayout();
	ScrollToSelection();
}

void ContentBrowserView::OnInitializeGL(GLWidget&)
{
	static bool s_first = true;
	AddView(this);

	if (s_first) {
		// The first view that is loaded will load the cached thumb sizes off disk
		// The first view is also the view that calculates the dimension of all
		// thumbs for layout, and during this process the thumb cache is built
		// so we disable saving during this process so we don't save the cache
		// a billion times (once for each asset).
		ContentAssetThumbDimensionCache::Get()->Load();
		ContentAssetThumbDimensionCache::Get()->enableSaves = false;
	}

	LoadIcons();
	CreateThumbs(*this);
	BuildAssetList();
	RecalcLayout();

	if (s_first) {
		ContentAssetThumbDimensionCache::Get()->enableSaves = true;
		ContentAssetThumbDimensionCache::Get()->Save();
	}

	s_first = false;
}

void ContentBrowserView::ThumbChanged()
{
	Update();
}

void ContentBrowserView::RegisterThumb(const ContentAssetThumb::Ref &thumb, asset::Type type)
{
	m_thumbs[type] = thumb;
}

void ContentBrowserView::SortChanged(int index)
{
	m_filter.sort = index;
	BuildAssetList();
	RecalcLayout(true);
}

void ContentBrowserView::SizeChanged(int index)
{
	if (index == m_sizes->count()-1)
	{
		m_maxs[0] = m_maxs[1] = 0;
		m_scroll->setPageStep(1024);
		m_scroll->setSingleStep(256);
		m_step = 256;
		RecalcLayout(true);
		ScrollToSelection();
	}
	else
	{
		m_maxs[0] = m_maxs[1] = 32<<index;
		m_step = m_maxs[0] / 4;
		m_scroll->setPageStep(m_maxs[0]);
		m_scroll->setSingleStep(m_step);
		RecalcLayout(true);
		ScrollToSelection();
	}
}

void ContentBrowserView::resolutionChanged(int index) {
	m_resolution = index;
	MainWindow::Get()->userPrefs->setValue("contentBrowser/resolution", index);
}

void ContentBrowserView::ZoomIn()
{
	int idx = std::min(m_sizes->currentIndex()+1, m_sizes->count()-1);
	if (idx != m_sizes->currentIndex())
	{
		m_sizes->setCurrentIndex(idx);
	}
}

void ContentBrowserView::ZoomOut()
{
	int idx = std::max(m_sizes->currentIndex()-1, 0);
	if (idx != m_sizes->currentIndex())
	{
		m_sizes->setCurrentIndex(idx);
	}
}

void ContentBrowserView::ZoomFull()
{
	if (m_sizes->currentIndex() != 7)
	{
		m_sizes->setCurrentIndex(7);
	}
}

void ContentBrowserView::DeleteSelection()
{
	enum { MaxItems = 10 };

	if (m_sel.empty())
		return;

	SelSet sel(m_sel);
	
	String msg;
	msg.Printf("Are you sure you want to delete the following %d item(s)?"RAD_NEWLINE, sel.size());

	{
		String x;
		int c = 0;
		SelSet::const_iterator it = sel.begin();

		for (; it != sel.end() && c < MaxItems; ++it, ++c)
		{
			if (!x.empty)
			{
				x += RAD_NEWLINE;
			}
			pkg::Package::Entry::Ref r = Packages()->FindEntry(*it);
			if (r)
			{
				x += r->name;
			}
		}

		if (it != sel.end())
		{
			x += "...";
		}

		msg += "Assets:"RAD_NEWLINE+x;
	}

	if (QMessageBox::question(
		parentWidget(),
		"Confirmation",
		msg.c_str.get(),
		QMessageBox::Yes|QMessageBox::No, 
		QMessageBox::No
	) != QMessageBox::Yes)
	{
		return;
	}

	ClearSelection(true);

	pkg::IdVec vec;
	for (SelSet::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		vec.push_back(*it);
	}

	pkg::PackageMap pkgs = Packages()->GatherPackages(vec);

	for (SelSet::const_iterator it = sel.begin(); it != sel.end(); ++it)
	{
		Packages()->Delete(*it);
	}

	Packages()->SavePackages(pkgs);
	ContentBrowserWindow::NotifyAddRemoveContent(pkg::IdVec(), vec);
}

void ContentBrowserView::CloneSelection()
{
	emit OnCloneSelection(m_sel);
}

void ContentBrowserView::MoveSelection()
{
	emit OnMoveSelection(m_sel);
}


void ContentBrowserView::RAD_IMPLEMENT_SET(selMode) (SelMode m)
{
	if (m != m_selMode)
	{
		m_selMode = m;
		switch (m)
		{
		case SelNone:
			if (!m_sel.empty() || m_hoverId != -1)
			{
				m_hoverId = -1;
				SelSet unsel;
				unsel.swap(m_sel);
				Update();
				DoSelectionChange(m_sel, unsel, m_sel);
			}
			break;
		case SelSingle:
			if (m_sel.size() > 1)
			{
				int id = *m_sel.begin();
				SelSet unsel;
				unsel.swap(m_sel);
				m_sel.insert(id);
				unsel.erase(id);
				Update();
				DoSelectionChange(SelSet(), unsel, m_sel);
			}
			break;
		default:
			break;
		}
	}
}

void ContentBrowserView::DoSelectionChange(
	const SelSet &sel,   // what was selected (if any) 
	const SelSet &unsel, // what was deselected
	const SelSet &total  // total selection
)
{
	m_delButton->setEnabled(!total.empty() && m_editable);
	m_cloneButton->setEnabled(!total.empty() && m_editable);
	m_moveButton->setEnabled(!total.empty() && m_editable);
	emit OnSelectionChanged(sel, unsel, total);
}

int ContentBrowserView::HitTestId(int x, int y, const Row** row, const AssetInfo **asset) const
{
	if (row)
		*row = 0;
	if (asset)
		*asset = 0;

	y += m_y[0];
	Row::Map::const_iterator it = m_rows.lower_bound(y);

	if (it != m_rows.end())
	{
		if (row)
			*row = &it->second;

		QFontMetrics fm(m_font);
		int fh = fm.height();

		for (;it != m_rows.end(); ++it)
		{
			const Row &r = it->second;
			if (r.y[0] > y) 
				break; // trivial reject.

			for (AssetInfoVec::const_iterator ait = r.assets.begin(); ait != r.assets.end(); ++ait)
			{
				const AssetInfo &info = *ait;
				if (x >= info.x && x <= info.x+info.w &&
					y <= r.y[0]+r.y[1]+VLabelBorder+fh)
				{
					if (asset)
						*asset = &info;

					return info.id;
				}
			}
		}
	}

	return -1;
}

ContentBrowserView::SelSet ContentBrowserView::ItemRange(int start, int end) const
{
	SelSet ids;

	int idxs[2];
	Row::Map::iterator rows[3];

	IdPos::Map::const_iterator it = m_ids.find(start);
	if (it == m_ids.end())
		return ids;
	rows[0] = it->second.row;
	idxs[0] = it->second.idx;

	it = m_ids.find(end);
	if (it == m_ids.end())
		return ids;

	rows[1] = it->second.row;
	idxs[1] = it->second.idx;

	++idxs[1];
	rows[2] = rows[1];
	++rows[2]; // exclusive

	for (Row::Map::iterator it = rows[0]; it != rows[2]; ++it)
	{
		const Row &row = it->second;

		int first = 0;
		int last  = (int)row.assets.size();

		if (it == rows[0])
		{ // partial row
			first = idxs[0];
		}
		if(it == rows[1])
		{ // partial row
			last = idxs[1];
		}

		for (int i = first; i < last; ++i)
			ids.insert(row.assets[i].id);
	}

	return ids;
}

int ContentBrowserView::TopLeft(const SelSet &ids, const Row** prow, const AssetInfo **pinfo) const
{
	int x = -1;
	int y = -1;
	int bestId = -1;

	if (prow)
		*prow = 0;
	if (pinfo)
		*pinfo = 0;

	for (SelSet::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		IdPos::Map::const_iterator it2 = m_ids.find(*it);
		if (it2 == m_ids.end())
			continue;
		const IdPos &pos = it2->second;
		const Row &row = pos.row->second;
		const AssetInfo &info = row.assets[pos.idx];

		if (((y == -1) || (row.y[0] < y)) || ((x == -1) || (info.x < x)))
		{
			y = row.y[0];
			x = info.x;
			bestId = info.id;

			if (prow)
				*prow = &row;
			if (pinfo)
				*pinfo = &info;
		}
	}

	return bestId;
}

int ContentBrowserView::BottomRight(const SelSet &ids, const Row** prow, const AssetInfo **pinfo) const
{
	int x = -1;
	int y = -1;
	int bestId = -1;

	if (prow)
		*prow = 0;
	if (pinfo)
		*pinfo = 0;

	for (SelSet::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		IdPos::Map::const_iterator it2 = m_ids.find(*it);
		if (it2 == m_ids.end())
			continue;
		const IdPos &pos = it2->second;
		const Row &row = pos.row->second;
		const AssetInfo &info = row.assets[pos.idx];

		if (row.y[0] > y || info.x > x)
		{
			y = row.y[0];
			x = info.x;
			bestId = info.id;

			if (prow)
				*prow = &row;
			if (pinfo)
				*pinfo = &info;
		}
	}

	return bestId;
}

void ContentBrowserView::KnockoutSelection()
{
	SelSet unsel;
	
	for (SelSet::iterator it = m_sel.begin(); it != m_sel.end();)
	{
		if (m_ids.find(*it) == m_ids.end())
		{
			SelSet::iterator next = it; ++next;
			unsel.insert(*it);
			m_sel.erase(it);
			it = next;
		}
		else
		{
			++it;
		}
	}

	DoSelectionChange(SelSet(), unsel, m_sel);
}

void ContentBrowserView::OnWheelEvent(QWheelEvent *e)
{
	if (e->orientation() == Qt::Vertical && m_scroll->isVisible())
	{
		int steps = -e->delta() * m_step / 90;
		m_scroll->setSliderPosition(m_y[0] + steps);
	}
}

void ContentBrowserView::OnMouseMoveEvent(QMouseEvent *e)
{
	if (m_selMode == SelNone)
		return;

	int id = HitTestId(e->x(), e->y());
	if (id != m_hoverId)
	{
		m_hoverId = id;
		Update();
	}
}

void ContentBrowserView::OnMousePressEvent(QMouseEvent *e)
{
	if (m_selMode == SelNone)
		return;

	bool redraw = false;

	const Row *row;
	const AssetInfo *info;

	int id = HitTestId(e->x(), e->y(), &row, &info);

	bool selected = id != -1 && m_sel.find(id) != m_sel.end();

	if (e->button() == Qt::LeftButton)
	{
		SelSet sel, unsel;

		if (m_selMode != SelMulti || (!(e->modifiers()&Qt::ControlModifier) && !(e->modifiers()&Qt::ShiftModifier)))
		{
			redraw = (!selected && !m_sel.empty()) || (selected && m_sel.size() > 1);
			unsel.swap(m_sel); // clear
		}

		if (id != -1)
		{
			if (m_selMode == SelMulti && (e->modifiers()&Qt::ControlModifier) && selected)
			{
				m_sel.erase(id);
				unsel.insert(id);
			}
			else if (m_selMode == SelMulti && (e->modifiers()&Qt::ShiftModifier) && !(e->modifiers()&Qt::ControlModifier) 
				&& !selected && !m_sel.empty())
			{ // range selection
				const Row *xrow;
				const AssetInfo *xinfo;

				int xid = BottomRight(m_sel, &xrow, &xinfo);
				if (xid != -1)
				{
					bool flip = false;

					if (row == xrow) // same row
					{
						flip = xinfo->x < info->x;
					}
					else
					{
						flip = xrow->y[0] < row->y[0];
					}

					if (flip)
					{
						xid = id;
						id = TopLeft(m_sel);
					}

					SelSet oldSel;
					oldSel.swap(m_sel);

					m_sel = ItemRange(id, xid);
					
					for (SelSet::const_iterator it = m_sel.begin(); it != m_sel.end(); ++it)
					{
						unsel.erase(*it);

						if (oldSel.find(*it) == oldSel.end())
						{
							sel.insert(*it);
						}
					}
				}
			}
			else if (selected)
			{
				unsel.erase(id);
				m_sel.insert(id);
			}
			else
			{
				m_sel.insert(id);
				sel.insert(id);
			}

			redraw = true;
		}

		DoSelectionChange(sel, unsel, m_sel);
		if (redraw)
			Update();
	}
	else if (selected && e->button() == Qt::RightButton)
	{
		pkg::Package::Entry::Ref asset = Packages()->FindEntry(id);
		if (asset)
		{
			ContentAssetThumb *thumb = ThumbForType(asset->type);
			if (thumb)
				thumb->RightClick(asset, e, m_editable, m_modal);
		}
	}
}

void ContentBrowserView::OnMouseReleaseEvent(QMouseEvent *e)
{
}

void ContentBrowserView::OnMouseDoubleClickEvent(QMouseEvent *e) {
	int id = HitTestId(e->x(), e->y());
	if (id != -1) {
		bool openEditor = true;
		emit OnItemDoubleClicked(id, openEditor);
		if (openEditor) {
			pkg::Package::Entry::Ref asset = Packages()->FindEntry(id);
			if (asset) {
				ContentAssetThumb *thumb = ThumbForType(asset->type);
				if (thumb)
					thumb->OpenEditor(asset, m_editable, m_modal);
			}
		}
	}
}

void ContentBrowserView::OnKeyPressEvent(QKeyEvent *e)
{
}

void ContentBrowserView::ClearSelection(bool event)
{
	if (m_sel.empty())
		return;
	
	SelSet unsel;
	unsel.swap(m_sel);

	if (event)
		DoSelectionChange(SelSet(), unsel, m_sel);
}

void ContentBrowserView::Update()
{
	if (m_inTick)
	{
		m_tickRedraw = true;
	}
	else
	{
		m_glw->update();
	}
}

bool ContentBrowserView::ScrollToSelection()
{
	if (!m_scroll->isVisible())
		return false;

	if (m_sel.empty())
	{
		if (m_scroll->sliderPosition() != 0)
		{
			m_scroll->setSliderPosition(0);
			return true;
		}
		return false;
	}

	return ScrollToAsset(*m_sel.begin());
}

bool ContentBrowserView::ScrollToAsset(int id)
{
	if (m_scroll->isVisible())
	{
		IdPos::Map::const_iterator it = m_ids.find(id);
		if (it != m_ids.end())
		{
			int vh = m_glw->height();
			int y = it->second.row->second.y[0] - (vh-it->second.row->second.y[1])/2;
			m_scroll->setSliderPosition(y < 0 ? 0 : y);
			return true;
		}
	}

	return false;
}

void ContentBrowserView::Select(int id, bool clear, bool redraw)
{
	pkg::IdVec ids;
	ids.push_back(id);
	Select(ids, clear, redraw);
}

void ContentBrowserView::Select(const pkg::IdVec &ids, bool clear, bool redraw)
{
	SelSet s, u;

	if (clear)
		u.swap(m_sel);

	for (pkg::IdVec::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		int id = *it;

		if (u.find(id) != u.end())
		{
			u.erase(id);
		}
		else if (clear || (m_sel.find(id) != m_sel.end()))
		{
			s.insert(id);
		}

		m_sel.insert(id);
	}

	DoSelectionChange(s, u, m_sel);
	if (redraw)
		Update();
}

void ContentBrowserView::BuildRows()
{
	m_hoverId = -1;
	SelSet sel;
	sel.swap(m_sel);

	m_rows.clear();
	m_ids.clear();

	QFontMetrics fm(m_font);
	int fh = fm.height();

	int vw = m_glw->width();
	int vh = m_glw->height();

	int y = VItemSpace/2;

	Row r;
	r.assets.reserve(32);
	int ymax[2];

	for (IdVec::iterator it = m_assets.begin(); it != m_assets.end();)
	{
		// do rows
		int x = HItemSpace/2;
		ymax[0] = ymax[1] = 0;
		ContentAssetThumb *thumb = 0;
		
		r.assets.clear();
		r.y[0] = y;

		for (;it != m_assets.end(); ++it)
		{
			const pkg::Package::Entry::Ref asset = Packages()->FindEntry(*it);
			RAD_ASSERT(asset);
			thumb = ThumbForType(asset->type);

			int iw = 256;
			int ih = 256;

			if (thumb)
			{
				thumb->Dimensions(asset, iw, ih);
			}

			if (iw>0 && ih>0)
			{
				// aspect ratio
				if (m_maxs[0] && m_maxs[1] && iw > m_maxs[0] && ih > m_maxs[1])
				{
					if (iw > ih)
					{
						float z = m_maxs[0] / (float)iw;
						iw = m_maxs[0];
						ih = (int)(z*ih);
					}
					else
					{
						float z = m_maxs[1] / (float)ih;
						ih = m_maxs[1];
						iw = (int)(z*iw);
					}
				}
				else if (m_maxs[0] && iw > m_maxs[0])
				{
					float z = m_maxs[0] / (float)iw;
					iw = m_maxs[0];
					ih = (int)(z*ih);
				}
				else if (m_maxs[1] && ih > m_maxs[1])
				{
					float z = m_maxs[1] / (float)ih;
					ih = m_maxs[1];
					iw = (int)(z*iw);
				}

				int tw = fm.width(QString(asset->path));
				int  w = std::max(tw+HLabelBorder, iw);
				
				if (r.assets.empty() || x+w+(HItemSpace/2) < vw)
				{
					ymax[0] = std::max(ymax[0], ih);
					ymax[1] = ymax[0]+fh+VLabelBorder*2+VItemSpace;
					AssetInfo info;
					info.x = x;
					info.y = y;
					info.w = w;
					info.iw = iw;
					info.ih = ih;
					info.tw = tw;
					// store id's so thumbs can control caching
					info.id = asset->id;
					if (sel.find(info.id) != sel.end()) // restore sel.
					{
						m_sel.insert(info.id);
					}
					r.assets.push_back(info);
					x += w+HItemSpace;
				}
				else
				{ // off edge
					r.y[1] = ymax[0];
					y += ymax[1];
					std::pair<Row::Map::iterator, bool> p = m_rows.insert(Row::Map::value_type(r.y[0]+ymax[1], r));
					// insert row into id map.
					for (AssetInfoVec::const_iterator it = p.first->second.assets.begin(); it != p.first->second.assets.end(); ++it)
					{
						const AssetInfo &info = *it;

						IdPos pos;
						pos.id = info.id;
						pos.idx = (int)(it-p.first->second.assets.begin());
						pos.row = p.first;
						m_ids.insert(IdPos::Map::value_type(info.id, pos));
					}
					break;
				}
			}
		}
	}

	if (!r.assets.empty())
	{
		r.y[1] = ymax[0];
		y += ymax[1];
		std::pair<Row::Map::iterator, bool> p = m_rows.insert(Row::Map::value_type(r.y[0]+ymax[1], r));

		// insert row into id map.
		for (AssetInfoVec::const_iterator it = p.first->second.assets.begin(); it != p.first->second.assets.end(); ++it)
		{
			const AssetInfo &info = *it;

			IdPos pos;
			pos.id = info.id;
			pos.idx = (int)(it-p.first->second.assets.begin());
			pos.row = p.first;
			m_ids.insert(IdPos::Map::value_type(info.id, pos));
		}
	}

	m_y[1] = y;

	if (y > vh)
	{
		int maxy = y - vh;
		m_scroll->setRange(0, maxy);
		m_y[0] = std::min(m_y[0], maxy);
		m_scroll->setValue(m_y[0]);
		ShowScroll(true);
	}
	else
	{
		m_y[0] = 0;
		ShowScroll(false);
	}

	{
		SelSet unsel;
		for (SelSet::const_iterator it = sel.begin(); it != sel.end(); ++it)
		{
			if (m_sel.find(*it) == m_sel.end())
			{
				unsel.insert(*it);
			}
		}

		sel.clear();
		if (!unsel.empty())
			DoSelectionChange(sel, unsel, m_sel);
	}
}

void ContentBrowserView::DrawThumb(ContentAssetThumb *thumb, const pkg::Package::Entry::Ref &asset, int x, int y, int w, int h)
{
	if (!thumb || !thumb->Render(asset, x, y, w, h))
	{
		r::gls.DisableTextures();
		r::gls.Set(
			r::DT_Disable|r::DWM_Disable, 
			r::BMS_One|r::BMD_InvSrcAlpha
		);
		r::gls.Commit();

		glColor4f(0.0f, 0.f, 0.f, 1.0f);
		glBegin(GL_QUADS);
			glVertex2i(x, y);
			glVertex2i(x+w, y);
			glVertex2i(x+w, y+h);
			glVertex2i(x, y+h);
		glEnd();

		glColor4f(0.2f, 1.f, 0.3f, 1.f);

		QFontMetrics fm(m_font);
		QString type(asset::TypeString(asset->type));
		int fw = fm.width(type);
		int fh = fm.height();

		m_glw->renderText(
			x+(w-fw)/2, 
			y+h/2+fh,
			0.0,
			type,
			m_font
		);
	}
}

void ContentBrowserView::OnRenderGL(GLWidget&)
{
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	QFontMetrics fm(m_font);
	int fh = fm.height();
	int vh = m_glw->height();

	glColor4f(1.f, 1.f, 1.f, 1.f);
	r::gls.BindBuffer(GL_ARRAY_BUFFER_ARB, r::GLVertexBufferRef());
	r::gls.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, r::GLVertexBufferRef());

	Row::Map::const_iterator it = m_rows.lower_bound(m_y[0]);
	if (it != m_rows.end())
	{
		for (ContentAssetThumb::Map::const_iterator tit = m_thumbs.begin(); tit != m_thumbs.end(); ++tit)
		{
			tit->second->Begin();
		}

		r::gl.OrthoViewport(0, m_y[0], m_glw->width(), m_glw->height());

		for (;it != m_rows.end(); ++it)
		{
			const Row &r = it->second;
			if (r.y[0]-m_y[0] > vh) break;

			for (AssetInfoVec::const_iterator it = r.assets.begin(); it != r.assets.end(); ++it)
			{
				const AssetInfo &info = *it;
				pkg::Package::Entry::Ref asset = Packages()->FindEntry(info.id);
				if (!asset) { continue; }
				ContentAssetThumb *thumb = ThumbForType(asset->type);

				int maxy = m_maxs[1] ? std::min(m_maxs[1], info.ih) : info.ih;
				bool selected = m_sel.find(info.id) != m_sel.end();

				if (selected)
				{
					r::gls.DisableTextures();
					r::gls.Set(
						r::DT_Disable|r::DWM_Disable, 
						r::BMS_One|r::BMD_InvSrcAlpha
					);
					r::gls.Commit();

					glColor4f(1.0f, 0.f, 0.f, 1.0f);
					glBegin(GL_QUADS);
						glVertex2i(info.x-HItemSpace/2+1, r.y[0]-VItemSpace/2);
						glVertex2i(info.x+info.w+HItemSpace/2, r.y[0]-VItemSpace/2);
						glVertex2i(info.x+info.w+HItemSpace/2, r.y[0]+r.y[1]+fh+VLabelBorder+VItemSpace/2);
						glVertex2i(info.x-HItemSpace/2+1, r.y[0]+r.y[1]+fh+VLabelBorder+VItemSpace/2);
					glEnd();
					glColor4f(0.0f, 0.f, 0.f, 1.0f);
					glBegin(GL_QUADS);
						glVertex2i(info.x, r.y[0]);
						glVertex2i(info.x+info.w, r.y[0]);
						glVertex2i(info.x+info.w, r.y[0]+r.y[1]);
						glVertex2i(info.x, r.y[0]+r.y[1]);
					glEnd();
				}
				
				glColor4f(1.f, 1.f, 1.f, 1.f);

				DrawThumb(
					thumb, 
					asset,
					info.x + info.w/2 - info.iw/2, 
					info.y + maxy/2 - info.ih/2, 
					info.iw,
					info.ih
				);

				r::gls.DisableTextures();
				r::gls.Set(
					r::DT_Disable|r::DWM_Disable, 
					r::BMS_One|r::BMD_InvSrcAlpha
				);
				r::gls.Commit();

				if (info.id == m_hoverId)
				{
					glColor4f(1.f, 1.f, 0.5f, 1.0f);
					glBegin(GL_LINE_LOOP);
						glVertex2i(info.x-HItemSpace/2+1, r.y[0]-VItemSpace/2);
						glVertex2i(info.x+info.w+HItemSpace/2, r.y[0]-VItemSpace/2);
						glVertex2i(info.x+info.w+HItemSpace/2, r.y[0]+r.y[1]+fh+VLabelBorder+VItemSpace/2);
						glVertex2i(info.x-HItemSpace/2+1, r.y[0]+r.y[1]+fh+VLabelBorder+VItemSpace/2);
					glEnd();
				}
				
				if (!selected)
				{
					glColor4f(0.0f, 0.2f, 0.8f, 1.0f);
					glBegin(GL_QUADS);
						glVertex2i(info.x, r.y[0]+r.y[1]+VLabelBorder);
						glVertex2i(info.x+info.w, r.y[0]+r.y[1]+VLabelBorder);
						glVertex2i(info.x+info.w, r.y[0]+r.y[1]+fh+VLabelBorder);
						glVertex2i(info.x, r.y[0]+r.y[1]+fh+VLabelBorder);
					glEnd();
				}

				glColor4f(1.f, 1.f, 1.f, 1.f);
				m_glw->renderText(
					info.x+info.w/2-info.tw/2, 
					r.y[0]+r.y[1]+VLabelBorder+VLabelBorder/2+fh/2,
					0.0,
					QString(asset->path),
					m_font
				);
			}
		}

		for (ContentAssetThumb::Map::const_iterator tit = m_thumbs.begin(); tit != m_thumbs.end(); ++tit)
		{
			tit->second->End();
		}
	}
}

void ContentBrowserView::OnScrollMove(int pos)
{
	if (m_y[0] != pos)
	{
		m_y[0] = pos;
		Update();
	}
}

void ContentBrowserView::LoadIcons()
{
	if (m_loadedIcons)
		return;

	m_loadedIcons = true;

	static const char *s_filenames[I_Max] =
	{
		"Editor/disk_blue.png",
		"Editor/disks.png",
		"Editor/disk_blue_error.png"
	};

	for (int i = 0; i < I_Max; ++i)
	{
		file::MMapping::Ref mm = Files()->MapFile(s_filenames[i], ZTools);

		if (mm)
		{
			image_codec::Image img;

			if (image_codec::png::Decode(
				mm->data,
				mm->size,
				img
			))
			{
				m_icons[i] = r::GLTexture::UploadImage(
					img,
					r::TX_Filter|r::TX_WrapAll,
					48,
					48
				);
			}
		}
	}
}

} // editor
} // tools

#include "moc_EditorContentBrowserView.cc"
