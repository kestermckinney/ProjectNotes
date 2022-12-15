#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H

#include <QStyledItemDelegate>
#include <QStringListModel>

class ComboBoxDelegate : public QStyledItemDelegate
{
public:
    Q_OBJECT

public:
    ComboBoxDelegate(QObject *t_parent, QStringListModel *t_model);

    QWidget *createEditor(QWidget *t_parent, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void setEditorData(QWidget *t_editor, const QModelIndex &t_index) const;
    void setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const;
    void updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;

private:
    QStringListModel* m_model;
};

#endif // COMBOBOXDELEGATE_H
