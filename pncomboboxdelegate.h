#ifndef PNCOMBOBOXDELEGATE_H
#define PNCOMBOBOXDELEGATE_H
#include "pnsqlquerymodel.h"

#include <QStyledItemDelegate>

class PNComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    PNComboBoxDelegate(QObject *t_parent, PNSqlQueryModel *t_model, int t_displaycolumn = 1, int t_datacolumn = 0);

    QWidget* createEditor(QWidget *t_parent, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void setEditorData(QWidget *t_editor, const QModelIndex &t_index) const;
    void setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const;
    void updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;

private:
    PNSqlQueryModel* m_model;
    int m_display_column;
    int m_data_column;
};

#endif // PNCOMBOBOXDELEGATE_H
