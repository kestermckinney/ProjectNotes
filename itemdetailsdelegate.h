#ifndef ITEMDETAILSDELEGATE_H
#define ITEMDETAILSDELEGATE_H

#include <QStyledItemDelegate>
#include <QObject>

class ItemDetailsDelegate : public QStyledItemDelegate
{
public:
    explicit ItemDetailsDelegate(QObject *parent = nullptr);
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    bool verifyProjectNumber(QVariant& t_project_id, QVariant& t_item_id) const;
};

#endif // ITEMDETAILSDELEGATE_H
