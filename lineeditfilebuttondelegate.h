// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef LINEEDITFILEBUTTONDELEGATE_H
#define LINEEDITFILEBUTTONDELEGATE_H

#include <QStyledItemDelegate>

class LineEditFileButtonDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit LineEditFileButtonDelegate(QObject *parent);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:

};

#endif // LINEEDITFILEBUTTONDELEGATE_H
