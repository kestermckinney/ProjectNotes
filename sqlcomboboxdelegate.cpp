// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "sqlcomboboxdelegate.h"
#include "combobox.h"

#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


SqlComboBoxDelegate::SqlComboBoxDelegate(QObject *parent, SortFilterProxyModel *model, int displaycolumn, int datacolumn)
:QStyledItemDelegate(parent)
{
    m_model = model;
    m_displayColumn = displaycolumn;
    m_dataColumn = datacolumn;
}

QWidget* SqlComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    dynamic_cast<SqlQueryModel*>(m_model->sourceModel())->refresh(); // had to refresh because inserted and updated rows wouldn't show in the completer

    ComboBox* editor = new ComboBox(parent);
    editor->setEditable(true);
    editor->setModel(m_model);
    editor->setModelColumn(m_displayColumn); // column to display

    QCompleter* completer = new QCompleter(m_model, editor);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionColumn(m_displayColumn);
    completer->setModel(m_model);
    editor->setCompleter(completer);

    return editor;
}

void SqlComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ComboBox *comboBox = static_cast<ComboBox*>(editor);
    QVariant lookupvalue = index.model()->data(index);
    QString value = dynamic_cast<SqlQueryModel*>(m_model->sourceModel())->findValue(lookupvalue, m_dataColumn, m_displayColumn).toString();

    comboBox->setCurrentText(value);
}

void SqlComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ComboBox *comboBox = static_cast<ComboBox*>(editor);
    int i;
    QVariant key_val;

    if (!comboBox->currentText().isEmpty() )
    {
        i = comboBox->findText(comboBox->currentText(), Qt::MatchFixedString);
        if (i >= 0)
        {
            comboBox->setCurrentIndex(i);
            key_val = m_model->data(m_model->index(i, m_dataColumn));
        }
    }

    model->setData(index, key_val, Qt::EditRole);
}

void SqlComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

void SqlComboBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem myOption = option;
    QVariant lookupvalue = index.model()->data(index);

    myOption.text = dynamic_cast<SqlQueryModel*>(m_model->sourceModel())->findValue(lookupvalue, m_dataColumn, m_displayColumn).toString();

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, painter);
}
