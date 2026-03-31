// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef FILTERDATADIALOG_H
#define FILTERDATADIALOG_H

#include "filtersavestructure.h"
#include "columnmodel.h"
#include "databaseobjects.h"

#include "valueselectmodel.h"
#include "sortfilterproxymodel.h"

#include <QDialog>

namespace Ui {
class FilterDataDialog;
}

class TableView;

class FilterDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDataDialog(QWidget *parent = nullptr);
    ~FilterDataDialog();

    void setSourceModelView(SqlQueryModel* model,TableView* view);
    void setEndValue(QVariant& text);
    void setBeginValue(QVariant& text);
    void setSearchText(QVariant& text);
    void setSearchTextEnabled( bool enabled );
    QVariant getEndValue();
    QVariant getStartValue();
    QVariant getSearchText();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void on_lineEditSearchText_textEdited(const QString &arg1);

    void on_lineEditStartValue_textEdited(const QString &arg1);

    void on_lineEditEndValue_textEdited(const QString &arg1);

    void on_pushButtonApply_clicked();

    void on_pushButtonCancel_clicked();

    void on_pushButtonAll_clicked();

    void on_pushButtonReset_clicked();

private:
    void setupFilters();
    Ui::FilterDataDialog *ui;

    ColumnModel* m_columnModel = nullptr;
    SortFilterProxyModel* m_columnProxyModel = nullptr;
    ValueSelectModel* m_valuesModel = nullptr;
    SortFilterProxyModel* m_valuesProxyModel = nullptr;
    SqlQueryModel* m_filteredModel = nullptr;
    TableView* m_sourceView = nullptr;

    int m_selectedColumn = -1;  // nothing selected until construction

    QHash<QString, FilterSaveStructure> m_savedFilters;
};

#endif // FILTERDATADIALOG_H
