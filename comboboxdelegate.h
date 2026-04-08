// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
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
    ComboBoxDelegate(QObject *parent, QStringListModel *model);

    void setReadOnly(bool readOnly) { m_readOnly = readOnly; }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    QStringListModel* m_model;
    bool m_readOnly = false;
};

#endif // COMBOBOXDELEGATE_H
