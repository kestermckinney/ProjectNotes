#include "projectnotespage.h"
#include "pndatabaseobjects.h"

#include "ui_mainwindow.h"

ProjectNotesPage::ProjectNotesPage()
{
    QString page_title = "Project Notes";
    setPageTitle(page_title);
}

ProjectNotesPage::~ProjectNotesPage()
{
    if (ui)
        disconnect(ui->tabWidgetNotes, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidgetNotes_currentChanged(int)));

    if (m_mapperProjectNotes != nullptr)
        delete m_mapperProjectNotes;

    if (m_project_notes_delegate)
        delete m_project_notes_delegate;
}

void ProjectNotesPage::newRecord()
{
    QVariant note_id = global_DBObjects.projecteditingnotesmodelproxy()->data(global_DBObjects.projecteditingnotesmodelproxy()->index(0,0));

    int lastrow = ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->rowCount(QModelIndex());

    ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->newRecord(&note_id);

    getCurrentView()->selectRow(lastrow);
    QModelIndex index = getCurrentView()->model()->index(lastrow, 0);
    getCurrentView()->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void ProjectNotesPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    connect(ui->tabWidgetNotes, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidgetNotes_currentChanged(int)));

    ui->dateEditMeetingDate->setNullable(true);

    if (m_mapperProjectNotes == nullptr)
        m_mapperProjectNotes = new QDataWidgetMapper(this);

    if (m_project_notes_delegate == nullptr)
        m_project_notes_delegate = new ProjectNotesDelegate(this);

    m_mapperProjectNotes->setItemDelegate(m_project_notes_delegate);

    m_mapperProjectNotes->setModel(global_DBObjects.projecteditingnotesmodelproxy());
    m_mapperProjectNotes->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

    m_mapperProjectNotes->addMapping(ui->lineEditMeetingTitle, 2);
    m_mapperProjectNotes->addMapping(ui->textEditNotes, 4);
    m_mapperProjectNotes->addMapping(ui->dateEditMeetingDate, 3);

    ui->tableViewAtendees->setModel(global_DBObjects.meetingattendeesmodelproxy());

    setCurrentModel(nullptr);
    setCurrentView(nullptr);

    //ui->tableViewTeam->setModel(global_DBObjects.projectteammembersmodelproxy());
    //ui->tableViewTrackerItems->setModel(global_DBObjects.projectactionitemsmodelproxy());   
    //ui->tableViewLocations->setModel(global_DBObjects.projectlocationsmodelproxy());
    //ui->tableViewProjectNotes->setModel(global_DBObjects.projecteditingnotesmodelproxy());
}

void ProjectNotesPage::toFirst()
{
    if (m_mapperProjectNotes != nullptr)
        m_mapperProjectNotes->toFirst();
}

void ProjectNotesPage::on_tabWidgetNotes_currentChanged(int index)
{
    // TODO: this goes out of alignment when a tab isn't clicked when first opened
    // maybe the open should select the tab too

    switch ( index )
    {
    case 0:
        setCurrentModel(nullptr);
        setCurrentView(nullptr);
        break;
    case 1:
        setCurrentModel(global_DBObjects.meetingattendeesmodelproxy());
        setCurrentView(ui->tableViewAtendees);
        break;
    case 2:
        //setCurrentModel(global_DBObjects.projectactionitemsmodelproxy());
        //setCurrentView(ui->tableViewTrackerItems);
        break;
    }
}
