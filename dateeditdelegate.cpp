// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
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
#include <QKeyEvent>

DateEditDelegate::DateEditDelegate(QObject *parent)
:QStyledItemDelegate(parent)
{

}


QWidget *DateEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    if (m_readOnly)
        return nullptr;

    DateEditEx* editor = new DateEditEx(parent);

    editor->setDisplayFormat("MM/dd/yyyy");
    editor->setProperty("EditMask","MM/dd/yyyy");
    editor->setCalendarPopup(true);
    editor->setNullable(true);

    editor->installEventFilter(const_cast<DateEditDelegate*>(this));
    if (QLineEdit* le = editor->getLineEdit())
        le->installEventFilter(const_cast<DateEditDelegate*>(this));

    return editor;
}

bool DateEditDelegate::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
        {
            // object may be the internal QLineEdit child — walk up to find the DateEditEx
            DateEditEx *editor = qobject_cast<DateEditEx*>(object);
            if (!editor)
                editor = qobject_cast<DateEditEx*>(static_cast<QWidget*>(object)->parent());

            if (editor)
            {
                emit commitData(editor);
                emit closeEditor(editor, keyEvent->key() == Qt::Key_Tab
                                         ? QAbstractItemDelegate::EditNextItem
                                         : QAbstractItemDelegate::EditPreviousItem);
                return true;
            }
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}

void DateEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    DateEditEx *dateedit = static_cast<DateEditEx*>(editor);
    QString value = index.model()->data(index, Qt::EditRole).toString();
    QDateTime datevalue = SqlQueryModel::parseDateTime(value);

    dateedit->setDate(datevalue.date());

    QLineEdit *le = dateedit->getLineEdit();
    if (le)
        le->setFocus(Qt::TabFocusReason);
    else
        dateedit->setFocus(Qt::TabFocusReason);
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
    QStyleOptionViewItem myOption = option;

    QVariant bgcolor = index.model()->data(index, Qt::BackgroundRole);
    if (!bgcolor.isValid() && m_readOnly)
        bgcolor = QApplication::palette().color(QPalette::Button);

    painter->save();

    QColor fillColor = bgcolor.isValid() ? bgcolor.value<QColor>() : option.palette.base().color();
    painter->fillRect(option.rect, fillColor);

    if (bgcolor.isValid())
    {
        myOption.palette.setColor(QPalette::Base, bgcolor.value<QColor>());
        myOption.palette.setColor(QPalette::AlternateBase, bgcolor.value<QColor>());
    }

    QStyledItemDelegate::paint(painter, myOption, index);

    painter->restore();
}


