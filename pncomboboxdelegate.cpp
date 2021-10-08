#include "pncomboboxdelegate.h"

#include <QComboBox>
#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>

PNComboBoxDelegate::PNComboBoxDelegate(QObject *parent, PNSqlQueryModel *model, int DisplayColumn)
:QItemDelegate(parent)
{
    m_Model = model;
    m_DisplayColumn = DisplayColumn;
}


QWidget *PNComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QComboBox* editor = new QComboBox(parent);
    editor->setEditable(true);
    editor->setModel(m_Model);
    editor->setModelColumn(m_DisplayColumn); // column to display

    QCompleter* completer = new QCompleter(m_Model);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionColumn(1);
    editor->setCompleter(completer);

    return editor;
}

void PNComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    QVariant lookupvalue = index.model()->data(index);
    QString value = m_Model->FindValue(lookupvalue, 0, 1).toString();
    comboBox->setCurrentText(value);
}

void PNComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    model->setData(index, m_Model->data(m_Model->index(comboBox->currentIndex(), 0)), Qt::EditRole);
}

void PNComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

void PNComboBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem myOption = option;

    QVariant lookupvalue = index.model()->data(index);

    myOption.text = m_Model->FindValue(lookupvalue, 0, 1).toString();

    // make light gray background when not editable
    //if (!((PNSqlQueryModel*)index.model())->isEditable(index.column()))
    //    myOption.backgroundBrush = QBrush(QColor("lightgray"));

    myOption.palette.setColor(QPalette::Text,index.model()->data(index, Qt::ForegroundRole).value<QColor>());
    QVariant color = index.model()->data(index, Qt::BackgroundColorRole);
    if (color.isValid())
        myOption.backgroundBrush = QBrush(color.value<QColor>());

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, painter);
}
