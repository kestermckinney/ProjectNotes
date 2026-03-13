// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseobjects.h"
#include "filterdatadialog.h"
#include "ui_filterdatadialog.h"
#include "columnmodel.h"
#include "valueselectmodel.h"
#include "appsettings.h"

FilterDataDialog::FilterDataDialog(QWidget *m_parent) :
    QDialog(m_parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint ),
    ui(new Ui::FilterDataDialog)
{
    ui->setupUi(this);

    setModal(true);

    m_columnModel = new ColumnModel(&global_DBObjects);
    m_columnProxyModel = new SortFilterProxyModel(this);
    m_columnProxyModel->setSourceModel(m_columnModel);

    m_valuesModel = new ValueSelectModel(&global_DBObjects);
    m_valuesProxyModel = new SortFilterProxyModel(this);
    m_valuesProxyModel->setSourceModel(m_valuesModel);

    ui->tableViewColumnName->setUI(this);

    QString storename = objectName();
    global_Settings.getWindowState(storename, this);

    m_selectedColumn = -1;
}

FilterDataDialog::~FilterDataDialog()
{
    QString storename = objectName();
    global_Settings.setWindowState(storename, this);

    ui->tableViewFilterValues->setModel(nullptr);
    ui->tableViewColumnName->setModel(nullptr);

    delete m_valuesProxyModel;
    delete m_valuesModel;
    delete m_columnProxyModel;
    delete m_columnModel;
    delete ui;
}

void FilterDataDialog::setEndValue(QVariant& text)
{
    ui->lineEditEndValue->setText(text.toString());
}

void FilterDataDialog::setBeginValue(QVariant& text)
{
    ui->lineEditStartValue->setText(text.toString());
}

void FilterDataDialog::setSearchText(QVariant& text)
{
    ui->lineEditSearchText->setText(text.toString());
}

void FilterDataDialog::setSearchTextEnabled( bool enabled )
{
    ui->lineEditSearchText->setEnabled( enabled );

    if (!enabled)
        ui->lineEditSearchText->clear();
}

QVariant FilterDataDialog::getEndValue()
{
    return ui->lineEditEndValue->text();
}

QVariant FilterDataDialog::getStartValue()
{
    return ui->lineEditStartValue->text();
}

QVariant FilterDataDialog::getSearchText()
{
    return ui->lineEditSearchText->text();
}

void FilterDataDialog::setSourceModelView(SqlQueryModel *model, TableView *view)
{
    m_filteredModel = model;
    m_sourceView = view;

    // set the names differently for each instance so the table view widths are saved differently for each view filter
    ui->tableViewColumnName->setObjectName("tableViewColumnName_" + view->objectName());
    ui->tableViewFilterValues->setObjectName("tableViewFilterValues_" + view->objectName());

    ui->tableViewColumnName->setSourceView( view );

    m_columnModel->setColumnModel(model, view);
    m_columnModel->setSavedFilters(&m_savedFilters);
    m_columnModel->setFilteringModel(model);
    m_columnModel->refresh();

    QModelIndex qi = m_columnModel->index(m_selectedColumn, 0);

    m_valuesModel->setFilteringModel(model);
    m_valuesModel->setValuesColumn(m_columnModel->data(qi).toString());
    m_valuesModel->refresh();

    ui->tableViewColumnName->setColumnValuesModel(m_valuesModel);
    ui->tableViewColumnName->setFilteredModel(model);
    ui->tableViewColumnName->setValuesView(ui->tableViewFilterValues, &m_savedFilters);
    ui->tableViewFilterValues->setSavedFilters(&m_savedFilters);

    ui->tableViewColumnName->setModel(m_columnProxyModel);
    ui->tableViewFilterValues->setModel(m_valuesProxyModel);

    // sometimes the column width get zeroed out
    if (ui->tableViewColumnName->columnWidth(0) <= 0)
        ui->tableViewColumnName->resizeColumnToContents(0);

    setupFilters();

    // ui->tableViewColumnName->selectRow(0);
    ui->tableViewColumnName->dataRowSelected(m_columnProxyModel->index(0,0));

    m_valuesModel->refresh();

}

void FilterDataDialog::on_lineEditSearchText_textEdited(const QString &arg1)
{
    QModelIndexList qil = ui->tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = m_columnModel->data(qi).toString();
        QString dbcolname = m_filteredModel->getColumnName(displaycolname);

        m_savedFilters[dbcolname].SearchString = arg1;
    }
}

void FilterDataDialog::on_lineEditStartValue_textEdited(const QString &arg1)
{
    QModelIndexList qil = ui->tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = m_columnModel->data(qi).toString();
        QString dbcolname = m_filteredModel->getColumnName(displaycolname);

        m_savedFilters[dbcolname].SearchBeginValue = arg1;
    }
}

void FilterDataDialog::on_lineEditEndValue_textEdited(const QString &arg1)
{
    QModelIndexList qil = ui->tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = m_columnModel->data(qi).toString();
        QString dbcolname = m_filteredModel->getColumnName(displaycolname);

        m_savedFilters[dbcolname].SearchEndValue = arg1;
    }
}

void FilterDataDialog::on_pushButtonApply_clicked()
{
    for (auto it = m_savedFilters.begin(); it != m_savedFilters.end(); ++it)
    {
        QString ColumnName = it.key();
        int columnNumber = m_filteredModel->getColumnNumber(ColumnName);

        if (columnNumber >= 0)
        {
            m_filteredModel->clearUserFilter(columnNumber);
            m_filteredModel->clearUserSearchRange(columnNumber);
            m_filteredModel->clearUserSearchString(columnNumber);

            // save the general search text
            if ( !it.value().SearchString.toString().isEmpty() )
                m_filteredModel->setUserSearchString( columnNumber, it.value().SearchString );

            // set the range based filter
            if ( !it.value().SearchBeginValue.toString().isEmpty() || !it.value().SearchEndValue.toString().isEmpty() )
                m_filteredModel->setUserSearchRange(columnNumber,it.value().SearchBeginValue, it.value().SearchEndValue);

            // capture all of the selected values to search for
            if ( it.value().ColumnValues.size() > 0 )
            {
                m_filteredModel->setUserFilter(columnNumber, it.value().ColumnValues);
            }
        }
    }

    QString filterName = m_filteredModel->objectName();
    m_filteredModel->saveUserFilter(filterName);
    m_filteredModel->activateUserFilter(filterName);

    close();
}

void FilterDataDialog::on_pushButtonCancel_clicked()
{
    close();
}

void FilterDataDialog::on_pushButtonAll_clicked()
{
    QModelIndexList qil = ui->tableViewColumnName->selectionModel()->selectedRows();
    if (qil.begin() != qil.end())
    {
        QString displaycolname = m_columnModel->data(*(qil.begin())).toString();
        QString dbcolname = m_filteredModel->getColumnName(displaycolname);

        // set all of the selected values
        ui->tableViewFilterValues->selectionModel()->clear();

        // set all of the search parameters
        m_savedFilters[dbcolname].SearchEndValue.clear();
        m_savedFilters[dbcolname].SearchBeginValue.clear();
        m_savedFilters[dbcolname].SearchString.clear();
        m_savedFilters[dbcolname].ColumnValues.clear();

        QVariant empty;
        setEndValue(empty);
        setBeginValue(empty);
        setSearchText(empty);
    }
}

void FilterDataDialog::on_pushButtonReset_clicked()
{
    m_filteredModel->clearAllUserSearches();
    m_columnModel->refresh();

    setupFilters();

    QVariant empty;
    setEndValue(empty);
    setBeginValue(empty);
    setSearchText(empty);

    // ui->tableViewColumnName->selectRow(0);
    ui->tableViewColumnName->dataRowSelected(m_columnProxyModel->index(0,0));
    m_valuesModel->refresh();
}

void FilterDataDialog::setupFilters()
{
    m_savedFilters.clear();

    // create storage for filters and load current ones
    int colcount = m_filteredModel->columnCount();
    int visiblecolumn = 0;
    for (int i = 0; i < colcount; i++)
    {
        if ( m_filteredModel->isSearchable(i) && (m_sourceView == nullptr || !m_sourceView->isColumnHidden(i)) )
        {
            FilterSaveStructure fs = FilterSaveStructure();
            QString colname = m_filteredModel->getColumnName(i);

            m_filteredModel->getUserSearchRange(i, fs.SearchBeginValue, fs.SearchEndValue);
            fs.SearchString = m_filteredModel->getUserSearchString(i);

            fs.ColumnValues = m_filteredModel->getUserFilter(i);

            m_savedFilters[colname] = fs;

            if (m_selectedColumn == -1)
                m_selectedColumn = visiblecolumn;

            visiblecolumn++;
        }
    }

    if (m_selectedColumn == -1)
        m_selectedColumn = 0;

    // trigger the column name was selected
    ui->tableViewColumnName->selectRow(m_selectedColumn);
}

