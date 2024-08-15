// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "comboboxdelegate.h"

#include "pncombobox.h"
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>

ComboBoxDelegate::ComboBoxDelegate(QObject *t_parent, QStringListModel *t_model)
:QStyledItemDelegate(t_parent)
{
    m_model = t_model;
}


QWidget *ComboBoxDelegate::createEditor(QWidget *t_parent, const QStyleOptionViewItem &/* t_option */, const QModelIndex &/* t_index */) const
{
    PNComboBox* editor = new PNComboBox(t_parent);
    editor->setEditable(false);
    editor->setModel(m_model);
    editor->setModelColumn(0); // column to display

    return editor;
}

void ComboBoxDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    PNComboBox *comboBox = static_cast<PNComboBox*>(t_editor);
    QVariant t_value = t_index.model()->data(t_index);

    comboBox->setCurrentText(t_value.toString());
}

void ComboBoxDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    PNComboBox *comboBox = static_cast<PNComboBox*>(t_editor);
    t_model->setData(t_index, m_model->data(m_model->index(comboBox->currentIndex(), 0)), Qt::EditRole);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &/* t_index */) const
{
    t_editor->setGeometry(t_option.rect);
}

void ComboBoxDelegate::paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const
{
    QStyleOptionViewItem myOption = t_option;
    myOption.rect = t_option.rect;

    QVariant bgcolor = t_index.model()->data(t_index, Qt::BackgroundRole);
    QVariant fgcolor = t_index.model()->data(t_index, Qt::ForegroundRole);

    t_painter->save();
    if (fgcolor.isValid())
    {
        myOption.palette.setColor(QPalette::Text, fgcolor.value<QColor>());
        t_painter->setPen(fgcolor.value<QColor>());
    }

    if (bgcolor.isValid())
        myOption.palette.setColor(QPalette::Base, bgcolor.value<QColor>());

    if (t_option.state & QStyle::State_Selected)
    {
        t_painter->fillRect(t_option.rect, t_option.palette.highlight());
    }
    QStyledItemDelegate::paint(t_painter, myOption, t_index);

    t_painter->restore();
}
