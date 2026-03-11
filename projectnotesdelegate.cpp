// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectnotesdelegate.h"
#include "databaseobjects.h"
#include "dateeditex.h"
#include "mainwindow.h"
#include "basepage.h"
#include "combobox.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QScrollBar>

ProjectNotesDelegate::ProjectNotesDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void ProjectNotesDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QVariant value = index.model()->data(index);

    switch (index.column())
    {
    case 2: // note title
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
        {
            DateEditEx* dateEdit = static_cast<DateEditEx*>(editor);

            if (value.isNull())
                dateEdit->setDateTime(QDateTime());
            else
            {
                QDateTime date_value = SqlQueryModel::parseDateTime(value.toString());
                dateEdit->setDate(date_value.date());

                QWidget* window = static_cast<QWidget*>(editor)->topLevelWidget();
                if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                    dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
            }
        }
        break;
    case 4: // note
        {
            QTextEdit* textedit = static_cast<QTextEdit*>(editor);

            // don't resent buffers if text hasn't changed
            if (value.toString().contains("<html>", Qt::CaseInsensitive))
            {
                if (value.toString().compare(textedit->toHtml()) != 0)
                    textedit->setHtml(value.toString());
            }
            else
            {
                if (value.toString().compare(textedit->toPlainText()) != 0)
                    textedit->setPlainText(value.toString());
            }

        }
        break;
    case 5: // note internal
        {
            QCheckBox* chkbox = static_cast<QCheckBox*>(editor);
            if (value.toString() == "0")
                chkbox->setChecked(false);
            else
                chkbox->setChecked(true);
        }
        break;
    }
}

void ProjectNotesDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QVariant key_val;

    switch (index.column())
    {
    case 2: // note title
        {
            QPlainTextEdit* lineedit = static_cast<QPlainTextEdit*>(editor);
            key_val = lineedit->toPlainText();

            QWidget* window = static_cast<QWidget*>(editor)->topLevelWidget();
            if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
        }
        break;
    case 3: // note date
        {
            DateEditEx* dateEdit = static_cast<DateEditEx*>(editor);
            if (!dateEdit->isNull())
                key_val = dateEdit->date().toString("MM/dd/yyyy");
            else
                key_val.clear();
        }
        break;
    case 4: // note text
        {
            QTextEdit* textedit = static_cast<QTextEdit*>(editor);
            key_val = textedit->toHtml();
        }
        break;
    case 5: // note internal
        {
            QCheckBox* chkbox = static_cast<QCheckBox*>(editor);
            if (chkbox->isChecked())
                key_val = QString("1");
            else
                key_val = QString("0");
        }
        break;
    }

    model->setData(index, key_val, Qt::EditRole);
}
