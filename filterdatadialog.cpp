#include "filterdatadialog.h"
#include "ui_filterdatadialog.h"
#include "pncolumnmodel.h"
#include "valueselectmodel.h"
#include "pnsettings.h"

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

    ui->tableViewColumnName->setUI(this);

    QString storename = objectName();
    global_Settings.getWindowState(storename, *this);

    m_selected_column = -1;
}

FilterDataDialog::~FilterDataDialog()
{
    QString storename = objectName();
    global_Settings.setWindowState(storename, *this);

    ui->tableViewFilterValues->setModel(nullptr);
    ui->tableViewColumnName->setModel(nullptr);

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

void FilterDataDialog::setSearchTextEnabled( bool t_enabled )
{
    ui->lineEditSearchText->setEnabled( t_enabled );

    if (!t_enabled)
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

void FilterDataDialog::setSourceModelView(PNSqlQueryModel *t_model, PNTableView *t_view)
{
    m_filtered_model = t_model;

    // set the names differently for each instance so the table view widths are saved differently for each view filter
    ui->tableViewColumnName->setObjectName("tableViewColumnName_" + t_view->objectName());
    ui->tableViewFilterValues->setObjectName("tableViewFilterValues_" + t_view->objectName());

    ui->tableViewColumnName->setSourceView( t_view );

    m_column_model->setColumnModel(t_model);
    m_column_model->setSavedFilters(&m_saved_filters);
    m_column_model->setFilteringModel(t_model);
    m_column_model->refresh();

    QModelIndex qi = m_column_model->index(m_selected_column, 0);

    m_values_model->setFilteringModel(t_model);
    m_values_model->setValuesColumn(m_column_model->data(qi).toString());
    m_values_model->refresh();

    ui->tableViewColumnName->setColumnValuesModel(m_values_model);
    ui->tableViewColumnName->setFilteredModel(t_model);
    ui->tableViewColumnName->setValuesView(ui->tableViewFilterValues, &m_saved_filters);
    ui->tableViewFilterValues->setSavedFilters(&m_saved_filters);

    ui->tableViewColumnName->setModel(m_column_proxy_model);
    ui->tableViewFilterValues->setModel(m_values_proxy_model);

    // sometimes the column width get zeroed out
    if (ui->tableViewColumnName->columnWidth(0) <= 0)
        ui->tableViewColumnName->resizeColumnToContents(0);

    m_values_model->refresh();

    setupFilters();

    ui->tableViewColumnName->selectRow(0);
    ui->tableViewColumnName->dataRowSelected(m_column_proxy_model->index(0,0));
}

void FilterDataDialog::on_lineEditSearchText_textEdited(const QString &t_arg1)
{
    QModelIndexList qil = ui->tableViewColumnName->selectionModel()->selectedRows();

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
    QModelIndexList qil = ui->tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = m_column_model->data(qi).toString();
        QString dbcolname = m_filtered_model->getColumnName(displaycolname);

        m_saved_filters[dbcolname].SearchBeginValue = t_arg1;
    }
}

void FilterDataDialog::on_lineEditEndValue_textEdited(const QString &t_arg1)
{
    QModelIndexList qil = ui->tableViewColumnName->selectionModel()->selectedRows();

    if (qil.begin() != qil.end())
    {
        QModelIndex qi = *(qil.begin());

        // translate the diplay name to the database field name
        QString displaycolname = m_column_model->data(qi).toString();
        QString dbcolname = m_filtered_model->getColumnName(displaycolname);

        m_saved_filters[dbcolname].SearchEndValue = t_arg1;
    }
}

void FilterDataDialog::on_pushButtonApply_clicked()
{
    for (auto it = m_saved_filters.begin(); it != m_saved_filters.end(); ++it)
    {
        QString ColumnName = it.key();
        int t_column_number = m_filtered_model->getColumnNumber(ColumnName);

        m_filtered_model->clearUserFilter(t_column_number);
        m_filtered_model->clearUserSearchRange(t_column_number);
        m_filtered_model->clearUserSearchString(t_column_number);

        // save the general search text
        if ( !it.value().SearchString.toString().isEmpty() )
            m_filtered_model->setUserSearchString( t_column_number, it.value().SearchString );

        // set the range based filter
        if ( !it.value().SearchBeginValue.toString().isEmpty() || !it.value().SearchEndValue.toString().isEmpty() )
            m_filtered_model->setUserSearchRange(t_column_number,it.value().SearchBeginValue, it.value().SearchEndValue);

        // capture all of the selected values to search for
        if ( it.value().ColumnValues.size() > 0 )
        {
            m_filtered_model->setUserFilter(t_column_number, it.value().ColumnValues);
        }
    }

    QString t_filter_name = m_filtered_model->objectName();
    m_filtered_model->saveUserFilter(t_filter_name);
    m_filtered_model->activateUserFilter(t_filter_name);

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
        QString displaycolname = m_column_model->data(*(qil.begin())).toString();
        QString dbcolname = m_filtered_model->getColumnName(displaycolname);

        // set all of the selected values
        ui->tableViewFilterValues->selectionModel()->clear();

        // set all of the search parameters
        m_saved_filters[dbcolname].SearchEndValue.clear();
        m_saved_filters[dbcolname].SearchBeginValue.clear();
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
    m_filtered_model->clearAllUserSearches();
    m_column_model->refresh();

    setupFilters();

    QVariant empty;
    setEndValue(empty);
    setBeginValue(empty);
    setSearchText(empty);

    ui->tableViewColumnName->selectRow(0);
    ui->tableViewColumnName->dataRowSelected(m_column_proxy_model->index(0,0));
    m_values_model->refresh();
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

            m_filtered_model->getUserSearchRange(i, fs.SearchBeginValue, fs.SearchEndValue);
            fs.SearchString = m_filtered_model->getUserSearchString(i);

            fs.ColumnValues = m_filtered_model->getUserFilter(i);

            m_saved_filters[colname] = fs;

            if (m_selected_column == -1)
                m_selected_column = visiblecolumn;

            visiblecolumn++;
        }
    }

    if (m_selected_column == -1)
        m_selected_column = 0;

    // trigger the column name was selected
    ui->tableViewColumnName->selectRow(m_selected_column);
}

