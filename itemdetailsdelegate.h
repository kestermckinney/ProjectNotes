#ifndef ITEMDETAILSDELEGATE_H
#define ITEMDETAILSDELEGATE_H

#include <QItemDelegate>
#include <QObject>

class ItemDetailsDelegate : public QItemDelegate
{
public:
    explicit ItemDetailsDelegate(QObject *parent = nullptr);
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

#endif // ITEMDETAILSDELEGATE_H
