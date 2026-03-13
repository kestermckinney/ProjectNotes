// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectdetailsdelegate.h"
#include "databaseobjects.h"
#include "dateeditex.h"
#include "basepage.h"
#include "mainwindow.h"
#include "combobox.h"

#include <QLineEdit>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;


ProjectDetailsDelegate::ProjectDetailsDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void ProjectDetailsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QVariant value = index.model()->data(index);

    switch (index.column())
    {
    case 2:
        {
            QPlainTextEdit* lineedit = static_cast<QPlainTextEdit*>(editor);

            // don't resent buffers if text hasn't changed
            if (value.toString().compare(lineedit->toPlainText()) != 0)
                lineedit->setPlainText(value.toString());

            QWidget* window = static_cast<QWidget*>(editor)->topLevelWidget();
            if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
        }
        break;
    case 3:
    case 4:
        {
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
    case 5: // primary contact
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
                    }
                    else
                    {
                        comboBox->setCurrentText(QString());
                    }
                }
                else
                {
                    comboBox->setCurrentText(QString());
                }
            }
        }
        break;
    case 11:
    case 12:
    case 14:
        {
            ComboBox *comboBox = static_cast<ComboBox*>(editor);
            comboBox->setCurrentText(value.toString());
        }
        break;

    case 13:
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
                        // test to see if you need to set the text after seetting the index comboBox->setCurrentText(list_value.toString());
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
            QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
            lineedit->setText(value.toString());
        }
    }
}

void ProjectDetailsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QVariant key_val;

    switch (index.column())
    {
    case 2:
        {
            QPlainTextEdit* lineedit = static_cast<QPlainTextEdit*>(editor);
            key_val = lineedit->toPlainText();

            QWidget* window = static_cast<QWidget*>(editor)->topLevelWidget();
            if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
        }
        break;
    case 3:
    case 4:
        {
            DateEditEx* dateEdit = static_cast<DateEditEx*>(editor);
            if (!dateEdit->isNull())
                key_val = dateEdit->date().toString("MM/dd/yyyy");
            else
                key_val.clear();
        }
        break;
    case 5:  // primary contact
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
    case 11:
    case 12:
    case 14:
        {
            ComboBox *comboBox = static_cast<ComboBox*>(editor);
            key_val = comboBox->itemText(comboBox->currentIndex());
        }
        break;
    case 13:
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
    default:
        {
            QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
            key_val = lineedit->text();
        }
    }

    model->setData(index, key_val, Qt::EditRole);
}
