#include "projectnotespage.h"
#include "pndatabaseobjects.h"
#include "notesactionitemsview.h"

#include "ui_mainwindow.h"

ProjectNotesPage::ProjectNotesPage()
{
    QString page_title = "Project Notes";
    setPageTitle(page_title);
}

ProjectNotesPage::~ProjectNotesPage()
{
   if (m_mapperProjectNotes != nullptr)
       delete m_mapperProjectNotes;

    if (m_project_notes_delegate)
        delete m_project_notes_delegate;
}

void ProjectNotesPage::newRecord()
{
    QVariant note_id = global_DBObjects.projecteditingnotesmodelproxy()->data(global_DBObjects.projecteditingnotesmodelproxy()->index(0,0));
    QVariant project_id = global_DBObjects.projecteditingnotesmodelproxy()->data(global_DBObjects.projecteditingnotesmodelproxy()->index(0,1));

    int lastrow = ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->rowCount(QModelIndex());

    ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->newRecord(&note_id, &project_id);

    getCurrentView()->selectRow(lastrow);
    QModelIndex index = getCurrentView()->model()->index(lastrow, 0);
    getCurrentView()->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void ProjectNotesPage::setupModels( Ui::MainWindow *t_ui )
{
    ui = t_ui;

    if (t_ui)
    {
        connect(ui->tabWidgetNotes, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidgetNotes_currentChanged(int)));
        connect(global_DBObjects.projecteditingnotesmodel(), SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(toFirst()));
    }
    else
    {
        disconnect(ui->tabWidgetNotes, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidgetNotes_currentChanged(int)));
        disconnect(global_DBObjects.projecteditingnotesmodel(), SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(toFirst()));
        return; // closing the application
    }

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
    ui->tableViewActionItems->setModel(global_DBObjects.notesactionitemsmodelproxy());

    setCurrentModel(nullptr);
    setCurrentView(nullptr);
}

void ProjectNotesPage::toFirst()
{
    if (m_mapperProjectNotes != nullptr)
        m_mapperProjectNotes->toFirst();

     ui->tabWidgetNotes->setCurrentIndex(0);  // always set to the first tab on open
}

void ProjectNotesPage::on_tabWidgetNotes_currentChanged(int index)
{
    emit setFocus(); // tell the main window to update to call the setButtonsAndMenus function

    switch ( index )
    {
    case 0:
        setCurrentModel(nullptr);
        setCurrentView(nullptr);
        this->setFocus();
        break;
    case 1:
        setCurrentModel(global_DBObjects.meetingattendeesmodelproxy());
        setCurrentView(ui->tableViewAtendees);
        ui->tableViewAtendees->setFocus();
        break;
    case 2:
        setCurrentModel(global_DBObjects.notesactionitemsmodelproxy());
        setCurrentView(ui->tableViewActionItems);
        ui->tableViewActionItems->setFocus();
        break;
    }
}



