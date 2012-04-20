/***************************************************************************************************************
**
**	ComboCheckBox head file
**
**	thanks jpn  (http://www.qtcentre.org/forum/f-qt-programming-2/t-checkcombobox-how-to-do-3411.html#post18648)
**
*****************************************************************************************************************/
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <QtGui/QComboBox>

class QStringList;

namespace tools {
namespace editor {

class ComboCheckBox : public QComboBox
{
	Q_OBJECT

public:
	ComboCheckBox(QWidget *parent = 0 , bool bHaveSelectAll=true);
	~ComboCheckBox();

	int addItem(QString text, bool bCheck=false);
	void setCheck(int nIndex, bool bCheck);
	void SelectAll(bool bCheck);
	bool isChecked(int nIndex);
	void getSelStringList(QStringList &stSelectedList);
	QString getSelString();
	void setStrings(const QString &all, const QString &multi) { m_all = all; m_multi = multi; }

signals:

	void OnItemChecked(int index, bool checked);
	void OnAllChecked(bool checked);

public slots:
	void _itemCB(const QModelIndex &);
	void _editTextChanged();

private:

	QString m_all;
	QString m_multi;
	QMap<int, QString> _mapSelString;
	bool _bHaveSelectAll;
};

} // editor
} // tools

