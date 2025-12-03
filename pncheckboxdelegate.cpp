// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pncheckboxdelegate.h"

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

PNCheckBoxDelegate::PNCheckBoxDelegate(QObject *t_parent)
:QStyledItemDelegate(t_parent)
{

}

QWidget* PNCheckBoxDelegate::createEditor(QWidget *t_parent, const QStyleOptionViewItem& t_option, const QModelIndex& t_index ) const
{
    if(t_index.isValid())
    {
        QCheckBox *editor = new QCheckBox(t_parent);

        editor->installEventFilter(const_cast<PNCheckBoxDelegate*>(this));
        return editor;
    }
    else
    {
        return QStyledItemDelegate::createEditor(t_parent, t_option, t_index);
    }
}

void PNCheckBoxDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    QCheckBox *checkBox = static_cast<QCheckBox*>(t_editor);
    QVariant checkvalue = t_index.model()->data(t_index);

    if ( checkvalue == "1")
        checkBox->setCheckState(Qt::Checked);
    else
        checkBox->setCheckState(Qt::Unchecked);
}

void PNCheckBoxDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    QCheckBox *checkBox = static_cast<QCheckBox*>(t_editor);

    if ( checkBox->checkState() == Qt::Checked )
        t_model->setData(t_index, "1", Qt::EditRole);
    else
        t_model->setData(t_index, "0", Qt::EditRole);
}

void PNCheckBoxDelegate::updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &/* t_index */) const
{
    QStyleOptionButton checkbox_indicator;

    // Set our button state to enabled
    checkbox_indicator.state |= QStyle::State_Enabled;

    // Get our deimensions
    checkbox_indicator.rect = QApplication::style()->subElementRect( QStyle::SE_CheckBoxIndicator, &checkbox_indicator, NULL );

    const int x = t_option.rect.center().x() - checkbox_indicator.rect.width() / 2;
    const int y = t_option.rect.center().y() - checkbox_indicator.rect.height() / 2;

    checkbox_indicator.rect.moveTo( x, y );

    t_editor->setGeometry(checkbox_indicator.rect);
}

void PNCheckBoxDelegate::paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const
{
    QStyleOptionViewItem myOption = t_option;
    myOption.text = " ";

    QVariant bgcolor = t_index.model()->data(t_index, Qt::BackgroundRole);
    QVariant fgcolor = t_index.model()->data(t_index, Qt::ForegroundRole);

    t_painter->save();
    if (fgcolor.isValid())
    {
        myOption.palette.setColor(QPalette::Text, fgcolor.value<QColor>());
        t_painter->setPen(fgcolor.value<QColor>());
    }

    if (bgcolor.isValid())
    {
        myOption.palette.setColor(QPalette::Base, bgcolor.value<QColor>());
        myOption.palette.setColor(QPalette::AlternateBase, bgcolor.value<QColor>());
    }

    // you have to draw this to get the highlighting to work correctly
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, t_painter);

    // Draw our checkbox indicator
    bool value = t_index.data(Qt::EditRole).toBool();
    QStyleOptionButton checkbox_indicator;

    checkbox_indicator.state |= t_option.state;
    checkbox_indicator.palette = t_option.palette;

    // Set our button state to enabled
    checkbox_indicator.state |= QStyle::State_Enabled;
    checkbox_indicator.state |= (value) ? QStyle::State_On : QStyle::State_Off;

    // Get our deimensions
    checkbox_indicator.rect = QApplication::style()->subElementRect( QStyle::SE_CheckBoxIndicator, &checkbox_indicator, NULL );

    // Position our indicator
    const int x = t_option.rect.center().x() - checkbox_indicator.rect.width() / 2;
    const int y = t_option.rect.center().y() - checkbox_indicator.rect.height() / 2;

    checkbox_indicator.rect.moveTo( x, y );

    QApplication::style()->drawControl( QStyle::CE_CheckBox, &checkbox_indicator, t_painter );
}

Qt::ItemFlags PNCheckBoxDelegate::flags ( const QModelIndex & t_index ) const
{
    Q_UNUSED(t_index);

    return Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}
