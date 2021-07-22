#ifndef PNCOMBOBOXDELEGATE_H
#define PNCOMBOBOXDELEGATE_H
#include "pnsqlquerymodel.h"

#include <QItemDelegate>

class PNComboBoxDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    PNComboBoxDelegate(QObject *parent, PNSqlQueryModel *model, int DisplayColumn = 1);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    PNSqlQueryModel* m_Model;
    int m_DisplayColumn;
};

#endif // PNCOMBOBOXDELEGATE_H
