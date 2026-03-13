// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "checkboxdelegate.h"

#include <QCheckBox>
#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QLayout>
#include <QHBoxLayout>
#include <QPainter>

#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

CheckBoxDelegate::CheckBoxDelegate(QObject *parent)
:QStyledItemDelegate(parent)
{

}

QWidget* CheckBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if(index.isValid())
    {
        QCheckBox *editor = new QCheckBox(parent);

        editor->installEventFilter(const_cast<CheckBoxDelegate*>(this));
        return editor;
    }
    else
    {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
}

void CheckBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QCheckBox *checkBox = static_cast<QCheckBox*>(editor);
    QVariant checkvalue = index.model()->data(index);

    if ( checkvalue == "1")
        checkBox->setCheckState(Qt::Checked);
    else
        checkBox->setCheckState(Qt::Unchecked);
}

void CheckBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QCheckBox *checkBox = static_cast<QCheckBox*>(editor);

    if ( checkBox->checkState() == Qt::Checked )
        model->setData(index, "1", Qt::EditRole);
    else
        model->setData(index, "0", Qt::EditRole);
}

void CheckBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    QStyleOptionButton checkbox_indicator;

    // Set our button state to enabled
    checkbox_indicator.state |= QStyle::State_Enabled;

    QSize checkBoxSize = QApplication::style()->sizeFromContents(
        QStyle::CT_CheckBox, &checkbox_indicator, QSize());

    // make it slightly wider so it draws better
    checkBoxSize.setWidth(checkBoxSize.width() * 1.3);

    // Center it horizontally and vertically
    QRect checkRect = QRect(
        option.rect.left() + (option.rect.width()  - checkBoxSize.width())  / 2,
        option.rect.top()  + (option.rect.height() - checkBoxSize.height()) / 2,
        checkBoxSize.width(),
        checkBoxSize.height()
        );

    checkbox_indicator.rect = checkRect;

    editor->setGeometry(checkbox_indicator.rect);
}

void CheckBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem myOption = option;
    myOption.text = " ";

    QVariant bgcolor = index.model()->data(index, Qt::BackgroundRole);
    QVariant fgcolor = index.model()->data(index, Qt::ForegroundRole);

    painter->save();
    if (fgcolor.isValid())
    {
        myOption.palette.setColor(QPalette::Text, fgcolor.value<QColor>());
        painter->setPen(fgcolor.value<QColor>());
    }

    if (bgcolor.isValid())
    {
        myOption.palette.setColor(QPalette::Base, bgcolor.value<QColor>());
        myOption.palette.setColor(QPalette::AlternateBase, bgcolor.value<QColor>());
    }

    // you have to draw this to get the highlighting to work correctly
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, painter);

    // Draw our checkbox indicator
    bool value = index.data(Qt::EditRole).toBool();
    QStyleOptionButton checkbox_indicator;

    checkbox_indicator.state |= option.state;
    checkbox_indicator.palette = option.palette;

    // Set our button state to enabled
    checkbox_indicator.state |= QStyle::State_Enabled;
    checkbox_indicator.state |= (value) ? QStyle::State_On : QStyle::State_Off;

    // Get our deimensions
    QSize checkBoxSize = QApplication::style()->sizeFromContents(
        QStyle::CT_CheckBox, &checkbox_indicator, QSize());

    // make it slightly wider so it draws better
    checkBoxSize.setWidth(checkBoxSize.width() * 1.3);

    // Center it horizontally and vertically
    QRect checkRect = QRect(
        option.rect.left() + (option.rect.width()  - checkBoxSize.width())  / 2,
        option.rect.top()  + (option.rect.height() - checkBoxSize.height()) / 2,
        checkBoxSize.width(),
        checkBoxSize.height()
        );

    checkbox_indicator.rect = checkRect;

    QApplication::style()->drawControl( QStyle::CE_CheckBox, &checkbox_indicator, painter );

    painter->restore();
}

Qt::ItemFlags CheckBoxDelegate::flags ( const QModelIndex & index ) const
{
    Q_UNUSED(index);

    return Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}
