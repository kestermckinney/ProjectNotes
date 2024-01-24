// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "searchresultsview.h"
#include "pndatabaseobjects.h"

SearchResultsView::SearchResultsView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewSearchResults");
    setHasOpen(true);
}

SearchResultsView::~SearchResultsView()
{
    if (m_text_edit_delegate) delete m_text_edit_delegate;
}

void SearchResultsView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(4, true);
        setColumnHidden(5, true);
        setColumnHidden(6, true);
        setColumnHidden(13, true);
        setColumnHidden(14, true);

        // search view delagets
        m_text_edit_delegate = new PNTextEditDelegate(this);

        setItemDelegateForColumn(3, m_text_edit_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}
/*
void SearchResultsView::slotOpenRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());

    QModelIndexList qil = this->selectionModel()->selectedRows();
    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);

    QVariant data_type = model()->data(index(qq.row(), 1));
    QVariant record_id = model()->data(index(t_index.row(), 0));

    if (data_type == tr("Client"))
    {
        global_DBObjects.clientsmodel()->deactivateUserFilter(global_DBObjects.clientsmodel()->objectName());

        emit ClientsListView::

        STOPPED HERE HOW DO YOU CALL navigateTo functions??  The list pages need to hightlight rows when called from navigateTo

        //emit signalOpenRecordWindow(record_id)
    }
    else if (data_type == tr("People"))
    {
        global_DBObjects.peoplemodel()->deactivateUserFilter(global_DBObjects.peoplemodel()->objectName());
    }
    else if (data_type == tr("Project"))
    {


        // only select the records another event will be fired to open the window to show them
        global_DBObjects.projectinformationmodel()->setFilter(0, record_id.toString());
        global_DBObjects.projectinformationmodel()->refresh();

        // filter team members by project
        global_DBObjects.projectteammembersmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectteammembersmodel()->refresh();

        // filter project status items
        global_DBObjects.statusreportitemsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.statusreportitemsmodel()->refresh();

        // filter team members by project for members list
        global_DBObjects.teamsmodel()->setFilter(2, record_id.toString());
        global_DBObjects.teamsmodel()->refresh();

        // filter tracker items by project
        global_DBObjects.trackeritemsmodel()->setFilter(14, record_id.toString());
        global_DBObjects.trackeritemsmodel()->refresh();

        // filter tracker items by project
        global_DBObjects.trackeritemsmodel()->setFilter(14, record_id.toString());
        global_DBObjects.trackeritemsmodel()->refresh();

        global_DBObjects.trackeritemsmeetingsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.trackeritemsmeetingsmodel()->refresh();

        global_DBObjects.projectlocationsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectlocationsmodel()->refresh();

        global_DBObjects.projectnotesmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectnotesmodel()->refresh();
    }
    else if (data_type == tr("Project Notes"))
    {
        QVariant note_id = model()->data(index(t_index.row(), 0));
        QVariant project_id = model()->data(index(t_index.row(), 13));

        global_DBObjects.projecteditingnotesmodel()->setFilter(0, note_id.toString());
        global_DBObjects.projecteditingnotesmodel()->refresh();

        // only select the records another event will be fired to open the window to show them
        global_DBObjects.meetingattendeesmodel()->setFilter(1, note_id.toString());
        global_DBObjects.meetingattendeesmodel()->refresh();

        global_DBObjects.notesactionitemsmodel()->setFilter(13, note_id.toString());
        global_DBObjects.notesactionitemsmodel()->refresh();

        // filter team members by project for members list
        global_DBObjects.teamsmodel()->setFilter(2, project_id.toString());
        global_DBObjects.teamsmodel()->refresh();

    }
    else if (data_type == tr("Meeting Attendees"))
    {
        QVariant note_id = model()->data(index(t_index.row(), 13));

        QVariant project_number = model()->data(index(t_index.row(), 7));
        QVariant project_id = global_DBObjects.execute(QString("select project_id from projects where project_number='%1'").arg(project_number.toString()));

        global_DBObjects.projecteditingnotesmodel()->setFilter(0, note_id.toString());
        global_DBObjects.projecteditingnotesmodel()->refresh();

        // only select the records another event will be fired to open the window to show them
        global_DBObjects.meetingattendeesmodel()->setFilter(1, note_id.toString());
        global_DBObjects.meetingattendeesmodel()->refresh();

        global_DBObjects.notesactionitemsmodel()->setFilter(13, note_id.toString());
        global_DBObjects.notesactionitemsmodel()->refresh();

        // filter team members by project for members list
        global_DBObjects.teamsmodel()->setFilter(2, project_id.toString());
        global_DBObjects.teamsmodel()->refresh();
    }
    else if (data_type == tr("Project Locations") || data_type == tr("Project Team") || data_type == tr("Status Report Item") )
    {
        QVariant record_id = model()->data(index(t_index.row(), 13));

        // only select the records another event will be fired to open the window to show them
        global_DBObjects.projectinformationmodel()->setFilter(0, record_id.toString());
        global_DBObjects.projectinformationmodel()->refresh();

        // filter team members by project
        global_DBObjects.projectteammembersmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectteammembersmodel()->refresh();

        // filter project status items
        global_DBObjects.statusreportitemsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.statusreportitemsmodel()->refresh();

        // filter team members by project for members list
        global_DBObjects.teamsmodel()->setFilter(2, record_id.toString());
        global_DBObjects.teamsmodel()->refresh();

        // filter tracker items by project
        global_DBObjects.trackeritemsmodel()->setFilter(14, record_id.toString());
        global_DBObjects.trackeritemsmodel()->refresh();

        // filter tracker items by project
        global_DBObjects.trackeritemsmodel()->setFilter(14, record_id.toString());
        global_DBObjects.trackeritemsmodel()->refresh();

        global_DBObjects.trackeritemsmeetingsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.trackeritemsmeetingsmodel()->refresh();

        global_DBObjects.projectlocationsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectlocationsmodel()->refresh();

        global_DBObjects.projectnotesmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectnotesmodel()->refresh();
    }
    else if ( data_type == tr("Item Tracker") )
    {
        QVariant record_id = model()->data(index(t_index.row(), 0));
        QVariant project_id = model()->data(index(t_index.row(), 13));

        // only select the records another event will be fired to open the window to show them
        global_DBObjects.actionitemsdetailsmodel()->setFilter(0, record_id.toString());
        global_DBObjects.actionitemsdetailsmodel()->refresh();

        global_DBObjects.actionitemsdetailsmeetingsmodel()->setFilter(1, project_id.toString());
        global_DBObjects.actionitemsdetailsmeetingsmodel()->refresh();

        global_DBObjects.trackeritemscommentsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.trackeritemscommentsmodel()->refresh();

        // filter team members by project for members list
        global_DBObjects.teamsmodel()->setFilter(2, project_id.toString());
        global_DBObjects.teamsmodel()->refresh();
    }
    else if ( data_type == tr("Tracker Update") )
    {
        QVariant record_id = model()->data(index(t_index.row(), 14));
        QVariant project_id = model()->data(index(t_index.row(), 13));
        //QVariant tracker_id = data(index(t_index.row(), 0));

        // only select the records another event will be fired to open the window to show them
        global_DBObjects.actionitemsdetailsmodel()->setFilter(0, record_id.toString());
        global_DBObjects.actionitemsdetailsmodel()->refresh();

        global_DBObjects.actionitemsdetailsmeetingsmodel()->setFilter(1, project_id.toString());
        global_DBObjects.actionitemsdetailsmeetingsmodel()->refresh();

        global_DBObjects.trackeritemscommentsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.trackeritemscommentsmodel()->refresh();

        // filter team members by project for members list
        global_DBObjects.teamsmodel()->setFilter(2, project_id.toString());
        global_DBObjects.teamsmodel()->refresh();
    }

    return true;
}

*/
