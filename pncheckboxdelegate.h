// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNCHECKBOXDELEGATE_H
#define PNCHECKBOXDELEGATE_H

#include "pndatabaseobjects.h"

#include <QStyledItemDelegate>
#include <QObject>

class PNCheckBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PNCheckBoxDelegate(QObject *t_parent);

    QWidget* createEditor(QWidget *t_parent, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const override;
    void setEditorData(QWidget *t_editor, const QModelIndex &t_index) const override;
    void setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const override;
    void updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const override;
    void paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const override;

private:
    Qt::ItemFlags flags ( const QModelIndex & index ) const;

    PNSqlQueryModel* m_model;
    int m_display_column;
};

#endif // PNCHECKBOXDELEGATE_H
