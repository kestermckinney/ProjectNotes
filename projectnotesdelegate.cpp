#include "projectnotesdelegate.h"
#include "pnsqlquerymodel.h"
#include "pndateeditex.h"

#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>

ProjectNotesDelegate::ProjectNotesDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void ProjectNotesDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    QVariant value = t_index.model()->data(t_index);

    switch (t_index.column())
    {
    case 2: // note title
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(t_editor);
            lineedit->setText(value.toString());
        }
        break;
    case 3:
        {
            PNDateEditEx* dateEdit = static_cast<PNDateEditEx*>(t_editor);

            if (value.isNull())
                dateEdit->setDateTime(QDateTime());
            else
            {
                QDateTime date_value = PNSqlQueryModel::parseDateTime(value.toString());
                dateEdit->setDate(date_value.date());
            }
        }
        break;
    case 4: // note
        {
            QTextEdit* textedit = static_cast<QTextEdit*>(t_editor);
            if (value.toString().contains("<html>", Qt::CaseInsensitive))
                textedit->setHtml(value.toString());
            else
                textedit->setPlainText(value.toString());
        }
        break;
    }
}

void ProjectNotesDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    QVariant key_val;

    switch (t_index.column())
    {
    case 2: // note title
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(t_editor);
            key_val = lineedit->text();
        }
        break;
    case 3: // note date
        {
            PNDateEditEx* dateEdit = static_cast<PNDateEditEx*>(t_editor);
            if (!dateEdit->isNull())
                key_val = dateEdit->date().toString("MM/dd/yyyy");
            else
                key_val.clear();
        }
        break;
    case 4: // note text
        {
            QTextEdit* textedit = static_cast<QTextEdit*>(t_editor);
            key_val = textedit->toHtml();
        }
        break;
    }

    t_model->setData(t_index, key_val, Qt::EditRole);
}
