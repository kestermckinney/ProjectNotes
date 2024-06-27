// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNDATEEDITDELEGATE_H
#define PNDATEEDITDELEGATE_H

#include <QStyledItemDelegate>

class PNDateEditDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PNDateEditDelegate(QObject *t_parent);

    QWidget *createEditor(QWidget *t_parent, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void setEditorData(QWidget *t_editor, const QModelIndex &t_index) const;
    void setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const;
    void updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;
    void paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const;

private:

};

#endif // PNDATEEDITDELEGATE_H
