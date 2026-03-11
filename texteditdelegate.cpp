// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#include "texteditdelegate.h"

#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>
#include <QTextEdit>

TextEditDelegate::TextEditDelegate(QObject *parent)
:QStyledItemDelegate(parent)
{

}


QWidget *TextEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QTextEdit* editor = new QTextEdit(parent);

    editor->setFont(QFont("Arial", 11));
    editor->setFontFamily("Arial");
    editor->setFontPointSize(11);

    return editor;
}

void TextEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QTextEdit *text_edit = static_cast<QTextEdit*>(editor);
    QString val = index.model()->data(index, Qt::EditRole).toString();

    text_edit->setHtml(val);

    if (val.isEmpty())
    {
        dynamic_cast<QTextEdit*>(editor)->setFont(QFont("Arial", 11));
        dynamic_cast<QTextEdit*>(editor)->setFontFamily("Arial");
        dynamic_cast<QTextEdit*>(editor)->setFontPointSize(11);
    }
}

void TextEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QTextEdit *text_edit = static_cast<QTextEdit*>(editor);
    model->setData(index, text_edit->toHtml(), Qt::EditRole);
}

void TextEditDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

void TextEditDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem myOption = option;

    QString val = index.model()->data(index, Qt::EditRole).toString();

    QTextEdit* te = new QTextEdit();
    te->setHtml(val);

    myOption.text = te->toPlainText();

    QVariant bgcolor = index.model()->data(index, Qt::BackgroundRole);
    QVariant fgcolor = index.model()->data(index, Qt::ForegroundRole);

    if (fgcolor.isValid())
        myOption.palette.setColor(QPalette::Text, fgcolor.value<QColor>());

    if (bgcolor.isValid())
        myOption.backgroundBrush = QBrush(bgcolor.value<QColor>());

    delete te;

    QStyledItemDelegate::paint(painter, myOption, index);
}
