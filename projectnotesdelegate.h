#ifndef PROJECTNOTESDELEGATE_H
#define PROJECTNOTESDELEGATE_H

#include <QStyledItemDelegate>
#include <QObject>

class ProjectNotesDelegate : public QStyledItemDelegate
{
public:
    explicit ProjectNotesDelegate(QObject *parent = nullptr);
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

#endif // PROJECTNOTESDELEGATE_H
