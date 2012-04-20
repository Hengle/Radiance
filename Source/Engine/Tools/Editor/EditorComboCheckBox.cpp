/***************************************************************************************************************
**
**	ComboCheckBox head file
**
**	thanks jpn  (http://www.qtcentre.org/forum/f-qt-programming-2/t-checkcombobox-how-to-do-3411.html#post18648)
**
*****************************************************************************************************************/
// See Radiance/LICENSE for licensing terms.

#include "EditorComboCheckBox.h"
#include <QtGui/QItemDelegate>
#include <QtGui/QStandardItemModel>
#include <QtGui/QListView>
#include <QtGui/QMouseEvent>
#include <QtGui/QLineEdit>
#include <QtCore/QStringList>
#include <QtCore/QEvent>

namespace tools {
namespace editor {

namespace {

// CheckView
class CheckView: public QListView
{
public:
	CheckView(QWidget* parent = 0): QListView(parent)
	{
	}

	bool eventFilter(QObject* object, QEvent* event)
	{
		if (event->type() == QEvent::MouseButtonRelease)
		{
			QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
			QModelIndex index = indexAt(mouse->pos());
			if (index.isValid())
			{
				// check if the mouse was released over the checkbox
				QStyleOptionButton option;
				option.QStyleOption::operator=(viewOptions());
				option.rect = visualRect(index);
				QRect rect = style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &option);
				if (rect.contains(mouse->pos()))
				{
					// mouse was release over a checkbox
					// bypass combobox and deliver the event just for the listview
					QListView::mouseReleaseEvent(mouse);
				}
			}
			return true;
		}
		return false;
	}
};

// CheckModel
class CheckModel: public QStandardItemModel
{
public:
	CheckModel(QObject* parent = 0): QStandardItemModel(parent)
	{
		// a must
		insertColumn(0);
	}

	Qt::ItemFlags flags(const QModelIndex& index) const
	{
		return QStandardItemModel::flags(index) | Qt::ItemIsUserCheckable;
	}
};

} // namespace

// ComboCheckBox
ComboCheckBox::ComboCheckBox (QWidget * parent , bool bHaveSelectAll/*=false*/) :  QComboBox(parent)
{
	setEditable(true);
	lineEdit()->setReadOnly(true);
	connect(lineEdit(), SIGNAL(textChanged(const QString &)), this, SLOT(_editTextChanged ()));
	CheckModel* model = new CheckModel(this);
	CheckView*  view  = new CheckView(this);
	setModel(model);
	setView(view);
	// these 2 lines below are important and must be
	// applied AFTER QComboBox::setView() because
	// QComboBox installs it's own filter on the view
	view->installEventFilter(view);				// <--- !!!
	view->viewport()->installEventFilter(view);	// <---	!!!

	setMinimumWidth(100);

	connect(view, SIGNAL(clicked(const QModelIndex &)), this, SLOT(_itemCB(const QModelIndex &)));

	_bHaveSelectAll = bHaveSelectAll;

	if(bHaveSelectAll)
	{
		addItem(tr("Select All"));
	}
 }

ComboCheckBox::~ComboCheckBox()
{
	_mapSelString.empty();
}

int ComboCheckBox::addItem(QString text, bool bCheck/*=false*/)
{
	QComboBox::addItem(text);

	int nCount = count();

	blockSignals(true);
	setCheck(nCount-1, bCheck);
	blockSignals(false);

	return nCount-1;
}

void ComboCheckBox::setCheck(int nIndex, bool bCheck)
{
	int nCount = count();
	if(nIndex<0 || nIndex>=nCount) return;

	bool changed = false;

	if(bCheck)
	{
		QComboBox::setItemData(nIndex, Qt::Checked, Qt::CheckStateRole);
		if(_mapSelString.find(nIndex) == _mapSelString.end())
		{
			QString str = QComboBox::itemText(nIndex);
			_mapSelString.insert(nIndex, str);
			changed = true;
		}
	}
	else
	{
		QComboBox::setItemData(nIndex, Qt::Unchecked, Qt::CheckStateRole); 
		if(_mapSelString.find(nIndex) != _mapSelString.end())
		{
			_mapSelString.remove(nIndex);
			changed = false;
		}
	}

	_editTextChanged ();

	if (changed && (nIndex>=0||!_bHaveSelectAll))
		emit OnItemChecked(nIndex, bCheck);
}

bool ComboCheckBox::isChecked(int nIndex)
{
	int nCount = count();
	if(nIndex<0 || nIndex>=nCount) return false;

	QVariant var = QComboBox::itemData(nIndex, Qt::CheckStateRole);
	if(var == Qt::Checked)
		return true;
	else
		return false;
}

void ComboCheckBox::_itemCB(const QModelIndex & index)
{
	int nIndex = index.row();

	if(_bHaveSelectAll && nIndex == 0)
	{
		SelectAll(isChecked(nIndex));
	}
	else
	{
		QString str = this->itemText(nIndex);
		bool changed = false;

		if(isChecked(nIndex))
		{
			if(_mapSelString.find(nIndex) == _mapSelString.end())
			{
				_mapSelString.insert(nIndex, str);
				changed = true;
			}
		}
		else
		{
			if(_bHaveSelectAll)
			{
				setCheck(0, false);
			}

			if(_mapSelString.find(nIndex) != _mapSelString.end())
			{
				_mapSelString.remove(nIndex);
				changed = true;
			}
		}

		_editTextChanged();

		if (changed)
			emit OnItemChecked(nIndex, isChecked(nIndex));
	}
}

void ComboCheckBox::_editTextChanged ()
{
	QString str;

	int nStart = 0;
	if(_bHaveSelectAll) 
		nStart = 1;

	if (!m_all.isEmpty() && _mapSelString.count() >= (count()-nStart))
	{
		str = m_all;
	}
	else if (!m_multi.isEmpty() && _mapSelString.count() > 1)
	{
		str = m_multi;
	}
	else
	{
		for (int i=nStart; i< count(); i++)
		{
			if(_mapSelString.value(i)!="")
			{
				if (!str.isEmpty())
					str += ";";
				str += _mapSelString.value(i);
			}
		}
	}

	this->setEditText(str);
	lineEdit()->setCursorPosition(0);
	this->setToolTip(str);
}

void ComboCheckBox::getSelStringList(QStringList &stSelectedList)
{
	stSelectedList.empty();

	int nStart = 0;
	if(_bHaveSelectAll) nStart = 1;
    for (int i=nStart; i< count(); i++)
	{
		if(_mapSelString.value(i)!="")
		{
			stSelectedList.append(_mapSelString.value(i));
		}
	}
}

QString ComboCheckBox::getSelString()
{
	QString str;
	int nStart = 0;
	if(_bHaveSelectAll) nStart = 1;
    for (int i=nStart; i< count(); i++)
	{
		if(_mapSelString.value(i)!="")
		{
			if (!str.isEmpty())
				str += ";";
			str += _mapSelString.value(i);
		}
	}
	return str;
}

void ComboCheckBox::SelectAll(bool bCheck)
{
	blockSignals(true);
	for(int i=0; i<count(); i++)
	{
		setCheck(i, bCheck);
	}
	blockSignals(false);
	emit OnAllChecked(bCheck);
}

} // editor
} // tools

#include "moc_EditorComboCheckBox.cc"
