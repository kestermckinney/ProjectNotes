#include "projectdetailsdelegate.h"
#include "pnsqlquerymodel.h"

#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>

ProjectDetailsDelegate::ProjectDetailsDelegate(QObject *parent) : QItemDelegate(parent)
{

}

void ProjectDetailsDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    QVariant value = t_index.model()->data(t_index);

    switch (t_index.column())
    {
    case 3:
    case 4:
        {
            QDateEdit* dateEdit = static_cast<QDateEdit*>(t_editor);
            QDateTime date_value = PNSqlQueryModel::parseDateTime(value.toString());
            dateEdit->setDate(date_value.date());
            break;
        }
    case 11:
    case 12:
    case 14:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
            comboBox->setCurrentText(value.toString());
        }
        break;
    case 5:
        // TODO: get model assigned to protential primary contact
        break;
    case 13:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
            PNSqlQueryModel *model = static_cast<PNSqlQueryModel*>(comboBox->model());

            if (model)
            {
                QString list_value = model->findValue(value, 0, 1).toString();
                comboBox->setCurrentText(list_value);
            }
        }
        break;
    default:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(t_editor);
            lineedit->setText(value.toString());
        }
    }
}

void ProjectDetailsDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    QVariant key_val;

    switch (t_index.column())
    {
    case 3:
    case 4:
        {
            QDateEdit* dateEdit = static_cast<QDateEdit*>(t_editor);
            key_val = dateEdit->date().toString("MM/dd/yyyy");
        }
        break;
    case 11:
    case 12:
    case 14:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
            key_val = comboBox->itemText(comboBox->currentIndex());
        }
        break;
    case 13:
    case 5:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);

            int i = comboBox->currentIndex();
            key_val = comboBox->model()->data(comboBox->model()->index(i, 0));
        }
        break;
    default:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(t_editor);
            key_val = lineedit->text();
        }
    }

    t_model->setData(t_index, key_val, Qt::EditRole);
}
