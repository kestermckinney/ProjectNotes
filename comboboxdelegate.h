// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

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

    QWidget *createEditor(QWidget *t_parent, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const override;
    void setEditorData(QWidget *t_editor, const QModelIndex &t_index) const override;
    void setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const override;
    void updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const override;

private:
    QStringListModel* m_model;
};

#endif // COMBOBOXDELEGATE_H
