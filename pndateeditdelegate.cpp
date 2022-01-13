#include "pndateeditdelegate.h"
#include "pnsqlquerymodel.h"

#include <QDateEdit>
#include <QCompleter>
#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>

PNDateEditDelegate::PNDateEditDelegate(QObject *t_parent)
:QItemDelegate(t_parent)
{

}


QWidget *PNDateEditDelegate::createEditor(QWidget *t_parent, const QStyleOptionViewItem &/* t_option */, const QModelIndex &/* t_index */) const
{
    QDateEdit* editor = new QDateEdit(t_parent);

    editor->setDisplayFormat("MM/dd/yyyy");
    editor->setProperty("EditMask","MM/dd/yyyy");
    editor->setCalendarPopup(true);
    return editor;
}

void PNDateEditDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    QDateEdit *dateedit = static_cast<QDateEdit*>(t_editor);
    QString t_value = t_index.model()->data(t_index, Qt::EditRole).toString();
    QDateTime datevalue = PNSqlQueryModel::ParseDateTime(t_value);

    dateedit->setDate(datevalue.date());
}

void PNDateEditDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    QDateEdit *dateedit = static_cast<QDateEdit*>(t_editor);
    t_model->setData(t_index, dateedit->date().toString("MM/dd/yyyy"), Qt::EditRole);
}

void PNDateEditDelegate::updateEditorGeometry(QWidget *t_editor, const QStyleOptionViewItem &t_option, const QModelIndex &/* t_index */) const
{
    t_editor->setGeometry(t_option.rect);
}

void PNDateEditDelegate::paint(QPainter *t_painter, const QStyleOptionViewItem &t_option, const QModelIndex &t_index) const
{
    QStyleOptionViewItem myOption = t_option;

    QString datevalue = t_index.model()->data(t_index, Qt::EditRole).toString();

    myOption.text = datevalue;

    // make light gray background when not edit_table
    //if (!((PNSqlQueryModel*)t_index.model())->isEdit_table(t_index.column()))
    //    myOption.backgroundBrush = QBrush(QColor("lightgray"));

    myOption.palette.setColor(QPalette::Text,t_index.model()->data(t_index, Qt::ForegroundRole).value<QColor>());
    QVariant color = t_index.model()->data(t_index, Qt::BackgroundColorRole);
    if (color.isValid())
        myOption.backgroundBrush = QBrush(color.value<QColor>());

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, t_painter);
}
