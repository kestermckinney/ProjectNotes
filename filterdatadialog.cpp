#include "filterdatadialog.h"
#include "ui_filterdatadialog.h"
#include "pncolumnmodel.h"
#include "valueselectmodel.h"

FilterDataDialog::FilterDataDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilterDataDialog)
{
    ui->setupUi(this);

    setModal(true);

    columnModel = new PNColumnModel(this);
    columnProxyModel = new PNSortFilterProxyModel(this);
    columnProxyModel->setSourceModel(columnModel);

    valuesModel = new ValueSelectModel(this);
    valuesProxyModel = new PNSortFilterProxyModel(this);
    valuesProxyModel->setSourceModel(valuesModel);

    ui->tableViewColumnName->setModel(columnProxyModel);
    ui->tableViewColumnName->setUI(this);
    ui->tableViewFilterValues->setModel(valuesProxyModel);
    QString storename = objectName();
    global_Settings.getWindowState(storename, *this);

    selectedColumn = -1;
}

FilterDataDialog::~FilterDataDialog()
{
    QString storename = objectName();
    global_Settings.setWindowState(storename, *this);

    ui->tableViewFilterValues->setModel(nullptr);
    ui->tableViewColumnName->setModel(nullptr);

    delete valuesProxyModel;
    delete valuesModel;
    delete columnProxyModel;
    delete columnModel;
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

void FilterDataDialog::setFilterModel(PNSqlQueryModel* model)
{
    filteredModel = model;

    columnModel->setColumnModel(model);
    columnModel->setSavedFilters(&savedFilters);
    columnModel->setFilteringModel(model);
    columnModel->Refresh();

    QModelIndex qi = columnModel->index(selectedColumn,0);

    valuesModel->setFilteringModel(model);
    valuesModel->setValuesColumn(columnModel->data(qi).toString());

    ui->tableViewColumnName->setColumnValuesModel(valuesModel);
    ui->tableViewColumnName->setFilteredModel(model);
    ui->tableViewColumnName->setValuesView(ui->tableViewFilterValues, &savedFilters);
    ui->tableViewFilterValues->setSavedFilters(&savedFilters);  

    setupFilters();
}

void FilterDataDialog::on_lineEditSearchText_textEdited(const QString &arg1)
{
    QModelIndexList qil = ui->tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = columnModel->data(qi).toString();
        QString dbcolname = filteredModel->getColumnName(displaycolname);

        savedFilters[dbcolname].SearchString = arg1;
    }
}

void FilterDataDialog::on_lineEditStartValue_textEdited(const QString &arg1)
{
    QModelIndexList qil = ui->tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = columnModel->data(qi).toString();
        QString dbcolname = filteredModel->getColumnName(displaycolname);

        savedFilters[dbcolname].SearchBeginValue = arg1;
    }
}

void FilterDataDialog::on_lineEditEndValue_textEdited(const QString &arg1)
{
    QModelIndexList qil = ui->tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = columnModel->data(qi).toString();
        QString dbcolname = filteredModel->getColumnName(displaycolname);

        savedFilters[dbcolname].SearchEndValue = arg1;
    }
}

void FilterDataDialog::on_pushButtonApply_clicked()
{
    for (auto it = savedFilters.begin(); it != savedFilters.end(); ++it)
    {
        QString ColumnName = it.key();
        int ColumnNumber = filteredModel->getColumnNumber(ColumnName);

        // save the general search text
        if ( !it.value().SearchString.toString().isEmpty() )
            filteredModel->SetUserSearchString( ColumnNumber, it.value().SearchString );
        else
            filteredModel->ClearUserSearchString( ColumnNumber );

        // set the range based filter
        if ( !it.value().SearchBeginValue.toString().isEmpty() || !it.value().SearchEndValue.toString().isEmpty() )
            filteredModel->SetUserSearchRange(ColumnNumber,it.value().SearchBeginValue, it.value().SearchEndValue);
        else
            filteredModel->ClearUserSearchRange(ColumnNumber);

        // capture all of the selected values to search for
        if ( it.value().ColumnValues.size() > 0 )
        {
            filteredModel->SetUserFilter(ColumnNumber, it.value().ColumnValues);
        }
        else
            filteredModel->ClearUserFilter( ColumnNumber );
    }

    QString filtername = filteredModel->objectName();
    filteredModel->SaveUserFilter(filtername);
    filteredModel->ActivateUserFilter(filtername);

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
        QString displaycolname = columnModel->data(*(qil.begin())).toString();
        QString dbcolname = filteredModel->getColumnName(displaycolname);

        // set all of the selected values
        ui->tableViewFilterValues->selectionModel()->clear();

        // set all of the search parameters
        savedFilters[dbcolname].SearchEndValue.clear();
        savedFilters[dbcolname].SearchBeginValue.clear();
        savedFilters[dbcolname].SearchString.clear();
        savedFilters[dbcolname].ColumnValues.clear();

        QVariant empty;
        setEndValue(empty);
        setBeginValue(empty);
        setSearchText(empty);
    }
}

void FilterDataDialog::on_pushButtonReset_clicked()
{
    filteredModel->ClearAllUserSearches();
    filteredModel->SaveUserFilter(filteredModel->objectName());
    filteredModel->DeactivateUserFilter(filteredModel->objectName());
    columnModel->Refresh();

    setupFilters();
}

void FilterDataDialog::setupFilters()
{
    savedFilters.clear();

    // create storage for filters and load current ones
    int colcount = filteredModel->columnCount();
    int visiblecolumn = 0;
    for (int i = 0; i < colcount; i++)
    {
        if ( filteredModel->isSearchable(i) )
        {
            FilterSaveStructure fs = FilterSaveStructure();
            QString colname = filteredModel->getColumnName(i);

            filteredModel->GetUserSearchRange(i, fs.SearchBeginValue, fs.SearchEndValue);
            fs.SearchString = filteredModel->GetUserSearchString(i);

            fs.ColumnValues = filteredModel->GetUserFilter(i);

            savedFilters[colname] = fs;

            if (selectedColumn == -1)
                selectedColumn = visiblecolumn;

            visiblecolumn++;
        }
    }

    if (selectedColumn == -1)
        selectedColumn = 0;

    // trigger the column name was selected
    ui->tableViewColumnName->selectRow(selectedColumn);
    QModelIndex qi = columnModel->index(selectedColumn, 0);
    ui->tableViewColumnName->dataRowSelected(qi);
}
