// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectnotespage.h"
#include "pndatabaseobjects.h"
#include "notesactionitemsview.h"
#include <QDebug>

#include "ui_mainwindow.h"

ProjectNotesPage::ProjectNotesPage()
{
    setTableName("project_notes");
}

ProjectNotesPage::~ProjectNotesPage()
{
   if (m_mapperProjectNotes != nullptr)
       delete m_mapperProjectNotes;

    if (m_project_notes_delegate)
        delete m_project_notes_delegate;
}

void ProjectNotesPage::openRecord(QVariant& t_record_id)
{
    setRecordId(t_record_id);

    PNSqlQueryModel::refreshDirty();

    global_DBObjects.projecteditingnotesmodel()->setFilter(0, t_record_id.toString());
    global_DBObjects.projecteditingnotesmodel()->refresh();

    QVariant project_id = global_DBObjects.projecteditingnotesmodel()->data(
                global_DBObjects.projecteditingnotesmodel()->index(0, 1)
                );

    // only select the records another event will be fired to open the window to show them
    global_DBObjects.meetingattendeesmodel()->setFilter(1, t_record_id.toString());
    global_DBObjects.meetingattendeesmodel()->refresh();

    // not needed global_DBObjects.notesactionitemsmodel()->setFilter(14, project_id.toString());
    global_DBObjects.notesactionitemsmodel()->setFilter(13, t_record_id.toString());
    global_DBObjects.notesactionitemsmodel()->refresh(); 

    if (m_mapperProjectNotes != nullptr)
        m_mapperProjectNotes->toFirst();

    loadState();
}

void ProjectNotesPage::setPageTitle()
{
    QString project_number= global_DBObjects.projecteditingnotesmodelproxy()->data(global_DBObjects.projecteditingnotesmodelproxy()->index(0,7)).toString();
    QString pagetitle = QString("%1 %2 %3").arg(project_number, ui->plainTextEditMeetingTitle->toPlainText(), ui->dateEditMeetingDate->text().left(25));

    topLevelWidget()->setWindowTitle(QString("Project Notes Meeting [%1]").arg(pagetitle));
    setHistoryText(pagetitle);
}

void ProjectNotesPage::newRecord()
{
    QVariant note_id = global_DBObjects.projecteditingnotesmodelproxy()->data(global_DBObjects.projecteditingnotesmodelproxy()->index(0,0));
    QVariant project_id = global_DBObjects.projecteditingnotesmodelproxy()->data(global_DBObjects.projecteditingnotesmodelproxy()->index(0,1));

    int lastrow = dynamic_cast<PNSqlQueryModel*>(getCurrentModel()->sourceModel())->rowCount(QModelIndex());

    dynamic_cast<PNSqlQueryModel*>(getCurrentModel()->sourceModel())->newRecord(&note_id, &project_id);

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
    }
    else
    {
        disconnect(ui->tabWidgetNotes, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidgetNotes_currentChanged(int)));
        return; // closing the application
    }

    ui->dateEditMeetingDate->setNullable(true);

    qreal fontWidth = QFontMetrics(ui->textEditNotes->currentCharFormat().font()).averageCharWidth();
    ui->textEditNotes->setTabStopDistance( 3.0 * fontWidth );

    if (m_mapperProjectNotes == nullptr)
        m_mapperProjectNotes = new QDataWidgetMapper(this);

    if (m_project_notes_delegate == nullptr)
        m_project_notes_delegate = new ProjectNotesDelegate(this);

    m_mapperProjectNotes->setItemDelegate(m_project_notes_delegate);

    m_mapperProjectNotes->setModel(global_DBObjects.projecteditingnotesmodelproxy());
    setPageModel(global_DBObjects.projecteditingnotesmodel());
    m_mapperProjectNotes->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

    m_mapperProjectNotes->addMapping(ui->plainTextEditMeetingTitle, 2);
    m_mapperProjectNotes->addMapping(ui->textEditNotes, 4);
    m_mapperProjectNotes->addMapping(ui->dateEditMeetingDate, 3);
    m_mapperProjectNotes->addMapping(ui->checkBoxInternalNote, 5);

    ui->tableViewAtendees->setModel(global_DBObjects.meetingattendeesmodelproxy());
    ui->tableViewActionItems->setModel(global_DBObjects.notesactionitemsmodelproxy());

    setCurrentModel(nullptr);
    setCurrentView(nullptr);

    ui->textEditNotes->setFontPointSize(10);
}

void ProjectNotesPage::on_tabWidgetNotes_currentChanged(int index)
{
    //qDebug() << "Project Notes Page on current tab changed called";

    PNSqlQueryModel::refreshDirty();

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

void ProjectNotesPage::setButtonAndMenuStates()
{
    if (global_DBObjects.getShowInternalItems())
       ui->tableViewActionItems->setColumnHidden(15, false);
    else
       ui->tableViewActionItems->setColumnHidden(15, true);
}
