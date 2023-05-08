#include "pndateeditdelegate.h"
#include "pnsqlquerymodel.h"
#include "pndateeditex.h"

#include <QCompleter>
#include <QCompleter>
#include <QWidget>
#include <QModelIndex>
#include <QApplication>
#include <QString>
#include <QPainter>

PNDateEditDelegate::PNDateEditDelegate(QObject *t_parent)
:QStyledItemDelegate(t_parent)
{

}


QWidget *PNDateEditDelegate::createEditor(QWidget *t_parent, const QStyleOptionViewItem &/* t_option */, const QModelIndex &/* t_index */) const
{
    PNDateEditEx* editor = new PNDateEditEx(t_parent);

    editor->setDisplayFormat("MM/dd/yyyy");
    editor->setProperty("EditMask","MM/dd/yyyy");
    editor->setCalendarPopup(true);
    editor->setNullable(true);

    return editor;
}

void PNDateEditDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    PNDateEditEx *dateedit = static_cast<PNDateEditEx*>(t_editor);
    QString t_value = t_index.model()->data(t_index, Qt::EditRole).toString();
    QDateTime datevalue = PNSqlQueryModel::parseDateTime(t_value);

    dateedit->setDate(datevalue.date());
}

void PNDateEditDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    PNDateEditEx *dateedit = static_cast<PNDateEditEx*>(t_editor);
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

    QVariant bgcolor = t_index.model()->data(t_index, Qt::BackgroundRole);
    QVariant fgcolor = t_index.model()->data(t_index, Qt::ForegroundRole);

    if (fgcolor.isValid())
        myOption.palette.setColor(QPalette::Text, fgcolor.value<QColor>());

    if (bgcolor.isValid())
        myOption.backgroundBrush = QBrush(bgcolor.value<QColor>());

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, t_painter);
}


// TODO: Can't delete a date on project tab after it has been entered
// TODO: Adding a team member doesn't auto-fill out the role
// TODO: after adding a person, they don't show in the drop down for adding them to be a team member
// TODO: can't seem to double click on the first row of the projects list to open the projects
// TODO: adding a team member row with blank items throws an insert error
// TODO: once a person is added to the team the primary contact drop down doesn't get updated
// TODO: once a person is added to the team the drop down in items track does not get updated


