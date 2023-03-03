#ifndef IXTREE_H
#define IXTREE_H

#include <QString>
#include <QVector>
#include <QHash>
#include <QDomNode>
#include <QDomDocument>
#include <QWidget>
/*
#include "pndatabaseobjects.h"
#include "pnsqlquerymodel.h"
#include "projectsmodel.h"
//#include "projectslistmodel.h"
#include "clientsmodel.h"
#include "peoplemodel.h"
//#include "teamsmodel.h"
#include "statusreportitemsmodel.h"
#include "projectteammembersmodel.h"
#include "projectlocationsmodel.h"
#include "projectnotesmodel.h"
//#include "actionitemprojectnotesmodel.h"
//#include "actionitemsdetailsmeetingsmodel.h"
#include "meetingattendeesmodel.h"
//#include "notesactionitemsmodel.h"
//#include "itemdetailteamlistmodel.h"
#include "trackeritemcommentsmodel.h"
#include "trackeritemsmodel.h"
*/

class PNSqlQueryModel;
class ClientsModel;
class PeopleModel;
class ProjectsModel;
class ProjectNotesModel;
class ProjectTeamMembersModel;
class StatusReportItemsModel;
class ProjectLocationsModel;
class ProjectLocationsModel;
class MeetingAttendeesModel;
class TrackerItemCommentsModel;
class TrackerItemsModel;

class IXTreeNode
{
private:
    QString m_ViewName;
    IXTreeNode* m_ChildNode = nullptr;
    IXTreeNode* m_NextNode = nullptr;

    PNSqlQueryModel* m_DataView = nullptr;

    QString m_ParentColumn;
    QString m_ChildColumn;

public:
    IXTreeNode(const QString& nodename, PNSqlQueryModel* dataview );
    ~IXTreeNode();

    IXTreeNode* addChild(const QString& name, PNSqlQueryModel* dataview );
    IXTreeNode* addNext(const QString& name, PNSqlQueryModel* dataview );

    IXTreeNode* getFirst() { return this; };
    IXTreeNode* getNext() { return m_NextNode; };
    IXTreeNode* getChild() { return m_ChildNode; };
    QString& getViewName() { return m_ViewName; };
    PNSqlQueryModel* findView(const QString& t_name);
    PNSqlQueryModel* getDataView() { return m_DataView; };

    void setForeignKey( const QString& parent, const QString& child) { m_ParentColumn = parent; m_ChildColumn = child; };
    QString& getFKParent() { return m_ParentColumn; };
    QString& getFKChild() { return m_ChildColumn; };
};


class IXTree
{

public:
    IXTree();
    //IXTree(DBObjects* dbobjects);
    ~IXTree();

    IXTreeNode* getNodes() { return m_RelationshipNodes; }

    //QDomNode findNode(QDomNode& t_xmlnode, const QString& t_nodename, const QString& t_attributename, const QString& t_attributevalue);
    //QString getNodeContent(QDomNode& t_xmlnode, const QString& t_nodename, const QString& t_attributename, const QString& t_attributevalue);
    //QString getNodeAttribute(QDomNode& t_xmlnode, const QString& t_nodename, const QString& t_attributename, const QString& t_attributevalue, const QString& t_getattribute);
    //QList<QDomNode> findAllNodes(QDomNode& t_xmlnode, const QString& t_nodename, const QString& t_attributename, const QString& t_attributevalue);

    //void nodeToXML( QDomElement& rootxmlnode, IXTreeNode* rootnode, const QString& viewname, const QString& fkfield, const QString& fkvalue );
    //QDomDocument* getXMLDocument(QString& viewname, QString& fkfield, const QVector<QString>& fkvalues );
    //void importXMLDocument(QWidget* t_parentwindow, QDomNode& t_xmlnode);
    //bool doRecordIdsExist(QDomNode& xmlnode);

    //int getNodeCount(QDomNode& t_xmlnode);

    //void setIgnoreRecordIds(bool ignore) { m_IgnoreRecordIds = ignore; };

private:
    //DBObjects* m_DBObjects = NULL;

    // track all relationships
    IXTreeNode* m_RelationshipNodes = nullptr;
    /*
    QHash<QString,QString> m_CorrespondingRefresh;

    static int m_NodeTotal;
    static int m_CurrentNode;
    static QDomElement& m_TopNode;

    bool m_IgnoreRecordIds = false;
    */
    ClientsModel* ix_clients;
    PeopleModel* ix_clients_people;

    // build out the people data view
    PeopleModel* ix_people;

    // main project list on the left side
    ProjectsModel* ix_projects;
    TrackerItemsModel* ix_projects_itemtracker;
    TrackerItemCommentsModel* ix_projects_itemtracker_itemtrackerupdates;

    // team members drop down for the currently selected project
    ProjectTeamMembersModel * ix_project_people;

    // status report items for the currently selected project
    StatusReportItemsModel* ix_statusreportitems;

    // editable project locations for the currently selected project
    ProjectLocationsModel* ix_projectlocations;

    // setup project notes dataview
    ProjectNotesModel* ix_projectnotes;

    // setup meetingattendees dataview
    MeetingAttendeesModel* ix_meetingattendees;

     // setup project action items dataview
    TrackerItemCommentsModel* ix_itemtrackerupdates;

     // setup project action items dataview
    TrackerItemsModel* ix_itemtracker;
};

// PUSH childnode, XML Row, ChildNode Column, and Child Node Search Value to Node Stack to prevent requering a DataView being traversed
// PUSH XML Column, CurrentView, and column Lookup Value to Lookup Stack to prevent requering a DataView being traversed
/*
class LookupItem
{
public:
    QDomNode* m_XmlColumn = NULL;
    PNSqlQueryModel* m_DBView = NULL;
    QString m_LookupColumn;
    QString m_LookupValue;
};

class XMLNodeItem
{
public:
    QDomElement* m_XmlRow;
    IXTreeNode* m_ChildNode = NULL;
    QString m_ChildColumn;
    QString m_SearchValue;
};
*/

#endif // IXTREE_H
