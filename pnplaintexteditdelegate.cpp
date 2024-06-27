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
    t_model->setData(t_index, editor->toPlainText(), Qt::EditRole);
}

void PNPlainTextEditDelegate::updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &/* t_index */) const
{
    t_editor->setGeometry(t_option.rect);
}

void PNPlainTextEditDelegate::paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const
{
    QStyleOptionViewItem myOption = t_option;

    QVariant bgcolor = t_index.model()->data(t_index, Qt::BackgroundRole);
    QVariant fgcolor = t_index.model()->data(t_index, Qt::ForegroundRole);

    if (fgcolor.isValid())
        myOption.palette.setColor(QPalette::Text, fgcolor.value<QColor>());

    if (bgcolor.isValid())
        myOption.backgroundBrush = QBrush(bgcolor.value<QColor>());

    QStyledItemDelegate::paint(t_painter, myOption, t_index);
}

