// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pnlineeditfilebuttondelegate.h"
#include "pnlineeditfilebutton.h"

#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>

PNLineEditFileButtonDelegate::PNLineEditFileButtonDelegate(QObject *t_parent)
:QStyledItemDelegate(t_parent)
{

}


QWidget *PNLineEditFileButtonDelegate::createEditor(QWidget *t_parent, const QStyleOptionViewItem &/* t_option */, const QModelIndex &/* t_index */) const
{
    PNLineEditFileButton* editor = new PNLineEditFileButton(t_parent);

    return editor;
}

void PNLineEditFileButtonDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    PNLineEditFileButton *filename_edit = static_cast<PNLineEditFileButton*>(t_editor);
    QString val = t_index.model()->data(t_index, Qt::EditRole).toString();

    filename_edit->setFileName(val);
}

void PNLineEditFileButtonDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    PNLineEditFileButton *filename_edit = static_cast<PNLineEditFileButton*>(t_editor);
    t_model->setData(t_index, filename_edit->fileName(), Qt::EditRole);
}

void PNLineEditFileButtonDelegate::updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &/* t_index */) const
{
    t_editor->setGeometry(t_option.rect);
}
