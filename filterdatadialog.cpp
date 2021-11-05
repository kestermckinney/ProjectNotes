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
    ui->tableViewFilterValues->setModel(valuesProxyModel);
    QString storename = objectName();
    global_Settings.getWindowState(storename, *this);

    m_SelectedColumn = -1;
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

void FilterDataDialog::setFilterModel(PNSqlQueryModel* model)
{
    m_FilterName = model->objectName();

    columnModel->setColumnModel(model);
    columnModel->Refresh();

    m_SavedFilters.clear();


    // create storage for filters and load current ones
    int colcount = model->columnCount();
    for (int i = 0; i < colcount; i++)
    {
        if ( columnModel->isSearchable(i) )
        {
            FilterSaveStructure fs = FilterSaveStructure();
            QString colname = columnModel->getColumnName(i);

            model->GetUserSearchRange(i, fs.m_SearchBeginValue, fs.m_SearchEndValue);
            fs.m_SearchString = columnModel->GetUserSearchString(i);

            fs.m_ColumnValues = columnModel->GetUserFilter(i);

            m_SavedFilters[colname] = fs;

            if (m_SelectedColumn == -1)
                m_SelectedColumn = i;
        }
    }

    if (m_SelectedColumn == -1)
        m_SelectedColumn = 0;

    ui->tableViewColumnName->selectRow(m_SelectedColumn);
    QModelIndex qi = columnModel->index(m_SelectedColumn,0);

    valuesModel->setValuesModel(model, columnModel->data(qi).toString());
}
