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

PNDateEditDelegate::PNDateEditDelegate(QObject *parent)
:QItemDelegate(parent)
{

}


QWidget *PNDateEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QDateEdit* editor = new QDateEdit(parent);

    editor->setDisplayFormat("MM/dd/yyyy");
    editor->setProperty("EditMask","MM/dd/yyyy");
    editor->setCalendarPopup(true);
    return editor;
}

void PNDateEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QDateEdit *dateedit = static_cast<QDateEdit*>(editor);
    QString value = index.model()->data(index, Qt::EditRole).toString();
    QDateTime datevalue = PNSqlQueryModel::ParseDateTime(value);

    dateedit->setDate(datevalue.date());
}

void PNDateEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QDateEdit *dateedit = static_cast<QDateEdit*>(editor);
    model->setData(index, dateedit->date().toString("MM/dd/yyyy"), Qt::EditRole);
}

void PNDateEditDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

void PNDateEditDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem myOption = option;

    QString datevalue = index.model()->data(index, Qt::EditRole).toString();

    myOption.text = datevalue;

    // make light gray background when not editable
    //if (!((PNSqlQueryModel*)index.model())->isEditable(index.column()))
    //    myOption.backgroundBrush = QBrush(QColor("lightgray"));

    myOption.palette.setColor(QPalette::Text,index.model()->data(index, Qt::ForegroundRole).value<QColor>());
    QVariant color = index.model()->data(index, Qt::BackgroundColorRole);
    if (color.isValid())
        myOption.backgroundBrush = QBrush(color.value<QColor>());

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, painter);
}
