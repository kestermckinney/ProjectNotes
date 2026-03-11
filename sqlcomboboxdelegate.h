// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SQLCOMBOBOXDELEGATE_H
#define SQLCOMBOBOXDELEGATE_H
#include "databaseobjects.h"

#include <QCompleter>
#include <QStyledItemDelegate>

class SqlComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SqlComboBoxDelegate(QObject *parent, SortFilterProxyModel *model, int displaycolumn = 1, int datacolumn = 0);

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    SortFilterProxyModel* m_model;
    int m_displayColumn;
    int m_dataColumn;
};

#endif // SQLCOMBOBOXDELEGATE_H
