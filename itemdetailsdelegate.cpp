#include "itemdetailsdelegate.h"
#include "pnsqlquerymodel.h"
#include "pndateeditex.h"

#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>

ItemDetailsDelegate::ItemDetailsDelegate(QObject *parent) : QItemDelegate(parent)
{

}

void ItemDetailsDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    QVariant value = t_index.model()->data(t_index);

    switch (t_index.column())
    {
    case 5:
    case 10:
    case 11:
    case 12:
        {
            // handle the date edit text boxex
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
    case 13:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
            PNSqlQueryModel *model = static_cast<PNSqlQueryModel*>(comboBox->model());

            if (model)
            {
                QString list_value = model->findValue(value, 0, 2).toString();
                comboBox->setCurrentText(list_value);
            }
        }
        break;
    case 14:
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
    case 2:
    case 8:
    case 9:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
            comboBox->setCurrentText(value.toString());
        }
        break;
    case 4:
    case 7:
        {
            // people drop downs
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
            PNSqlQueryModel *model = static_cast<PNSqlQueryModel*>(comboBox->model());

            if (model)
            {
                QString list_value = model->findValue(value, 3, 1).toString();
                comboBox->setCurrentText(list_value);
            }
        }
        break;
    case 15:
        {
            QCheckBox *checkbox = static_cast<QCheckBox*>(t_editor);

            if (value == "1")
                checkbox->setCheckState(Qt::Checked);
            else
                checkbox->setCheckState(Qt::Unchecked);
        }
        break;
    default:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(t_editor);
            lineedit->setText(value.toString());
        }
    }
}

void ItemDetailsDelegate::setModelData(QWidget *t_editor, QAbstractItemModel *t_model, const QModelIndex &t_index) const
{
    QVariant key_val;

    switch (t_index.column())
    {
    case 5:
    case 10:
    case 11:
    case 12:
        {
            // handle date fields
            PNDateEditEx* dateEdit = static_cast<PNDateEditEx*>(t_editor);
            if (!dateEdit->isNull())
                key_val = dateEdit->date().toString("MM/dd/yyyy");
            else
                key_val.clear();
        }
        break;
    case 13:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);

            int i = comboBox->currentIndex();
            key_val = comboBox->model()->data(comboBox->model()->index(i, 0));
        }
        break;
    case 14:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);

            int i = comboBox->currentIndex();
            key_val = comboBox->model()->data(comboBox->model()->index(i, 0));
        }
        break;
    case 2:
    case 8:
    case 9:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);
            key_val = comboBox->itemText(comboBox->currentIndex());
        }
        break;
    case 4:
    case 7:
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);

            int i = comboBox->currentIndex();
            key_val = comboBox->model()->data(comboBox->model()->index(i, 3));
        }
        break;
    case 15:
        {
            QCheckBox *checkbox = static_cast<QCheckBox*>(t_editor);

            if ( checkbox->isChecked() )
                key_val = "1";
            else
                key_val = "0";
        }
        break;
    default:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(t_editor);
            key_val = lineedit->text();
        }
        break;
    }

    t_model->setData(t_index, key_val, Qt::EditRole);
}
