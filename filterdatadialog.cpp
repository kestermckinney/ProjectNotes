#include "filterdatadialog.h"
#include "ui_filterdatadialog.h"
#include "pncolumnmodel.h"
#include "valueselectmodel.h"

FilterDataDialog::FilterDataDialog(QWidget *m_parent) :
    QDialog(m_parent),
    ui(new Ui::FilterDataDialog)
{
    ui->setupUi(this);

    setModal(true);

    m_column_model = new PNColumnModel(this);
    m_column_proxy_model = new PNSortFilterProxyModel(this);
    m_column_proxy_model->setSourceModel(m_column_model);

    m_values_model = new ValueSelectModel(this);
    m_values_proxy_model = new PNSortFilterProxyModel(this);
    m_values_proxy_model->setSourceModel(m_values_model);

    ui->t_tableViewColumnName->setModel(m_column_proxy_model);
    ui->t_tableViewColumnName->setUI(this);
    ui->t_tableViewt_filter_values->setModel(m_values_proxy_model);
    QString storename = objectName();
    global_Settings.getWindowState(storename, *this);

    m_selected_column = -1;
}

FilterDataDialog::~FilterDataDialog()
{
    QString storename = objectName();
    global_Settings.setWindowState(storename, *this);

    ui->t_tableViewt_filter_values->setModel(nullptr);
    ui->t_tableViewColumnName->setModel(nullptr);

    delete m_values_proxy_model;
    delete m_values_model;
    delete m_column_proxy_model;
    delete m_column_model;
    delete ui;
}

void FilterDataDialog::setEndValue(QVariant& t_text)
{
    ui->lineEditEndValue->setText(t_text.toString());
}

void FilterDataDialog::setBeginValue(QVariant& t_text)
{
    ui->lineEditStartValue->setText(t_text.toString());
}

void FilterDataDialog::setSearchText(QVariant& t_text)
{
    ui->lineEditSearchText->setText(t_text.toString());
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

void FilterDataDialog::setFilterModel(PNSqlQueryModel* t_model)
{
    m_filtered_model = t_model;

    m_column_model->setColumnModel(t_model);
    m_column_model->setSavedFilters(&m_saved_filters);
    m_column_model->setFilteringModel(t_model);
    m_column_model->Refresh();

    QModelIndex qi = m_column_model->index(m_selected_column, 0);

    m_values_model->setFilteringModel(t_model);
    m_values_model->setValuesColumn(m_column_model->data(qi).toString());

    ui->t_tableViewColumnName->setColumnValuesModel(m_values_model);
    ui->t_tableViewColumnName->setFilteredModel(t_model);
    ui->t_tableViewColumnName->setValuesView(ui->t_tableViewt_filter_values, &m_saved_filters);
    ui->t_tableViewt_filter_values->setSavedFilters(&m_saved_filters);

    setupFilters();
}

void FilterDataDialog::on_lineEditSearchText_textEdited(const QString &t_arg1)
{
    QModelIndexList qil = ui->t_tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = m_column_model->data(qi).toString();
        QString dbcolname = m_filtered_model->getColumnName(displaycolname);

        m_saved_filters[dbcolname].SearchString = t_arg1;
    }
}

void FilterDataDialog::on_lineEditStartValue_textEdited(const QString &t_arg1)
{
    QModelIndexList qil = ui->t_tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = m_column_model->data(qi).toString();
        QString dbcolname = m_filtered_model->getColumnName(displaycolname);

        m_saved_filters[dbcolname].t_search_begin_value = t_arg1;
    }
}

void FilterDataDialog::on_lineEditEndValue_textEdited(const QString &t_arg1)
{
    QModelIndexList qil = ui->t_tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = m_column_model->data(qi).toString();
        QString dbcolname = m_filtered_model->getColumnName(displaycolname);

        m_saved_filters[dbcolname].t_search_end_value = t_arg1;
    }
}

void FilterDataDialog::on_pushButtonApply_clicked()
{
    for (auto it = m_saved_filters.begin(); it != m_saved_filters.end(); ++it)
    {
        QString ColumnName = it.key();
        int t_column_number = m_filtered_model->getColumnNumber(ColumnName);

        // save the general search text
        if ( !it.value().SearchString.toString().isEmpty() )
            m_filtered_model->SetUserSearchString( t_column_number, it.value().SearchString );
        else
            m_filtered_model->ClearUserSearchString( t_column_number );

        // set the range based filter
        if ( !it.value().t_search_begin_value.toString().isEmpty() || !it.value().t_search_end_value.toString().isEmpty() )
            m_filtered_model->SetUserSearchRange(t_column_number,it.value().t_search_begin_value, it.value().t_search_end_value);
        else
            m_filtered_model->ClearUserSearchRange(t_column_number);

        // capture all of the selected values to search for
        if ( it.value().ColumnValues.size() > 0 )
        {
            m_filtered_model->SetUserFilter(t_column_number, it.value().ColumnValues);
        }
        else
            m_filtered_model->ClearUserFilter( t_column_number );
    }

    QString t_filter_name = m_filtered_model->objectName();
    m_filtered_model->SaveUserFilter(t_filter_name);
    m_filtered_model->ActivateUserFilter(t_filter_name);

    close();
}

void FilterDataDialog::on_pushButtonCancel_clicked()
{
    close();
}

void FilterDataDialog::on_pushButtonAll_clicked()
{
    QModelIndexList qil = ui->t_tableViewColumnName->selectionModel()->selectedRows();
    if (qil.begin() != qil.end())
    {
        QString displaycolname = m_column_model->data(*(qil.begin())).toString();
        QString dbcolname = m_filtered_model->getColumnName(displaycolname);

        // set all of the selected values
        ui->t_tableViewt_filter_values->selectionModel()->clear();

        // set all of the search parameters
        m_saved_filters[dbcolname].t_search_end_value.clear();
        m_saved_filters[dbcolname].t_search_begin_value.clear();
        m_saved_filters[dbcolname].SearchString.clear();
        m_saved_filters[dbcolname].ColumnValues.clear();

        QVariant empty;
        setEndValue(empty);
        setBeginValue(empty);
        setSearchText(empty);
    }
}

void FilterDataDialog::on_pushButtonReset_clicked()
{
    m_filtered_model->ClearAllUserSearches();
    m_filtered_model->SaveUserFilter(m_filtered_model->objectName());
    m_filtered_model->DeactivateUserFilter(m_filtered_model->objectName());
    m_column_model->Refresh();

    setupFilters();
}

void FilterDataDialog::setupFilters()
{
    m_saved_filters.clear();

    // create storage for filters and load current ones
    int colcount = m_filtered_model->columnCount();
    int visiblecolumn = 0;
    for (int i = 0; i < colcount; i++)
    {
        if ( m_filtered_model->isSearchable(i) )
        {
            FilterSaveStructure fs = FilterSaveStructure();
            QString colname = m_filtered_model->getColumnName(i);

            m_filtered_model->GetUserSearchRange(i, fs.t_search_begin_value, fs.t_search_end_value);
            fs.SearchString = m_filtered_model->GetUserSearchString(i);

            fs.ColumnValues = m_filtered_model->GetUserFilter(i);

            m_saved_filters[colname] = fs;

            if (m_selected_column == -1)
                m_selected_column = visiblecolumn;

            visiblecolumn++;
        }
    }

    if (m_selected_column == -1)
        m_selected_column = 0;

    // trigger the column name was selected
    ui->t_tableViewColumnName->selectRow(m_selected_column);
    QModelIndex qi = m_column_model->index(m_selected_column, 0);
    ui->t_tableViewColumnName->dataRowSelected(qi);
}
