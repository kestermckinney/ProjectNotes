#include "comboboxdelegate.h"

#include <QComboBox>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>

ComboBoxDelegate::ComboBoxDelegate(QObject *parent, QStringListModel *model)
:QItemDelegate(parent)
{
    m_Model = model;
}


QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QComboBox* editor = new QComboBox(parent);
    editor->setEditable(false);
    editor->setModel(m_Model);
    editor->setModelColumn(0); // column to display

    return editor;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    QVariant value = index.model()->data(index);

    comboBox->setCurrentText(value.toString());
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    model->setData(index, m_Model->data(m_Model->index(comboBox->currentIndex(), 0)), Qt::EditRole);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

void ComboBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem myOption = option;

    QVariant value = index.model()->data(index);

    myOption.text = value.toString();

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, painter);
}
