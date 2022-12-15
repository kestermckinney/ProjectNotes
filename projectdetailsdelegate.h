#ifndef PROJECTDETAILSDELEGATE_H
#define PROJECTDETAILSDELEGATE_H

#include <QStyledItemDelegate>
#include <QObject>

class ProjectDetailsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ProjectDetailsDelegate(QObject *parent = nullptr);
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

#endif // PROJECTDETAILSDELEGATE_H
