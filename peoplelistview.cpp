// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "peoplelistview.h"
#include "databaseobjects.h"
#include "vcardparser.h"

PeopleListView::PeopleListView(QWidget* parent) : TableView(parent)
{
    setObjectName("tableViewPeople");
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
}

PeopleListView::~PeopleListView()
{
    if (m_unfilteredClientsDelegate) delete m_unfilteredClientsDelegate;
    if (m_nameDelegate) delete m_nameDelegate;
    if (m_roleDelegate) delete m_roleDelegate;
}

void PeopleListView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);

        DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(dynamic_cast<SortFilterProxyModel*>(model)->sourceModel())->getDBOs();

        // setup model lists
        m_unfilteredClientsDelegate = new SqlComboBoxDelegate(this, dbo->unfilteredclientsmodelproxy());
        m_nameDelegate = new PlainTextEditDelegate(this);
        m_roleDelegate = new PlainTextEditDelegate(this);

        setItemDelegateForColumn(5, m_unfilteredClientsDelegate);
        setItemDelegateForColumn(1, m_nameDelegate);
        setItemDelegateForColumn(6, m_roleDelegate);
    }
    else
    {
        TableView::setModel(model);
    }
}

void PeopleListView::dragEnterEvent(QDragEnterEvent *event)
{
    if (mimeDataHasVCard(event->mimeData()))
        event->acceptProposedAction();
    else
        event->ignore();
}

void PeopleListView::dragMoveEvent(QDragMoveEvent *event)
{
    if (mimeDataHasVCard(event->mimeData()))
        event->acceptProposedAction();
    else
        event->ignore();
}

void PeopleListView::dropEvent(QDropEvent *event)
{
    QString vcardText = extractVCardText(event->mimeData());
    if (vcardText.isEmpty()) { event->ignore(); return; }

    QList<VCardContact> contacts = parseVCards(vcardText);
    if (contacts.isEmpty()) { event->ignore(); return; }

    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    if (!sortmodel) { event->ignore(); return; }

    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());
    if (!currentmodel) { event->ignore(); return; }

    DatabaseObjects* dbo = currentmodel->getDBOs();

    for (const VCardContact& contact : contacts)
    {
        QString clientId = findOrCreateClient(dbo, contact.company);
        findOrCreatePerson(dbo, contact, clientId);
    }

    event->acceptProposedAction();
}
