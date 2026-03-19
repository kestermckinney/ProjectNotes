// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "itemdetailsdelegate.h"
#include "databaseobjects.h"
#include "dateeditex.h"
#include "databaseobjects.h"
#include "mainwindow.h"
#include "basepage.h"
#include "combobox.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QScrollBar>

ItemDetailsDelegate::ItemDetailsDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void ItemDetailsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QVariant value = index.model()->data(index);

    switch (index.column())
    {
    case 1:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
            lineedit->setText(value.toString());

            QWidget* window = static_cast<QWidget*>(editor)->topLevelWidget();
            if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
        }
        break;
    case 5:
    case 10:
    case 11:
    case 12:
        {
            // handle the date edit text box
            DateEditEx* dateEdit = static_cast<DateEditEx*>(editor);

            if (value.isNull())
                dateEdit->setDateTime(QDateTime());
            else
            {
                QDateTime date_value = SqlQueryModel::parseDateTime(value.toString());
                dateEdit->setDate(date_value.date());
            }
            break;
        }
    case 13: // meeting
        {
            ComboBox *comboBox = static_cast<ComboBox*>(editor);
            SqlQueryModel *model = static_cast<SqlQueryModel*>(comboBox->model());

            if (model)
            {
                QVariant list_value = model->findValue(value, 0, 2);

                if (!list_value.isNull())
                {
                    int i = comboBox->findText(list_value.toString(), Qt::MatchFixedString);
                    if (i >= 0)
                    {
                        comboBox->setCurrentIndex(i);
                        // test do you need to do this after you set the index comboBox->setCurrentText(list_value.toString());
                    }
                    else
                        comboBox->setCurrentText(QString());
                }
                else
                     comboBox->setCurrentText(QString());
            }
        }
        break;
    case 14: // project number
        {
            ComboBox *comboBox = static_cast<ComboBox*>(editor);
            SqlQueryModel *model = static_cast<SqlQueryModel*>(comboBox->model());

            if (model)
            {
                QVariant list_value = model->findValue(value, 0, 1);

                if (!list_value.isNull())
                {
                    int i = comboBox->findText(list_value.toString(), Qt::MatchFixedString);
                    if (i >= 0)
                    {
                        comboBox->setCurrentIndex(i);
                        // test set current text after index comboBox->setCurrentText(list_value.toString());
                    }
                    else
                        comboBox->setCurrentText(QString());
                }
                else
                     comboBox->setCurrentText(QString());
            }
        }
        break;
    case 2:
    case 8:
    case 9:
        {
            ComboBox *comboBox = static_cast<ComboBox*>(editor);
            comboBox->setCurrentText(value.toString());
        }
        break;
    case 4:
    case 7:
        {
            ComboBox *comboBox = static_cast<ComboBox*>(editor);
            SqlQueryModel *model = static_cast<SqlQueryModel*>(comboBox->model());

            if (model)
            {
                QVariant list_value = model->findValue(value, 3, 1);

                if (!list_value.isNull())
                {
                    int i = comboBox->findText(list_value.toString(), Qt::MatchFixedString);
                    if (i >= 0)
                    {
                        comboBox->setCurrentIndex(i);
                        // test do you need to set current text after setting the index comboBox->setCurrentText(list_value.toString());
                    }
                    else
                        comboBox->setCurrentText(QString());
                }
                else
                     comboBox->setCurrentText(QString());
            }
        }
        break;
    case 15:
        {
            QCheckBox *checkbox = static_cast<QCheckBox*>(editor);

            if (value == "1")
                checkbox->setCheckState(Qt::Checked);
            else
                checkbox->setCheckState(Qt::Unchecked);
        }
        break;
    case 3:
    case 6:
        {
            QPlainTextEdit* lineedit = static_cast<QPlainTextEdit*>(editor);

            // don't resent buffers if text hasn't changed
            if (value.toString().compare(lineedit->toPlainText()) != 0)
                lineedit->setPlainText(value.toString());
        }
        break;
    default:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
            lineedit->setText(value.toString());
        }
    }
}

void ItemDetailsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QVariant key_val;

    switch (index.column())
    {
    case 1:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
            key_val = lineedit->text();

            QWidget* window = static_cast<QWidget*>(editor)->topLevelWidget();
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
            DateEditEx* dateEdit = static_cast<DateEditEx*>(editor);
            if (!dateEdit->isNull())
                key_val = dateEdit->date().toString("MM/dd/yyyy");
            else
                key_val.clear();
        }
        break;
    case 13: // meeting
        {
            ComboBox *comboBox = static_cast<ComboBox*>(editor);
            int i;

            if (!comboBox->currentText().isEmpty() )
            {
                i = comboBox->findText(comboBox->currentText(), Qt::MatchFixedString);
                comboBox->setCurrentIndex(i);

                if (i >= 0)
                {
                    key_val = comboBox->model()->data(comboBox->model()->index(i, 0));
                }
            }
        }
        break;
    case 14: // project number
        {
            QModelIndex i_qi = model->index(index.row(), 0);
            QModelIndex p_qi = model->index(index.row(), 14);

            QVariant item_id = model->data(i_qi);
            QVariant project_id = model->data(p_qi);

            ComboBox *comboBox = static_cast<ComboBox*>(editor);
            int i;

            if (!comboBox->currentText().isEmpty() )
            {
                i = comboBox->findText(comboBox->currentText(), Qt::MatchFixedString);
                comboBox->setCurrentIndex(i);

                if (i >= 0)
                {
                    key_val = comboBox->model()->data(comboBox->model()->index(i, 0));
                }
            }

            // if project number changes verify and clear the meeting
            if ( key_val != project_id )
            {
                if ( !verifyProjectNumber(key_val, item_id))
                {
                    setEditorData(editor, index); // set the value back
                    return;
                }

                DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(dynamic_cast<SortFilterProxyModel*>(model)->sourceModel())->getDBOs();

                // reset the filters for all of the drop downs
                dbo->actionitemsdetailsmeetingsmodel()->setFilter(1, key_val.toString());
                dbo->actionitemsdetailsmeetingsmodel()->refresh();

                QModelIndex n_qi = model->index(index.row(), 13);
                QVariant nothing;

                model->setData(n_qi, nothing); // set the meeting to blank since it won't be in the new project
            }
        }
        break;
    case 2:
    case 8:
    case 9:
        {
            ComboBox *comboBox = static_cast<ComboBox*>(editor);
            key_val = comboBox->itemText(comboBox->currentIndex());
        }
        break;
    case 4:
    case 7:
        {
            ComboBox *comboBox = static_cast<ComboBox*>(editor);
            int i;

            if (!comboBox->currentText().isEmpty() )
            {
                i = comboBox->findText(comboBox->currentText(), Qt::MatchFixedString);
                comboBox->setCurrentIndex(i);

                if (i >= 0)
                {
                    key_val = comboBox->model()->data(comboBox->model()->index(i, 3));
                }
            }
        }
        break;
    case 15:
        {
            QCheckBox *checkbox = static_cast<QCheckBox*>(editor);

            if ( checkbox->isChecked() )
                key_val = "1";
            else
                key_val = "0";
        }
        break;
    case 3:
    case 6:
        {
            QPlainTextEdit* lineedit = static_cast<QPlainTextEdit*>(editor);
            key_val = lineedit->toPlainText();
        }
        break;
    default:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
            key_val = lineedit->text();
        }
        break;
    }

    model->setData(index, key_val, Qt::EditRole);
}


bool ItemDetailsDelegate::verifyProjectNumber(QVariant& projectId, QVariant& itemId) const
{
    QString msg;
    int issuestoresolve = 0;

    QSqlDatabase db = global_DBObjects.getDb();

    // check to see if identified by is in the project team
    {
        DB_LOCK;
        QSqlQuery select(db);
        select.prepare("select identified_by, (select name from people p where p.id=identified_by) people_id_name from item_tracker where id = ? and identified_by not in (select people_id from project_people where project_id = ?)");
        select.bindValue(0, itemId);
        select.bindValue(1, projectId);

        if (select.exec())
        {
            while (select.next())
            {
                issuestoresolve++;
                msg += "Identified By, " + select.record().value(1).toString() + " is not found on the selected projects team.  Remove or change this person before changing to this project number.\n";
            }
        }
        DB_UNLOCK;
    }

    // check to see if assigned to is in the project team
    {
        DB_LOCK;
        QSqlQuery select(db);
        select.prepare("select assigned_to, (select name from people p where p.id=assigned_to) people_id_name from item_tracker where id = ? and assigned_to not in (select people_id from project_people where project_id = ?)");
        select.bindValue(0, itemId);
        select.bindValue(1, projectId);

        if (select.exec())
        {
            while (select.next())
            {
                issuestoresolve++;
                msg += "Assigned To, " + select.record().value(1).toString() + " is not found on the selected projects team.  Remove or change this person before changing to this project number.\n";
            }
        }
        DB_UNLOCK;
    }

    // check to see if updated by value is in team
    {
        DB_LOCK;
        QSqlQuery select(db);
        select.prepare("select updated_by, (select name from people p where p.id=updated_by) people_id_name from item_tracker_updates where item_id = ? and updated_by not in (select people_id from project_people where project_id = ? )");
        select.bindValue(0, itemId);
        select.bindValue(1, projectId);

        if (select.exec())
        {
            while (select.next())
            {
                issuestoresolve++;
                msg += "Comment Updated By, " + select.record().value(1).toString() + " is not found on the selected projects team.  Remove or change this person before changing to this project number.\n";
            }
        }
        DB_UNLOCK;
    }

    if (issuestoresolve)
    {
        QMessageBox::critical(nullptr, QObject::tr("Cannot reassign project"), msg);
        return false;
    }

    // note the meeting value will be cleared
    // have user confirm the change
    {
        DB_LOCK;
        QSqlQuery select(db);
        select.prepare("select note_id from item_tracker where id = ?");
        select.bindValue(0, itemId);

        if (select.exec())
        {
            if (select.next() && !select.record().value(0).isNull())
            {
                DB_UNLOCK;

                if ( QMessageBox::question(nullptr, QObject::tr("Associatd Meeting"),
                   "The associated meeting will be removed.  Still reassign the project?\n", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No )
                    return false;
            }
            else
            {
                DB_UNLOCK;
            }
        }
        else
        {
            DB_UNLOCK;
        }
    }

    return true;
}
