#include "importexport.h"
#include "pndatabaseobjects.h"

//int IXTree::m_CurrentNode = 0;
//int IXTree::m_NodeTotal = 0;
//TODO: QDomElement& IXTree::m_TopNode;

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
/*
QDomNode IXTree::findNode(QDomNode& t_xmlnode, const QString& t_nodename, const QString& t_attributename, const QString& t_attributevalue)
{
    QDomNode n = t_xmlnode.firstChild();

    while (!n.isNull())
    {

        if ( t_nodename == n.nodeName() && !n.attributes().isEmpty() )
        {
            QString attrval = n.attributes().namedItem(t_attributename).nodeValue();

            if (attrval == t_attributevalue)
                return n;
        }

        QDomNode childnode = findNode(n, t_nodename, t_attributename, t_attributevalue);

        if (!childnode.isNull())
            return childnode;

        n = n.nextSibling();
    }

    return QDomNode();
}

QString IXTree::getNodeContent(QDomNode& t_xmlnode, const QString& t_nodename, const QString& t_attributename, const QString& t_attributevalue)
{
    QDomNode n = findNode(t_xmlnode, t_nodename, t_attributename, t_attributevalue);

    if (!n.isNull())
    {
        return n.nodeValue();
    }

    return QString();
}

QString IXTree::getNodeAttribute(QDomNode& t_xmlnode, const QString& t_nodename, const QString& t_attributename, const QString& t_attributevalue, const QString& t_getattribute)
{
    QDomNode n = findNode(t_xmlnode, t_nodename, t_attributename, t_attributevalue);

    if (!n.isNull())
    {
        return n.attributes().namedItem(t_getattribute).nodeValue();
    }

    return QString();
}

QList<QDomNode> IXTree::findAllNodes(QDomNode& t_xmlnode, const QString& t_nodename, const QString& t_attributename, const QString& t_attributevalue)
{
    QDomNode n = t_xmlnode.firstChild();
    QList<QDomNode> dl;

    while (!n.isNull())
    {

        if (t_nodename == n.nodeName() && n.attributes().contains(t_attributename))
        {
            QString attrval = n.attributes().namedItem(t_attributename).nodeValue();

            if (attrval == t_attributevalue)
                dl.append(n);
        }

        dl.append(findAllNodes(n, t_nodename, t_attributename, t_attributevalue));

        n = n.nextSibling();
    }

    return dl;
}
*/
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
/*
QDomDocument* IXTree::getXMLDocument(QString& viewname, QString& fkfield, const QVector<QString>& fkvalues )
{
    m_NodeTotal = 1;
    m_CurrentNode = 0;

    QString msg = QString("Processing %1 of %2 nodes: %3%% Complete.").arg(m_CurrentNode, m_NodeTotal, (int)((float)m_CurrentNode / (float)m_NodeTotal * 100.0));

//TODO: set status message window    ((MainFrameClass*)wxGetApp().GetTopWindow())->SetStatusMessage(msg);

    QDomDocument* doc = new QDomDocument();
    QDomElement root = doc->createElement("projectnotes");
    m_TopNode = doc->appendChild(root).toElement();

    root.setAttribute("filepath", global_DBObjects.getDatabaseFile());
    root.setAttribute("export_date", QDateTime::currentDateTime().toString("MM/dd/yyyy h:m:s ap"));
    root.setAttribute("filter_field", fkfield);

    QString companyname = global_DBObjects.execute(QString("select client_name from clients where client_id='%1'").arg(global_DBObjects.getManagingCompany()));
    QString managername = global_DBObjects.execute(QString("select name from people where people_id='%1'").arg(global_DBObjects.getProjectManager()));

    root.setAttribute("project_manager_id", global_DBObjects.getProjectManager());
    root.setAttribute("managing_company_id", global_DBObjects.getManagingCompany());
    root.setAttribute("managing_company_name", companyname);
    root.setAttribute("managing_manager_name", managername);

    QString values;

    for ( int i = 0; i < fkvalues.count(); i++)
    {
        if (!values.isEmpty())
            values += ",";

        values += fkvalues[0];
    }

    root.setAttribute("filter_values", values);

    if ( fkvalues.count() >0 )
    {
        for ( int i = 0; i < fkvalues.count(); i++)
        {
            nodeToXML(root, m_RelationshipNodes, viewname, fkfield, fkvalues[i]);
        }
    }
    else
        nodeToXML(root, m_RelationshipNodes, viewname, fkfield, QString());

    // doc->SetRoot(root); should be done above

    //TODO: ((MainFrameClass*)wxGetApp().GetTopWindow())->SetStatusMessage(wxT("Export Complete."));

    //TODO: m_TopNode = nullptr;
    m_NodeTotal = 0;
    m_CurrentNode = 0;

    return doc;
}

int IXTree::getNodeCount(QDomNode& t_xmlnode)
{
    int c = 0;

    QDomNode n = t_xmlnode.firstChild();

    while (!n.isNull())
    {
        c++;

        c += getNodeCount(n);

        n = n.nextSibling();
    }

    return c;
}

bool IXTree::doRecordIdsExist(QDomNode& xmlnode)
{
    QDomNode child = xmlnode.firstChild();

    while (!child.isNull())
    {
        if (!child.toElement().attribute("id").isEmpty())
            return true;

        if (doRecordIdsExist(child))
        {
            return true;
        }

        child = child.nextSibling();
    }

    return false;
}

void IXTree::importXMLDocument(QWidget* t_parentwindow, QDomNode& t_xmlnode)
{
    if (m_NodeTotal == 0)
    {
        //TODO: m_TopNode = xmlnode;
        m_NodeTotal = getNodeCount(t_xmlnode);
        m_CurrentNode = 0;

        QString msg = QString("Processing %1 of %2 nodes: %3%% Complete.").arg( m_CurrentNode, m_NodeTotal, (int)((float)m_CurrentNode / (float)m_NodeTotal * 100.0));

        //TODO: ((MainFrameClass*)wxGetApp().GetTopWindow())->SetStatusMessage(msg);
        //TODO: Most of the time the percentage reported sits at 100%
    }

    QDomNode child = t_xmlnode.firstChild();

    while (!child.isNull())
    {
        m_CurrentNode++;
        QString msg = QString("Processing %1 of %2 nodes: %3%% Complete.").arg(m_CurrentNode, m_NodeTotal, (int)((float)m_CurrentNode / (float)m_NodeTotal * 100.0));

        //TODO:  ((MainFrameClass*)wxGetApp().GetTopWindow())->SetStatusMessage(msg);

        QString tag_name = child.nodeName();

        if (tag_name == "table")
        {
            PNSqlQueryModel* tableview = m_RelationshipNodes->findView(child.toElement().attribute("name"));

            // if the view doesn't exist move on
            if (tableview == nullptr)
            {
                child = child.nextSibling();
                continue;
            }

            //QString keycol = tableview->getColumnName(0);

            QDomNode rows = child.firstChild();

            while (!rows.isNull())
            {
                if (rows.nodeName() == "row")
                {
                    // if id exists it should be an update
                    QString id = rows.toElement().attribute("id");
                    //QSqlRecord sqlrec = tableview->emptyrecord();

                    //DBRow* dbrow = nullptr;
// stopped here
// probably don't want to call update every column it will slow down interface
// may not want to update displayed records until end of import
                    if (!id.isEmpty() && !m_IgnoreRecordIds)
                    {
                        tableview->clearAllFilters();
                        tableview->setFilter(0, id);
                        tableview->refresh();

                        //dbrow = tableview->GetRow(id);

                        if (tableview->rowCount(QModelIndex()) == 0)
                        {
                            QModelIndex qi = tableview->index(0, 0);
                            //dbrow = tableview->newRecord();
                            //dbrow->SetCol(0, id);  // use the provided row id
                            tableview->newRecord();
                        }

                        tableview->setData(qi, id, Qt::EditRole);
                    }
                    else if (tableview->getUniqueColumnCount() > 0)
                    {
                        for (int colnum = 0; colnum < tableview->columnCount(); colnum++)
                        {
                            if ( tableview->isUniqueColumn(colnum) )
                            {
                                QString lookupvalue = getNodeAttribute(rows, "column", "name", tableview->getColumnName(colnum), "lookupvalue");
                                QString content = getNodeContent(rows, "column", "name", tableview->getColumnName(colnum));

                                if (!lookupvalue.isEmpty())
                                {
                                    int fkcol = tableview->GetColNumber(tableview->GetUniqueColumns()[colnum]);

                                    PNSqlQueryModel* luv = tableview->GetColDef(fkcol)->m_LookupView;
                                    QString luc = tableview->GetColDef(fkcol)->m_LookupValue;

                                    // make sure the view was setup with a lookup value
                                    if (luv)
                                    {
                                        // only load the lookup value in the foreign key list
                                        luv->ClearAllFilters();
                                        luv->SetFilter(luc, lookupvalue);
                                        luv->RefreshData();

                                        if (luv->GetRows().begin() != luv->GetRows().end())
                                        {
                                            tableview->SetFilter(tableview->GetUniqueColumns()[colnum], luv->GetColValue(luv->GetRows().begin()->second, tableview->GetColDef(fkcol)->m_LookupFK));
                                        }

    #if defined DEBUG_IMPORT
    #if defined _WINDOWS
                                        OutputDebugStringA(QString::Format("Lookup Value '%s' set to '%s' for column '%s' on table '%s'", lookupvalue, dbrow->GetCol(colnum), colname, child->GetAttribute(wxT("name")) ).mbc_str());
                                        OutputDebugStringA("\n");
    #else
                                        std::cout << QString::Format("Lookup Value '%s' set to '%s' for column '%s' on table '%s'", lookupvalue, dbrow->GetCol(colnum), colname, child->GetAttribute(wxT("name")) )<< std::endl;
    #endif
    #endif
                                    }
                                }
                                else // if no lookup exists then just use the empy value, could potentially clear a value
                                {
                                    tableview->SetFilter(tableview->GetUniqueColumns()[colnum], content);
                                }
                            }
                        }

                        tableview->RefreshData();

#if defined DEBUG_IMPORT
#if defined _WINDOWS
                        for (int i = 0; i < tableview->GetUniqueColumns().Count(); i++)
                            OutputDebugStringA(QString::Format("Looking for unique value '%s' on column '%s' on table '%s'",
                                XMLTools::GetNodeContent(rows, wxT("column"), wxT("name"), tableview->GetUniqueColumns()[i]),
                                tableview->GetUniqueColumns()[i],
                                child->GetAttribute(wxT("name"))).mbc_str());
                        OutputDebugStringA("\n");
#else
                        for (int i = 0; i < tableview->GetUniqueColumns().Count(); i++)
                            std::cout << QString::Format("Looking for unique value '%s' on column '%s' on table '%s'",
                            XMLTools::GetNodeContent(rows, wxT("column"), wxT("name"), tableview->GetUniqueColumns()[i]),
                            tableview->GetUniqueColumns()[i],
                            child->GetAttribute(wxT("name"))) << std::endl;
#endif
#endif

                        if (tableview->GetRows().size() == 0)
                        {
#if defined DEBUG_IMPORT
#if defined _WINDOWS
                            OutputDebugStringA("Decided to add a new row.\n");
#else
                            std::cout << "Decided to add a new row.\n" << std::endl;
#endif
#endif
                            dbrow = tableview->AddRow();
                        }
                        else
                        {

                            dbrow = *(tableview->GetFirst());
#if defined DEBUG_IMPORT
#if defined _WINDOWS
                            OutputDebugStringA("Decided update an existing row.\n");

                            for (int i = 0; i < tableview->GetColumnCount(); i++)
                            {
                                OutputDebugStringA(QString::Format(" Col: %s Val '%s'", tableview->GetColName(i), dbrow->GetCol(i)));
                            }
                            OutputDebugStringA("\n");
#else
                            for (int i = 0; i < tableview->GetColumnCount(); i++)
                            {
                                std::cout << QString::Format(" Col: %s Val '%s'", tableview->GetColName(i), dbrow->GetCol(i));
                            }

                            std::cout << std::endl;
#endif
#endif
                        }
                    }
                    else
                    {
                        tableview->ClearAllFilters();
                        tableview->SetFilter(keycol, wxT("LOADEMPTYROWS"));
                        tableview->RefreshData();

                        dbrow = tableview->AddRow();
                    }

                    wxXmlNode* columns = rows->GetChildren();

                    while (columns)
                    {
                        QString coltagname = columns->GetName();

                        if (coltagname == wxT("column"))
                        {
                            QString colname = columns->GetAttribute("name");
                            QString colvalue = wxEmptyString;
                            wxInt32 colnum = tableview->GetColNumber(colname);

                            if (columns->GetChildren())
                                colvalue = columns->GetChildren()->GetContent();

                            // if the XML contains a value from a lookup list and the value isn't found don't do anything with this row
                            if (tableview->GetColDef(colnum)->m_LookupList.Count() > 0 && tableview->GetColDef(colnum)->m_LookupList.Index(colvalue) == wxNOT_FOUND)
                            {
                                if (dbrow->IsNew())
                                {
                                    dbrow->BackoutChanges();
                                    dbrow->MarkToDelete();
                                }
                                else
                                {
                                    dbrow->BackoutChanges();
                                }

                                // skip past the rest of the row processing
                                break;
                            }

                            if (colnum != -1)
                            {
                                // see if a lookup value exists if no value was given
                                if (colvalue.IsEmpty())
                                {
                                    QString lookupvalue = columns->GetAttribute("lookupvalue");
                                    if (!lookupvalue.IsEmpty())
                                    {
                                        DBDataView* luv = tableview->GetColDef(colnum)->m_LookupView;
                                        QString luc = tableview->GetColDef(colnum)->m_LookupValue;

                                        // make sure the view was setup with a lookup value
                                        if (luv)
                                        {
                                            // only load the lookup value in the foreign key list
                                            luv->ClearAllFilters();
                                            luv->SetFilter(luc, lookupvalue);
                                            luv->RefreshData();

                                            tableview->SetColByLookup(dbrow, colnum, lookupvalue);

#if defined DEBUG_IMPORT
#if defined _WINDOWS
                                            OutputDebugStringA(QString::Format("Lookup Value '%s' set to '%s' for column '%s' on table '%s'", lookupvalue, dbrow->GetCol(colnum), colname, child->GetAttribute(wxT("name")) ).mbc_str());
                                            OutputDebugStringA("\n");
#else
                                            std::cout << QString::Format("Lookup Value '%s' set to '%s' for column '%s' on table '%s'", lookupvalue, dbrow->GetCol(colnum), colname, child->GetAttribute(wxT("name")) )<< std::endl;
#endif
#endif
                                        }
                                    }
                                    else // if no lookup exists then just use the empy value, could potentially clear a value
                                    {
                                        dbrow->SetCol(colnum, colvalue);
                                    }
                                }
                                else
                                {
                                    dbrow->SetCol(colnum, colvalue);
                                }
                            }
                        }
                        else if (coltagname == wxT("table"))
                        {
                            // look for child related records
                            ImportXMLDocument(parentwindow, rows);
                        }

                        columns = columns->GetNext();
                    }

                    tableview->SaveData();

                    DBDataView* refreshview = wxGetApp().GetDBObjects()->SubscribeToDataView(m_CorrespondingRefresh[child->GetAttribute(wxT("name"))] , nullptr);
                    refreshview->SendSubscriberRefresh(nullptr);
                }

                rows = rows->GetNext();
            }
        }

        child = child->GetNext();
    }

    if (m_TopNode == xmlnode)
    {
        m_NodeTotal = 0;
        m_CurrentNode = 0;

        ((MainFrameClass*)wxGetApp().GetTopWindow())->SetStatusMessage(wxT("Import Complete."));
    }
}

void IXTree::NodeToXML( wxXmlNode* rootxmlnode, IXTreeNode* rootnode, const QString& viewname, const QString& fkfield, const QString& fkvalue )
{
    wxStack<LookupItem*> LookUpStack;
    wxStack<XMLNodeItem*> XMLNodeStack;
    IXTreeNode* currentnode = rootnode;
    wxXmlNode* newxmlnode = nullptr;

    while (currentnode)
    {
#if defined DEBUG_IMPORT
#if defined _WINDOWS
        OutputDebugStringA(QString::Format("Processing Node ViewName: '%s'  View Name Passes '%s'", currentnode->GetViewName().mbc_str(), viewname.mbc_str()));
        OutputDebugStringA("\n");
#else
            std::cout << QString::Format("Processing Node ViewName: '%s'  View Name Passes '%s'", currentnode->GetViewName().mbc_str(), viewname.mbc_str() ) << std::endl;
#endif
#endif

        if ( currentnode->GetViewName() == viewname || viewname.IsEmpty() )
        {
            newxmlnode = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("table"));
            newxmlnode->AddAttribute(wxT("name"), currentnode->GetViewName());

            newxmlnode->AddAttribute("filter_field", fkfield);
            newxmlnode->AddAttribute("filter_value", fkvalue);

            // setup the filter
            // if it is a foreign key set that filter
            currentnode->GetDataView()->ClearAllFilters();
            if (!viewname.IsEmpty() && !fkfield.IsEmpty() && !fkvalue.IsEmpty() )
                currentnode->GetDataView()->SetFilter(fkfield, fkvalue);

            // load the records into the node
            currentnode->GetDataView()->RefreshData();

            DBRow* row;
            wxXmlNode* xmlrow;

            m_NodeTotal += currentnode->GetDataView()->GetRows().size();

            // loop through rows and create xml
            for (DBRowList::iterator itrow = currentnode->GetDataView()->GetFirst(); itrow != currentnode->GetDataView()->GetEnd(); ++itrow )
            {
                m_CurrentNode++;
                QString msg = QString::Format(wxT("Processing %i of %i nodes: %i%% Complete."), m_CurrentNode, m_NodeTotal, (int)((float)m_CurrentNode / (float)m_NodeTotal * 100.0));
                ((MainFrameClass*)wxGetApp().GetTopWindow())->SetStatusMessage(msg);

                row = (DBRow*) *itrow;

                xmlrow = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("row"));
                xmlrow->AddAttribute(wxT("id"), row->GetCol(0));
                newxmlnode->addChild(xmlrow);

                for (int i = 1; i < currentnode->GetDataView()->GetColumnCount(); i++)
                {
                    wxXmlNode* xmlcol = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("column"));
                    xmlcol->AddAttribute(wxT("name"), currentnode->GetDataView()->GetColDef(i)->m_DisplayName);
                    xmlcol->AddAttribute(wxT("number"), QString::Format("%i", i));

                    DBDataView* lokup = currentnode->GetDataView()->GetColDef(i)->m_LookupView;
                    if ( lokup )
                    {
                        LookupItem* li = new LookupItem;
                        li->m_DBView = lokup;
                        li->m_LookupValue = row->GetCol(i);
                        li->m_LookupColumn = currentnode->GetDataView()->GetColDef(i)->m_LookupValue;
                        li->m_XmlColumn = xmlcol;

                        LookUpStack.push(li);
                    }

                    // PUSH XML Column, CurrentView, and column Lookup Value to Lookup Stack to prevent requering a DataView being traversed
                    xmlrow->addChild(xmlcol);

                    wxXmlNode* txtnode = new wxXmlNode(wxXML_TEXT_NODE, wxT("text"));
                    txtnode->SetContent(row->GetCol(i));
                    xmlcol->addChild(txtnode);
                }

                // add all child nodes
                IXTreeNode* childnode = currentnode->GetChild();
                while ( childnode )
                {
                    QString& pcol = childnode->GetFKParent();
                    QString& ccol = childnode->GetFKChild();

                    QString childval = currentnode->GetDataView()->GetColValue(row, pcol);
                    if (!childval.IsEmpty())
                    {
                        // PUSH childnode, XML Row, ChildNode Column, and Child Node Search Value to Node Stack to prevent requering a DataView being traversed
                        XMLNodeItem* ni = new XMLNodeItem;
                        ni->m_ChildColumn = ccol;
                        ni->m_ChildNode = childnode;
                        ni->m_SearchValue = childval;
                        ni->m_XmlRow = xmlrow;

                        XMLNodeStack.push(ni);
                    }

                    childnode = childnode->GetNext();
                }
            }

            // POP all information from  Node Stack and pass to NodeToXML
            while ( !XMLNodeStack.empty() )
            {
                XMLNodeItem* ni = XMLNodeStack.top();
                XMLNodeStack.pop();

                NodeToXML( ni->m_XmlRow, ni->m_ChildNode, ni->m_ChildNode->GetViewName(), ni->m_ChildColumn, ni->m_SearchValue );

                delete ni;
            }

            // POP all information from  Lookup Stack and Assing lookup values
            while ( !LookUpStack.empty() )
            {
                LookupItem* li = LookUpStack.top();
                LookUpStack.pop();

                li->m_DBView->ClearAllFilters();
                li->m_DBView->SetFilter(li->m_DBView->GetColName(0), li->m_LookupValue);
                li->m_DBView->RefreshData();

                DBRowList::iterator it =  li->m_DBView->GetFirst();

                if ( it != li->m_DBView->GetEnd() )
                {
                    QString colval = li->m_DBView->GetColValue(*it, li->m_LookupColumn);

                    li->m_XmlColumn->AddAttribute(wxT("lookupvalue"), colval);
                }

                delete li;
            }

            rootxmlnode->addChild(newxmlnode);
        }

        currentnode = currentnode->GetNext();
    }
}
*/

