// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pncomboboxdelegate.h"
#include "pncombobox.h"

#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


PNComboBoxDelegate::PNComboBoxDelegate(QObject *t_parent, PNSqlQueryModel *t_model, int t_displaycolumn, int t_datacolumn)
:QStyledItemDelegate(t_parent)
{
    m_model = t_model;
    m_display_column = t_displaycolumn;
    m_data_column = t_datacolumn;
}

QWidget* PNComboBoxDelegate::createEditor(QWidget *t_parent, const QStyleOptionViewItem& t_option, const QModelIndex& t_index ) const
{
    Q_UNUSED(t_option);
    Q_UNUSED(t_index);

    PNComboBox* editor = new PNComboBox(t_parent);
    editor->setEditable(true);
    editor->setModel(m_model);
    editor->setModelColumn(m_display_column); // column to display

    QCompleter* completer = new QCompleter(m_model);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionColumn(m_display_column);
    editor->setCompleter(completer);

    return editor;
}

void PNComboBoxDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    PNComboBox *comboBox = static_cast<PNComboBox*>(t_editor);
    QVariant lookupvalue = t_index.model()->data(t_index);
    QString value = m_model->findValue(lookupvalue, m_data_column, m_display_column).toString();

    comboBox->setCurrentText(value);
}

void PNComboBoxDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    PNComboBox *comboBox = static_cast<PNComboBox*>(t_editor);
    int i;
    QVariant key_val;

    if (!comboBox->currentText().isEmpty() )
    {
        i = comboBox->findText(comboBox->currentText(), Qt::MatchFixedString);
        if (i >= 0)
        {
            comboBox->setCurrentIndex(i);
            key_val = m_model->data(m_model->index(i, m_data_column));
        }
    }

    t_model->setData(t_index, key_val, Qt::EditRole);
}

void PNComboBoxDelegate::updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &/* t_index */) const
{
    t_editor->setGeometry(t_option.rect);
}

void PNComboBoxDelegate::paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const
{
    QStyleOptionViewItem myOption = t_option;
    QVariant lookupvalue = t_index.model()->data(t_index);

    myOption.text = m_model->findValue(lookupvalue, m_data_column, m_display_column).toString();

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, t_painter);
}
