// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pnplaintexteditdelegate.h"
#include "pnplaintextedit.h"
#include <QCompleter>
#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>

PNPlainTextEditDelegate::PNPlainTextEditDelegate(QObject *t_parent)
:QStyledItemDelegate(t_parent)
{

}


QWidget *PNPlainTextEditDelegate::createEditor(QWidget *t_parent, const QStyleOptionViewItem &/* t_option */, const QModelIndex &/* t_index */) const
{
    PNPlainTextEdit* editor = new PNPlainTextEdit(t_parent);

    return editor;
}

void PNPlainTextEditDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    PNPlainTextEdit *editor = static_cast<PNPlainTextEdit*>(t_editor);
    QString t_value = t_index.model()->data(t_index, Qt::EditRole).toString();

    if (editor->toPlainText().compare(t_value))
        editor->setPlainText(t_value);
}

void PNPlainTextEditDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    PNPlainTextEdit *editor = static_cast<PNPlainTextEdit*>(t_editor);
    editor->setAutoFillBackground(true);

    t_model->setData(t_index, editor->toPlainText(), Qt::EditRole);
}


