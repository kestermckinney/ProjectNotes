// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectnotesdelegate.h"
#include "pnsqlquerymodel.h"
#include "pndateeditex.h"
#include "mainwindow.h"
#include "pnbasepage.h"

#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QCheckBox>

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
        {
            PNDateEditEx* dateEdit = static_cast<PNDateEditEx*>(t_editor);

            if (value.isNull())
                dateEdit->setDateTime(QDateTime());
            else
            {
                QDateTime date_value = PNSqlQueryModel::parseDateTime(value.toString());
                dateEdit->setDate(date_value.date());

                QWidget* window = static_cast<QWidget*>(t_editor)->topLevelWidget();
                if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                    dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
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
    case 5: // note internal
        {
            QCheckBox* chkbox = static_cast<QCheckBox*>(t_editor);
            if (value.toString() == "0")
                chkbox->setChecked(false);
            else
                chkbox->setChecked(true);
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
            QPlainTextEdit* lineedit = static_cast<QPlainTextEdit*>(t_editor);
            key_val = lineedit->toPlainText();

            QWidget* window = static_cast<QWidget*>(t_editor)->topLevelWidget();
            if (dynamic_cast<MainWindow*>(window)->navigateCurrentPage())
                dynamic_cast<MainWindow*>(window)->navigateCurrentPage()->setPageTitle();
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
    case 5: // note internal
        {
            QCheckBox* chkbox = static_cast<QCheckBox*>(t_editor);
            if (chkbox->isChecked())
                key_val = QString("1");
            else
                key_val = QString("0");
        }
        break;
    }

    t_model->setData(t_index, key_val, Qt::EditRole);
}
