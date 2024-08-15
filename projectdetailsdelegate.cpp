// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectdetailsdelegate.h"
#include "pnsqlquerymodel.h"
#include "pndateeditex.h"
#include "pnbasepage.h"
#include "mainwindow.h"
#include "pncombobox.h"

#include <QLineEdit>
#include <QDebug>

ProjectDetailsDelegate::ProjectDetailsDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void ProjectDetailsDelegate::setEditorData(QWidget *t_editor, const QModelIndex &t_index) const
{
    QVariant value = t_index.model()->data(t_index);

    switch (t_index.column())
    {
    case 2:
        {
            QPlainTextEdit* lineedit = static_cast<QPlainTextEdit*>(t_editor);

            // don't resent buffers if text hasn't changed
            if (value.toString().compare(lineedit->toPlainText()) != 0)
                lineedit->setPlainText(value.toString());

            QWidget* window = static_cast<QWidget*>(t_editor)->topLevelWidget();
            if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
        }
        break;
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
    case 5: // primary contact
        {
            PNComboBox *comboBox = static_cast<PNComboBox*>(t_editor);
            PNSqlQueryModel *model = static_cast<PNSqlQueryModel*>(comboBox->model());

            if (model)
            {
                QVariant list_value = model->findValue(value, 3, 1);

                if (!list_value.isNull())
                {
                    int i = comboBox->findText(list_value.toString(), Qt::MatchFixedString);
                    if (i >= 0)
                    {
                        comboBox->setCurrentIndex(i);
                        //qDebug() << "Setting Combo Box Delegate Index to : " << i;
                        comboBox->setCurrentText(list_value.toString());
                        //qDebug() << "Setting Combo Box Delegate to : " << list_value.toString();
                    }
                    else
                    {
                        comboBox->setCurrentText(QString());
                        //qDebug() << "Setting Combo Box Delegate to : EMPTY";
                    }
                }
                else
                {
                    comboBox->setCurrentText(QString());
                    //qDebug() << "Setting Combo Box Delegate to : EMPTY";
                }
            }
        }
        break;
    case 11:
    case 12:
    case 14:
        {
            PNComboBox *comboBox = static_cast<PNComboBox*>(t_editor);
            comboBox->setCurrentText(value.toString());
        }
        break;

    case 13:
        {
            PNComboBox *comboBox = static_cast<PNComboBox*>(t_editor);
            PNSqlQueryModel *model = static_cast<PNSqlQueryModel*>(comboBox->model());

            if (model)
            {
                QVariant list_value = model->findValue(value, 0, 1);

                if (!list_value.isNull())
                {
                    int i = comboBox->findText(list_value.toString(), Qt::MatchFixedString);
                    if (i >= 0)
                    {
                        comboBox->setCurrentIndex(i);
                        comboBox->setCurrentText(list_value.toString());
                    }
                    else
                        comboBox->setCurrentText(QString());
                }
                else
                     comboBox->setCurrentText(QString());
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
    case 2:
        {
            QPlainTextEdit* lineedit = static_cast<QPlainTextEdit*>(t_editor);
            key_val = lineedit->toPlainText();

            QWidget* window = static_cast<QWidget*>(t_editor)->topLevelWidget();
            if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
        }
        break;
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
    case 5:  // primary contact
        {
            PNComboBox *comboBox = static_cast<PNComboBox*>(t_editor);
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
    case 11:
    case 12:
    case 14:
        {
            PNComboBox *comboBox = static_cast<PNComboBox*>(t_editor);
            key_val = comboBox->itemText(comboBox->currentIndex());
        }
        break;
    case 13:
        {
            PNComboBox *comboBox = static_cast<PNComboBox*>(t_editor);
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
    default:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(t_editor);
            key_val = lineedit->text();
        }
    }

    t_model->setData(t_index, key_val, Qt::EditRole);
}
