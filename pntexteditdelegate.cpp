// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#include "pntexteditdelegate.h"

#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>
#include <QTextEdit>

PNTextEditDelegate::PNTextEditDelegate(QObject *t_parent)
:QStyledItemDelegate(t_parent)
{

}


QWidget *PNTextEditDelegate::createEditor(QWidget *t_parent, const QStyleOptionViewItem &/* t_option */, const QModelIndex &/* t_index */) const
{
    QTextEdit* editor = new QTextEdit(t_parent);

    return editor;
}

void PNTextEditDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    QTextEdit *text_edit = static_cast<QTextEdit*>(t_editor);
    QString val = t_index.model()->data(t_index, Qt::EditRole).toString();

    text_edit->setHtml(val);
}

void PNTextEditDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    QTextEdit *text_edit = static_cast<QTextEdit*>(t_editor);
    t_model->setData(t_index, text_edit->toHtml(), Qt::EditRole);
}

void PNTextEditDelegate::updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &/* t_index */) const
{
    t_editor->setGeometry(t_option.rect);
}

void PNTextEditDelegate::paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const
{
    QStyleOptionViewItem myOption = t_option;

    QString val = t_index.model()->data(t_index, Qt::EditRole).toString();

    QTextEdit* te = new QTextEdit();
    te->setHtml(val);

    myOption.text = te->toPlainText();

    QVariant bgcolor = t_index.model()->data(t_index, Qt::BackgroundRole);
    QVariant fgcolor = t_index.model()->data(t_index, Qt::ForegroundRole);

    if (fgcolor.isValid())
        myOption.palette.setColor(QPalette::Text, fgcolor.value<QColor>());

    if (bgcolor.isValid())
        myOption.backgroundBrush = QBrush(bgcolor.value<QColor>());

    delete te;

    QStyledItemDelegate::paint(t_painter, myOption, t_index);
}
