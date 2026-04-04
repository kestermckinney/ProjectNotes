// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "lineeditfilebuttondelegate.h"
#include "lineeditfilebutton.h"

#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>

LineEditFileButtonDelegate::LineEditFileButtonDelegate(QObject *parent)
:QStyledItemDelegate(parent)
{

}


QWidget *LineEditFileButtonDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    LineEditFileButton* editor = new LineEditFileButton(parent);

    return editor;
}

void LineEditFileButtonDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    LineEditFileButton *filename_edit = static_cast<LineEditFileButton*>(editor);
    QString val = index.model()->data(index, Qt::EditRole).toString();

    filename_edit->setFileName(val);
}

void LineEditFileButtonDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    LineEditFileButton *filename_edit = static_cast<LineEditFileButton*>(editor);
    model->setData(index, filename_edit->fileName(), Qt::EditRole);
}

void LineEditFileButtonDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
