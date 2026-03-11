// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "dateeditdelegate.h"
#include "databaseobjects.h"
#include "dateeditex.h"

#include <QCompleter>
#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>

DateEditDelegate::DateEditDelegate(QObject *parent)
:QStyledItemDelegate(parent)
{

}


QWidget *DateEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    DateEditEx* editor = new DateEditEx(parent);

    editor->setDisplayFormat("MM/dd/yyyy");
    editor->setProperty("EditMask","MM/dd/yyyy");
    editor->setCalendarPopup(true);
    editor->setNullable(true);

    return editor;
}

void DateEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    DateEditEx *dateedit = static_cast<DateEditEx*>(editor);
    QString value = index.model()->data(index, Qt::EditRole).toString();
    QDateTime datevalue = SqlQueryModel::parseDateTime(value);

    dateedit->setDate(datevalue.date());
}

void DateEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    DateEditEx *dateedit = static_cast<DateEditEx*>(editor);
    model->setData(index, dateedit->date().toString("MM/dd/yyyy"), Qt::EditRole);
}

void DateEditDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

void DateEditDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    painter->fillRect(option.rect, option.palette.base().color());

    QStyledItemDelegate::paint(painter, option, index);

    painter->restore();
}


