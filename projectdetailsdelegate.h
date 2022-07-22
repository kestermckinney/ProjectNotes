#ifndef PROJECTDETAILSDELEGATE_H
#define PROJECTDETAILSDELEGATE_H

#include <QItemDelegate>
#include <QObject>

class ProjectDetailsDelegate : public QItemDelegate
{
public:
    explicit ProjectDetailsDelegate(QObject *parent = nullptr);
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

#endif // PROJECTDETAILSDELEGATE_H
