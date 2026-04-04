// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PLAINTEXTEDITDELEGATE_H
#define PLAINTEXTEDITDELEGATE_H

#include <QStyledItemDelegate>

class PlainTextEditDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PlainTextEditDelegate(QObject *parent);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:

};

#endif // PLAINTEXTEDITDELEGATE_H
