#ifndef PNCOMBOBOXDELEGATE_H
#define PNCOMBOBOXDELEGATE_H
#include "pnsqlquerymodel.h"

#include <QItemDelegate>

class PNComboBoxDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    PNComboBoxDelegate(QObject *t_parent, PNSqlQueryModel *t_model, int t_displaycolumn = 1);

    QWidget* createEditor(QWidget *t_parent, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void setEditorData(QWidget *t_editor, const QModelIndex &t_index) const;
    void setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const;
    void updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;

private:
    PNSqlQueryModel* m_model;
    int m_display_column;
};

#endif // PNCOMBOBOXDELEGATE_H
