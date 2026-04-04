// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectteammembersview.h"
#include "databaseobjects.h"
#include "vcardparser.h"

ProjectTeamMembersView::ProjectTeamMembersView(QWidget* parent) : TableView(parent)
{
    setHasOpen(true);
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
}

ProjectTeamMembersView::~ProjectTeamMembersView()
{
    if (m_unfilteredPeopleDelegate) delete m_unfilteredPeopleDelegate;
    if (m_receiveStatusDelegate) delete m_receiveStatusDelegate;
    if (m_roleDelegate) delete m_roleDelegate;
}

void ProjectTeamMembersView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(3, true);
        setColumnHidden(6, true);
        setColumnHidden(7, true);
        setColumnHidden(8, true);
        setColumnHidden(9, true);
        // setup model lists

        DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(dynamic_cast<SortFilterProxyModel*>(model)->sourceModel())->getDBOs();

        // projects list panel delagets
        m_unfilteredPeopleDelegate = new SqlComboBoxDelegate(this, dbo->unfilteredpeoplemodelproxy());
        m_receiveStatusDelegate = new CheckBoxDelegate(this);
        m_roleDelegate = new PlainTextEditDelegate(this);

        setItemDelegateForColumn(2, m_unfilteredPeopleDelegate);
        setItemDelegateForColumn(5, m_roleDelegate);
        setItemDelegateForColumn(4, m_receiveStatusDelegate);
    }
    else
    {
        TableView::setModel(model);
    }
}

void ProjectTeamMembersView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<ProjectTeamMembersModel*>(currentmodel)->getFilter(1); // get the project id

    dynamic_cast<ProjectTeamMembersModel*>(currentmodel)->newRecord(&fk_value1);
}

void ProjectTeamMembersView::dragEnterEvent(QDragEnterEvent *event)
{
    if (mimeDataHasVCard(event->mimeData()))
        event->acceptProposedAction();
    else
        event->ignore();
}

void ProjectTeamMembersView::dragMoveEvent(QDragMoveEvent *event)
{
    if (mimeDataHasVCard(event->mimeData()))
        event->acceptProposedAction();
    else
        event->ignore();
}

void ProjectTeamMembersView::dropEvent(QDropEvent *event)
{
    QString vcardText = extractVCardText(event->mimeData());
    if (vcardText.isEmpty()) { event->ignore(); return; }

    QList<VCardContact> contacts = parseVCards(vcardText);
    if (contacts.isEmpty()) { event->ignore(); return; }

    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    if (!sortmodel) { event->ignore(); return; }

    ProjectTeamMembersModel* currentmodel = dynamic_cast<ProjectTeamMembersModel*>(sortmodel->sourceModel());
    if (!currentmodel) { event->ignore(); return; }

    DatabaseObjects* dbo = currentmodel->getDBOs();
    QVariant projectId = currentmodel->getFilter(1);
    if (projectId.isNull() || projectId.toString().isEmpty()) { event->ignore(); return; }

    for (const VCardContact& contact : contacts)
    {
        QString clientId = findOrCreateClient(dbo, contact.company);
        QString personId = findOrCreatePerson(dbo, contact, clientId);

        if (personId.isEmpty())
            continue;

        // Check if person is already on this project
        QString escapedProject = projectId.toString();
        escapedProject.replace("'", "''");
        QString escapedPerson = personId;
        escapedPerson.replace("'", "''");

        QString existing = dbo->execute(
            QString("SELECT id FROM project_people WHERE project_id = '%1' AND people_id = '%2' AND deleted = 0")
            .arg(escapedProject, escapedPerson));

        if (!existing.isEmpty())
            continue;

        QVector<QVariant> qr = currentmodel->emptyrecord();
        qr[1] = projectId;
        qr[2] = personId;
        QModelIndex newIdx = currentmodel->addRecord(qr);
        if (newIdx.isValid())
            currentmodel->insertCacheRow(newIdx.row());
    }

    event->acceptProposedAction();
}
