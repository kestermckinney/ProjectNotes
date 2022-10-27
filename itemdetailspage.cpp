
#include "ui_mainwindow.h"

#include "itemdetailspage.h"
#include "pndatabaseobjects.h"

ItemDetailsPage::ItemDetailsPage()
{
    QString page_title = "Item Details";
    setPageTitle(page_title);
}

ItemDetailsPage::~ItemDetailsPage()
{
    if (ui)
        connect(global_DBObjects.actionitemsdetailsmodel(), SIGNAL(dataChanged( const QModelIndex &, const QModelIndex & )), this, SLOT(toFirst( const QModelIndex &, const QModelIndex & )));

    if (m_mapperItemDetails != nullptr)
        delete m_mapperItemDetails;

    if (m_item_details_delegate)
        delete m_item_details_delegate;
}

void ItemDetailsPage::newRecord()
{
    QVariant tracker_id = global_DBObjects.actionitemsdetailsmodelproxy()->data(global_DBObjects.actionitemsdetailsmodelproxy()->index(0,0));

    int lastrow = ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->rowCount(QModelIndex());

    ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->newRecord(&tracker_id);

    getCurrentView()->selectRow(lastrow);
    QModelIndex index = getCurrentView()->model()->index(lastrow, 0);
    getCurrentView()->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void ItemDetailsPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    connect(global_DBObjects.actionitemsdetailsmodel(), SIGNAL(dataChanged( const QModelIndex &, const QModelIndex & )), this, SLOT(toFirst( const QModelIndex &, const QModelIndex & )));

    ui->dateEditDueDate->setNullable(true);
    ui->dateEditDateResolved->setNullable(true);
    ui->dateEditLastUpdated->setNullable(true);

    if (m_mapperItemDetails == nullptr)
        m_mapperItemDetails = new QDataWidgetMapper(this);

    if (m_item_details_delegate == nullptr)
        m_item_details_delegate = new ItemDetailsDelegate(this);

    m_mapperItemDetails->setItemDelegate(m_item_details_delegate);

    m_mapperItemDetails->setModel(global_DBObjects.actionitemsdetailsmodelproxy());
    m_mapperItemDetails->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

    m_mapperItemDetails->addMapping(ui->lineEditItemNumber, 1);
    m_mapperItemDetails->addMapping(ui->comboBoxType, 2);
    m_mapperItemDetails->addMapping(ui->lineEditName, 3);
    m_mapperItemDetails->addMapping(ui->comboBoxProject, 14);
    m_mapperItemDetails->addMapping(ui->comboBoxMeeting, 13);
    m_mapperItemDetails->addMapping(ui->comboBoxAssignedTo, 7);
    m_mapperItemDetails->addMapping(ui->comboBoxIdentifiedBy, 4);
    m_mapperItemDetails->addMapping(ui->dateEditIdentifiedDate, 5);
    m_mapperItemDetails->addMapping(ui->comboBoxPriority, 8);
    m_mapperItemDetails->addMapping(ui->comboBoxStatus, 9);
    m_mapperItemDetails->addMapping(ui->dateEditDueDate, 10);
    m_mapperItemDetails->addMapping(ui->dateEditDateResolved, 12);
    m_mapperItemDetails->addMapping(ui->dateEditLastUpdated, 11);
    m_mapperItemDetails->addMapping(ui->checkBoxInternal, 15);


    ui->comboBoxType->clear();
    ui->comboBoxType->addItems(PNDatabaseObjects::item_type);

    ui->comboBoxStatus->clear();
    ui->comboBoxStatus->addItems(PNDatabaseObjects::item_status);

    ui->comboBoxPriority->clear();
    ui->comboBoxPriority->addItems(PNDatabaseObjects::item_priority);

    ui->comboBoxProject->setModel(global_DBObjects.projectslistmodel());
    ui->comboBoxProject->setModelColumn(1);
    ui->comboBoxProject->setEditable(true);

    ui->comboBoxMeeting->setModel(global_DBObjects.actionitemsdetailsmeetingsmodel());
    ui->comboBoxMeeting->setModelColumn(2);
    ui->comboBoxMeeting->setEditable(true);

    ui->comboBoxAssignedTo->setModel(global_DBObjects.teamsmodel());
    ui->comboBoxAssignedTo->setModelColumn(1);
    ui->comboBoxAssignedTo->setEditable(true);

    ui->comboBoxIdentifiedBy->setModel(global_DBObjects.teamsmodel());
    ui->comboBoxIdentifiedBy->setModelColumn(1);
    ui->comboBoxIdentifiedBy->setEditable(true);


    ui->tableViewComments->setModel(global_DBObjects.trackeritemscommentsmodel());
    ui->tableViewComments->setWordWrap(true);
    ui->tableViewComments->resizeRowsToContents();

    setCurrentModel(global_DBObjects.trackeritemscommentsmodelproxy());
    setCurrentView( ui->tableViewComments );
}

void ItemDetailsPage::toFirst()
{
    if (m_mapperItemDetails != nullptr)
        m_mapperItemDetails->toFirst();
}
