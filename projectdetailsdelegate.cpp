#include "projectdetailsdelegate.h"
#include "pnsqlquerymodel.h"
#include "pndateeditex.h"

#include <QLineEdit>
#include <QComboBox>
#include <QDebug>

ProjectDetailsDelegate::ProjectDetailsDelegate(QObject *parent) : QStyledItemDelegate(parent)
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
            PNDateEditEx* dateEdit = static_cast<PNDateEditEx*>(t_editor);

            if (value.isNull())
                dateEdit->setDateTime(QDateTime());
            else
            {
                QDateTime date_value = PNSqlQueryModel::parseDateTime(value.toString());
                dateEdit->setDate(date_value.date());
            }
            break;
        }
    case 5:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
            PNSqlQueryModel *model = static_cast<PNSqlQueryModel*>(comboBox->model());


            qDebug() << " Primary Contact Mapper Called";

            if (model)
            {
                QString list_value = model->findValue(value, 3, 1).toString();
                comboBox->setCurrentText(list_value);

                qDebug() << " .. Set text value to " << list_value;
            }
        }
        break;
    case 11:
    case 12:
    case 14:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
            comboBox->setCurrentText(value.toString());
        }
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
            PNDateEditEx* dateEdit = static_cast<PNDateEditEx*>(t_editor);
            if (!dateEdit->isNull())
                key_val = dateEdit->date().toString("MM/dd/yyyy");
            else
                key_val.clear();
        }
        break;
    case 5:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);

            int i = comboBox->currentIndex();
            key_val = comboBox->model()->data(comboBox->model()->index(i, 3));
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
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);

            int i = comboBox->currentIndex();
            key_val = comboBox->model()->data(comboBox->model()->index(i, 3));
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
