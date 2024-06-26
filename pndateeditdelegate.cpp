// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pndateeditdelegate.h"
#include "pnsqlquerymodel.h"
#include "pndateeditex.h"

#include <QCompleter>
#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>

PNDateEditDelegate::PNDateEditDelegate(QObject *t_parent)
:QStyledItemDelegate(t_parent)
{

}


QWidget *PNDateEditDelegate::createEditor(QWidget *t_parent, const QStyleOptionViewItem &/* t_option */, const QModelIndex &/* t_index */) const
{
    PNDateEditEx* editor = new PNDateEditEx(t_parent);

    editor->setDisplayFormat("MM/dd/yyyy");
    editor->setProperty("EditMask","MM/dd/yyyy");
    editor->setCalendarPopup(true);
    editor->setNullable(true);

    return editor;
}

void PNDateEditDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    PNDateEditEx *dateedit = static_cast<PNDateEditEx*>(t_editor);
    QString t_value = t_index.model()->data(t_index, Qt::EditRole).toString();
    QDateTime datevalue = PNSqlQueryModel::parseDateTime(t_value);

    dateedit->setDate(datevalue.date());
}

void PNDateEditDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    PNDateEditEx *dateedit = static_cast<PNDateEditEx*>(t_editor);
    t_model->setData(t_index, dateedit->date().toString("MM/dd/yyyy"), Qt::EditRole);
}

void PNDateEditDelegate::updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &/* t_index */) const
{
    t_editor->setGeometry(t_option.rect);
}

void PNDateEditDelegate::paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const
{
    QStyleOptionComboBox dateOption;
    dateOption.rect = t_option.rect;
    dateOption.state = t_option.state | QStyle::State_Enabled;
    dateOption.currentText = t_index.data(Qt::DisplayRole).toString();

    QVariant bgcolor = t_index.model()->data(t_index, Qt::BackgroundRole);
    QVariant fgcolor = t_index.model()->data(t_index, Qt::ForegroundRole);

    t_painter->save();

    if (fgcolor.isValid())
    {
        dateOption.palette.setColor(QPalette::BrightText, fgcolor.value<QColor>());
        t_painter->setPen(fgcolor.value<QColor>());
    }

    if (bgcolor.isValid())
        dateOption.palette.setColor(QPalette::Base, bgcolor.value<QColor>());

    QApplication::style()->drawComplexControl(QStyle::CC_ComboBox, &dateOption, t_painter);
    QApplication::style()->drawControl(QStyle::CE_ComboBoxLabel, &dateOption, t_painter);
    t_painter->restore();
}


