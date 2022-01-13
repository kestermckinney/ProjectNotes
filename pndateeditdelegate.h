#ifndef PNDATEEDITDELEGATE_H
#define PNDATEEDITDELEGATE_H

#include <QItemDelegate>

class PNDateEditDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    PNDateEditDelegate(QObject *t_parent);

    QWidget *createEditor(QWidget *t_parent, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void setEditorData(QWidget *t_editor, const QModelIndex &t_index) const;
    void setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const;
    void updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;

private:

};

#endif // PNDATEEDITDELEGATE_H
