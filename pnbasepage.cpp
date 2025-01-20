// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pnbasepage.h"
#include "pndatabaseobjects.h"
#include "pndatabaseobjects.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QApplication>
#include <QScrollBar>
#include <QAbstractItemModel>
#include <QTextCursor>

#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;
//TODO: On a plugin module load you will have to rebuild the plugin menus

PNBasePage::PNBasePage(QWidget *parent) : QWidget(parent)
{

}

void PNBasePage::setPageTitle()
{

}

void PNBasePage::saveState()
{
    QDomDocument doc;
    QDomElement root = doc.createElement("PageState");
    doc.appendChild(root);

    QDomElement child;

    QList<QWidget*> childwidgets = findChildren<QWidget*>();

    if (focusWidget())
    {
        root.setAttribute("FocusWidget", focusWidget()->objectName());
    }

    for (QWidget* widget : childwidgets)
    {
        if ( QString("PNTableView").compare( widget->metaObject()->className() ) == 0)
        {
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            if (qobject_cast<PNTableView*>(widget)->selectionModel()->hasSelection())
                child.setAttribute("SelectedRow", QString("%1").arg(qobject_cast<PNTableView*>(widget)->selectionModel()->selectedRows().at(0).row()));
            child.setAttribute("VerticalSroll", QString("%1").arg(qobject_cast<PNTableView*>(widget)->verticalScrollBar()->value()));
            child.setAttribute("HorizontalScroll", QString("%1").arg(qobject_cast<PNTableView*>(widget)->horizontalScrollBar()->value()));

            root.appendChild(child);
        }
        else if ( QString("PNPlainTextEdit").compare( widget->metaObject()->className() ) == 0)
        {
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            child.setAttribute("VerticalSroll", QString("%1").arg(qobject_cast<PNPlainTextEdit*>(widget)->verticalScrollBar()->value()));
            child.setAttribute("HorizontalScroll", QString("%1").arg(qobject_cast<PNPlainTextEdit*>(widget)->horizontalScrollBar()->value()));
            child.setAttribute("SelectionStart", QString("%1").arg(qobject_cast<PNPlainTextEdit*>(widget)->textCursor().selectionStart()));
            child.setAttribute("SelectionEnd", QString("%1").arg(qobject_cast<PNPlainTextEdit*>(widget)->textCursor().selectionEnd()));
            child.setAttribute("CursorPosition", QString("%1").arg(qobject_cast<PNPlainTextEdit*>(widget)->textCursor().position()));

            root.appendChild(child);
        }
        else if ( QString("QTabWidget").compare( widget->metaObject()->className() ) == 0)
        {
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            child.setAttribute("CurrentIndex", QString("%1").arg(qobject_cast<QTabWidget*>(widget)->currentIndex()));

            root.appendChild(child);
        }
        else if ( QString("PNTextEdit").compare( widget->metaObject()->className() ) == 0)
        {
            child = doc.createElement(widget->metaObject()->className());
            child.setAttribute("Name", widget->objectName());
            child.setAttribute("VerticalScroll", QString("%1").arg(qobject_cast<PNTextEdit*>(widget)->verticalScrollBar()->value()));
            child.setAttribute("HorizontalScroll", QString("%1").arg(qobject_cast<PNTextEdit*>(widget)->horizontalScrollBar()->value()));
            child.setAttribute("SelectionStart", QString("%1").arg(qobject_cast<PNTextEdit*>(widget)->textCursor().selectionStart()));
            child.setAttribute("SelectionEnd", QString("%1").arg(qobject_cast<PNTextEdit*>(widget)->textCursor().selectionEnd()));
            child.setAttribute("CursorPosition", QString("%1").arg(qobject_cast<PNTextEdit*>(widget)->textCursor().position()));

            root.appendChild(child);
        }

    }

    // Write the output to a QString.
    QString xml = doc.toString();

    QString parmname = QString("PageState:%1:%2").arg(objectName(), m_record_id.toString());

//    qDebug() << "save state of " << parmname;
//    qDebug() << xml;

    global_DBObjects.saveParameter( parmname, xml );
}

void PNBasePage::loadState()
{
    QString parmname;

    parmname = QString("PageState:%1:%2").arg(objectName(), m_record_id.toString());

    QString xml = global_DBObjects.loadParameter(parmname);

//    qDebug() << "load state of " << parmname;
//    qDebug() << xml;

    if (xml.isEmpty())
        return;

    QDomDocument doc;
    doc.setContent(xml);

    QDomElement root = doc.documentElement();
    QDomNode child;

    if (!root.isNull())
    {
        child = root.firstChild();

        QWidget* focus = findChild<QWidget*>(root.toElement().attribute("FocusWidget"));
        if (focus)
            focus->setFocus();
    }

    while (!child.isNull())
    {
        QWidget* widget = findChild<QWidget*>(child.toElement().attribute("Name"));

        if (widget)
        {
            QString classname = widget->metaObject()->className();

            if ( QString("PNTableView").compare( classname ) == 0)
            {
                int selectedrow = child.toElement().attribute("SelectedRow", "-1").toInt();
                if (selectedrow > -1)
                {
                    QAbstractItemModel* model = qobject_cast<PNTableView*>(widget)->model();

                    if (model)
                        qobject_cast<PNTableView*>(widget)->selectionModel()->select(model->index(selectedrow, 0),  QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                }

                int vscroll = child.toElement().attribute("VerticalScroll").toInt();
                int hscroll = child.toElement().attribute("HorizontalScroll").toInt();

                qobject_cast<PNTableView*>(widget)->verticalScrollBar()->setValue(vscroll);
                qobject_cast<PNTableView*>(widget)->horizontalScrollBar()->setValue(hscroll);
            }
            else if ( QString("PNPlainTextEdit").compare( widget->metaObject()->className() ) == 0)
            {
                int vscroll = child.toElement().attribute("VerticalScroll").toInt();
                int hscroll = child.toElement().attribute("HorizontalScroll").toInt();

                qobject_cast<PNPlainTextEdit*>(widget)->verticalScrollBar()->setValue(vscroll);
                qobject_cast<PNPlainTextEdit*>(widget)->horizontalScrollBar()->setValue(hscroll);

                int selectionstart = child.toElement().attribute("SelectionStart","0").toInt();
                int selectionend = child.toElement().attribute("SelectionEnd","0").toInt();
                int cursorposition = child.toElement().attribute("CurosrPosition","0").toInt();

                QTextCursor qt = qobject_cast<PNPlainTextEdit*>(widget)->textCursor();

                qt.setPosition(cursorposition);
                qt.setPosition(selectionstart, QTextCursor::MoveAnchor);
                qt.setPosition(selectionend, QTextCursor::KeepAnchor);

                qobject_cast<PNPlainTextEdit*>(widget)->setTextCursor(qt);
            }
            else if ( QString("QTabWidget").compare( widget->metaObject()->className() ) == 0)
            {
                qobject_cast<QTabWidget*>(widget)->setCurrentIndex(child.toElement().attribute("CurrentIndex","0").toInt());
            }
            else if ( QString("PNTextEdit").compare( widget->metaObject()->className() ) == 0)
            {
                int vscroll = child.toElement().attribute("VerticalScroll").toInt();
                int hscroll = child.toElement().attribute("HorizontalScroll").toInt();

                qobject_cast<PNTextEdit*>(widget)->verticalScrollBar()->setValue(vscroll);
                qobject_cast<PNTextEdit*>(widget)->horizontalScrollBar()->setValue(hscroll);

                int selectionstart = child.toElement().attribute("SelectionStart","0").toInt();
                int selectionend = child.toElement().attribute("SelectionEnd","0").toInt();
                int cursorposition = child.toElement().attribute("CurosrPosition","0").toInt();

                QTextCursor qt = qobject_cast<PNTextEdit*>(widget)->textCursor();

                qt.setPosition(cursorposition);
                qt.setPosition(selectionstart, QTextCursor::MoveAnchor);
                qt.setPosition(selectionend, QTextCursor::KeepAnchor);

                qobject_cast<PNTextEdit*>(widget)->setTextCursor(qt);
            }
        }

        child = child.nextSibling();
    }

}

void PNBasePage::openRecord(QVariant& t_record_id)
{
    m_record_id = t_record_id;

    global_DBObjects.refreshDirty();

    loadState();
}

void PNBasePage::newRecord()
{
    QModelIndex index = dynamic_cast<PNSqlQueryModel*>(getCurrentModel()->sourceModel())->newRecord();

    //  check if column is visible
    int col = 1;
    while (getCurrentView()->isColumnHidden(col))
        col++;

    QModelIndex sort_index = dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model())->index(
        dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model())->mapFromSource(index).row(), col);

    getCurrentView()->selectionModel()->select(sort_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    getCurrentView()->scrollTo(sort_index, QAbstractItemView::PositionAtCenter);
}

void PNBasePage::deleteItem()
{
    QModelIndexList qi = getCurrentView()->selectionModel()->selectedRows();

    for (int i = qi.count() - 1; i >= 0; i--)
    {
        dynamic_cast<PNSqlQueryModel*>(getCurrentModel()->sourceModel())->deleteRecord(getCurrentModel()->mapToSource(qi[i]));
    }
}

void PNBasePage::copyItem()
{
    QModelIndexList qi = getCurrentView()->selectionModel()->selectedRows();
    QModelIndex index;

    for (int i = qi.count() - 1; i >= 0; i--)
    {
        index = dynamic_cast<PNSqlQueryModel*>(getCurrentModel()->sourceModel())->copyRecord(getCurrentModel()->mapToSource(qi[i]));
    }

    //  check if column is visible
    int col = 1;
    while (getCurrentView()->isColumnHidden(col))
        col++;

    QModelIndex sort_index = dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model())->index(
        dynamic_cast<QSortFilterProxyModel*>(getCurrentView()->model())->mapFromSource(index).row(), col);

    getCurrentView()->selectionModel()->select(sort_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    getCurrentView()->scrollTo(sort_index, QAbstractItemView::PositionAtCenter);
}

void PNBasePage::openItem()
{
    QModelIndexList qi = getCurrentView()->selectionModel()->selectedRows();

    if (qi.count() > 0)
        getCurrentView()->slotOpenRecord();
}

void PNBasePage::setButtonAndMenuStates()
{

}

void PNBasePage::buildPluginMenu(PluginManager* t_pm, QMenu* t_menu)
{
    t_menu->addSeparator();

    // add menus relevant to the current table
    for ( Plugin* p : t_pm->plugins())
    {
        for (QMap<QString, PluginMenu>::const_iterator it = p->pythonplugin().menus().cbegin(); it != p->pythonplugin().menus().cend(); ++it)
        //for ( PluginMenu pm : p->pythonplugin().menus())  //TODO: Remove
        {
            if (it.value().dataexport().isEmpty()) // don't include any right-click data menus
            {
                if (it.value().submenu().isEmpty())
                {
                    QAction* bact = nullptr;
                    int pastseparator = 0;

                    for (QAction* action : t_menu->actions())
                    {
                        if (pastseparator > 0 && action->text().compare(it.key(), Qt::CaseInsensitive) > 0)
                            bact = action;

                        if (action->isSeparator())
                            pastseparator++;
                    }

                    if (bact)
                    {
                        QAction* act = new QAction(QIcon(":/icons/add-on.png"), it.key(), this);
                        connect(act, &QAction::triggered, this,[p, it, this](){slotPluginMenu(p, it.value().functionname());});
                        t_menu->insertAction(bact, act);
                    }
                    else
                    {
                        QAction* act = t_menu->addAction(it.key(), [p, it, this](){slotPluginMenu(p, it.value().functionname());});
                        act->setIcon(QIcon(":/icons/add-on.png"));
                    }
                }
                else
                {
                    // find the submenu if it exists
                    QMenu* submenu = nullptr;

                    for (QAction* action : t_menu->actions())
                    {
                        if (action->text().compare(it.value().submenu(), Qt::CaseInsensitive) == 0)
                            submenu = action->menu();
                    }

                    // if it didn't exist create it sorted
                    if (!submenu)
                    {
                        int pastseparator = 0;

                        for (QAction* action : t_menu->actions())
                        {
                            if (pastseparator > 0 && action->text().compare(it.value().submenu(), Qt::CaseInsensitive) > 0)
                            {
                                submenu = new QMenu(it.value().submenu());
                                t_menu->insertMenu(action, submenu);
                                break;
                            }

                            if (action->isSeparator())
                                pastseparator++;
                        }
                    }

                    if (!submenu)
                        submenu = t_menu->addMenu(it.value().submenu());

                    QAction* act = submenu->addAction(it.key(), [p, it, this](){slotPluginMenu(p, it.value().functionname());});
                    act->setIcon(QIcon(":/icons/add-on.png"));
                }
            }
        }
    }
}

void PNBasePage::slotPluginMenu(Plugin* t_plugin, const QString& t_functionname)
{
    // QString response;

    // if (m_page_model)
    // {
        // QVariant keyval;
        // keyval = m_page_model->data(m_page_model->index(0, 0));

        //TODO: Remove QApplication::processEvents();

        // PNSqlQueryModel *exportmodel = m_page_model->createExportVersion();
        // exportmodel->setFilter(0, keyval.toString());
        // exportmodel->refresh();

        // QDomDocument* xdoc = global_DBObjects.createXMLExportDoc(exportmodel, t_plugin->getChildTablesFilter());
        // QString xmlstr = xdoc->toString();

        // call the menu plugin with the data structure
        //response = t_plugin->callDataRightClickEvent(xmlstr);
    t_plugin->callMethod(t_functionname);

        // delete xdoc;

        //TODO: Remove QApplication::processEvents();
    // }

    //TODO: Remove
    // if (!response.isEmpty())
    // {
    //    QApplication::processEvents();

    //    QDomDocument doc;
    //    doc.setContent(response);

    //    if (!global_DBObjects.importXMLDoc(doc))
    //        QMessageBox::critical(this, tr("Plugin Response Failed"), "Parsing XML file failed.");

    //    QApplication::processEvents();
    // }
}

