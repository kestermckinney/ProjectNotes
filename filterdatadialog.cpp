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

void FilterDataDialog::setEndValue(QString& text)
{
    ui->lineEditEndValue->setText(text);
}

void FilterDataDialog::setBeginValue(QString& text)
{
    ui->lineEditStartValue->setText(text);
}

void FilterDataDialog::setSearchText(QString& text)
{
    ui->lineEditSearchText->setText(text);
}

QString FilterDataDialog::getEndValue()
{
    return ui->lineEditEndValue->text();
}

QString FilterDataDialog::getStartValue()
{
    return ui->lineEditStartValue->text();
}

QString FilterDataDialog::getSearchText()
{
    return ui->lineEditSearchText->text();
}

void FilterDataDialog::setFilterModel(PNSqlQueryModel* model)
{
    filterName = model->objectName();
    filteredModel = model;

    columnModel->setColumnModel(model);
    columnModel->Refresh();

    savedFilters.clear();


    // create storage for filters and load current ones
    int colcount = model->columnCount();
    for (int i = 0; i < colcount; i++)
    {
        if ( columnModel->isSearchable(i) )
        {
            FilterSaveStructure fs = FilterSaveStructure();
            QString colname = columnModel->getColumnName(i);

            model->GetUserSearchRange(i, fs.SearchBeginValue, fs.SearchEndValue);
            fs.SearchString = columnModel->GetUserSearchString(i);

            fs.ColumnValues = columnModel->GetUserFilter(i);

            savedFilters[colname] = fs;

            if (selectedColumn == -1)
                selectedColumn = i;
        }
    }

    if (selectedColumn == -1)
        selectedColumn = 0;

    ui->tableViewColumnName->selectRow(selectedColumn);
    QModelIndex qi = columnModel->index(selectedColumn,0);

    valuesModel->setFilteringModel(model);
    valuesModel->setValuesColumn(columnModel->data(qi).toString());

    ui->tableViewColumnName->setColumnValuesModel(valuesModel);
    ui->tableViewColumnName->setFilteredModel(model);
    ui->tableViewColumnName->setValuesView(ui->tableViewFilterValues, &savedFilters);
    ui->tableViewFilterValues->setSavedFilters(&savedFilters);
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

        if ( !it.value().SearchString.isEmpty() )
            filteredModel->SetUserSearchString( ColumnNumber, it.value().SearchString );
        else
            filteredModel->ClearUserSearchString( ColumnNumber );

        filteredModel->SetUserSearchRange(ColumnNumber,it.value().SearchBeginValue, it.value().SearchEndValue);

        if ( it.value().ColumnValues.size() > 0 )
        {
            filteredModel->SetUserFilter(ColumnNumber, it.value().ColumnValues);
        }
        else
            filteredModel->ClearUserFilter( ColumnNumber );
    }

    filteredModel->SaveUserFilter(filterName);
    filteredModel->ActivateUserFilter(filterName);

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

        QString empty;
        setEndValue(empty);
        setBeginValue(empty);
        setSearchText(empty);
    }
}
