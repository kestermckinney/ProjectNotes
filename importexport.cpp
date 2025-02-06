#include "importexport.h"
#include "pndatabaseobjects.h"

IXTreeNode::IXTreeNode( const QString& t_nodename, PNSqlQueryModel* t_dataview )
{
    m_ViewName = t_nodename;
    m_DataView = t_dataview;

    m_NextNode = nullptr;
    m_ChildNode = nullptr;
}

IXTreeNode::~IXTreeNode()
{
    if (m_NextNode)
        delete m_NextNode;

    if (m_ChildNode)
        delete m_ChildNode;
}

IXTreeNode* IXTreeNode::addChild(const QString& t_name, PNSqlQueryModel* t_dataview )
{
    m_ChildNode = new IXTreeNode( t_name, t_dataview  );

    return m_ChildNode;
}

IXTreeNode* IXTreeNode::addNext(const QString& t_name, PNSqlQueryModel* t_dataview )
{
    m_NextNode = new IXTreeNode( t_name, t_dataview );

    return m_NextNode;
}

PNSqlQueryModel* IXTreeNode::findView(const QString& t_name)
{
    IXTreeNode* currentnode = getFirst();

    while (currentnode)
    {
        if (currentnode->getViewName() == t_name)
            return currentnode->getDataView();

        PNSqlQueryModel* childnode = currentnode->findView(t_name);
        if (childnode)
            return childnode;

        currentnode = currentnode->getNext();
    }

    return nullptr;
}

IXTree::IXTree()
{
//    m_DBObjects = dbobjects;
    IXTreeNode* level1;
    IXTreeNode* level2;
    IXTreeNode* level3;
    IXTreeNode* level4;

    // setup data views
    // build out the clients data view
    ix_clients = new ClientsModel(nullptr);
    ix_clients_people = new PeopleModel(nullptr);

    // build out the people data view
    ix_people = new PeopleModel(nullptr);

    // main project list on the left side
    ix_projects = new ProjectsModel(nullptr);
    ix_projects_itemtracker = new TrackerItemsModel(nullptr);
    ix_projects_itemtracker_itemtrackerupdates = new TrackerItemCommentsModel(nullptr);
    ix_projects_projectnotes = new ProjectNotesModel(nullptr);

    // team members drop down for the currently selected project
    ix_project_people = new ProjectTeamMembersModel(nullptr);

    // status report items for the currently selected project
    ix_statusreportitems = new StatusReportItemsModel(nullptr);

    // editable project locations for the currently selected project
    ix_projectlocations = new ProjectLocationsModel(nullptr);

    // setup project notes dataview
    ix_projectnotes = new ProjectNotesModel(nullptr);

    // setup meetingattendees dataview
    ix_meetingattendees = new MeetingAttendeesModel(nullptr);

     // setup project action items dataview
    ix_itemtrackerupdates = new TrackerItemCommentsModel(nullptr);

     // setup project action items dataview
    ix_itemtracker = new TrackerItemsModel(nullptr);

    // setup all the possible export/import structures
    // start tree at client level
    m_RelationshipNodes = new IXTreeNode( QString("clients"), (PNSqlQueryModel*)ix_clients );
        level2 = m_RelationshipNodes->addChild(QString("clients/people"), (PNSqlQueryModel*)ix_clients_people);
        level2->setForeignKey(QString("client_id"), QString("client_id"));

    // start tree a people level
    level1 = m_RelationshipNodes->addNext( QString("people"), (PNSqlQueryModel*)ix_people );

    // start tree at the project level
    level1 = level1->addNext(QString("projects"), (PNSqlQueryModel*)ix_projects);
        level2 = level1->addChild(QString("projects/item_tracker"), (PNSqlQueryModel*)ix_projects_itemtracker);
        level2->setForeignKey(QString("project_id"), QString("project_id"));

            level3 = level2->addChild(QString("projects/item_tracker/item_tracker_updates"), (PNSqlQueryModel*)ix_projects_itemtracker_itemtrackerupdates);
            level3->setForeignKey(QString("item_id"), QString("item_id"));

        level2 = level2->addNext(QString("ix_project_notes"), (PNSqlQueryModel*)ix_projects_projectnotes);
        level2->setForeignKey(QString("project_id"), QString("project_id"));

            level3 = level2->addChild(QString("ix_meeting_attendees"), (PNSqlQueryModel*)ix_meetingattendees);
            level3->setForeignKey(QString("note_id"), QString("note_id"));

            level3 = level3->addNext(QString("ix_item_tracker"), (PNSqlQueryModel*)ix_itemtracker);
            level3->setForeignKey(QString("note_id"), QString("note_id"));

                level4 = level3->addChild(QString("ix_item_tracker_updates"), (PNSqlQueryModel*)ix_itemtrackerupdates);
                level4->setForeignKey(QString("item_id"), QString("item_id"));

        level2 = level2->addNext(QString("ix_project_locations"), (PNSqlQueryModel*)ix_projectlocations);
        level2->setForeignKey(QString("project_id"), QString("project_id"));

        level2 = level2->addNext(QString("ix_project_people"), (PNSqlQueryModel*)ix_project_people);
        level2->setForeignKey(QString("project_id"), QString("project_id"));

        level2 = level2->addNext(QString("ix_status_report_items"), (PNSqlQueryModel*)ix_statusreportitems);
        level2->setForeignKey(QString("project_id"), QString("project_id"));

    // start tree at the item tracker level
    level1 = level1->addNext(QString("ix_item_tracker"), (PNSqlQueryModel*)ix_itemtracker);

        level2 = level1->addChild(QString("ix_item_tracker_updates"), (PNSqlQueryModel*)ix_itemtrackerupdates);
        level2->setForeignKey(QString("item_id"), QString("item_id"));

    // start tree at the project notes level
    level1 = level1->addNext(QString("ix_project_notes"), (PNSqlQueryModel*)ix_projectnotes);

        level2 = level1->addChild(QString("ix_meeting_attendees"), (PNSqlQueryModel*)ix_meetingattendees);
        level2->setForeignKey(QString("note_id"), QString("note_id"));

        level2 = level2->addNext(QString("ix_item_tracker"), (PNSqlQueryModel*)ix_itemtracker);
        level2->setForeignKey(QString("note_id"), QString("note_id"));

            level3 = level2->addChild(QString("ix_item_tracker_updates"), (PNSqlQueryModel*)ix_itemtrackerupdates);
            level3->setForeignKey(QString("item_id"), QString("item_id"));

    // start tree at the meeting attendees level
    level1 = level1->addNext(QString("ix_meeting_attendees"), (PNSqlQueryModel*)ix_meetingattendees);

    // start tree at project project locations
    level1 = level1->addNext(QString("ix_project_locations"), (PNSqlQueryModel*)ix_projectlocations);

    // start tree at project level
    level1 = level1->addNext(QString("ix_project_people"), (PNSqlQueryModel*)ix_project_people);

    // start tree at status report level
    level1 = level1->addNext(QString("ix_status_report_items"), (PNSqlQueryModel*)ix_statusreportitems);
}

IXTree::~IXTree()
{
    if (m_RelationshipNodes)
        delete m_RelationshipNodes;

    delete ix_clients;
    delete ix_people;
    delete ix_projects;
    delete ix_itemtracker;
    delete ix_itemtrackerupdates;
    delete ix_projectnotes;
    delete ix_meetingattendees;
    delete ix_projectlocations;
    delete ix_project_people;
    delete ix_statusreportitems;
}

