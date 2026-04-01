// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "comboboxdelegate.h"

#include "combobox.h"
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>
#include <QKeyEvent>

ComboBoxDelegate::ComboBoxDelegate(QObject *parent, QStringListModel *model)
:QStyledItemDelegate(parent)
{
    m_model = model;
}


QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    ComboBox* editor = new ComboBox(parent);
    editor->setEditable(false);
    editor->setModel(m_model);
    editor->setModelColumn(0); // column to display

    editor->installEventFilter(const_cast<ComboBoxDelegate*>(this));

    return editor;
}

bool ComboBoxDelegate::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
        {
            ComboBox *editor = qobject_cast<ComboBox*>(object);
            if (!editor)
                editor = qobject_cast<ComboBox*>(static_cast<QWidget*>(object)->parent());

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

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ComboBox *comboBox = static_cast<ComboBox*>(editor);
    QVariant value = index.model()->data(index);

    comboBox->setCurrentText(value.toString());
    comboBox->setFocus(Qt::TabFocusReason);
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ComboBox *comboBox = static_cast<ComboBox*>(editor);
    model->setData(index, m_model->data(m_model->index(comboBox->currentIndex(), 0)), Qt::EditRole);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
