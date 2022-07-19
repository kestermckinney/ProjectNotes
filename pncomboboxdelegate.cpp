#include "pncomboboxdelegate.h"

#include <QComboBox>
#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QDebug>

PNComboBoxDelegate::PNComboBoxDelegate(QObject *t_parent, PNSqlQueryModel *t_model, int t_displaycolumn)
:QItemDelegate(t_parent)
{
    m_model = t_model;
    m_display_column = t_displaycolumn;
}

QWidget* PNComboBoxDelegate::createEditor(QWidget *t_parent, const QStyleOptionViewItem& t_option, const QModelIndex& t_index ) const
{
    QComboBox* editor = new QComboBox(t_parent);
    editor->setEditable(true);
    editor->setModel(m_model);
    editor->setModelColumn(m_display_column); // column to display

    QCompleter* completer = new QCompleter(m_model);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    //completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionColumn(m_display_column);
    editor->setCompleter(completer);

    return editor;
}

void PNComboBoxDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
    QVariant lookupvalue = t_index.model()->data(t_index);
    QString value = m_model->findValue(lookupvalue, 0, 1).toString();

    qDebug() << "Editor Data: " << value;
    //comboBox->setCurrentIndex(value);
    comboBox->setCurrentText(value);
    //QItemDelegate::setEditorData(t_editor, t_index);
}

void PNComboBoxDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
    QVariant key_val = m_model->data(m_model->index(comboBox->currentIndex(), 0));
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

    myOption.text = m_model->findValue(lookupvalue, 0, 1).toString();


    //qDebug() << "option.text = " << myOption.text;
    // make light gray background when not edit_table
    //if (!((PNSqlQueryModel*)t_index.model())->isEdit_table(t_index.column()))
    //    myOption.backgroundBrush = QBrush(QColor("lightgray"));

    myOption.palette.setColor(QPalette::Text, t_index.model()->data(t_index, Qt::ForegroundRole).value<QColor>());
    QVariant color = t_index.model()->data(t_index, Qt::BackgroundColorRole);
    if (color.isValid())
        myOption.backgroundBrush = QBrush(color.value<QColor>());

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, t_painter);
}
