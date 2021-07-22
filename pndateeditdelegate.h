#ifndef PNDATEEDITDELEGATE_H
#define PNDATEEDITDELEGATE_H

#include <QItemDelegate>

class PNDateEditDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    PNDateEditDelegate(QObject *parent);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:

};

#endif // PNDATEEDITDELEGATE_H
