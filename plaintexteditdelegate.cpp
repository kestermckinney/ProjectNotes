// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "plaintexteditdelegate.h"
#include "plaintextedit.h"
#include <QCompleter>
#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>

PlainTextEditDelegate::PlainTextEditDelegate(QObject *parent)
:QStyledItemDelegate(parent)
{

}


QWidget *PlainTextEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    PlainTextEdit* editor = new PlainTextEdit(parent);

    return editor;
}

void PlainTextEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    PlainTextEdit *plainTextEditor = static_cast<PlainTextEdit*>(editor);
    QString value = index.model()->data(index, Qt::EditRole).toString();

    if (plainTextEditor->toPlainText().compare(value))
        plainTextEditor->setPlainText(value);
}

void PlainTextEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    PlainTextEdit *plainTextEditor = static_cast<PlainTextEdit*>(editor);
    plainTextEditor->setAutoFillBackground(true);

    model->setData(index, plainTextEditor->toPlainText(), Qt::EditRole);
}


