// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "itemdetailsdelegate.h"
#include "pnsqlquerymodel.h"
#include "pndateeditex.h"
#include "pndatabaseobjects.h"
#include "mainwindow.h"
#include "pnbasepage.h"

#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QPlainTextEdit>

ItemDetailsDelegate::ItemDetailsDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void ItemDetailsDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    QVariant value = t_index.model()->data(t_index);

    switch (t_index.column())
    {
    case 1:
    case 3:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(t_editor);
            lineedit->setText(value.toString());

            QWidget* window = static_cast<QWidget*>(t_editor)->topLevelWidget();
            if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
        }
        break;
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
    case 13: // meeting
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
    case 14: // project number
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
    case 6:
        {
            QPlainTextEdit* lineedit = static_cast<QPlainTextEdit*>(t_editor);

            // don't resent buffers if text hasn't changed
            if (value.toString().compare(lineedit->toPlainText()) != 0)
                lineedit->setPlainText(value.toString());
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
    case 1:
    case 3:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(t_editor);
            key_val = lineedit->text();

            QWidget* window = static_cast<QWidget*>(t_editor)->topLevelWidget();
            if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
        }
        break;
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
    case 13: // meeting
        {
            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);

            int i = comboBox->currentIndex();
            key_val = comboBox->model()->data(comboBox->model()->index(i, 0));

            //  if the value was typed in use the typed in value
            if (i == 0 && !comboBox->lineEdit()->text().isEmpty())
            {
                i = comboBox->findText(comboBox->lineEdit()->text(), Qt::MatchFixedString);
                key_val = comboBox->model()->data(comboBox->model()->index(i, 0));
            }
        }
        break;
    case 14: // project number
        {
            QModelIndex i_qi = t_model->index(t_index.row(), 0);
            QModelIndex p_qi = t_model->index(t_index.row(), 14);

            QVariant item_id = t_model->data(i_qi);
            QVariant project_id = t_model->data(p_qi);

            QComboBox *comboBox = static_cast<QComboBox*>(t_editor);

            int i = comboBox->currentIndex();
            key_val = comboBox->model()->data(comboBox->model()->index(i, 0));

            //  if the value was typed in use the typed in value
            if (i == 0 && !comboBox->lineEdit()->text().isEmpty())
            {
                i = comboBox->findText(comboBox->lineEdit()->text(), Qt::MatchFixedString);
                key_val = comboBox->model()->data(comboBox->model()->index(i, 0));
            }

            // if project number changes verify and clear the meeting
            if ( key_val != project_id )
            {
                if ( !verifyProjectNumber(key_val, item_id))
                {
                    setEditorData(t_editor, t_index); // set the value back
                    return;
                }

                // reset the filters for all of the drop downs
                global_DBObjects.actionitemsdetailsmeetingsmodel()->setFilter(1, key_val.toString());
                global_DBObjects.actionitemsdetailsmeetingsmodel()->refresh();

                QModelIndex n_qi = t_model->index(t_index.row(), 13);
                QVariant nothing;

                t_model->setData(n_qi, nothing); // set the meeting to blank since it won't be in the new project
            }
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

            //  if the value was typed in use the typed in value
            if (i == 0 && !comboBox->lineEdit()->text().isEmpty())
            {
                i = comboBox->findText(comboBox->lineEdit()->text(), Qt::MatchFixedString);
                key_val = comboBox->model()->data(comboBox->model()->index(i, 0));
            }
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
    case 6:
        {
            QPlainTextEdit* lineedit = static_cast<QPlainTextEdit*>(t_editor);
            key_val = lineedit->toPlainText();
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


bool ItemDetailsDelegate::verifyProjectNumber(QVariant& t_project_id, QVariant& t_item_id) const
{
    QString msg;
    int issuestoresolve = 0;

    // check to see if identified by is in the project team
    {
        QSqlQuery select;
        select.prepare("select identified_by, (select name from people p where p.people_id=identified_by) people_id_name from item_tracker where item_id = ? and identified_by not in (select people_id from project_people where project_id = ?)");
        select.bindValue(0, t_item_id);
        select.bindValue(1, t_project_id);

        if (select.exec())
        {
            while (select.next())
            {
                issuestoresolve++;
                msg += "Identified By, " + select.record().value(1).toString() + " is not found on the selected projects team.  Remove or change this person before changing to this project number.\n";
            }
        }
    }

    // check to see if assigned to is in the project team
    {
        QSqlQuery select;
        select.prepare("select assigned_to, (select name from people p where p.people_id=assigned_to) people_id_name from item_tracker where item_id = ? and assigned_to not in (select people_id from project_people where project_id = ?)");
        select.bindValue(0, t_item_id);
        select.bindValue(1, t_project_id);

        if (select.exec())
        {
            while (select.next())
            {
                issuestoresolve++;
                msg += "Assigned To, " + select.record().value(1).toString() + " is not found on the selected projects team.  Remove or change this person before changing to this project number.\n";
            }
        }
    }

    // check to see if updated by value is in team
    {
        QSqlQuery select;
        select.prepare("select updated_by, (select name from people p where p.people_id=updated_by) people_id_name from item_tracker_updates where item_id = ? and updated_by not in (select people_id from project_people where project_id = ? )");
        select.bindValue(0, t_item_id);
        select.bindValue(1, t_project_id);

        if (select.exec())
        {
            while (select.next())
            {
                issuestoresolve++;
                msg += "Comment Updated By, " + select.record().value(1).toString() + " is not found on the selected projects team.  Remove or change this person before changing to this project number.\n";
            }
        }
    }

    if (issuestoresolve)
    {
        QMessageBox::critical(nullptr, QObject::tr("Cannot Reassign Project"), msg);
        return false;
    }

    // note the meeting value will be cleared
    // have user confirm the change
    {
        QSqlQuery select;
        select.prepare("select note_id from item_tracker where item_id = ?");
        select.bindValue(0, t_item_id);

        if (select.exec())
        {
            if (select.next() && !select.record().value(0).isNull())
            {
                if ( QMessageBox::question(nullptr, QObject::tr("Associatd Meeting"),
                   "The associated meeting will be removed.  Still reassign the project?\n", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No )
                    return false;
            }
        }
    }

    return true;
}
